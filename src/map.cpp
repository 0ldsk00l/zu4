/*
 * $Id: map.cpp 3066 2014-07-21 00:18:48Z darren_janeczek $
 */

#include "u4.h"
#include "map.h"
#include "annotation.h"
#include "context.h"
#include "direction.h"
#include "error.h"
#include "location.h"
#include "movement.h"
#include "object.h"
#include "person.h"
#include "player.h"
#include "portal.h"
#include "savegame.h"
#include "tileset.h"
#include "tilemap.h"
#include "types.h"
#include "utils.h"
#include "settings.h"

void wrap(Coords *oc, const Map *map) {
    if (map && map->border_behavior == Map::BORDER_WRAP) {
        while (oc->x < 0)
            oc->x += map->width;
        while (oc->y < 0)
            oc->y += map->height;
        while (oc->x >= (int)map->width)
            oc->x -= map->width;
        while (oc->y >= (int)map->height)
            oc->y -= map->height;
    }
}

void putInBounds(Coords *oc, const Map *map) {
    if (map) {
        if (oc->x < 0)
            oc->x = 0;
        if (oc->x >= (int) map->width)
            oc->x = map->width - 1;
        if (oc->y < 0)
            oc->y = 0;
        if (oc->y >= (int) map->height)
            oc->y = map->height - 1;
        if (oc->z < 0)
            oc->z = 0;
        if (oc->z >= (int) map->levels)
            oc->z = map->levels - 1;
    }
}

void movedir(Coords *oc, Direction d, const Map *map) {
	switch(d) {
		case DIR_NORTH: oc->y--; break;
		case DIR_EAST: oc->x++; break;
		case DIR_SOUTH: oc->y++; break;
		case DIR_WEST: oc->x--; break;
		default: break;
	}
	
	// Wrap the coordinates if necessary
	wrap(oc, map);
}

void movexy(Coords *oc, int dx, int dy, const Map *map) {
	oc->x += dx;
	oc->y += dy;        
	
	// Wrap the coordinates if necessary
	wrap(oc, map);
}

/**
 * Returns a mask of directions that indicate where one point is relative
 * to another.  For instance, if the object at (x, y) is
 * northeast of (c.x, c.y), then this function returns
 * (MASK_DIR(DIR_NORTH) | MASK_DIR(DIR_EAST))
 * This function also takes into account map boundaries and adjusts
 * itself accordingly. If the two coordinates are not on the same z-plane,
 * then this function return DIR_NONE.
 */
int getRelativeDirection(Coords oc, Coords c, const Map *map) {
    int dx, dy, dirmask;        

    dirmask = DIR_NONE;
    if (oc.z != c.z)
        return dirmask;
    
    // adjust our coordinates to find the closest path
    if (map && map->border_behavior == Map::BORDER_WRAP) {
        Coords me = oc;            
        
        if (abs(int(me.x - c.x)) > abs(int(me.x + map->width - c.x)))
            me.x += map->width;
        else if (abs(int(me.x - c.x)) > abs(int(me.x - map->width - c.x)))
            me.x -= map->width;

        if (abs(int(me.y - c.y)) > abs(int(me.y + map->width - c.y)))
            me.y += map->height;
        else if (abs(int(me.y - c.y)) > abs(int(me.y - map->width - c.y)))
            me.y -= map->height;

        dx = me.x - c.x;
        dy = me.y - c.y;
    }
    else {        
        dx = oc.x - c.x;
        dy = oc.y - c.y;
    }

    // add x directions that lead towards to_x to the mask
    if (dx < 0)         dirmask |= MASK_DIR(DIR_EAST);
    else if (dx > 0)    dirmask |= MASK_DIR(DIR_WEST);

    // add y directions that lead towards to_y to the mask
    if (dy < 0)         dirmask |= MASK_DIR(DIR_SOUTH);
    else if (dy > 0)    dirmask |= MASK_DIR(DIR_NORTH);

    // return the result
    return dirmask;
}

