/*
 * $Id: imageloader_u4.cpp 2931 2011-06-25 17:06:39Z twschulz $
 */


#include <vector>

#include "config.h"
#include "debug.h"
#include "error.h"
#include "image.h"
#include "imageloader.h"
#include "imageloader_u4.h"
#include "rle.h"
#include "lzw/u4decode.h"

using std::vector;

ImageLoader *U4RawImageLoader::instance = ImageLoader::registerLoader(new U4RawImageLoader, "image/x-u4raw");
ImageLoader *U4RleImageLoader::instance = ImageLoader::registerLoader(new U4RleImageLoader, "image/x-u4rle");
ImageLoader *U4LzwImageLoader::instance = ImageLoader::registerLoader(new U4LzwImageLoader, "image/x-u4lzw");

RGBA *U4PaletteLoader::bwPalette = NULL;
RGBA *U4PaletteLoader::egaPalette = NULL;
RGBA *U4PaletteLoader::vgaPalette = NULL;

/**
 * Loads in the raw image and apply the standard U4 16 or 256 color
 * palette.
 */
Image *U4RawImageLoader::load(U4FILE *file, int width, int height, int bpp) {
    if (width == -1 || height == -1 || bpp == -1) {
          xu4_error(XU4_LOG_ERR, "dimensions not set for u4raw image");
    }

    ASSERT(bpp == 1 || bpp == 4 || bpp == 8 || bpp == 24 || bpp == 32, "invalid bpp: %d", bpp);

    long rawLen = file->length();
    unsigned char *raw = (unsigned char *) malloc(rawLen);
    file->read(raw, 1, rawLen);

    long requiredLength = (width * height * bpp / 8);
    if (rawLen < requiredLength) {
        if (raw)
            free(raw);
        xu4_error(XU4_LOG_WRN, "u4Raw Image of size %ld does not fit anticipated size %ld", rawLen, requiredLength);
        return NULL;
    }

    Image *image = Image::create(width, height, bpp <= 8, Image::HARDWARE);
    if (!image) {
        if (raw)
            free(raw);
        return NULL;
    }

    U4PaletteLoader paletteLoader;
    if (bpp == 8)
        image->setPalette(paletteLoader.loadVgaPalette(), 256);
    else if (bpp == 4)
        image->setPalette(paletteLoader.loadEgaPalette(), 16);
    else if (bpp == 1)
        image->setPalette(paletteLoader.loadBWPalette(), 2);

    setFromRawData(image, width, height, bpp, raw);

    free(raw);

    return image;
}

/**
 * Loads in the rle-compressed image and apply the standard U4 16 or
 * 256 color palette.
 */
Image *U4RleImageLoader::load(U4FILE *file, int width, int height, int bpp) {
    if (width == -1 || height == -1 || bpp == -1) {
          xu4_error(XU4_LOG_ERR, "dimensions not set for u4rle image");
    }

    ASSERT(bpp == 1 || bpp == 4 || bpp == 8 || bpp == 24 || bpp == 32, "invalid bpp: %d", bpp);

    long compressedLen = file->length();
    unsigned char *compressed = (unsigned char *) malloc(compressedLen);
    file->read(compressed, 1, compressedLen);

    unsigned char *raw = NULL;
    long rawLen = rleDecompressMemory(compressed, compressedLen, (void **) &raw);
    free(compressed);

    if (rawLen != (width * height * bpp / 8)) {
        if (raw)
            free(raw);
        return NULL;
    }

    Image *image = Image::create(width, height, bpp <= 8, Image::HARDWARE);
    if (!image) {
        if (raw)
            free(raw);
        return NULL;
    }

    U4PaletteLoader paletteLoader;
    if (bpp == 8)
        image->setPalette(paletteLoader.loadVgaPalette(), 256);
    else if (bpp == 4)
        image->setPalette(paletteLoader.loadEgaPalette(), 16);
    else if (bpp == 1)
        image->setPalette(paletteLoader.loadBWPalette(), 2);

    setFromRawData(image, width, height, bpp, raw);

    free(raw);

    return image;
}

/**
 * Loads in the lzw-compressed image and apply the standard U4 16 or
 * 256 color palette.
 */
Image *U4LzwImageLoader::load(U4FILE *file, int width, int height, int bpp) {
    if (width == -1 || height == -1 || bpp == -1) {
          xu4_error(XU4_LOG_ERR, "dimensions not set for u4lzw image");
    }

    ASSERT(bpp == 1 || bpp == 4 || bpp == 8 || bpp == 24 || bpp == 32, "invalid bpp: %d", bpp);

    long compressedLen = file->length();
    unsigned char *compressed = (unsigned char *) malloc(compressedLen);
    file->read(compressed, 1, compressedLen);

    unsigned char *raw = NULL;
    long rawLen = decompress_u4_memory(compressed, compressedLen, (void **) &raw);
    free(compressed);

    if (rawLen != (width * height * bpp / 8)) {
        if (raw)
            free(raw);
        return NULL;
    }

    Image *image = Image::create(width, height, bpp <= 8, Image::HARDWARE);
    if (!image) {
        if (raw)
            free(raw);
        return NULL;
    }

    U4PaletteLoader paletteLoader;
    if (bpp == 8)
        image->setPalette(paletteLoader.loadVgaPalette(), 256);
    else if (bpp == 4)
        image->setPalette(paletteLoader.loadEgaPalette(), 16);
    else if (bpp == 1)
        image->setPalette(paletteLoader.loadBWPalette(), 2);

    setFromRawData(image, width, height, bpp, raw);

    free(raw);

    return image;
}

/**
 * Loads a simple black & white palette
 */
RGBA *U4PaletteLoader::loadBWPalette() {
    if (bwPalette == NULL) {
        bwPalette = new RGBA[2];

        bwPalette[0].r = 0;
        bwPalette[0].g = 0;
        bwPalette[0].b = 0;

        bwPalette[1].r = 255;
        bwPalette[1].g = 255;
        bwPalette[1].b = 255;

    }
    return bwPalette;
}

/**
 * Loads the basic EGA palette from egaPalette.xml
 */
RGBA *U4PaletteLoader::loadEgaPalette() {
    if (egaPalette == NULL) {
        int index = 0;
        const Config *config = Config::getInstance();
        
        egaPalette = new RGBA[16];

        vector<ConfigElement> paletteConf = config->getElement("egaPalette").getChildren();
        for (std::vector<ConfigElement>::iterator i = paletteConf.begin(); i != paletteConf.end(); i++) {

            if (i->getName() != "color")
                continue;
        
            egaPalette[index].r = i->getInt("red");
            egaPalette[index].g = i->getInt("green");
            egaPalette[index].b = i->getInt("blue");

            index++;
        }
    }
    return egaPalette;
}

/**
 * Load the 256 color VGA palette from a file.
 */
RGBA *U4PaletteLoader::loadVgaPalette() {
    if (vgaPalette == NULL) {
        U4FILE *pal = u4fopen("u4vga.pal");
        if (!pal)
            return NULL;

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
