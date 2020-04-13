#include "coords.h"

bool zu4_coords_equal(Coords a, Coords b) {
	return a.x == b.x && a.y == b.y && a.z == b.z;
}

Coords zu4_coords_nowhere() {
	return (Coords){-1, -1, -1};
}
