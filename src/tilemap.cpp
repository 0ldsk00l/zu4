/**
 * $Id: tilemap.cpp 3021 2012-03-18 11:31:48Z daniel_santos $
 */

#include <vector>

#include "tilemap.h"
#include "tile.h"

#include "config.h"
#include "error.h"
#include "tileset.h"

using std::vector;

/**
 * Static variables
 */
TileMap::TileIndexMapMap TileMap::tileMaps;

/**
 * Load all tilemaps from the specified xml file
 */
void TileMap::loadAll() {
    const Config *config = Config::getInstance();    
    vector<ConfigElement> conf;

    /* FIXME: make sure tilesets are loaded by now */    

    zu4_error(ZU4_LOG_DBG, "Unloading all tilemaps");
    unloadAll();

    /* open the filename for the tileset and parse it! */
    zu4_error(ZU4_LOG_DBG, "Loading tilemaps from config");
    conf = config->getElement("tilesets").getChildren();    
    
    /* load all of the tilemaps */
    for (std::vector<ConfigElement>::iterator i = conf.begin(); i != conf.end(); i++) {
        if (i->getName() == "tilemap") {
        
            /* load the tilemap ! */
            load(*i);
        }
    }
}
 
/**
 * Delete all tilemaps
 */
void TileMap::unloadAll() {    
    TileIndexMapMap::iterator map;       
        
    /* free all the memory for the tile maps */
    for (map = tileMaps.begin(); map != tileMaps.end(); map++)
        delete map->second;
    
    /* Clear the map so we don't attempt to delete the memory again
     * next time.
     */
    tileMaps.clear();
}
 
/**
 * Loads a tile map which translates between tile indices and tile
 * names.  Tile maps are useful to translate from dos tile indices to
 * xu4 tile ids.
 */
void TileMap::load(const ConfigElement &tilemapConf) {
    TileMap *tm = new TileMap;
    
    string name = tilemapConf.getString("name");
    zu4_error(ZU4_LOG_DBG, "Tilemap name is: %s",  name.c_str());
    
    string tileset = tilemapConf.getString("tileset");

    int index = 0;
    vector<ConfigElement> children = tilemapConf.getChildren();
    for (std::vector<ConfigElement>::iterator i = children.begin(); i != children.end(); i++) {
        if (i->getName() != "mapping")
            continue;

        /* we assume tiles have already been loaded at this point,
           so let's do some translations! */
        
        int frames = 1;
        string tile = i->getString("tile");

        zu4_error(ZU4_LOG_DBG, "\tLoading %s", tile.c_str());
        
        /* find the tile this references */
        Tile *t = Tileset::get(tileset)->getByName(tile);
        if (!t)
            zu4_error(ZU4_LOG_ERR, "Error: tile '%s' from '%s' was not found in tileset %s", tile.c_str(), name.c_str(), tileset.c_str());
        
        if (i->exists("index"))
            index = i->getInt("index");        
        if (i->exists("frames"))
            frames = i->getInt("frames");

        /* insert the tile into the tile map */
        for (int i = 0; i < frames; i++) {
            if (i < t->getFrames())
                tm->tilemap[index+i] = MapTile(t->getId(), i);
            /* frame fell out of the scope of the tile -- frame is set to 0 */
            else
                tm->tilemap[index+i] = MapTile(t->getId(), 0);
        }
        
        index += frames;
    }
    
    /* add the tilemap to our list */
    tileMaps[name] = tm;
}

/**
 * Returns the Tile index map with the specified name
 */
TileMap *TileMap::get(string name) {
    if (tileMaps.find(name) != tileMaps.end())
        return tileMaps[name];
    else return NULL;    
}

/**
 * Translates a raw index to a MapTile.
 */
MapTile TileMap::translate(unsigned int index) {
    return tilemap[index];
}

unsigned int TileMap::untranslate(MapTile &tile) {
    unsigned int index = 0;

    for (std::map<unsigned int, MapTile>::iterator i = tilemap.begin(); i != tilemap.end(); i++) {
        if (i->second == tile) {
            index = i->first;
            break;
        }
    }

    index += tile.frame;

    return index;
}