/**
 * Finds the appropriate direction to travel to get from one point to
 * another.  This algorithm will avoid getting trapped behind simple
 * obstacles, but still fails with anything mildly complicated.
 * This function also takes into account map boundaries and adjusts
 * itself accordingly, provided the 'map' parameter is passed
 */
Direction pathTo(Coords oc, Coords c, int valid_directions, bool towards, const Map *map) {
    int directionsToObject;

    // find the directions that lead [to/away from] our target
    directionsToObject = towards ? getRelativeDirection(oc, c, map) : ~getRelativeDirection(oc, c, map);

    // make sure we eliminate impossible options
    directionsToObject &= valid_directions;

    // get the new direction to move
    if (directionsToObject > DIR_NONE)
        return dirRandomDir(directionsToObject);

    // there are no valid directions that lead to our target, just move wherever we can!
    else return dirRandomDir(valid_directions);
}

/**
 * Finds the appropriate direction to travel to move away from one point
 */
Direction pathAway(Coords oc, Coords c, int valid_directions) {
    return pathTo(oc, c, valid_directions, false);
}
/**
 * Finds the movement distance (not using diagonals) from point a to point b
 * on a map, taking into account map boundaries and such.  If the two coords
 * are not on the same z-plane, then this function returns -1;
 */
int movementDistance(Coords oc, Coords c, const Map *map) {
    int dirmask = DIR_NONE;
    int dist = 0;
    Coords me;
    me.x = oc.x; me.y = oc.y; me.z = oc.z;

    if (oc.z != c.z)
        return -1;

    // get the direction(s) to the coordinates
    dirmask = getRelativeDirection(oc, c, map);

    while ((me.x != c.x) || (me.y != c.y))
    {
        if (me.x != c.x) {
            if (dirmask & MASK_DIR_WEST)
                movedir(&me, DIR_WEST, map);
            else movedir(&me, DIR_EAST, map);

            dist++;
        }
        if (me.y != c.y) {
            if (dirmask & MASK_DIR_NORTH)
                movedir(&me, DIR_NORTH, map);
            else movedir(&me, DIR_SOUTH, map);

            dist++;
        }            
    }

    return dist;
}

/**
 * Finds the distance (using diagonals) from point a to point b on a map
 * If the two coordinates are not on the same z-plane, then this function
 * returns -1. This function also takes into account map boundaries.
 */ 
int distance(Coords oc, Coords c, const Map *map) {
    int dist = movementDistance(oc, c, map);
    if (dist <= 0)
        return dist;

    // calculate how many fewer movements there would have been
    dist -= abs(oc.x - c.x) < abs(oc.y - c.y) ? abs(oc.x - c.x) : abs(oc.y - c.y);

    return dist;
}

/**
 * Map Class Implementation
 */ 

Map::Map() {
    annotations = new AnnotationMgr();
    flags = 0;
    width = 0;
    height = 0;
    levels = 1;
    chunk_width = 0;
    chunk_height = 0;    
    offset = 0;    
    id = 0;
    tileset = NULL;
    tilemap = NULL;
}

Map::~Map() {
    for (PortalList::iterator i = portals.begin(); i != portals.end(); i++)
        delete *i;
    delete annotations;
}

string Map::getName() {
    return baseSource.fname;
}

/**
 * Returns the object at the given (x,y,z) coords, if one exists.
 * Otherwise, returns NULL.
 */
Object *Map::objectAt(const Coords &coords) {
    /* FIXME: return a list instead of one object */
    ObjectDeque::const_iterator i;        
    Object *objAt = NULL;    

    for(i = objects.begin(); i != objects.end(); i++) {
        Object *obj = *i;
        
        if (xu4_coords_equal(obj->getCoords(), coords)) {
            /* get the most visible object */
            if (objAt && (objAt->getType() == Object::UNKNOWN) && (obj->getType() != Object::UNKNOWN))
                objAt = obj;
            /* give priority to objects that have the focus */
            else if (objAt && (!objAt->hasFocus()) && (obj->hasFocus()))
                objAt = obj;
            else if (!objAt)
                objAt = obj;
        }
    }
    return objAt;
}

