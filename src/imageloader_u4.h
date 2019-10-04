#ifndef IMAGELOADER_U4_H
#define IMAGELOADER_U4_H

struct RGBA;

/**
 * Loader for U4 raw images.  Raw images are just an uncompressed
 * stream of pixel data with no palette information (e.g. shapes.ega,
 * charset.ega).  This loader handles the original 4-bit images, as
 * well as the 8-bit VGA upgrade images.
 */
Image* xu4_u4raw_load(U4FILE *file, int width, int height, int bpp);


/**
 * Loader for U4 images with RLE compression.  Like raw images, the
 * data is just a stream of pixel data with no palette information
 * (e.g. start.ega, rune_*.ega).  This loader handles the original
 * 4-bit images, as well as the 8-bit VGA upgrade images.
 */
Image* xu4_u4rle_load(U4FILE *file, int width, int height, int bpp);

/**
 * Loader for U4 images with LZW compression.  Like raw images, the
 * data is just a stream of pixel data with no palette information
 * (e.g. title.ega, tree.ega).  This loader handles the original 4-bit
 * images, as well as the 8-bit VGA upgrade images.
 */
Image* xu4_u4lzw_load(U4FILE *file, int width, int height, int bpp);

#endif /* IMAGELOADER_U4_H */
