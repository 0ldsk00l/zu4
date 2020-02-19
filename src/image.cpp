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
    xu4_img_draw_on(im, image, 0, 0);
    return im;
}

Image::~Image() {
    free(surface->pixels);
    free(surface);
}

uint32_t xu4_img_get_pixel(Image *s, int x, int y) {
	if (x > (s->w - 1) || y > (s->h - 1) || x < 0 || y < 0) {
		//printf("getPixel %d,%d out of range - max %d,%d\n", x, y, w-1, h-1);
		return 0;
	}
	return *((uint32_t*)(s->surface->pixels) + (y * s->w) + x);
}

void xu4_img_set_pixel(Image *d, int x, int y, uint32_t value) {
	if (x > (d->w - 1) || y > (d->h - 1) || x < 0 || y < 0) {
		//printf("putPixel %d,%d out of range - max %d,%d\n", x, y, w-1, h-1);
		return;
	}
	*((uint32_t*)(d->surface->pixels) + (y * d->w) + x) = value;
}

void xu4_img_fill(Image *d, int x, int y, int width, int height, int r, int g, int b, int a) {
	uint32_t pixel = (a & 0xff) << 24 | (b & 0xff) << 16 | (g & 0xff) << 8 | (r & 0xff);
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			*((uint32_t*)d->surface->pixels + ((y * d->w) + x) + (i * d->w) + j) = pixel;
		}
	}
}

void xu4_img_draw_on(Image *d, Image *s, int x, int y) {
	if (d == NULL) {
		d = xu4_img_get_screen();
	}
	
	for (int i = 0; i < s->h; i++) {
		memcpy((uint32_t*)d->surface->pixels + ((y * d->w) + x) + (i * d->w),
			(uint32_t*)s->surface->pixels + (i * s->w), s->w * sizeof(uint32_t));
	}
}

void xu4_img_draw(Image *s, int x, int y) {
	xu4_img_draw_on(NULL, s, x, y);
}

void xu4_img_draw_subrect_on(Image *d, Image *s, int x, int y, int rx, int ry, int rw, int rh) {
	if (d == NULL) {
		d = xu4_img_get_screen();
	}
	
	for (int i = 0; i < rh; i++) {
		for (int j = 0; j < rw; j++) {
			xu4_img_set_pixel(d, x + j, y + i, xu4_img_get_pixel(s, rx + j, ry + i));
		}
	}
}

void xu4_img_draw_subrect(Image *s, int x, int y, int rx, int ry, int rw, int rh) {
    xu4_img_draw_subrect_on(NULL, s, x, y, rx, ry, rw, rh);
}

void xu4_img_draw_subrect_inv(Image *d, Image *s, int x, int y, int rx, int ry, int rw, int rh) {
	if (d == NULL) {
		d = xu4_img_get_screen();
	}
	
	for (int i = 0; i < rh; i++) {
		for (int j = 0; j < rw; j++) {
			xu4_img_set_pixel(d, x + j, y + rh - 1 - i, xu4_img_get_pixel(s, rx + j, ry + i));
		}
	}
}

void xu4_img_draw_highlighted(Image *d) {
	uint32_t pixel;
	RGBA c;
	for (int i = 0; i < d->h; i++) {
		for (int j = 0; j < d->w; j++) {
			pixel = xu4_img_get_pixel(d, j, i);
			c.r = 0xff - (pixel & 0xff);
			c.g = 0xff - ((pixel & 0xff00) >> 8);
			c.b = 0xff - ((pixel & 0xff0000) >> 16);
			c.a = (pixel & 0xff000000) >> 24;
			pixel = c.a << 24 | c.b << 16 | c.g << 8 | c.r;
			xu4_img_set_pixel(d, j, i, pixel);
        }
    }
}

Image* xu4_img_get_screen() {
	return imageMgr->get("screen")->image;
}
