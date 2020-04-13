/**
 * $Id: movement.cpp 3014 2012-03-05 21:24:47Z twschulz $
 */

#include "movement.h"
#include "annotation.h"
#include "combat.h"
#include "context.h"
#include "dungeon.h"
#include "error.h"
#include "event.h"
#include "location.h"
#include "creature.h"
#include "object.h"
#include "player.h"
#include "random.h"
#include "savegame.h"
#include "tile.h"
#include "utils.h"

bool collisionOverride = false;

/**
 * Attempt to move the avatar in the given direction.  User event
 * should be set if the avatar is being moved in response to a
 * keystroke.  Returns zero if the avatar is blocked.
 */
void moveAvatar(MoveEvent &event) {    
    Coords newCoords;
    int slowed = 0;
    SlowedType slowedType = SLOWED_BY_TILE;

    /* Check to see if we're on the balloon */
    if (c->transportContext == TRANSPORT_BALLOON && event.userEvent) {
        event.result = (MoveResult)(MOVE_DRIFT_ONLY | MOVE_END_TURN);
        return;
    }

    if (c->transportContext == TRANSPORT_SHIP)
        slowedType = SLOWED_BY_WIND;
    else if (c->transportContext == TRANSPORT_BALLOON)
        slowedType = SLOWED_BY_NOTHING;

    /* if you're on ship, you must turn first! */
    if (c->transportContext == TRANSPORT_SHIP) {
        if (c->party->getDirection() != event.dir) {
            c->party->setDirection(event.dir);
            event.result = (MoveResult)(MOVE_TURNED | MOVE_END_TURN);
            return;
        }
    }
    
    /* change direction of horse, if necessary */
    if (c->transportContext == TRANSPORT_HORSE) {
        if ((event.dir == DIR_WEST || event.dir == DIR_EAST) && (c->party->getDirection() != event.dir))
            c->party->setDirection(event.dir);
    }

    /* figure out our new location we're trying to move to */
    newCoords = c->location->coords;    
    movedir(&newCoords, event.dir, c->location->map);

    /* see if we moved off the map */
    if (MAP_IS_OOB(c->location->map, newCoords)) {
        event.result = (MoveResult)(MOVE_MAP_CHANGE | MOVE_EXIT_TO_PARENT | MOVE_SUCCEEDED);
        return;
    }

    if (!collisionOverride && !c->party->isFlying()) {
        int movementMask = c->location->map->getValidMoves(c->location->coords, c->party->getTransport());
        /* See if movement was blocked */
        if (!DIR_IN_MASK(event.dir, movementMask)) {
            event.result = (MoveResult)(MOVE_BLOCKED | MOVE_END_TURN);
            return;
        }

        /* Are we slowed by terrain or by wind direction? */
        switch(slowedType) {
        case SLOWED_BY_TILE:
          // TODO: CHEST: Make a user option to not make chests always fast to
          // travel over
            slowed = slowedByTile(c->location->map->tileTypeAt(newCoords, WITH_OBJECTS));
            break;
        case SLOWED_BY_WIND:
            slowed = slowedByWind(event.dir);
            break;
        case SLOWED_BY_NOTHING:
        default:
            break;
        }
        
        if (slowed) {
            event.result = (MoveResult)(MOVE_SLOWED | MOVE_END_TURN);
            return;
        }
    }

    /* move succeeded */
    c->location->coords = newCoords;    

    /* if the avatar moved onto a creature (whirlpool, twister), then do the creature's special effect (this current code does double damage according to changeset 2753.
      
    Object *destObj = c->location->map->objectAt(newCoords);
    if (destObj && destObj->getType() == Object::CREATURE) {
        Creature *m = dynamic_cast<Creature*>(destObj);
        //m->specialEffect();
    }
    */

    event.result = (MoveResult)(MOVE_SUCCEEDED | MOVE_END_TURN);
}

/**
 * Moves the avatar while in dungeon view
 */
