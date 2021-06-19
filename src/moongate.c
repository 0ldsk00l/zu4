#include "coords.h"
#include "moongate.h"

static Coords gates[8] = {};

void moongateAdd(int phase, Coords coords) {
	gates[phase] = coords;
}

Coords *moongateGetGateCoordsForPhase(int phase) {
	return &gates[phase];
}

bool moongateFindActiveGateAt(int trammel, int felucca, Coords src, Coords *dest) {
	Coords *mg_coords = moongateGetGateCoordsForPhase(trammel);
	if (src.x == mg_coords->x && src.y == mg_coords->y && src.z == mg_coords->z) {
		mg_coords = moongateGetGateCoordsForPhase(felucca);
		*dest = *mg_coords;
		return true;
	}
	return false;
}

bool moongateIsEntryToShrineOfSpirituality(int trammel, int felucca) {
    return trammel == 4 && felucca == 4;
}
