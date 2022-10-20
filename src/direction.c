/*
 * direction.c
 * Copyright (C) 2009 Andrew Taylor
 * Copyright (C) 2019 R. Danbrook
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 * 
 */

#include "direction.h"
#include "error.h"
#include "event.h"
#include "random.h"

Direction dirReverse(Direction dir) {
	// Returns the opposite direction.
	switch (dir) {
		case DIR_NONE: return DIR_NONE;
		case DIR_WEST: return DIR_EAST;
		case DIR_NORTH: return DIR_SOUTH;
		case DIR_EAST: return DIR_WEST;
		case DIR_SOUTH: return DIR_NORTH;
		case DIR_ADVANCE:
		case DIR_RETREAT:
		default: break;
	}

	zu4_error(ZU4_LOG_ERR, "invalid direction: %d", dir);
	return DIR_NONE;
}

Direction dirFromMask(int dir_mask) {
	if (dir_mask & MASK_DIR_NORTH) return DIR_NORTH;
	else if (dir_mask & MASK_DIR_EAST) return DIR_EAST;
	else if (dir_mask & MASK_DIR_SOUTH) return DIR_SOUTH;
	else if (dir_mask & MASK_DIR_WEST) return DIR_WEST;
	return DIR_NONE;
}

Direction dirRotateCW(Direction dir) {
	dir = (Direction)(dir + 1);
	if (dir > DIR_SOUTH) { dir = DIR_WEST; }
	return dir;
}

Direction dirRotateCCW(Direction dir) {
	dir = (Direction)(dir - 1);
	if (dir < DIR_WEST) { dir = DIR_SOUTH; }
	return dir;
}

int dirGetBroadsidesDirs(Direction dir) {
	// Returns the a mask containing the broadsides directions for a
	// given direction. For instance, dirGetBroadsidesDirs(DIR_NORTH)
	// returns: (MASK_DIR(DIR_WEST) | MASK_DIR(DIR_EAST))
	int dirmask = MASK_DIR_ALL;
	dirmask = DIR_REMOVE_FROM_MASK(dir, dirmask);
	dirmask = DIR_REMOVE_FROM_MASK(dirReverse(dir), dirmask);
	return dirmask;
}

Direction dirRandomDir(int valid_directions_mask) {
	// Returns random direction from mask of available directions
	int n = 0;
	Direction d[4];
	
	for (int i = DIR_WEST; i <= DIR_SOUTH; i++) {
		if (DIR_IN_MASK(i, valid_directions_mask)) {
			d[n] = (Direction)(i);
			n++;
		}
	}
	
	return !n ? DIR_NONE : d[zu4_random(n)];
}

Direction dirNormalize(Direction orientation, Direction dir) {
	// Normalizes the direction based on the orientation given
	// (if facing west, and 'up' is pressed, the 'up' is translated
	// into DIR_NORTH -- this function tranlates that direction 
	// to DIR_WEST, the correct direction in this case).
	Direction temp = orientation;
	Direction realDir = dir;
	
	while (temp != DIR_NORTH) {        
		temp = dirRotateCW(temp);
		realDir = dirRotateCCW(realDir);
	}
	
	return realDir;
}

Direction keyToDirection(int key) {        
	// Translates a keyboard code into a direction
	switch (key) {
		case U4_UP: return DIR_NORTH;
		case U4_DOWN: return DIR_SOUTH;
		case U4_LEFT: return DIR_WEST;
		case U4_RIGHT: return DIR_EAST;
		default: return DIR_NONE;
	}    
}
