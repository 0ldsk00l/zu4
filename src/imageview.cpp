/*
 * $Id: imageview.cpp 2671 2006-09-04 18:56:09Z solus $
 */

#include <stdint.h>

#include "error.h"
#include "image.h"
#include "imagemgr.h"
#include "imageview.h"
#include "settings.h"

ImageView::ImageView(int x, int y, int width, int height) : View(x, y, width, height) {
}

ImageView::~ImageView() {
}

/**
 * Draw the image at the optionally specified offset.
 */
void ImageView::draw(const string &imageName, int x, int y) {
    ImageInfo *info = imageMgr->get(imageName);
    if (info) {
        zu4_img_draw(info->image, this->x + x, this->y + y);
        return;
    }

    SubImage *subimage = imageMgr->getSubImage(imageName);
    if (subimage) {
        info = imageMgr->get(subimage->srcImageName);

        if (info) {
            zu4_img_draw_subrect(info->image, this->x + x, this->y + y,
                                     subimage->x,
                                     subimage->y,
                                     subimage->width,
                                     subimage->height);
            return;
        }
    }
    zu4_error(ZU4_LOG_ERR, "ERROR 1005: Unable to load the image \"%s\".\t\n\nIs Ultima IV installed?\n\nVisit the XU4 website for additional information.\n\thttp://xu4.sourceforge.net/", imageName.c_str());
}
