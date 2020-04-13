/*
 * image.c
 * Copyright (C) 2012 twschulz
 * Copyright (C) 2014 Darren Janeczek
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

// Return the address of the main "screen" image
Image* zu4_img_get_screen() { return screen; }

Image* zu4_img_create(int w, int h) {
	// Create a new image
	Image *im = (Image*)malloc(sizeof(Image));
	im->w = w;
	im->h = h;
	im->pixels = (uint32_t*)malloc(sizeof(uint32_t) * w * h);

	if (!im->pixels) {
		zu4_img_free(im);
		return NULL;
	}

	return im;
}

Image* zu4_img_create_screen() {
	// Create the main screen image
	screen = (Image*)malloc(sizeof(Image));
	screen->pixels = (uint32_t*)malloc(sizeof(uint32_t) * SCREEN_WIDTH * SCREEN_HEIGHT);
	screen->w = SCREEN_WIDTH;
	screen->h = SCREEN_HEIGHT;
	return screen;
}

Image* zu4_img_dup(Image *image) {
	// Duplicate an image
	Image *im = zu4_img_create(image->w, image->h);
	zu4_img_draw_on(im, image, 0, 0);
	return im;
}

Image* zu4_img_scaleup(Image *s, int scale) {
	// Scale an image up
	Image *d = NULL;
	
	if (scale != 1) {
		d = zu4_img_create(s->w * scale, s->h * scale);
		
		for (int y = 0; y < s->h; y++) {
			for (int x = 0; x < s->w; x++) {
				for (int i = 0; i < scale; i++) {
					for (int j = 0; j < scale; j++) {
						uint32_t v = zu4_img_get_pixel(s, x, y);
						zu4_img_set_pixel(d, x * scale + j, y * scale + i, v);
					}
				}
			}
		}
	}
	
	if (!d) {
		d = zu4_img_dup(s);
	}
	
	return d;
}

Image* zu4_img_scaledown(Image *s, int scale) {
	// Scale an image down
	Image *d;
	
	d = zu4_img_create(s->w / scale, s->h / scale);
	if (!d) {
		return NULL;
	}
	
	if (!d) {
		d = zu4_img_dup(s);
	}
	
	for (int y = 0; y < s->h; y += scale) {
		for (int x = 0; x < s->w; x += scale) {
			uint32_t v = zu4_img_get_pixel(s, x, y);
			zu4_img_set_pixel(d, x / scale, y / scale, v);
		}
	}
	
	return d;
}

void zu4_img_free(Image *image) {
	// Free resources allocated for an image
	if (image->pixels) free(image->pixels);
	if (image) free(image);
	image = NULL;
}

uint32_t zu4_img_get_pixel(Image *s, int x, int y) {
	// Get a pixel's value
	if (x > (s->w - 1) || y > (s->h - 1) || x < 0 || y < 0) {
		//printf("getPixel %d,%d out of range - max %d,%d\n", x, y, w-1, h-1);
		return 0;
	}
	return *((uint32_t*)(s->pixels) + (y * s->w) + x);
}

void zu4_img_set_pixel(Image *d, int x, int y, uint32_t value) {
	// Set a pixel's value
	if (x > (d->w - 1) || y > (d->h - 1) || x < 0 || y < 0) {
		//printf("putPixel %d,%d out of range - max %d,%d\n", x, y, w-1, h-1);
		return;
	}
	if ((value >> 24) == 0x00) return;
	*((uint32_t*)(d->pixels) + (y * d->w) + x) = value;
}

void zu4_img_fill(Image *d, int x, int y, int width, int height, int r, int g, int b, int a) {
	// Create a rectangle and fill it
	uint32_t pixel = (a & 0xff) << 24 | (b & 0xff) << 16 | (g & 0xff) << 8 | (r & 0xff);
	for (int i = 0; i < height; i++) {
		for (int j = 0; j < width; j++) {
			*((uint32_t*)d->pixels + ((y * d->w) + x) + (i * d->w) + j) = pixel;
		}
	}
}

void zu4_img_draw_on(Image *d, Image *s, int x, int y) {
	// Draw an image onto another
	if (d == NULL) {
		d = zu4_img_get_screen();
	}
	
	for (int i = 0; i < s->h; i++) {
		for (int j = 0; j < s->w; j++) {
			zu4_img_set_pixel(d, x + j, y + i, zu4_img_get_pixel(s, j, i));
		}
	}
}

void zu4_img_draw(Image *s, int x, int y) {
	// Draw an image onto the main screen image
	zu4_img_draw_on(NULL, s, x, y);
}

void zu4_img_draw_subrect_on(Image *d, Image *s, int x, int y, int rx, int ry, int rw, int rh) {
	// Draw a portion of an image onto another
	if (d == NULL) {
		d = zu4_img_get_screen();
	}
	
	for (int i = 0; i < rh; i++) {
		for (int j = 0; j < rw; j++) {
			zu4_img_set_pixel(d, x + j, y + i, zu4_img_get_pixel(s, rx + j, ry + i));
		}
	}
}

void zu4_img_draw_subrect(Image *s, int x, int y, int rx, int ry, int rw, int rh) {
	// Draw a portion of an image onto the main screen image
	zu4_img_draw_subrect_on(NULL, s, x, y, rx, ry, rw, rh);
}

void zu4_img_draw_subrect_inv(Image *d, Image *s, int x, int y, int rx, int ry, int rw, int rh) {
	// Draw a portion of an image with inverted y coordinates
	if (d == NULL) {
		d = zu4_img_get_screen();
	}
	
	for (int i = 0; i < rh; i++) {
		for (int j = 0; j < rw; j++) {
			zu4_img_set_pixel(d, x + j, y + rh - 1 - i, zu4_img_get_pixel(s, rx + j, ry + i));
		}
	}
}

void zu4_img_draw_highlighted(Image *d) {
	// Draw an image with highlights
	uint32_t pixel;
	RGBA c;
	for (int i = 0; i < d->h; i++) {
		for (int j = 0; j < d->w; j++) {
			pixel = zu4_img_get_pixel(d, j, i);
			c.r = 0xff - (pixel & 0xff);
			c.g = 0xff - ((pixel & 0xff00) >> 8);
			c.b = 0xff - ((pixel & 0xff0000) >> 16);
			c.a = (pixel & 0xff000000) >> 24;
			pixel = c.a << 24 | c.b << 16 | c.g << 8 | c.r;
			zu4_img_set_pixel(d, j, i, pixel);
		}
	}
}
