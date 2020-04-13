/*
 * $Id: portal.cpp 2996 2011-12-03 22:41:33Z twschulz $
 */


#include "portal.h"

#include "annotation.h"
#include "city.h"
#include "context.h"
#include "dungeon.h"
#include "game.h"
#include "location.h"
#include "mapmgr.h"
#include "names.h"
#include "screen.h"
#include "shrine.h"
#include "tile.h"

/**
 * Creates a dungeon ladder portal based on the action given
 */
void createDngLadder(Location *location, PortalTriggerAction action, Portal *p) {
    if (!p) return;
    else {
        p->destid = location->map->id;
        if (action == ACTION_KLIMB && location->coords.z == 0) {
            p->exitPortal = true;
            p->destid = 1;
        }
        else p->exitPortal = false;
        p->message = "";
        p->portalConditionsMet = NULL;
        p->portalTransportRequisites = TRANSPORT_FOOT_OR_HORSE;
        p->retroActiveDest = NULL;
        p->saveLocation = false;
        p->start = location->coords;
        p->start.z += (action == ACTION_KLIMB) ? -1 : 1;
    }
}

/**
 * Finds a portal at the given (x,y,z) coords that will work with the action given
 * and uses it.  If in a dungeon and trying to use a ladder, it creates a portal
 * based on the ladder and uses it.
 */
int usePortalAt(Location *location, Coords coords, PortalTriggerAction action) {
    Map *destination;
    char msg[32] = {0};
    
    const Portal *portal = location->map->portalAt(coords, action);
    Portal dngLadder;

    /* didn't find a portal there */
    if (!portal) {
        
        /* if it's a dungeon, then ladders are predictable.  Create one! */
        if (location->context == CTX_DUNGEON) {
            Dungeon *dungeon = dynamic_cast<Dungeon *>(location->map);
            if ((action & ACTION_KLIMB) && dungeon->ladderUpAt(coords)) 
                createDngLadder(location, action, &dngLadder);                
            else if ((action & ACTION_DESCEND) && dungeon->ladderDownAt(coords))
                createDngLadder(location, action, &dngLadder);
            else return 0;
            portal = &dngLadder;
        }
        else return 0;
    }

    /* conditions not met for portal to work */
    if (portal && portal->portalConditionsMet && !(*portal->portalConditionsMet)(portal))
        return 0;
    /* must klimb or descend on foot! */
    else if (c->transportContext & ~TRANSPORT_FOOT && (action == ACTION_KLIMB || action == ACTION_DESCEND)) {
        screenMessage("%sOnly on foot!\n", action == ACTION_KLIMB ? "Klimb\n" : "");
        return 1;
    }    
    
    destination = mapMgr->get(portal->destid);

    if (portal->message.empty()) {

        switch(action) {
        case ACTION_DESCEND:
            sprintf(msg, "Descend down to level %d\n", portal->start.z+1);
            break;
        case ACTION_KLIMB:
            if (portal->exitPortal)
                sprintf(msg, "Klimb up!\nLeaving...\n");
            else sprintf(msg, "Klimb up!\nTo level %d\n", portal->start.z+1);
            break;
        case ACTION_ENTER:
            switch (destination->type) {
            case Map::CITY: 
                {
                    City *city = dynamic_cast<City*>(destination);
                    screenMessage("Enter %s!\n\n%s\n\n", city->type.c_str(), city->getName().c_str());
                }
                break;            
            case Map::SHRINE:
                screenMessage("Enter the %s!\n\n", destination->getName().c_str());
                break;
            case Map::DUNGEON:
                screenMessage("Enter dungeon!\n\n%s\n\n", destination->getName().c_str());
                break;
            default:
                break;
            }
            break;
        case ACTION_NONE:
        default: break;
        }
    }

    /* check the transportation requisites of the portal */
    if (c->transportContext & ~portal->portalTransportRequisites) {
        screenMessage("Only on foot!\n");        
        return 1;
    }
    /* ok, we know the portal is going to work -- now display the custom message, if any */
    else if (!portal->message.empty() || strlen(msg))
        screenMessage("%s", portal->message.empty() ? msg : portal->message.c_str());

    /* portal just exits to parent map */
    if (portal->exitPortal) {        
        game->exitToParentMap();
        zu4_music_play(c->location->map->music);
        return 1;
    }
    else if (portal->destid == location->map->id)
        location->coords = portal->start;        
    
    else {
        game->setMap(destination, portal->saveLocation, portal);
        zu4_music_play(c->location->map->music);
    }

    /* if the portal changes the map retroactively, do it here */
    /* 
     * note that we use c->location instead of location, since
     * location has probably been invalidated above 
     */
    if (portal->retroActiveDest && c->location->prev) {
        c->location->prev->coords = portal->retroActiveDest->coords;        
        c->location->prev->map = mapMgr->get(portal->retroActiveDest->mapid);
    }

    if (destination->type == Map::SHRINE) {
        Shrine *shrine = dynamic_cast<Shrine*>(destination);
        shrine->enter();
    }

    return 1;
}
