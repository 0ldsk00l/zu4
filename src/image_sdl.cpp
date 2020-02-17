/*
 * $Id: image_sdl.cpp 3066 2014-07-21 00:18:48Z darren_janeczek $
 */

#include "image.h"
#include "settings.h"
#include "error.h"

/**
 * Creates a new image.  Scale is stored to allow drawing using U4
 * (320x200) coordinates, regardless of the actual image scale.
 * Indexed is true for palette based images, or false for RGB images.
 * Image type determines whether to create a hardware (i.e. video ram)
 * or software (i.e. normal ram) image.
 */
Image *Image::create(int w, int h) {
    Uint32 rmask, gmask, bmask, amask;
    Uint32 flags;
    Image *im = new Image;

    im->w = w;
    im->h = h;

#if SDL_BYTEORDER == SDL_BIG_ENDIAN
    rmask = 0xff000000;
    gmask = 0x00ff0000;
    bmask = 0x0000ff00;
    amask = 0x000000ff;
#else
    rmask = 0x000000ff;
    gmask = 0x0000ff00;
    bmask = 0x00ff0000;
    amask = 0xff000000;
#endif

    flags = SDL_SWSURFACE | SDL_SRCALPHA;

    im->surface = SDL_CreateRGBSurface(flags, w, h, 32, rmask, gmask, bmask, amask);

    if (!im->surface) {
        delete im;
        return NULL;
    }

    return im;
}

/**
 * Create a special purpose image the represents the whole screen.
 */
Image *Image::createScreenImage() {
    Image *screen = new Image();

    screen->surface = SDL_GetVideoSurface();
    xu4_assert(screen->surface != NULL, "SDL_GetVideoSurface() returned a NULL screen surface!");
    screen->w = screen->surface->w;
    screen->h = screen->surface->h;
    return screen;
}

/**
 * Creates a duplicate of another image
 */
Image *Image::duplicate(Image *image) {    
    bool alphaOn = image->isAlphaOn();
    Image *im = create(image->w, image->h);

    /* Turn alpha off before blitting to non-screen surfaces */
    if (alphaOn)
        image->alphaOff();
    
    image->drawOn(im, 0, 0);

    if (alphaOn)
        image->alphaOn();

    im->backgroundColor = image->backgroundColor;

    return im;
}

/**
 * Frees the image.
 */
Image::~Image() {
    SDL_FreeSurface(surface);
}

void Image::initializeToBackgroundColor(RGBA backgroundColor)
{
	this->backgroundColor = backgroundColor;
    this->fillRect(0,0,this->w,this->h,
    		backgroundColor.r,
    		backgroundColor.g,
    		backgroundColor.b,
    		backgroundColor.a);
}

bool Image::isAlphaOn() const
{
    return surface->flags & SDL_SRCALPHA;
}

void Image::alphaOn()
{
    surface->flags |= SDL_SRCALPHA;
}

void Image::alphaOff()
{
    surface->flags &= ~SDL_SRCALPHA;
}

/**
 * Sets the palette index of a single pixel.  If the image is in
 * indexed mode, then the index is simply the palette entry number.
 * If the image is RGB, it is a packed RGB triplet.
 */
void Image::putPixel(int x, int y, uint32_t value) {
    *((uint32_t*)(surface->pixels) + (y * w) + x) = value;
}

/**
 * Fills a rectangle in the image with a given color.
 */
void Image::fillRect(int x, int y, int w, int h, int r, int g, int b, int a) {
    SDL_Rect dest;
    uint32_t pixel;

    pixel = SDL_MapRGBA(surface->format, static_cast<Uint8>(r), static_cast<Uint8>(g), static_cast<Uint8>(b), static_cast<Uint8>(a));
    dest.x = x;
    dest.y = y;
    dest.w = w;
    dest.h = h;

    SDL_FillRect(surface, &dest, pixel);
}

/**
 * Gets the color of a single pixel.
 */
void Image::getPixel(int x, int y, unsigned int &r, unsigned int &g, unsigned int &b, unsigned int &a) const {
    unsigned int index;
    Uint8 r1, g1, b1, a1;

    getPixelIndex(x, y, index);

    SDL_GetRGBA(index, surface->format, &r1, &g1, &b1, &a1);
    r = r1;
    g = g1;
    b = b1;
    a = a1;
}

/**
 * Gets the palette index of a single pixel.  If the image is in
 * indexed mode, then the index is simply the palette entry number.
 * If the image is RGB, it is a packed RGB triplet.
 */
void Image::getPixelIndex(int x, int y, unsigned int &index) const {
    Uint8 *p = static_cast<Uint8 *>(surface->pixels) + y * surface->pitch + x * sizeof(uint32_t);
    index = *reinterpret_cast<Uint32 *>(p);
}

/**
 * Draws the image onto another image.
 */
void Image::drawOn(Image *d, int x, int y) const {
    SDL_Rect r;
    SDL_Surface *destSurface;

    if (d == NULL)
        destSurface = SDL_GetVideoSurface();
    else
        destSurface = d->surface;

    r.x = x;
    r.y = y;
    r.w = w;
    r.h = h;
    SDL_BlitSurface(surface, NULL, destSurface, &r);
}

/**
 * Draws a piece of the image onto another image.
 */
void Image::drawSubRectOn(Image *d, int x, int y, int rx, int ry, int rw, int rh) const {
    SDL_Rect src, dest;
    SDL_Surface *destSurface;

    if (d == NULL)
        destSurface = SDL_GetVideoSurface();
    else
        destSurface = d->surface;

    src.x = rx;
    src.y = ry;
    src.w = rw;
    src.h = rh;

    dest.x = x;
    dest.y = y;
    /* dest w & h unused */

    SDL_BlitSurface(surface, &src, destSurface, &dest);
}

/**
 * Draws a piece of the image onto another image, inverted.
 */
void Image::drawSubRectInvertedOn(Image *d, int x, int y, int rx, int ry, int rw, int rh) const {
    int i;
    SDL_Rect src, dest;
    SDL_Surface *destSurface;

    if (d == NULL)
        destSurface = SDL_GetVideoSurface();
    else
        destSurface = d->surface;

    for (i = 0; i < rh; i++) {
        src.x = rx;
        src.y = ry + i;
        src.w = rw;
        src.h = 1;

        dest.x = x;
        dest.y = y + rh - i - 1;
        /* dest w & h unused */

        SDL_BlitSurface(surface, &src, destSurface, &dest);
    }
}

// FIXME: drawHighlighted needs to be reimplemented
