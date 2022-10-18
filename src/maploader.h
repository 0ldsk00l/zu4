/*
 * $Id: maploader.h 2623 2005-11-16 07:43:15Z andrewtaylor $
 */

#ifndef MAPLOADER_H
#define MAPLOADER_H

#include <map>
#include <string>

#include "map.h"

struct U4FILE;
struct Dungeon;

/**
 * The generic map loader interface.  Map loaders should override the
 * load method to load a map from the meta data already initialized in
 * the map object passed in. They must also register themselves with
 * registerLoader for one or more Map::Types.
 *
 * @todo
 * <ul>
 *      <li>
 *          Instead of loading dungeon room data into a u4dos-style structure and converting it to
 *          an xu4 Map when it's needed, convert it to an xu4 Map immediately upon loading it.
 *      </li>
 * </ul>
 */
struct MapLoader {
public:
    virtual ~MapLoader() {}

    static MapLoader *getLoader(Map::Type type);

    virtual bool load(Map *map) = 0;

protected:
    static MapLoader *registerLoader(MapLoader *loader, Map::Type type);
    static bool loadData(Map *map, U4FILE *f);
    static bool isChunkCompressed(Map *map, int chunk);

private:
    static std::map<Map::Type, MapLoader *> *loaderMap;
};

struct CityMapLoader : public MapLoader {
    static MapLoader *instance;

public:
    virtual bool load(Map *map);

};

struct ConMapLoader : public MapLoader {
    static MapLoader *instance;

public:
    virtual bool load(Map *map);

};

struct DngMapLoader : public MapLoader {
    static MapLoader *instance;

public:
    virtual bool load(Map *map);

private:
    void initDungeonRoom(Dungeon *dng, int room);
};

struct WorldMapLoader : public MapLoader {
    static MapLoader *instance;

public:
    virtual bool load(Map *map);

};

#endif /* MAPLOADER_H */