/**
 * Returns the portal for the correspoding action(s) given.
 * If there is no portal that corresponds to the actions flagged
 * by 'actionFlags' at the given (x,y,z) coords, it returns NULL.
 */
const Portal *Map::portalAt(const Coords &coords, int actionFlags) {
    PortalList::const_iterator i;    

    for(i = portals.begin(); i != portals.end(); i++) {
        if (xu4_coords_equal((*i)->coords, coords) &&
            ((*i)->trigger_action & actionFlags))
            return *i;
    }
    return NULL;
}

/**
 * Returns the raw tile for the given (x,y,z) coords for the given map
 */
MapTile *Map::getTileFromData(const Coords &coords) {
    static MapTile blank(0);

    if (MAP_IS_OOB(this, coords))
        return &blank;

    int index = coords.x + (coords.y * width) + (width * height * coords.z);
    return &data[index];
}

/**
 * Returns the current ground tile at the given point on a map.  Visual-only
 * annotations like moongates and attack icons are ignored.  Any walkable tiles
 * are taken into account (treasure chests, ships, balloon, etc.)
 */
MapTile *Map::tileAt(const Coords &coords, int withObjects) {
    /* FIXME: this should return a list of tiles, with the most visible at the front */
    MapTile *tile;
    std::list<Annotation *> a = annotations->ptrsToAllAt(coords);
    std::list<Annotation *>::iterator i;
    Object *obj = objectAt(coords);
 
    tile = getTileFromData(coords);

    /* FIXME: this only returns the first valid annotation it can find */
    if (a.size() > 0) {
        for (i = a.begin(); i != a.end(); i++) {
            if (!(*i)->isVisualOnly())        
                return &(*i)->getTile();
        }
    }

    if ((withObjects == WITH_OBJECTS) && obj)
        tile = &obj->getTile();
    else if ((withObjects == WITH_GROUND_OBJECTS) && 
             obj && 
             obj->getTile().getTileType()->isWalkable())
        tile = &obj->getTile();
    
    return tile;
}

const Tile *Map::tileTypeAt(const Coords &coords, int withObjects) {
    MapTile *tile = tileAt(coords, withObjects);
    return tile->getTileType();
}

/**
 * Returns true if the given map is the world map
 */
bool Map::isWorldMap() {
    return type == WORLD;
}



/**
 * Returns true if the map is enclosed (to see if gem layouts should cut themselves off)
 */ 
bool Map::isEnclosed(const Coords &party) {
    unsigned int x, y;
    int *path_data;

    if (border_behavior != BORDER_WRAP)
        return true;

    path_data = new int[width * height];
    memset(path_data, -1, sizeof(int) * width * height);

    // Determine what's walkable (1), and what's border-walkable (2)
    findWalkability(party, path_data);

    // Find two connecting pathways where the avatar can reach both without wrapping
    for (x = 0; x < width; x++) {
        int index = x;
        if (path_data[index] == 2 && path_data[index + ((height-1)*width)] == 2)
            return false;        
    }

    for (y = 0; y < width; y++) {
        int index = (y * width);
        if (path_data[index] == 2 && path_data[index + width - 1] == 2)
            return false;
    }

    return true;
}

