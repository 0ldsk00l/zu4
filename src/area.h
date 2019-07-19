/*
 * $Id: area.h 2475 2005-08-22 05:46:10Z andrewtaylor $
 */

#ifndef AREA_H
#define AREA_H

#include "map.h"

#define AREA_MONSTERS 16
#define AREA_PLAYERS 8

struct Area {
    MapCoords monster_start[AREA_MONSTERS];
    MapCoords player_start[AREA_PLAYERS];
};

#endif
