/*
 * $Id: image_sdl.cpp 3066 2014-07-21 00:18:48Z darren_janeczek $
 */

#include "image.h"
#include "imagemgr.h"
#include "settings.h"
#include "error.h"

Image *Image::create(int w, int h) {
    Image *im = new Image;

    im->w = w;
    im->h = h;

    im->surface = (xu4_vsurface_t*)malloc(sizeof(xu4_vsurface_t));
    im->surface->pixels = (uint32_t*)malloc(sizeof(uint32_t) * w * h);
    im->surface->w = w;
    im->surface->h = h;

    if (!im->surface) {
        delete im;
        return NULL;
    }

    return im;
}

Image *Image::createScreenImage() {
    Image *screen = new Image();
    screen->surface = (xu4_vsurface_t*)malloc(sizeof(xu4_vsurface_t));
    screen->surface->pixels = (uint32_t*)malloc(sizeof(uint32_t) * SCREEN_WIDTH * SCREEN_HEIGHT);
    screen->surface->w = SCREEN_WIDTH;
    screen->surface->h = SCREEN_HEIGHT;
    screen->w = screen->surface->w;
    screen->h = screen->surface->h;
    return screen;
}

Image *Image::duplicate(Image *image) {    
    Image *im = create(image->w, image->h);
    image->drawOn(im, 0, 0);
    im->backgroundColor = image->backgroundColor;
    return im;
}

Image::~Image() {
    free(surface->pixels);
    free(surface);
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

uint32_t Image::getPixel(int x, int y) {
	if (x > (w - 1) || y > (h - 1) || x < 0 || y < 0) {
		//printf("getPixel %d,%d out of range - max %d,%d\n", x, y, w-1, h-1);
		return 0;
	}
	return *((uint32_t*)(surface->pixels) + (y * w) + x);
}

void Image::putPixel(int x, int y, uint32_t value) {
	if (x > (w - 1) || y > (h - 1) || x < 0 || y < 0) {
		//printf("putPixel %d,%d out of range - max %d,%d\n", x, y, w-1, h-1);
		return;
	}
	*((uint32_t*)(surface->pixels) + (y * w) + x) = value;
}

void Image::fillRect(int x, int y, int width, int height, int r, int g, int b, int a) {
	uint32_t pixel = (a & 0xff) << 24 | (b & 0xff) << 16 | (g & 0xff) << 8 | (r & 0xff);
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			*((uint32_t*)surface->pixels + ((y * w) + x) + (i * w) + j) = pixel;
		}
	}
}

void Image::drawOn(Image *d, int x, int y) {
	if (d == NULL) {
		d = imageMgr->get("screen")->image;
	}
	
	for (int i = 0; i < h; i++) {
		memcpy((uint32_t*)d->surface->pixels + ((y * d->w) + x) + (i * d->w),
			(uint32_t*)surface->pixels + (i * w), w * sizeof(uint32_t));
	}
}

void Image::drawSubRectOn(Image *d, int x, int y, int rx, int ry, int rw, int rh) {
	if (d == NULL) {
		d = imageMgr->get("screen")->image;
	}
	
	for (int i = 0; i < rh; i++) {
		for (int j = 0; j < rw; j++) {
			//d->putPixel(x + j, y + i, getPixel(rx + j, ry + i));
			d->putPixel(x + j, y + i, getPixel(rx + j, ry + i));
		}
	}
}

void Image::drawSubRectInvertedOn(Image *d, int x, int y, int rx, int ry, int rw, int rh) {
	if (d == NULL) {
		d = imageMgr->get("screen")->image;
	}
	
	for (int i = 0; i < rh; i++) {
		for (int j = 0; j < rw; j++) {
			d->putPixel(x + j, y + rh - 1 - i, getPixel(rx + j, ry + i));
		}
	}
}

void Image::drawHighlighted() {
	uint32_t pixel;
	RGBA c;
	for (int i = 0; i < h; i++) {
		for (int j = 0; j < w; j++) {
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