void Map::findWalkability(Coords coords, int *path_data) {
    const Tile *mt = tileTypeAt(coords, WITHOUT_OBJECTS);
    int index = coords.x + (coords.y * width);

    if (mt->isWalkable()) {        
        bool isBorderTile = (coords.x == 0) || (coords.x == signed(width-1)) || (coords.y == 0) || (coords.y == signed(height-1));
        path_data[index] = isBorderTile ? 2 : 1;

        if ((coords.x > 0) && path_data[coords.x - 1 + (coords.y * width)] < 0)
            findWalkability((Coords){coords.x - 1, coords.y, coords.z}, path_data);
        if ((coords.x < signed(width-1)) && path_data[coords.x + 1 + (coords.y * width)] < 0)
            findWalkability((Coords){coords.x + 1, coords.y, coords.z}, path_data);
        if ((coords.y > 0) && path_data[coords.x + ((coords.y - 1) * width)] < 0)
            findWalkability((Coords){coords.x, coords.y - 1, coords.z}, path_data);
        if ((coords.y < signed(height-1)) && path_data[coords.x + ((coords.y + 1) * width)] < 0)
            findWalkability((Coords){coords.x, coords.y + 1, coords.z}, path_data);    
    }
    else path_data[index] = 0;    
}

/**
 * Adds a creature object to the given map
 */
Creature *Map::addCreature(const Creature *creature, Coords coords) {
    Creature *m = new Creature;
    
    /* make a copy of the creature before placing it */
    *m = *creature;

    m->setInitialHp();
    m->setStatus(STAT_GOOD);    
    m->setCoords(coords);
    m->setMap(this);
    
    /* initialize the creature before placing it */
    if (m->wanders())
        m->setMovementBehavior(MOVEMENT_WANDER);
    else if (m->isStationary())
        m->setMovementBehavior(MOVEMENT_FIXED);
    else m->setMovementBehavior(MOVEMENT_ATTACK_AVATAR);

    /* hide camouflaged creatures from view during combat */
    if (m->camouflages() && (type == COMBAT))
        m->setVisible(false);
    
    /* place the creature on the map */
    objects.push_back(m);
    return m;
}

/**
 * Adds an object to the given map
 */
Object *Map::addObject(Object *obj, Coords coords) {
    objects.push_front(obj);
    return obj;
}

Object *Map::addObject(MapTile tile, MapTile prevtile, Coords coords) {
    Object *obj = new Object;

    obj->setTile(tile);
    obj->setPrevTile(prevtile);
    obj->setCoords(coords);    
    obj->setPrevCoords(coords);
    obj->setMap(this);
    
    objects.push_front(obj);    

    return obj;
}

/**
 * Removes an object from the map
 */ 

// This function should only be used when not iterating through an
// ObjectDeque, as the iterator will be invalidated and the
// results will be unpredictable.  Instead, use the function
// below.
void Map::removeObject(const Object *rem, bool deleteObject) {
    ObjectDeque::iterator i;
    for (i = objects.begin(); i != objects.end(); i++) {
        if (*i == rem) {
            /* Party members persist through different maps, so don't delete them! */
            if (!isPartyMember(*i) && deleteObject)
                delete (*i);
            objects.erase(i);
            return;
        }
    }
}

ObjectDeque::iterator Map::removeObject(ObjectDeque::iterator rem, bool deleteObject) {
    /* Party members persist through different maps, so don't delete them! */
    if (!isPartyMember(*rem) && deleteObject)
        delete (*rem);
    return objects.erase(rem);
}

/**
 * Moves all of the objects on the given map.
 * Returns an attacking object if there is a creature attacking.
 * Also performs special creature actions and creature effects.
 */
