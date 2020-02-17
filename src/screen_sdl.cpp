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

/**
 * A simple row and column duplicating scaler. FIXME use OpenGL instead
 */
Image *scalePoint(Image *src, int scale, int n) {
    unsigned int x, y;
    Image *dest;

    dest = Image::create(src->w * scale, src->h * scale);
    if (!dest)
        return NULL;

    for (y = 0; y < src->h; y++) {
        for (x = 0; x < src->w; x++) {
            for (int i = 0; i < scale; i++) {
                for (int j = 0; j < scale; j++) {
                    uint32_t index = src->getPixel(x, y);
                    dest->putPixel(x * scale + j, y * scale + i, index);
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
    
    if (!SDL_SetVideoMode(320 * settings.scale, 200 * settings.scale, 0, SDL_HWSURFACE | SDL_ANYFORMAT))
        xu4_error(XU4_LOG_ERR, "unable to set video: %s", SDL_GetError());

    SDL_ShowCursor(SDL_DISABLE);
}

void screenDelete_sys() {
    u4_SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

void screenRedrawScreen() {
    SDL_Flip(SDL_GetVideoSurface());
}

void screenRedrawTextArea(int x, int y, int width, int height) {
	SDL_UpdateRect(SDL_GetVideoSurface(), x * CHAR_WIDTH * settings.scale, y * CHAR_HEIGHT * settings.scale, width * CHAR_WIDTH * settings.scale, height * CHAR_HEIGHT * settings.scale);
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
    unsigned int x, y;
    Image *dest;
    bool alpha = src->isAlphaOn();

    src->alphaOff();

    dest = Image::create(src->w / scale, src->h / scale);
    if (!dest) {
        return NULL;
    }

	if (!dest) {
		dest = Image::duplicate(src);
    }

    for (y = 0; y < src->h; y+=scale) {
        for (x = 0; x < src->w; x+=scale) {
            uint32_t index = src->getPixel(x, y);
            dest->putPixel(x / scale, y / scale, index);
        }
    }

    if (alpha)
        src->alphaOn();

    return dest;
}
