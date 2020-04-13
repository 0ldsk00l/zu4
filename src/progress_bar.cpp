/*
 * $Id: progress_bar.cpp 2679 2007-01-01 20:00:21Z solus $
 */

#include <stdint.h>

#include "progress_bar.h"

#include "image.h"
#include "settings.h"

ProgressBar::ProgressBar(int x, int y, int width, int height, int _min, int _max) :
    View(x, y, width, height),
    min(_min),
    max(_max) {
    current = min;
}

ProgressBar& ProgressBar::operator++()  { current++; draw(); return *this; }
ProgressBar& ProgressBar::operator--()  { current--; draw(); return *this; }
void ProgressBar::draw() {
    Image *bar = zu4_img_create(width, height);
    int pos = static_cast<int>((double(current - min) / double(max - min)) * (width - (bwidth * 2)));

    // border color
    zu4_img_fill(bar, 0, 0, width, height, bcolor.r, bcolor.g, bcolor.b, 255); 

    // color
    zu4_img_fill(bar, bwidth, bwidth, pos, (height - (bwidth * 2)), color.r, color.g, color.b, 255); 

    zu4_img_draw_on(screen, bar, x, y);
    update();

    zu4_img_free(bar);
}

void ProgressBar::setBorderColor(int r, int g, int b, int a) {
    bcolor.r = r;
    bcolor.g = g;
    bcolor.b = b;
    bcolor.a = a;
}

void ProgressBar::setBorderWidth(unsigned int width) {
    bwidth = width;
}

void ProgressBar::setColor(int r, int g, int b, int a) {
    color.r = r;
    color.g = g;
    color.b = b;
    color.a = a;
}
