/*
 * image.c
 * Copyright (C) 2014 Darren Janeczek
 * Copyright (C) 2012 twschulz
 * Copyright (C) 2020 R. Danbrook
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 */

#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>

#include "image.h"
#include "settings.h"
#include "error.h"

static Image *screen = NULL;

Image* xu4_img_create(int w, int h) {
    Image *im = (Image*)malloc(sizeof(Image));
    im->w = w;
    im->h = h;
    im->pixels = (uint32_t*)malloc(sizeof(uint32_t) * w * h);

    if (!im->pixels) {
        xu4_img_free(im);
        return NULL;
    }

    return im;
}

Image* xu4_img_create_screen() {
    screen = (Image*)malloc(sizeof(Image));
    screen->pixels = (uint32_t*)malloc(sizeof(uint32_t) * SCREEN_WIDTH * SCREEN_HEIGHT);
    screen->w = SCREEN_WIDTH;
    screen->h = SCREEN_HEIGHT;
    return screen;
}

Image* xu4_img_dup(Image *image) {    
    Image *im = xu4_img_create(image->w, image->h);
    xu4_img_draw_on(im, image, 0, 0);
    return im;
}

void xu4_img_free(Image *image) {
	if (image->pixels) free(image->pixels);
    if (image) free(image);
}

uint32_t xu4_img_get_pixel(Image *s, int x, int y) {
	if (x > (s->w - 1) || y > (s->h - 1) || x < 0 || y < 0) {
		//printf("getPixel %d,%d out of range - max %d,%d\n", x, y, w-1, h-1);
		return 0;
	}
	return *((uint32_t*)(s->pixels) + (y * s->w) + x);
}

void xu4_img_set_pixel(Image *d, int x, int y, uint32_t value) {
	if (x > (d->w - 1) || y > (d->h - 1) || x < 0 || y < 0) {
		//printf("putPixel %d,%d out of range - max %d,%d\n", x, y, w-1, h-1);
		return;
	}
	*((uint32_t*)(d->pixels) + (y * d->w) + x) = value;
}

void xu4_img_fill(Image *d, int x, int y, int width, int height, int r, int g, int b, int a) {
	uint32_t pixel = (a & 0xff) << 24 | (b & 0xff) << 16 | (g & 0xff) << 8 | (r & 0xff);
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			*((uint32_t*)d->pixels + ((y * d->w) + x) + (i * d->w) + j) = pixel;
		}
	}
}

void xu4_img_draw_on(Image *d, Image *s, int x, int y) {
	if (d == NULL) {
		d = xu4_img_get_screen();
	}
	
	for (int i = 0; i < s->h; i++) {
		memcpy((uint32_t*)d->pixels + ((y * d->w) + x) + (i * d->w),
			(uint32_t*)s->pixels + (i * s->w), s->w * sizeof(uint32_t));
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
	return screen;
}
