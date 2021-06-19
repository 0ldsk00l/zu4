#ifndef IMAGELOADER_H
#define IMAGELOADER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "u4file.h"

enum ImageType {
	ZU4_IMG_RAW,
	ZU4_IMG_RLE,
	ZU4_IMG_LZW,
	ZU4_IMG_PNG,
};

/**
 * Loader for U4 raw images.  Raw images are just an uncompressed
 * stream of pixel data with no palette information (e.g. shapes.ega,
 * charset.ega).  This loader handles the original 4-bit images, as
 * well as the 8-bit VGA upgrade images. This handles RLE and LZW as well.
 */
Image* zu4_img_load(U4FILE *file, int width, int height, int bpp, int type);

Image* zu4_png_load(const char *filename, int *x, int *y);

#ifdef __cplusplus
}
#endif

#endif
