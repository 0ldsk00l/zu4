/*
 * $Id: portal.h 2077 2004-08-19 07:33:19Z andrewtaylor $
 */

#ifndef PORTAL_H
#define PORTAL_H

#include <string.h>
#include "context.h"
#include "map.h"

struct Map;
struct Location;
struct _Portal;

typedef enum {
    ACTION_NONE         = 0x0,
    ACTION_ENTER        = 0x1,
    ACTION_KLIMB        = 0x2,
    ACTION_DESCEND      = 0x4,
    ACTION_EXIT_NORTH   = 0x8,
    ACTION_EXIT_EAST    = 0x10,
    ACTION_EXIT_SOUTH   = 0x20,
    ACTION_EXIT_WEST    = 0x40
} PortalTriggerAction;

typedef bool (*PortalConditionsMet)(const Portal *p);

typedef struct _PortalDestination {
    Coords coords;
    MapId mapid;
} PortalDestination;

struct Portal {
    Coords coords;
    MapId destid;
    Coords start;    
    PortalTriggerAction trigger_action;
    PortalConditionsMet portalConditionsMet;
    struct _PortalDestination *retroActiveDest;
    bool saveLocation;
    std::string message;
    TransportContext portalTransportRequisites;
    bool exitPortal;
};

void createDngLadder(Location *location, PortalTriggerAction action, Portal *p);
int usePortalAt(Location *location, Coords coords, PortalTriggerAction action);

#endif
