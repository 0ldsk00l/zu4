/*
 * $Id: image.h 3032 2012-05-18 19:39:47Z twschulz $
 */

#ifndef IMAGE_H
#define IMAGE_H

#include <string>
#include <stdint.h>
#include "types.h"
#include "u4file.h"
#include "textcolor.h"

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200

using std::string;

typedef struct xu4_vsurface_t {
	int w;
	int h;
	void *pixels;
} xu4_vsurface_t;

typedef struct RGBA {
    uint8_t r, g, b, a;
} RGBA;

typedef struct SubImage {
    string name;
    string srcImageName;
    int x, y, width, height;
} SubImage;

#define IM_OPAQUE (unsigned int) 255
#define IM_TRANSPARENT 0

class Image {
public:
    static Image *create(int w, int h);
    static Image *createScreenImage();
    static Image *duplicate(Image *image);
    ~Image();

    int w, h;
    xu4_vsurface_t *surface;
};

Image* xu4_img_get_screen();

uint32_t xu4_img_get_pixel(Image *s, int x, int y);
void xu4_img_set_pixel(Image *d, int x, int y, uint32_t value);

void xu4_img_fill(Image *d, int x, int y, int width, int height, int r, int g, int b, int a);

void xu4_img_draw_on(Image *d, Image *s, int x, int y);
void xu4_img_draw(Image *s, int x, int y);

void xu4_img_draw_subrect_on(Image *d, Image *s, int x, int y, int rx, int ry, int rw, int rh);
void xu4_img_draw_subrect(Image *s, int x, int y, int rx, int ry, int rw, int rh);

void xu4_img_draw_subrect_inv(Image *d, Image *s, int x, int y, int rx, int ry, int rw, int rh);

void xu4_img_draw_highlighted(Image *d);

#endif
