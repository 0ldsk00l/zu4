/*
 * $Id: image_sdl.cpp 3066 2014-07-21 00:18:48Z darren_janeczek $
 */

#include "image.h"
#include "imagemgr.h"
#include "settings.h"
#include "error.h"

/**
 * Creates a new image.  Scale is stored to allow drawing using U4
 * (320x200) coordinates, regardless of the actual image scale.
 */
Image *Image::create(int w, int h) {
    Uint32 rmask, gmask, bmask, amask;
    Uint32 flags;
    Image *im = new Image;

    im->w = w;
    im->h = h;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif

    flags = SDL_SWSURFACE;

    im->surface = SDL_CreateRGBSurface(flags, w, h, 32, rmask, gmask, bmask, amask);

    if (!im->surface) {
        delete im;
        return NULL;
    }

    return im;
}

/**
 * Create a special purpose image the represents the whole screen.
 */
Image *Image::createScreenImage() {
    Image *screen = new Image();

    screen->surface = SDL_GetVideoSurface();
    xu4_assert(screen->surface != NULL, "SDL_GetVideoSurface() returned a NULL screen surface!");
    screen->w = screen->surface->w;
    screen->h = screen->surface->h;
    return screen;
}

/**
 * Creates a duplicate of another image
 */
Image *Image::duplicate(Image *image) {    
    bool alphaOn = image->isAlphaOn();
    Image *im = create(image->w, image->h);

    /* Turn alpha off before blitting to non-screen surfaces */
    if (alphaOn)
        image->alphaOff();
    
    image->drawOn(im, 0, 0);

    if (alphaOn)
        image->alphaOn();

    im->backgroundColor = image->backgroundColor;

    return im;
}

/**
 * Frees the image.
 */
Image::~Image() {
    SDL_FreeSurface(surface);
}

void Image::initializeToBackgroundColor(RGBA backgroundColor)
{
	this->backgroundColor = backgroundColor;
    this->fillRect(0,0,this->w,this->h,
    		backgroundColor.r,
    		backgroundColor.g,
    		backgroundColor.b,
    		backgroundColor.a);
}

bool Image::isAlphaOn() {
    return false;//return surface->flags & SDL_SRCALPHA;
}

void Image::alphaOn() {
    //surface->flags |= SDL_SRCALPHA;
}

void Image::alphaOff() {
    //surface->flags &= ~SDL_SRCALPHA;
}

uint32_t Image::getPixel(int x, int y) {
	if (x > (int)(w - 1) || y > (int)(h - 1) || x < 0 || y < 0) {
		//printf("getPixel %d,%d out of range - max %d,%d\n", x, y, w-1, h-1);
		return 0;
	}
	return *((uint32_t*)(surface->pixels) + (y * w) + x);
}

void Image::putPixel(int x, int y, uint32_t value) {
	if (x > (int)(w - 1) || y > (int)(h - 1) || x < 0 || y < 0) {
		//printf("putPixel %d,%d out of range - max %d,%d\n", x, y, w-1, h-1);
		return;
	}
	*((uint32_t*)(surface->pixels) + (y * w) + x) = value;
}

void Image::fillRect(int x, int y, int width, int height, int r, int g, int b, int a) {
    uint32_t pixel = (isAlphaOn() ? a & 0xff : 0xff) << 24 | (b & 0xff) << 16 | (g & 0xff) << 8 | (r & 0xff);
    for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			*((uint32_t*)surface->pixels + ((y * w) + x) + (i * w) + j) = pixel;
		}
	}
}

/**
 * Draws the image onto another image.
 */
void Image::drawOn(Image *d, int x, int y) {
    if (d == NULL) {
        d = imageMgr->get("screen")->image;
	}
	
    for (unsigned int i = 0; i < h; i++) {
		memcpy((uint32_t*)d->surface->pixels + ((y * d->w) + x) + (i * d->w),
			(uint32_t*)surface->pixels + (i * w), w * sizeof(uint32_t));
	}
}

/**
 * Draws a piece of the image onto another image.
 */
void Image::drawSubRectOn(Image *d, int x, int y, int rx, int ry, int rw, int rh) {
    if (d == NULL) {
        d = imageMgr->get("screen")->image;
	}
	
	for (int i = 0; i < rh; i++) {
		for (int j = 0; j < rw; j++) {
			d->putPixel(x + j, y + i, getPixel(rx + j, ry + i));
		}
	}
}

/**
 * Draws a piece of the image onto another image, inverted.
 */
void Image::drawSubRectInvertedOn(Image *d, int x, int y, int rx, int ry, int rw, int rh) {
    if (d == NULL) {
        d = imageMgr->get("screen")->image;
	}

    /*SDL_Rect src, dest;
    for (int i = 0; i < rh; i++) {
        src.x = rx;
        src.y = ry + i;
        src.w = rw;
        src.h = 1;

        dest.x = x;
        dest.y = y + rh - i - 1;

        SDL_BlitSurface(surface, &src, d->surface, &dest);
    }//*/
    // FIXME - reimplement this
}

void Image::drawHighlighted() {
	uint32_t pixel;
	RGBA c;
	for (unsigned i = 0; i < h; i++) {
		for (unsigned j = 0; j < w; j++) {
			pixel = getPixel(j, i);
			c.r = 0xff - (pixel & 0xff);
			c.g = 0xff - ((pixel & 0xff00) >> 8);
			c.b = 0xff - ((pixel & 0xff0000) >> 16);
			c.a = (pixel & 0xff000000) >> 24;
			pixel = c.a << 24 | c.b << 16 | c.g << 8 | c.r;
			putPixel(j, i, pixel);
        }
    }
}
