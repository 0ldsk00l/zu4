/*
 * $Id: context.h 3071 2014-07-26 18:01:08Z darren_janeczek $
 */

#ifndef CONTEXT_H
#define CONTEXT_H

#include <vector>

#include "location.h"
#include "aura.h"
#include "names.h"
#include "person.h"
#include "types.h"
#include "savegame.h"

class Object;
class Party;
class Person;
class Script;
class StatsArea;

typedef enum {
    TRANSPORT_FOOT      = 0x1,
    TRANSPORT_HORSE     = 0x2,
    TRANSPORT_SHIP      = 0x4,
    TRANSPORT_BALLOON		= 0x8,
    TRANSPORT_FOOT_OR_HORSE	= TRANSPORT_FOOT | TRANSPORT_HORSE,
    TRANSPORT_ANY			= 0xffff
} TransportContext;

typedef struct Context {
    Party *party;
    SaveGame *saveGame;
    class Location *location;
    int line, col;
    StatsArea *stats;
    int moonPhase;
    int windDirection;
    int windCounter;
    bool windLock;
    Aura *aura;    
    int horseSpeed;
    int opacity;
    TransportContext transportContext;
    time_t lastCommandTime;
    class Object *lastShip;
} Context;

extern Context *c;

Context *zu4_ctx_init();
void zu4_ctx_deinit();

#endif
