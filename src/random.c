/*
 * random.c
 * Copyright (C) 2014 Darren Janeczek
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

#include <stdlib.h>
#include <time.h>

/**
 * Seed the random number generator.
 */
void xu4_srandom() {
#if (defined(BSD) && (BSD >= 199103))
	srandom(time(NULL));
#else
	srand((unsigned int)time(NULL));
#endif
}

/**
 * Generate a random number between 0 and (upperRange - 1).  This
 * routine uses the upper bits of the random number provided by rand()
 * to compensate for older generators that have low entropy in the
 * lower bits (e.g. MacOS X).
 */
int xu4_random(int upperRange) {
#if (defined(BSD) && (BSD >= 199103))
	int r = random();
#else
	int r = rand();
#endif
	return (int) ((((double)upperRange) * r) / (RAND_MAX+1.0));
}
