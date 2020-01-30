/*
 * $Id: coords.h 1855 2004-05-17 21:17:32Z andrewtaylor $
 */

#ifndef COORDS_H
#define COORDS_H

class Coords {
public:
    int x, y, z;
    Coords(int initx = 0, int inity = 0, int initz = 0) : x(initx), y(inity), z(initz) {}
};

bool xu4_coords_equal(Coords a, Coords b);

#endif /* COORDS_H */