void moveAvatarInDungeon(MoveEvent &event) {
    Coords newCoords;
    Direction realDir = dirNormalize((Direction)c->saveGame->orientation, event.dir); /* get our real direction */  
    int advancing = realDir == c->saveGame->orientation;
    int retreating = realDir == dirReverse((Direction)c->saveGame->orientation);
    MapTile *tile;
    
    /* we're not in a dungeon, failed! */
    zu4_assert(c->location->context & CTX_DUNGEON, "moveAvatarInDungeon() called outside of dungeon, failed!");    
        
    /* you must turn first! */
    if (!advancing && !retreating) {        
        c->saveGame->orientation = realDir;
        event.result = MOVE_TURNED;
        return;
    }
    
    /* figure out our new location */
    newCoords = c->location->coords;    
    movedir(&newCoords, realDir, c->location->map);

    tile = c->location->map->tileAt(newCoords, WITH_OBJECTS);

    /* see if we moved off the map (really, this should never happen in a dungeon) */
    if (MAP_IS_OOB(c->location->map, newCoords)) {
        event.result = (MoveResult)(MOVE_MAP_CHANGE | MOVE_EXIT_TO_PARENT | MOVE_SUCCEEDED);
        return;
    }

    if (!collisionOverride) {
        int movementMask = c->location->map->getValidMoves(c->location->coords, c->party->getTransport());

        if (advancing && !tile->getTileType()->canWalkOn(DIR_ADVANCE))
            movementMask = DIR_REMOVE_FROM_MASK(realDir, movementMask);
        else if (retreating && !tile->getTileType()->canWalkOn(DIR_RETREAT))
            movementMask = DIR_REMOVE_FROM_MASK(realDir, movementMask);

        if (!DIR_IN_MASK(realDir, movementMask)) {
            event.result = (MoveResult)(MOVE_BLOCKED | MOVE_END_TURN);
            return;
        }
    }

    /* move succeeded */
    c->location->coords = newCoords;    

    event.result = (MoveResult)(MOVE_SUCCEEDED | MOVE_END_TURN);
}

/**
 * Moves an object on the map according to its movement behavior
 * Returns 1 if the object was moved successfully, 0 if slowed,
 * tile direction changed, or object simply cannot move
 * (fixed objects, nowhere to go, etc.)
 */
int moveObject(Map *map, Creature *obj, Coords avatar) {
    int dirmask = DIR_NONE;
    Direction dir;
    Coords new_coords = obj->getCoords();    
    int slowed = 0;    
    
    /* determine a direction depending on the object's movement behavior */
    dir = DIR_NONE;
    switch (obj->getMovementBehavior()) {
    case MOVEMENT_FIXED:
        break;

    case MOVEMENT_WANDER:
        /* World map wandering creatures always move, whereas
           town creatures that wander sometimes stay put */
        if (map->isWorldMap() || zu4_random(2) == 0)
            dir = dirRandomDir(map->getValidMoves(new_coords, obj->getTile()));
        break;

    case MOVEMENT_FOLLOW_AVATAR:
    case MOVEMENT_ATTACK_AVATAR:
        dirmask = map->getValidMoves(new_coords, obj->getTile());
        
        /* If the pirate ship turned last move instead of moving, this time it must
           try to move, not turn again */
        if (obj->getTile().getTileType()->isPirateShip() && DIR_IN_MASK(obj->getTile().getDirection(), dirmask) &&
            (obj->getTile() != obj->getPrevTile()) && zu4_coords_equal(obj->getPrevCoords(), obj->getCoords())) {
            dir = obj->getTile().getDirection();
            break;
        }

        dir = pathTo(new_coords, avatar, dirmask, true, c->location->map);
        break;
    }
    
    /* now, get a new x and y for the object */
    if (dir)
        movedir(&new_coords, dir, c->location->map);        
    else
        return 0;

    /* figure out what method to use to tell if the object is getting slowed */   
    SlowedType slowedType = SLOWED_BY_TILE;
    if (obj->getType() == Object::CREATURE)
        slowedType = obj->getSlowedType();
    
    /* is the object slowed by terrain or by wind direction? */
    switch(slowedType) {
    case SLOWED_BY_TILE:
        slowed = slowedByTile(map->tileTypeAt(new_coords, WITHOUT_OBJECTS));
        break;
    case SLOWED_BY_WIND:
        slowed = slowedByWind(obj->getTile().getDirection());
        break;
    case SLOWED_BY_NOTHING:
    default:
        break;
    }
    
    obj->setPrevCoords(obj->getCoords());
    
    /* see if the object needed to turn instead of move */
    if (obj->setDirection(dir))
        return 0;    
    
    /* was the object slowed? */
    if (slowed)
        return 0;

    /**
     * Set the new coordinates
     */ 
    if (!zu4_coords_equal(new_coords, obj->getCoords()) && 
        !MAP_IS_OOB(map, new_coords))
    {
    	obj->setCoords(new_coords);
    }
    return 1;
}