Creature *Map::moveObjects(Coords avatar) {        
    Creature *attacker = NULL;
    
    for (unsigned int i = 0; i < objects.size(); i++) {
        Creature *m = dynamic_cast<Creature*>(objects[i]);
        
        if (m) {
            /* check if the object is an attacking creature and not
               just a normal, docile person in town or an inanimate object */
            if ((m->getType() == Object::PERSON && m->getMovementBehavior() == MOVEMENT_ATTACK_AVATAR) ||
                (m->getType() == Object::CREATURE && m->willAttack())) {
                Coords o_coords = m->getCoords();
            
                /* don't move objects that aren't on the same level as us */
                if (o_coords.z != avatar.z)
                    continue;

                if (movementDistance(o_coords, avatar, this) <= 1) {
                    attacker = m;
                    continue;
                }
            }

            /* Before moving, Enact any special effects of the creature (such as storms eating objects, whirlpools teleporting, etc.) */
            m->specialEffect();


            /* Perform any special actions (such as pirate ships firing cannons, sea serpents' fireblast attect, etc.) */
            if (!m->specialAction())
            {
                if	(moveObject(this, m, avatar))
                {
                	m->animateMovement();
                	/* After moving, Enact any special effects of the creature (such as storms eating objects, whirlpools teleporting, etc.) */
                	m->specialEffect();
                }
            }
        }
    }

    return attacker;
}

/**
 * Resets object animations to a value that is acceptable for
 * savegame compatibility with u4dos.
 */
void Map::resetObjectAnimations() {
    ObjectDeque::iterator i;
    
    for (i = objects.begin(); i != objects.end(); i++) {
        Object *obj = *i;
        
        if (obj->getType() == Object::CREATURE)
            obj->setPrevTile(creatureMgr->getByTile(obj->getTile())->getTile());
    }
}

/**
 * Removes all objects from the given map
 */
void Map::clearObjects() {
    objects.clear();    
}

/**
 * Returns the number of creatures on the given map
 */
int Map::getNumberOfCreatures() {
    ObjectDeque::const_iterator i;
    int n = 0;

    for (i = objects.begin(); i != objects.end(); i++) {
        Object *obj = *i;

        if (obj->getType() == Object::CREATURE)
            n++;
    }

    return n;
}

/**
 * Returns a mask of valid moves for the given transport on the given map
 */
