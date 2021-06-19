/*
 * $Id: moongate.h 1879 2004-05-21 04:03:31Z andrewtaylor $
 */

#ifndef MOONGATE_H
#define MOONGATE_H

#ifdef __cplusplus
extern "C" {
#endif

void moongateAdd(int phase, Coords coords);
Coords *moongateGetGateCoordsForPhase(int phase);
bool moongateFindActiveGateAt(int trammel, int felucca, Coords src, Coords *dest);
bool moongateIsEntryToShrineOfSpirituality(int trammel, int felucca);

#ifdef __cplusplus
}
#endif

#endif