/**
 * Moves an object in combat according to its chosen combat action
 */
int moveCombatObject(int act, Map *map, Creature *obj, Coords target) {
    Coords new_coords = obj->getCoords();
    int valid_dirs = map->getValidMoves(new_coords, obj->getTile());
    Direction dir;
    CombatAction action = (CombatAction)act;
    SlowedType slowedType = SLOWED_BY_TILE;
    int slowed = 0;

    /* fixed objects cannot move */
    if (obj->getMovementBehavior() == MOVEMENT_FIXED)
        return 0;

    if (action == CA_FLEE) {
        /* run away from our target instead! */
        dir = pathAway(new_coords, target, valid_dirs);
    
    } else {
        zu4_assert(action == CA_ADVANCE, "action must be CA_ADVANCE or CA_FLEE");
        // If they're not fleeing, make sure they don't flee on accident
        if (new_coords.x == 0)
            valid_dirs = DIR_REMOVE_FROM_MASK(DIR_WEST, valid_dirs);
        else if (new_coords.x >= (signed)(map->width - 1))
            valid_dirs = DIR_REMOVE_FROM_MASK(DIR_EAST, valid_dirs);
        if (new_coords.y == 0)
            valid_dirs = DIR_REMOVE_FROM_MASK(DIR_NORTH, valid_dirs);
        else if (new_coords.y >= (signed)(map->height - 1))
            valid_dirs = DIR_REMOVE_FROM_MASK(DIR_SOUTH, valid_dirs);        

        dir = pathTo(new_coords, target, valid_dirs);
    }

    if (dir)
        movedir(&new_coords, dir, c->location->map);
    else
        return 0;

    /* figure out what method to use to tell if the object is getting slowed */   
    if (obj->getType() == Object::CREATURE)
        slowedType = obj->getSlowedType();

    /* is the object slowed by terrain or by wind direction? */
    switch(slowedType) {
    case SLOWED_BY_TILE:
        slowed = slowedByTile(map->tileTypeAt(new_coords, WITHOUT_OBJECTS));
        break;
    case SLOWED_BY_WIND:
        slowed = slowedByWind(obj->getTile().getDirection());
        break;
    case SLOWED_BY_NOTHING:
    default:
        break;
    }

    /* if the object wan't slowed... */
    if (!slowed) {        
        // Set the new coordinates
    	obj->setCoords(new_coords);
        return 1;
    }

    return 0;
}

/**
 * Moves a party member during combat screens
 */
