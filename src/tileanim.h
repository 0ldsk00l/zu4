/*
 * $Id: tileanim.h 3019 2012-03-18 11:31:13Z daniel_santos $
 */

#ifndef TILEANIM_H
#define TILEANIM_H

#include <string>
#include <map>
#include <vector>

#include "direction.h"

struct ConfigElement;
struct Image;
struct Tile;
struct RGBA;

/**
 * The interface for tile animation transformations.
 */
struct TileAnimTransform {
public:
    static TileAnimTransform *create(const ConfigElement &config);
    static RGBA *loadColorFromConf(const ConfigElement &conf);

    virtual void draw(Image *dest, Tile *tile, MapTile &mapTile) = 0;
    virtual ~TileAnimTransform() {}
    virtual bool drawsTile() const = 0;

    // Properties
    int random;

//private:
//    bool replaces;
};

/**
 * A tile animation transformation that turns a piece of the tile
 * upside down.  Used for animating the flags on building and ships.
 */
struct TileAnimInvertTransform : public TileAnimTransform {
public:
    TileAnimInvertTransform(int x, int y, int w, int h);
    virtual void draw(Image *dest, Tile *tile, MapTile &mapTile);
    virtual bool drawsTile() const;

private:
    int x, y, w, h;
};

/**
 * A tile animation transformation that changes a single pixels to a
 * random color selected from a list.  Used for animating the
 * campfire in EGA mode.
 */
struct TileAnimPixelTransform : public TileAnimTransform {
public:
    TileAnimPixelTransform(int x, int y);
    virtual void draw(Image *dest, Tile *tile, MapTile &mapTile);
    virtual bool drawsTile() const;

    int x, y;
    std::vector<RGBA *> colors;
};

/**
 * A tile animation transformation that scrolls the tile's contents
 * vertically within the tile's boundaries.
 */
struct TileAnimScrollTransform : public TileAnimTransform {
public:
    TileAnimScrollTransform(int increment);
    virtual void draw(Image *dest, Tile *tile, MapTile &mapTile);
    virtual bool drawsTile() const;
private:
    int increment, current, lastOffset;
};

/**
 * A tile animation transformation that advances the tile's frame
 * by 1.
 */
struct TileAnimFrameTransform : public TileAnimTransform {
public:
	TileAnimFrameTransform() : currentFrame(0){}
    virtual void draw(Image *dest, Tile *tile, MapTile &mapTile);
    virtual bool drawsTile() const;
protected:
    int currentFrame;
};

/**
 * A tile animation transformation that changes pixels with colors
 * that fall in a given range to another color.  Used to animate
 * the campfire in VGA mode.
 */
struct TileAnimPixelColorTransform : public TileAnimTransform {
public:
    TileAnimPixelColorTransform(int x, int y, int w, int h);
    virtual void draw(Image *dest, Tile *tile, MapTile &mapTile);
    virtual bool drawsTile() const;

    int x, y, w, h;
    RGBA *start, *end;
};

/**
 * A context in which to perform the animation
 */
struct TileAnimContext {
public:
    typedef std::vector<TileAnimTransform *> TileAnimTransformList;
    typedef enum {
        FRAME,
        DIR
    } Type;

    static TileAnimContext* create(const ConfigElement &config);

    void add(TileAnimTransform*);
    virtual bool isInContext(Tile *t, MapTile &mapTile, Direction d) = 0;
	TileAnimTransformList& getTransforms() {return animTransforms;}	/**< Returns a list of transformations under the context. */
    virtual ~TileAnimContext() {}
private:

    TileAnimTransformList animTransforms;
};

/**
 * An animation context which changes the animation based on the tile's current frame
 */
struct TileAnimFrameContext : public TileAnimContext {
public:
    TileAnimFrameContext(int frame);
    virtual bool isInContext(Tile *t, MapTile &mapTile, Direction d);

private:
    int frame;
};

/**
 * An animation context which changes the animation based on the player's current facing direction
 */
struct TileAnimPlayerDirContext : public TileAnimContext {
public:
    TileAnimPlayerDirContext(Direction dir);
    virtual bool isInContext(Tile *t, MapTile &mapTile, Direction d);

private:
    Direction dir;
};

/**
 * Instructions for animating a tile.  Each tile animation is made up
 * of a list of transformations which are applied to the tile after it
 * is drawn.
 */
struct TileAnim {
public:
    TileAnim(const ConfigElement &conf);

    std::string name;
    std::vector<TileAnimTransform *> transforms;
    std::vector<TileAnimContext *> contexts;

    /* returns the frame to set the mapTile to (only relevent if persistent) */
    void draw(Image *dest, Tile *tile, MapTile &mapTile, Direction dir);

    int random;   /* true if the tile animation occurs randomely */
};

/**
 * A set of tile animations.  Tile animations are associated with a
 * specific image set which shares the same name.
 */
struct TileAnimSet {
    typedef std::map<std::string, TileAnim *> TileAnimMap;

public:
    TileAnimSet(const ConfigElement &conf);

    TileAnim *getByName(const std::string &name);

    std::string name;
    TileAnimMap tileanims;
};

#endif
