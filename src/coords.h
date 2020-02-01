#ifndef COORDS_H
#define COORDS_H

#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>

typedef struct Coords {
	int x;
	int y;
	int z;
} Coords;

bool xu4_coords_equal(Coords a, Coords b);
Coords xu4_coords_nowhere();

#ifdef __cplusplus
}
#endif

#endif
