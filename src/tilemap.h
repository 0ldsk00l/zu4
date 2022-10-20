/**
 * $Id: tilemap.h 2552 2005-09-26 07:34:16Z andrewtaylor $
 */

#ifndef TILEMAP_H
#define TILEMAP_H

#include <map>
#include <string>
#include "types.h"

struct ConfigElement;

/**
 * A tilemap maps the raw bytes in a map file to MapTiles.
 */
struct TileMap {
public:
    typedef std::map<std::string, TileMap *> TileIndexMapMap;

    MapTile translate(unsigned int index);
    unsigned int untranslate(MapTile &tile);

    static void loadAll();
    static void unloadAll();
    static TileMap *get(std::string name);

private:
    static void load(const ConfigElement &tilemapConf);
    static TileIndexMapMap tileMaps;

    std::map<unsigned int, MapTile> tilemap;
};

#endif
