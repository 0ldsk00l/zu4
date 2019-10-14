/*
 * $Id: scale.cpp 2674 2006-12-20 08:25:50Z solus $
 */

#include "image.h"
#include "scale.h"

using std::string;

Image *scalePoint(Image *src, int scale, int n);

Scaler scalerGet(const string &filter) {
	return &scalePoint;
}

/**
 * A simple row and column duplicating scaler.
 */
Image *scalePoint(Image *src, int scale, int n) {
    int x, y, i, j;
    Image *dest;

    dest = Image::create(src->width() * scale, src->height() * scale, src->isIndexed(), Image::HARDWARE);
    if (!dest)
        return NULL;

    if (dest->isIndexed())
        dest->setPaletteFromImage(src);

    for (y = 0; y < src->height(); y++) {
        for (x = 0; x < src->width(); x++) {
            for (i = 0; i < scale; i++) {
                for (j = 0; j < scale; j++) {
                    unsigned int index;
                    src->getPixelIndex(x, y, index);
                    dest->putPixelIndex(x * scale + j, y * scale + i, index);
                }
            }
        }
    }

    return dest;
}
