/*
 * $Id: names.h 2475 2005-08-22 05:46:10Z andrewtaylor $
 */

#ifndef NAMES_H
#define NAMES_H

#include "savegame.h"
#include "direction.h"

/*
 * These routines convert the various enumerations for classes, reagents,
 * etc. into the textual representations used in the game.
 */
const char *getClassName(ClassType klass);
const char *getReagentName(Reagent reagent);
const char *getVirtueName(Virtue virtue);
const char *getBaseVirtueName(int virtueMask);
int getBaseVirtues(Virtue virtue);
const char *getVirtueAdjective(Virtue virtue);
const char *getStoneName(Virtue virtue);
const char *getItemName(Item item);
const char *getDirectionName(Direction dir);

#endif
