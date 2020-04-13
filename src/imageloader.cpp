/*
 * imageloader.cpp
 * Copyright (C) 2011 Andrew Taylor
 * Copyright (C) 2011 Darren Janeczek
 * Copyright (C) 2019-2020 R. Danbrook
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
#include "imageloader.h"
#include "rle.h"
#include "lzw/u4decode.h"

static RGBA *vgaPalette = NULL;

// Load the 256 color VGA palette from a file.
static RGBA* loadVgaPalette() {
    if (vgaPalette == NULL) {
        U4FILE *pal = u4fopen("u4vga.pal");
        if (!pal) { return NULL; }

        vgaPalette = new RGBA[256];

        for (int i = 0; i < 256; i++) {
            vgaPalette[i].r = u4fgetc(pal) * 255 / 63;
            vgaPalette[i].g = u4fgetc(pal) * 255 / 63;
            vgaPalette[i].b = u4fgetc(pal) * 255 / 63;
        }
        u4fclose(pal);

    }
    return vgaPalette;
}

static void zu4_u4raw_ega_conv(int w, int h, uint8_t *in, uint32_t *out, bool paletteswap) {
	// http://upload.wikimedia.org/wikipedia/commons/d/df/EGA_Table.PNG
	uint32_t palette_rgba[16] = {
		0x000000ff, 0x0000aaff, 0x00aa00ff, 0x00aaaaff,
		0xaa0000ff, 0xaa00aaff, 0xaa5500ff, 0xaaaaaaff,
		0x555555ff, 0x5555ffff, 0x55ff55ff, 0x55ffffff,
		0xff5555ff, 0xff55ffff, 0xffff55ff, 0xffffffff,
	};
	
	uint32_t palette_abgr[16] = {
		0xff000000, 0xffaa0000, 0xff00aa00, 0xffaaaa00,
		0xff0000aa, 0xffaa00aa, 0xff0055aa, 0xffaaaaaa,
		0xff555555, 0xffff5555, 0xff55ff55, 0xffffff55,
		0xff5555ff, 0xffff55ff, 0xff55ffff, 0xffffffff,
	};
	
	uint32_t *palette = paletteswap ? palette_abgr : palette_rgba;
	
	for (int i = 0; i < (w * h) / 2; i++) {
		out[i * 2] = palette[(in[i] >> 4) & 0xf];
		out[(i * 2) + 1] = palette[in[i] & 0xf];
	}
}

static void zu4_u4raw_vga_conv(int w, int h, uint8_t *in, uint32_t *out, bool paletteswap) {
	RGBA *pp = loadVgaPalette();
	if (paletteswap) { // ABGR
		for (int i = 0; i < (w * h); i++) {
			out[i] = 0xff000000 | pp[in[i] & 0xff].b << 16 |pp[in[i] & 0xff].g << 8 | pp[in[i] & 0xff].r;
		}
	}
	else { // RGBA
		for (int i = 0; i < (w * h); i++) {
			out[i] =  pp[in[i] & 0xff].r << 24 |pp[in[i] & 0xff].g << 16 | pp[in[i] & 0xff].b << 8 | 0xff;
		}
	}
}

Image* zu4_img_load(U4FILE *file, int width, int height, int bpp, int type) {
	if (width == -1 || height == -1 || bpp == -1) {
		  zu4_error(ZU4_LOG_ERR, "dimensions not set for image");
	}

	zu4_assert(bpp == 4 || bpp == 8, "invalid bpp: %d", bpp);
	
	uint8_t *raw = NULL;
	uint8_t *compressed = NULL;
	uint32_t *converted = (uint32_t*)malloc(width * height * sizeof(uint32_t));
	
	long rawLen;
	long compressedLen;
	
	if (type == ZU4_IMG_RAW) {
		rawLen = u4flength(file);
		raw = (uint8_t*)malloc(rawLen);
		u4fread(file, raw, 1, rawLen);
	}
	else if (type == ZU4_IMG_RLE) {
		compressedLen = u4flength(file);
		compressed = (uint8_t*)malloc(compressedLen);
		u4fread(file, compressed, 1, compressedLen);
		rawLen = rleDecompressMemory(compressed, compressedLen, (void **) &raw);
		free(compressed);
	}
	else if (type == ZU4_IMG_LZW) {
		compressedLen = u4flength(file);
		compressed = (uint8_t*)malloc(compressedLen);
		u4fread(file, compressed, 1, compressedLen);
		rawLen = decompress_u4_memory(compressed, compressedLen, (void **) &raw);
		free(compressed);
	}
	
	if (rawLen != (width * height * bpp / 8)) {
		if (raw) { free(raw); }
		return NULL;
	}
	
	Image *image = zu4_img_create(width, height);
	if (!image) {
		if (raw) { free(raw); }
		if (converted) { free(converted); }
		return NULL;
	}
	
	if (bpp == 8) { // VGA
		zu4_u4raw_vga_conv(width, height, raw, converted, true);
		memcpy((uint32_t*)image->pixels, converted, sizeof(uint32_t) * width * height);
	}
	else if (bpp == 4) { // EGA
		zu4_u4raw_ega_conv(width, height, raw, converted, true);
		memcpy((uint32_t*)image->pixels, converted, sizeof(uint32_t) * width * height);
	}
	
	free(raw);
	free(converted);
	
	return image;
}

Image* zu4_png_load(const char *filename, int *x, int *y) {
	uint8_t *pixels = stbi_load(filename, x, y, NULL, STBI_rgb_alpha);
	Image *image = zu4_img_create(*x, *y);
	memcpy((uint32_t*)image->pixels, (uint32_t*)pixels, sizeof(uint32_t) * *x * *y);
	stbi_image_free(pixels);
	return image;
}
