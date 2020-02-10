/*
 * imageloader_u4.cpp
 * Copyright (C) 2011 Andrew Taylor
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

#include "error.h"
#include "image.h"
#include "imageloader_u4.h"
#include "rle.h"
#include "lzw/u4decode.h"

using std::vector;

static RGBA *bwPalette = NULL;
static RGBA *egaPalette = NULL;
static RGBA *vgaPalette = NULL;

// Loads a simple black & white palette
static RGBA* loadBWPalette() {
    if (bwPalette == NULL) {
        bwPalette = new RGBA[2];
        bwPalette[0] = { 0, 0, 0, 255 };
        bwPalette[1] = { 255, 255, 255, 255 };
    }
    return bwPalette;
}

// http://upload.wikimedia.org/wikipedia/commons/d/df/EGA_Table.PNG
static RGBA* loadEgaPalette() {
    if (egaPalette == NULL) {
		egaPalette = new RGBA[16];
		egaPalette[0] = { 0, 0, 0, 255 }; // black
		egaPalette[1] = { 0, 0, 170, 255 }; // blue
		egaPalette[2] = { 0, 170, 0, 255 }; // green
		egaPalette[3] = { 0, 170, 170, 255 }; // green-blue
		egaPalette[4] = { 170, 0, 0, 255 }; // red
		egaPalette[5] = { 170, 0, 170, 255 }; // purple
		egaPalette[6] = { 170, 85, 0, 255 }; // brown
		egaPalette[7] = { 170, 170, 170, 255 }; // light grey
		egaPalette[8] = { 85, 85, 85, 255 }; // grey
		egaPalette[9] = { 85, 85, 255, 255 }; // bright blue
		egaPalette[10] = { 85, 255, 80, 255 }; // bright green
		egaPalette[11] = { 85, 255, 255, 255 }; // bright blue-green
		egaPalette[12] = { 255, 85, 85, 255 }; // bright red
		egaPalette[13] = { 255, 85, 255, 255 }; // magenta
		egaPalette[14] = { 255, 255, 85, 255 }; // yellow
		egaPalette[15] = { 255, 255, 255, 255 }; // white
	}
	return egaPalette;
}

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

static void setFromRawData(Image *image, int width, int height, int bpp, unsigned char *rawData) {
    int x, y;

    switch (bpp) {

    case 32:
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x++)
                image->putPixel(x, y, 
                                rawData[(y * width + x) * 4], 
                                rawData[(y * width + x) * 4 + 1], 
                                rawData[(y * width + x) * 4 + 2],
                                rawData[(y * width + x) * 4 + 3]);
        }
        break;

    case 24:
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x++)
                image->putPixel(x, y, 
                                rawData[(y * width + x) * 3], 
                                rawData[(y * width + x) * 3 + 1], 
                                rawData[(y * width + x) * 3 + 2],
                                IM_OPAQUE);
        }
        break;

    case 8:
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x++)
                image->putPixelIndex(x, y, rawData[y * width + x]);
        }
        break;

    case 4:
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x+=2) {
                image->putPixelIndex(x, y, rawData[(y * width + x) / 2] >> 4);
                image->putPixelIndex(x + 1, y, rawData[(y * width + x) / 2] & 0x0f);
            }
        }
        break;

    case 1:
        for (y = 0; y < height; y++) {
            for (x = 0; x < width; x+=8) {
                image->putPixelIndex(x + 0, y, (rawData[(y * width + x) / 8] >> 7) & 0x01);
                image->putPixelIndex(x + 1, y, (rawData[(y * width + x) / 8] >> 6) & 0x01);
                image->putPixelIndex(x + 2, y, (rawData[(y * width + x) / 8] >> 5) & 0x01);
                image->putPixelIndex(x + 3, y, (rawData[(y * width + x) / 8] >> 4) & 0x01);
                image->putPixelIndex(x + 4, y, (rawData[(y * width + x) / 8] >> 3) & 0x01);
                image->putPixelIndex(x + 5, y, (rawData[(y * width + x) / 8] >> 2) & 0x01);
                image->putPixelIndex(x + 6, y, (rawData[(y * width + x) / 8] >> 1) & 0x01);
                image->putPixelIndex(x + 7, y, (rawData[(y * width + x) / 8] >> 0) & 0x01);
            }
        }
        break;

    default:
        xu4_assert(0, "invalid bits-per-pixel (bpp): %d", bpp);
    }
}

// Load raw image and apply standard U4 16 or 256 color palette.
Image* xu4_u4raw_load(U4FILE *file, int width, int height, int bpp) {
    if (width == -1 || height == -1 || bpp == -1) {
          xu4_error(XU4_LOG_ERR, "dimensions not set for u4raw image");
    }

    xu4_assert(bpp == 1 || bpp == 4 || bpp == 8 || bpp == 24 || bpp == 32, "invalid bpp: %d", bpp);

    long rawLen = file->length();
    unsigned char *raw = (unsigned char *) malloc(rawLen);
    file->read(raw, 1, rawLen);

    long requiredLength = (width * height * bpp / 8);
    if (rawLen < requiredLength) {
        if (raw) { free(raw); }
        xu4_error(XU4_LOG_WRN, "u4Raw Image of size %ld does not fit anticipated size %ld", rawLen, requiredLength);
        return NULL;
    }

    Image *image = Image::create(width, height, bpp <= 8, Image::HARDWARE);
    if (!image) {
        if (raw) { free(raw); }
        return NULL;
    }

    if (bpp == 8) { image->setPalette(loadVgaPalette(), 256); }
    else if (bpp == 4) { image->setPalette(loadEgaPalette(), 16); }
    else if (bpp == 1) { image->setPalette(loadBWPalette(), 2); }

    setFromRawData(image, width, height, bpp, raw);

    free(raw);

    return image;
}

// Load rle-compressed image and apply standard U4 16 or 256 color palette
Image* xu4_u4rle_load(U4FILE *file, int width, int height, int bpp) {
    if (width == -1 || height == -1 || bpp == -1) {
          xu4_error(XU4_LOG_ERR, "dimensions not set for u4rle image");
    }

    xu4_assert(bpp == 1 || bpp == 4 || bpp == 8 || bpp == 24 || bpp == 32, "invalid bpp: %d", bpp);

    long compressedLen = file->length();
    unsigned char *compressed = (unsigned char *) malloc(compressedLen);
    file->read(compressed, 1, compressedLen);

    unsigned char *raw = NULL;
    long rawLen = rleDecompressMemory(compressed, compressedLen, (void **) &raw);
    free(compressed);

    if (rawLen != (width * height * bpp / 8)) {
        if (raw) { free(raw); }
        return NULL;
    }

    Image *image = Image::create(width, height, bpp <= 8, Image::HARDWARE);
    if (!image) {
        if (raw) { free(raw); }
        return NULL;
    }

    if (bpp == 8) { image->setPalette(loadVgaPalette(), 256); }
    else if (bpp == 4) { image->setPalette(loadEgaPalette(), 16); }
    else if (bpp == 1) { image->setPalette(loadBWPalette(), 2); }

    setFromRawData(image, width, height, bpp, raw);

    free(raw);

    return image;
}

// Load lzw-compressed image and apply standard U4 16 or 256 color palette
Image* xu4_u4lzw_load(U4FILE *file, int width, int height, int bpp) {
    if (width == -1 || height == -1 || bpp == -1) {
          xu4_error(XU4_LOG_ERR, "dimensions not set for u4lzw image");
    }

    xu4_assert(bpp == 1 || bpp == 4 || bpp == 8 || bpp == 24 || bpp == 32, "invalid bpp: %d", bpp);

    long compressedLen = file->length();
    unsigned char *compressed = (unsigned char *) malloc(compressedLen);
    file->read(compressed, 1, compressedLen);

    unsigned char *raw = NULL;
    long rawLen = decompress_u4_memory(compressed, compressedLen, (void **) &raw);
    free(compressed);

    if (rawLen != (width * height * bpp / 8)) {
        if (raw) { free(raw); }
        return NULL;
    }

    Image *image = Image::create(width, height, bpp <= 8, Image::HARDWARE);
    if (!image) {
        if (raw) { free(raw); }
        return NULL;
    }

    if (bpp == 8) { image->setPalette(loadVgaPalette(), 256); }
    else if (bpp == 4) { image->setPalette(loadEgaPalette(), 16); }
    else if (bpp == 1) { image->setPalette(loadBWPalette(), 2); }

    setFromRawData(image, width, height, bpp, raw);

    free(raw);

    return image;
}
