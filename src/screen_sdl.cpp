/*
 * $Id: screen_sdl.cpp 3087 2015-01-24 03:44:46Z darren_janeczek $
 */

#include <SDL.h>

#include "error.h"
#include "image.h"
#include "settings.h"
#include "screen.h"
#include "u4.h"
#include "u4_sdl.h"

void screenRefreshThreadInit();
void screenRefreshThreadEnd();

/**
 * A simple row and column duplicating scaler. FIXME use OpenGL instead
 */
Image *scalePoint(Image *src, int scale, int n) {
    int x, y, i, j;
    Image *dest;

    dest = Image::create(src->width() * scale, src->height() * scale, src->isIndexed(), Image::HARDWARE);
    if (!dest)
        return NULL;

    if (dest->isIndexed())
        dest->setPaletteFromImage(src);

    for (y = 0; y < src->height(); y++) {
        for (x = 0; x < src->width(); x++) {
            for (i = 0; i < scale; i++) {
                for (j = 0; j < scale; j++) {
                    unsigned int index;
                    src->getPixelIndex(x, y, index);
                    dest->putPixelIndex(x * scale + j, y * scale + i, index);
                }
            }
        }
    }

    return dest;
}

void screenInit_sys() {
    /* start SDL */
    if (u4_SDL_InitSubSystem(SDL_INIT_VIDEO) < 0)
        xu4_error(XU4_LOG_ERR, "unable to init SDL: %s", SDL_GetError());    
    SDL_EnableUNICODE(1);
    SDL_SetGamma(settings.gamma / 100.0f, settings.gamma / 100.0f, settings.gamma / 100.0f);
    atexit(SDL_Quit);

    SDL_WM_SetCaption("Ultima IV", NULL);

    if (!SDL_SetVideoMode(320 * settings.scale, 200 * settings.scale, 0, SDL_HWSURFACE | SDL_ANYFORMAT | (settings.fullscreen ? SDL_FULLSCREEN : 0)))
        xu4_error(XU4_LOG_ERR, "unable to set video: %s", SDL_GetError());

    SDL_ShowCursor(SDL_DISABLE);

    screenRefreshThreadInit();
}

void screenDelete_sys() {
	screenRefreshThreadEnd();
    u4_SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

/**
 * Force a redraw.
 */

SDL_mutex *screenLockMutex = NULL;
int frameDuration = 0;

void screenLock() {
	//SDL_LockSurface(SDL_GetVideoSurface());
	SDL_mutexP(screenLockMutex);
}

void screenUnlock() {
	//SDL_UnlockSurface(SDL_GetVideoSurface());
	SDL_mutexV(screenLockMutex);
}

void screenRedrawScreen() {
	screenLock();
    SDL_Flip(SDL_GetVideoSurface());
    screenUnlock();
}

void screenRedrawTextArea(int x, int y, int width, int height) {
	/*screenLock();
	SDL_UpdateRect(SDL_GetVideoSurface(), x * CHAR_WIDTH * settings.scale, y * CHAR_HEIGHT * settings.scale, width * CHAR_WIDTH * settings.scale, height * CHAR_HEIGHT * settings.scale);
	screenUnlock();*/
}

void screenWait(int numberOfAnimationFrames) {
	SDL_Delay(numberOfAnimationFrames * frameDuration);
}

bool continueScreenRefresh = true;
SDL_Thread *screenRefreshThread = NULL;

int screenRefreshThreadFunction(void *unused) {

	while (continueScreenRefresh) {
		SDL_Delay(frameDuration);
		screenRedrawScreen();
	}
	return 0;
}

void screenRefreshThreadInit() {
	screenLockMutex = SDL_CreateMutex();;

	frameDuration = 1000 / settings.screenAnimationFramesPerSecond;

	continueScreenRefresh = true;
	if (screenRefreshThread) {
		xu4_error(XU4_LOG_WRN, "Screen refresh thread already exists.");
		return;
	}

	screenRefreshThread = SDL_CreateThread(screenRefreshThreadFunction, NULL);
	if (!screenRefreshThread) {
		xu4_error(XU4_LOG_WRN, SDL_GetError());
		return;
	}
}

void screenRefreshThreadEnd() {
	continueScreenRefresh = false;
	SDL_WaitThread(screenRefreshThread, NULL);
	screenRefreshThread = NULL;
}

/**
 * Scale an image up.  The resulting image will be scale * the
 * original dimensions.  The original image is no longer deleted.
 * n is the number of tiles in the image; each tile is filtered 
 * seperately. filter determines whether or not to filter the 
 * resulting image.
 */
Image *screenScale(Image *src, int scale, int n, int filter) {
    Image *dest = NULL;
	bool alpha = src->isAlphaOn();

	if (n == 0)
		n = 1;

	src->alphaOff();

	while (filter && (scale % 2 == 0)) {
		dest = scalePoint(src, 2, n);
		src = dest;
		scale /= 2;
	}

	if (scale != 1)
		dest = scalePoint(src, scale, n);

	if (!dest)
		dest = Image::duplicate(src);

	if (alpha)
		src->alphaOn();

    return dest;
}

/**
 * Scale an image down.  The resulting image will be 1/scale * the
 * original dimensions.  The original image is no longer deleted.
 */
Image *screenScaleDown(Image *src, int scale) {
    int x, y;
    Image *dest;
    bool alpha = src->isAlphaOn();

    src->alphaOff();

    dest = Image::create(src->width() / scale, src->height() / scale, src->isIndexed(), Image::HARDWARE);
    if (!dest) {
        return NULL;
    }

	if (!dest) {
		dest = Image::duplicate(src);
    }

    if (dest->isIndexed()) {
        dest->setPaletteFromImage(src);
    }

    for (y = 0; y < src->height(); y+=scale) {
        for (x = 0; x < src->width(); x+=scale) {
            unsigned int index;
            src->getPixelIndex(x, y, index);                
            dest->putPixelIndex(x / scale, y / scale, index);
        }
    }

    if (alpha)
        src->alphaOn();

    return dest;
}
