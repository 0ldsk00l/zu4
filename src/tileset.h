/*
 * $Id: tileset.h 3019 2012-03-18 11:31:13Z daniel_santos $
 */

#ifndef TILESET_H
#define TILESET_H

#include <string>
#include <map>
#include "types.h"

struct ConfigElement;
struct Tile;

typedef std::map<std::string, struct TileRule *> TileRuleMap;

/**
 * TileRule struct
 */
struct TileRule {
public:    
    static TileRule *findByName(const std::string &name);
    static void load();
    static TileRuleMap rules;   // A map of rule names to rules

    bool initFromConf(const ConfigElement &tileRuleConf);

    std::string name;
    unsigned short mask;    
    unsigned short movementMask;
    TileSpeed speed;
    TileEffect effect;
    int walkonDirs;
    int walkoffDirs;
};

/**
 * Tileset struct
 */
struct Tileset {
public:
    typedef std::map<std::string, Tileset*> TilesetMap;
    typedef std::map<TileId, Tile*> TileIdMap;
    typedef std::map<std::string, Tile*> TileStrMap;

    static void loadAll();
    static void unloadAll();
    static void unloadAllImages();
    static Tileset* get(const std::string &name);

    static Tile* findTileByName(const std::string &name);        
    static Tile* findTileById(TileId id);        

public:
    void load(const ConfigElement &tilesetConf);
    void unload();
    void unloadImages();
    Tile* get(TileId id);
    Tile* getByName(const std::string &name);
    std::string getImageName() const;
    unsigned int numTiles() const;
    unsigned int numFrames() const;    
    
private:
    static TilesetMap tilesets;

    std::string name;
    TileIdMap tiles;
    unsigned int totalFrames;
    std::string imageName;
    Tileset* extends;

    TileStrMap nameMap;
};

#endif