int Map::getValidMoves(Coords from, MapTile transport) {
    int retval;
    Direction d;
    Object *obj;    
    const Creature *m, *to_m;
    int ontoAvatar, ontoCreature;    
    Coords coords = {from.x, from.y, from.z};

    // get the creature object, if it exists (the one that's moving)
    m = creatureMgr->getByTile(transport);

    bool isAvatar = xu4_coords_equal(c->location->coords, coords);
    if (m && m->canMoveOntoPlayer())
    	isAvatar = false;

    retval = 0;
    for (d = DIR_WEST; d <= DIR_SOUTH; d = (Direction)(d+1)) {
        coords = {from.x, from.y, from.z};
        ontoAvatar = 0;
        ontoCreature = 0;

        // Move the coordinates in the current direction and test it
        movedir(&coords, d, this);
        
        // you can always walk off the edge of the map
        if (MAP_IS_OOB(this, coords)) {
            retval = DIR_ADD_TO_MASK(d, retval);
            continue;
        }

        obj = objectAt(coords);

        // see if it's trying to move onto the avatar
        if ((flags & SHOW_AVATAR) && xu4_coords_equal(coords, c->location->coords))
            ontoAvatar = 1;
        
        // see if it's trying to move onto a person or creature
        else if (obj && (obj->getType() != Object::UNKNOWN))                 
            ontoCreature = 1;
            
        // get the destination tile
        MapTile tile;
        if (ontoAvatar)
            tile = c->party->getTransport();
        else if (ontoCreature)
            tile = obj->getTile();
        else 
            tile = *tileAt(coords, WITH_OBJECTS);

        MapTile prev_tile = *tileAt(from, WITHOUT_OBJECTS);

        // get the other creature object, if it exists (the one that's being moved onto)        
        to_m = dynamic_cast<Creature*>(obj);

        // move on if unable to move onto the avatar or another creature
        if (m && !isAvatar) { // some creatures/persons have the same tile as the avatar, so we have to adjust
            // If moving onto the avatar, the creature must be able to move onto the player
            // If moving onto another creature, it must be able to move onto other creatures,
            // and the creature must be able to have others move onto it.  If either of
            // these conditions are not met, the creature cannot move onto another.

        	if ((ontoAvatar && m->canMoveOntoPlayer()) || (ontoCreature && m->canMoveOntoCreatures()))
               	tile = *tileAt(coords, WITHOUT_OBJECTS); //Ignore all objects, and just consider terrain
        	  if ((ontoAvatar && !m->canMoveOntoPlayer())
            	||	(
            			ontoCreature &&
            			(
            				(!m->canMoveOntoCreatures() && !to_m->canMoveOntoCreatures())
            				|| (m->isForceOfNature() && to_m->isForceOfNature())
            			)
            		)
            	)
                continue;
        }

        // avatar movement
        if (isAvatar) {
            // if the transport is a ship, check sailable
            if (transport.getTileType()->isShip() && tile.getTileType()->isSailable())
                retval = DIR_ADD_TO_MASK(d, retval);
            // if it is a balloon, check flyable
            else if (transport.getTileType()->isBalloon() && tile.getTileType()->isFlyable())
                retval = DIR_ADD_TO_MASK(d, retval);        
            // avatar or horseback: check walkable
            else if (transport == tileset->getByName("avatar")->getId() || transport.getTileType()->isHorse()) {
                if (tile.getTileType()->canWalkOn(d) &&
                	(!transport.getTileType()->isHorse() || tile.getTileType()->isCreatureWalkable()) &&
                    prev_tile.getTileType()->canWalkOff(d))
                    retval = DIR_ADD_TO_MASK(d, retval);
            }
//            else if (ontoCreature && to_m->canMoveOntoPlayer()) {
//            	retval = DIR_ADD_TO_MASK(d, retval);
//            }
        }
        
        // creature movement
        else if (m) {
            // flying creatures
            if (tile.getTileType()->isFlyable() && m->flies()) {
                // FIXME: flying creatures behave differently on the world map?
                if (isWorldMap())
                    retval = DIR_ADD_TO_MASK(d, retval);
                else if (tile.getTileType()->isWalkable() || 
                         tile.getTileType()->isSwimable() || 
                         tile.getTileType()->isSailable())
                    retval = DIR_ADD_TO_MASK(d, retval);
            }
            // swimming creatures and sailing creatures
            else if (tile.getTileType()->isSwimable() || 
                     tile.getTileType()->isSailable() ||
                     tile.getTileType()->isShip()) {
                if (m->swims() && tile.getTileType()->isSwimable())
                    retval = DIR_ADD_TO_MASK(d, retval);
                if (m->sails() && tile.getTileType()->isSailable())
                    retval = DIR_ADD_TO_MASK(d, retval);
                if (m->canMoveOntoPlayer() && tile.getTileType()->isShip())
                	retval = DIR_ADD_TO_MASK(d, retval);
            }
            // ghosts and other incorporeal creatures
            else if (m->isIncorporeal()) {
                // can move anywhere but onto water, unless of course the creature can swim
                if (!(tile.getTileType()->isSwimable() || 
                      tile.getTileType()->isSailable()))
                    retval = DIR_ADD_TO_MASK(d, retval);
            }
            // walking creatures
            else if (m->walks()) {
                if (tile.getTileType()->canWalkOn(d) &&
                    prev_tile.getTileType()->canWalkOff(d) &&
                    tile.getTileType()->isCreatureWalkable())
                    retval = DIR_ADD_TO_MASK(d, retval);
            }
            // Creatures that can move onto player
            else if (ontoAvatar && m->canMoveOntoPlayer())
            {

            	//tile should be transport
            	if (tile.getTileType()->isShip() && m->swims())
            		retval = DIR_ADD_TO_MASK(d, retval);

            }
        }
    }

    return retval;
}

