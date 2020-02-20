#ifndef IMAGE_H
#define IMAGE_H

#ifdef __cplusplus
extern "C" {
#endif

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200

#define IM_OPAQUE (unsigned int) 255
#define IM_TRANSPARENT 0

typedef struct xu4_vsurface_t {
	int w;
	int h;
	void *pixels;
} xu4_vsurface_t;

typedef struct RGBA {
    uint8_t r, g, b, a;
} RGBA;

typedef struct SubImage {
    char name[32];
    char srcImageName[32];
    int x, y, width, height;
} SubImage;

typedef struct Image {
    int w, h;
    xu4_vsurface_t *surface;
} Image;

Image* xu4_img_create(int w, int h);
Image* xu4_img_create_screen();
Image* xu4_img_dup(Image *image);
Image* xu4_img_get_screen();
void xu4_img_free(Image *image);

uint32_t xu4_img_get_pixel(Image *s, int x, int y);
void xu4_img_set_pixel(Image *d, int x, int y, uint32_t value);

void xu4_img_fill(Image *d, int x, int y, int width, int height, int r, int g, int b, int a);

void xu4_img_draw_on(Image *d, Image *s, int x, int y);
void xu4_img_draw(Image *s, int x, int y);

void xu4_img_draw_subrect_on(Image *d, Image *s, int x, int y, int rx, int ry, int rw, int rh);
void xu4_img_draw_subrect(Image *s, int x, int y, int rx, int ry, int rw, int rh);

void xu4_img_draw_subrect_inv(Image *d, Image *s, int x, int y, int rx, int ry, int rw, int rh);

void xu4_img_draw_highlighted(Image *d);

#ifdef __cplusplus
}
#endif

#endif
