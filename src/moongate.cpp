#include "coords.h"
#include "moongate.h"

static Coords gates[8] = {};

void moongateAdd(int phase, const Coords &coords) {
	gates[phase] = coords;
}

const Coords *moongateGetGateCoordsForPhase(int phase) {
	return &gates[phase];
}

bool moongateFindActiveGateAt(int trammel, int felucca, const Coords &src, Coords &dest) {
	const Coords *mg_coords;
	mg_coords = moongateGetGateCoordsForPhase(trammel);
	if (src.x == mg_coords->x && src.y == mg_coords->y && src.z == mg_coords->z) {
		mg_coords = moongateGetGateCoordsForPhase(felucca);
		dest = *mg_coords;
		return true;
	}
	return false;
}

bool moongateIsEntryToShrineOfSpirituality(int trammel, int felucca) {
    return trammel == 4 && felucca == 4;
}