void movePartyMember(MoveEvent &event) {
    CombatController *ct = dynamic_cast<CombatController *>(eventHandler->getController());
    CombatMap *cm = getCombatMap();
    int member = ct->getFocus();
    Coords newCoords;
    PartyMemberVector *party = ct->getParty();

    event.result = MOVE_SUCCEEDED;    

    /* find our new location */
    newCoords = (*party)[member]->getCoords();
    movedir(&newCoords, event.dir, c->location->map);

    if (MAP_IS_OOB(c->location->map, newCoords)) {
        bool sameExit = (!cm->isDungeonRoom() || (ct->getExitDir() == DIR_NONE) || (event.dir == ct->getExitDir()));
        if (sameExit) {
            /* if in a win-or-lose battle and not camping, then it can be bad to flee while healthy */
            if (ct->isWinOrLose() && !ct->isCamping()) {
                /* A fully-healed party member fled from an evil creature :( */
                if (ct->getCreature() && ct->getCreature()->isEvil() && 
                    c->party->member(member)->getHp() == c->party->member(member)->getMaxHp())
                    c->party->adjustKarma(KA_HEALTHY_FLED_EVIL);
            }

            ct->setExitDir(event.dir);
            c->location->map->removeObject((*party)[member]);
            (*party)[member] = NULL;
            event.result = (MoveResult)(MOVE_EXIT_TO_PARENT | MOVE_MAP_CHANGE | MOVE_SUCCEEDED | MOVE_END_TURN);
            return;
        }
        else {
            event.result = (MoveResult)(MOVE_MUST_USE_SAME_EXIT | MOVE_END_TURN);
            return;
        }
    }

    int movementMask = c->location->map->getValidMoves((*party)[member]->getCoords(), (*party)[member]->getTile());
    if (!DIR_IN_MASK(event.dir, movementMask)) {
        event.result = (MoveResult)(MOVE_BLOCKED | MOVE_END_TURN);
        return;
    }

    /* is the party member slowed? */
    if (!slowedByTile(c->location->map->tileTypeAt(newCoords, WITHOUT_OBJECTS)))
    {
        /* move succeeded */        
        (*party)[member]->setCoords(newCoords);

        /* handle dungeon room triggers */
        if (cm->isDungeonRoom()) {
            Dungeon *dungeon = dynamic_cast<Dungeon*>(c->location->prev->map);
            int i;
            Trigger *triggers = dungeon->rooms[dungeon->currentRoom].triggers;            

            for (i = 0; i < 4; i++) {
                /*const Creature *m = creatures.getByTile(triggers[i].tile);*/

                /* FIXME: when a creature is created by a trigger, it can be created over and over and over...
                   how do we fix this? In the c64 version is appears that such triggers (on the world map)
                   wipe the creature table and replace it with the triggered creatures. Thus, retriggering
                   it will reset the creatures.
                   */
                Coords trigger = {triggers[i].x, triggers[i].y, c->location->coords.z};

                /* see if we're on a trigger */
                if (zu4_coords_equal(newCoords, trigger)) {
                    Coords change1 = {triggers[i].change_x1, triggers[i].change_y1, c->location->coords.z};
                    Coords change2 = {triggers[i].change_x2, triggers[i].change_y2, c->location->coords.z};

                    /**
                     * Remove any previous annotations placed at our target coordinates
                     */ 
                    c->location->map->annotations->remove(c->location->map->annotations->allAt(change1));
                    c->location->map->annotations->remove(c->location->map->annotations->allAt(change2));

                    /* change the tiles! */
                    if (change1.x || change1.y) {
                        /*if (m) combatAddCreature(m, triggers[i].change_x1, triggers[i].change_y1, c->location->coords.z);
                        else*/ c->location->map->annotations->add(change1, triggers[i].tile, false, true);
                    }
                    if (change2.x || change2.y) {
                        /*if (m) combatAddCreature(m, triggers[i].change_x2, triggers[i].change_y2, c->location->coords.z);
                        else*/ c->location->map->annotations->add(change2, triggers[i].tile, false, true);
                    }
                }
            }
        }    
    }
    else {
        event.result = (MoveResult)(MOVE_SLOWED | MOVE_END_TURN);
        return;
    }
}
 
/**
 * Default handler for slowing movement.
 * Returns true if slowed, false if not slowed
 */
bool slowedByTile(const Tile *tile) {
    bool slow;
    
    switch (tile->getSpeed()) {
    case SLOW:
        slow = zu4_random(8) == 0;
        break;
    case VSLOW:
        slow = zu4_random(4) == 0;
        break;
    case VVSLOW:
        slow = zu4_random(2) == 0;
        break;
    case FAST:
    default:
        slow = false;
        break;
    }

    return slow;
}

/**
 * Slowed depending on the direction of object with respect to wind direction
 * Returns true if slowed, false if not slowed
 */
bool slowedByWind(int direction) {
    /* 1 of 4 moves while trying to move into the wind succeeds */
    if (direction == c->windDirection)
        return (c->saveGame->moves % 4) != 0;
    /* 1 of 4 moves while moving directly away from wind fails */
    else if (direction == dirReverse((Direction) c->windDirection))
        return (c->saveGame->moves % 4) == 3;    
    else
        return false;
}
