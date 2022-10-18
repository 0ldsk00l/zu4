/*
 * $Id: map.h 2809 2011-01-28 06:22:04Z darren_janeczek $
 */

#ifndef MAP_H
#define MAP_H

#include <list>
#include <map>
#include <string>
#include <vector>

#include "coords.h"
#include "direction.h"
#include "music.h"
#include "object.h"
#include "savegame.h"
#include "types.h"
#include "u4file.h"

using std::string;

#define MAP_IS_OOB(mapptr, c) (((c).x) < 0 || ((c).x) >= (static_cast<int>((mapptr)->width)) || ((c).y) < 0 || ((c).y) >= (static_cast<int>((mapptr)->height)) || ((c).z) < 0 || ((c).z) >= (static_cast<int>((mapptr)->levels)))

struct AnnotationMgr;
struct Map;
struct Object;
struct Person;
struct Creature;
struct TileMap;
struct Tileset;
struct Portal;
struct _Dungeon;

typedef std::vector<Portal *> PortalList;
typedef std::list<int> CompressedChunkList;
typedef std::vector<MapTile> MapData;

/* flags */
#define SHOW_AVATAR (1 << 0)
#define NO_LINE_OF_SIGHT (1 << 1)
#define FIRST_PERSON (1 << 2)

/* mapTileAt flags */
#define WITHOUT_OBJECTS     0
#define WITH_GROUND_OBJECTS 1
#define WITH_OBJECTS        2

int movementDistance(Coords oc, Coords c, const struct Map *map = NULL);
int distance(Coords oc, Coords c, const struct Map *map = NULL);
void movedir(Coords *oc, Direction d, const struct Map *map = NULL);
void movexy(Coords *oc, int dx, int dy, const struct Map *map = NULL);
void wrap(Coords *oc, const Map *map);
void putInBounds(Coords *oc, const struct Map *map);
int getRelativeDirection(Coords oc, Coords c, const struct Map *map = NULL);
Direction pathTo(Coords oc, Coords c, int valid_dirs = MASK_DIR_ALL, bool towards = true, const struct Map *map = NULL);
Direction pathAway(Coords oc, Coords c, int valid_dirs = MASK_DIR_ALL);

/**
 * Map struct
 */ 
struct Map {
public:
    enum Type {
        WORLD,
        CITY,    
        SHRINE,
        COMBAT,
        DUNGEON
    };

    enum BorderBehavior {
        BORDER_WRAP,
        BORDER_EXIT2PARENT,
        BORDER_FIXED
    };


    struct Source {
    public:
        Source() {}
        Source(const string &f, Type t) : fname(f), type(t) {}

        string fname;
        Type type;
    };

    Map();
    virtual ~Map();

    // Member functions
    virtual string getName();
    
    struct Object *objectAt(const Coords &coords);    
    const Portal *portalAt(const Coords &coords, int actionFlags);
    MapTile* getTileFromData(const Coords &coords);
    MapTile* tileAt(const Coords &coords, int withObjects);
    const Tile *tileTypeAt(const Coords &coords, int withObjects);
    bool isWorldMap();
    bool isEnclosed(const Coords &party);
    struct Creature *addCreature(const struct Creature *m, Coords coords);
    struct Object *addObject(MapTile tile, MapTile prevTile, Coords coords);
    struct Object *addObject(Object *obj, Coords coords);
    void removeObject(const struct Object *rem, bool deleteObject = true);
    ObjectDeque::iterator removeObject(ObjectDeque::iterator rem, bool deleteObject = true);    
    void clearObjects();
    struct Creature *moveObjects(Coords avatar);
    void resetObjectAnimations();
    int getNumberOfCreatures();
    int getValidMoves(Coords from, MapTile transport);
    bool move(Object *obj, Direction d);
    void alertGuards();
    const Coords &getLabel(const string &name) const;

    // u4dos compatibility
    bool fillMonsterTable();    
    MapTile translateFromRawTileIndex(int c) const;
    unsigned int translateToRawTileIndex(MapTile &tile) const;

public:
    MapId           id;    
    string          fname;
    Type            type;
    unsigned int    width,
                    height,
                    levels;
    unsigned int    chunk_width,
                    chunk_height;
    unsigned int    offset;

    Source          baseSource;
    std::list<Source> extraSources;
    
    CompressedChunkList     compressed_chunks;
    BorderBehavior          border_behavior;

    PortalList      portals;
    AnnotationMgr  *annotations;
    int             flags;
    int             music;
    MapData         data;
    ObjectDeque     objects;
    std::map<string, Coords> labels;
    Tileset        *tileset;
    TileMap        *tilemap;

    // u4dos compatibility
    SaveGameMonsterRecord monsterTable[MONSTERTABLE_SIZE];

private:
    // disallow map copying: all maps should be created and accessed
    // through the MapMgr
    Map(const Map &map);
    Map &operator=(const Map &map);

    void findWalkability(Coords coords, int *path_data);
};

#endif
