#include "coords.h"

bool xu4_coords_equal(Coords a, Coords b) {
	return a.x == b.x && a.y == b.y && a.z == b.z;
}

Coords xu4_coords_nowhere() {
	return (Coords){-1, -1, -1};
}
