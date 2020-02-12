/*
 * imageloader_png.cpp
 * Copyright (C) 2011 Darren Janeczek
 * Copyright (C) 2019 R. Danbrook
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
#include <stdio.h>
#include <stdlib.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include "error.h"
#include "image.h"
#include "imageloader_png.h"

static void setFromRawData(Image *image, int width, int height, unsigned char *rawData) {
    int x, y;
	for (y = 0; y < height; y++) {
		for (x = 0; x < width; x++)
			image->putPixel(x, y, 
				rawData[(y * width + x) * 4], 
				rawData[(y * width + x) * 4 + 1], 
				rawData[(y * width + x) * 4 + 2],
				rawData[(y * width + x) * 4 + 3]
			);
	}
}

Image* xu4_png_load(const char *filename, int *x, int *y) {
	unsigned char *pixels = stbi_load(filename, x, y, NULL, STBI_rgb_alpha);
	Image *image = Image::create(*x, *y, 0);
	setFromRawData(image, *x, *y, pixels);
	stbi_image_free(pixels);
	return image;
}