bool Map::move(Object *obj, Direction d) {
    Coords new_coords = obj->getCoords();
    movedir(&new_coords, d);
    if (!xu4_coords_equal(new_coords, obj->getCoords())) {
        obj->setCoords(new_coords);
        return true;
    }
    return false;
}

/**
 * Alerts the guards that the avatar is doing something bad
 */ 
void Map::alertGuards() {
    ObjectDeque::iterator i;    
    const Creature *m;

    /* switch all the guards to attack mode */
    for (i = objects.begin(); i != objects.end(); i++) {
        m = creatureMgr->getByTile((*i)->getTile());
        if (m && (m->getId() == GUARD_ID || m->getId() == LORDBRITISH_ID))
            (*i)->setMovementBehavior(MOVEMENT_ATTACK_AVATAR);
    }
}

const Coords &Map::getLabel(const string &name) const {
    std::map<string, Coords>::const_iterator i = labels.find(name);
    if (i == labels.end()) {
        //return xu4_coords_nowhere();
        xu4_error(XU4_LOG_ERR, "Label not found\n");
	}
    return i->second;
}

bool Map::fillMonsterTable() {
    ObjectDeque::iterator current;
    Object *obj;    
    ObjectDeque monsters;
    ObjectDeque other_creatures;
    ObjectDeque inanimate_objects;    
    Object empty;
    
    int nCreatures = 0;
    int nObjects = 0;    
    int i;
    
    memset(monsterTable, 0, MONSTERTABLE_SIZE * sizeof(SaveGameMonsterRecord));

    /**
     * First, categorize all the objects we have
     */
    for (current = objects.begin(); current != objects.end(); current++) {
        obj = *current;

        /* moving objects first */
        if ((obj->getType() == Object::CREATURE) && (obj->getMovementBehavior() != MOVEMENT_FIXED)) {
            Creature *c = dynamic_cast<Creature*>(obj);            
            /* whirlpools and storms are separated from other moving objects */
            if (c->getId() == WHIRLPOOL_ID || c->getId() == STORM_ID)            
                monsters.push_back(obj);
            else other_creatures.push_back(obj);
        }
        else inanimate_objects.push_back(obj);
    }

    /**
     * Add other monsters to our whirlpools and storms
     */
    while (other_creatures.size() && nCreatures < MONSTERTABLE_CREATURES_SIZE) {
        monsters.push_back(other_creatures.front());
        other_creatures.pop_front();
    }

    /**
     * Add empty objects to our list to fill things up
     */
    while (monsters.size() < MONSTERTABLE_CREATURES_SIZE)
        monsters.push_back(&empty);

    /**
     * Finally, add inanimate objects
     */
    while (inanimate_objects.size() && nObjects < MONSTERTABLE_OBJECTS_SIZE) {
        monsters.push_back(inanimate_objects.front());
        inanimate_objects.pop_front();
    }

    /**
     * Fill in the blanks
     */
    while (monsters.size() < MONSTERTABLE_SIZE)
        monsters.push_back(&empty);

    /**
     * Fill in our monster table
     */
    for (i = 0; i < MONSTERTABLE_SIZE; i++) {
        Coords c = monsters[i]->getCoords(),
               prevc = monsters[i]->getPrevCoords();

        monsterTable[i].tile = translateToRawTileIndex(monsters[i]->getTile());
        monsterTable[i].x = c.x;
        monsterTable[i].y = c.y;
        monsterTable[i].prevTile = translateToRawTileIndex(monsters[i]->getPrevTile());
        monsterTable[i].prevx = prevc.x;
        monsterTable[i].prevy = prevc.y;
    }
    
    return true;
}

MapTile Map::translateFromRawTileIndex(int raw) const {
    xu4_assert(tilemap != NULL, "tilemap hasn't been set");

    return tilemap->translate(raw);
}

unsigned int Map::translateToRawTileIndex(MapTile &tile) const {
    return tilemap->untranslate(tile);
}
