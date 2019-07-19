/*
 * $Id: scale.h 2476 2005-08-22 05:53:38Z andrewtaylor $
 */

#ifndef SCALE_H
#define SCALE_H

#include <string>

#include "settings.h"

class Image;

typedef Image *(*Scaler)(Image *src, int scale, int n);

Scaler scalerGet(const std::string &filter);
int scaler3x(const std::string &filter);

#endif /* SCALE_H */
