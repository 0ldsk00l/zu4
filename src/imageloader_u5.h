/*
 * $Id: imageloader_u5.h 2885 2011-04-03 19:18:32Z andrewtaylor $
 */

#ifndef IMAGELOADER_U5_H
#define IMAGELOADER_U5_H

#include "imageloader.h"

/**
 * Loader for U5 images with LZW compression.  Similar to U4 LZW
 * images, but with a slightly different variation on the LZW
 * algorithm.
 */
class U5LzwImageLoader : public ImageLoader {
    static ImageLoader *instance;

public:
    virtual Image *load(U4FILE *file, int width, int height, int bpp);
    
};

#endif /* IMAGELOADER_U5_H */
