/*
 * $Id: tile.cpp 3028 2012-03-18 12:02:46Z daniel_santos $
 */


#include "tile.h"

#include "config.h"
#include "context.h"
#include "creature.h"
#include "error.h"
#include "image.h"
#include "imagemgr.h"
#include "location.h"
#include "settings.h"
#include "tileanim.h"
#include "tilemap.h"
#include "tileset.h"
#include "utils.h"
#include "assert.h"

TileId Tile::nextId = 0;

Tile::Tile(Tileset *tileset)
    : id(nextId++)
    , name()
    , tileset(tileset)
    , w(0)
    , h(0)
    , frames(0)
    , scale(1)
    , anim(NULL)
    , opaque(false)
    , foreground()
    , waterForeground()
    , rule(NULL)
    , imageName()
    , looks_like()
    , image(NULL)
    , tiledInDungeon(false)
    , directions()
    , animationRule("") {
}

/**
 * Loads tile information.
 */
void Tile::loadProperties(const ConfigElement &conf) {
    if (conf.getName() != "tile")
        return;
            
    name = conf.getString("name"); /* get the name of the tile */

    /* get the animation for the tile, if one is specified */
    if (conf.exists("animation")) {
        animationRule = conf.getString("animation");
    }

    /* see if the tile is opaque */
    opaque = conf.getBool("opaque"); 

    foreground = conf.getBool("usesReplacementTileAsBackground");
    waterForeground = conf.getBool("usesWaterReplacementTileAsBackground");

    /* find the rule that applies to the current tile, if there is one.
       if there is no rule specified, it defaults to the "default" rule */
    if (conf.exists("rule")) {
        rule = TileRule::findByName(conf.getString("rule"));
        if (rule == NULL)
            rule = TileRule::findByName("default");
    }
    else rule = TileRule::findByName("default");

    /* get the number of frames the tile has */    
    frames = conf.getInt("frames", 1);

    /* get the name of the image that belongs to this tile */
    if (conf.exists("image"))
        imageName = conf.getString("image");
    else 
        imageName = std::string("tile_") + name;

    tiledInDungeon = conf.getBool("tiledInDungeon");

    if (conf.exists("directions")) {
        std::string dirs = conf.getString("directions");
        if (dirs.length() != (unsigned) frames)
            zu4_error(ZU4_LOG_ERR, "Error: %ld directions for tile but only %d frames", (long) dirs.length(), frames);
        for (unsigned i = 0; i < dirs.length(); i++) {
            if (dirs[i] == 'w')
                directions.push_back(DIR_WEST);
            else if (dirs[i] == 'n')
                directions.push_back(DIR_NORTH);
            else if (dirs[i] == 'e')
                directions.push_back(DIR_EAST);
            else if (dirs[i] == 's')
                directions.push_back(DIR_SOUTH);
            else
                zu4_error(ZU4_LOG_ERR, "Error: unknown direction specified by %c", dirs[i]);
        }
    }
}

Image *Tile::getImage() {
    if (!image)
        loadImage();
    return image;
}

/**
 * Loads the tile image
 */ 
void Tile::loadImage() {
    if (!image) {
    	SubImage *subimage = NULL;

        ImageInfo *info = imageMgr->get(imageName);
        if (!info) {
            subimage = imageMgr->getSubImage(imageName);
            if (subimage)            
                info = imageMgr->get(subimage->srcImageName);            
        }
        if (!info) //IF still no info loaded
        {
            zu4_error(ZU4_LOG_WRN, "Error: couldn't load image for tile '%s'", name.c_str());
            return;
        }

        /* FIXME: This is a hack to address the fact that there are 4
           frames for the guard in VGA mode, but only 2 in EGA. Is there
           a better way to handle this? */
        if (name == "guard")
        {
        	if (!settings.videoType) // EGA
        		frames = 2;
        	else
        		frames = 4;
        }

        if (info) {
            w = (subimage ? subimage->width : info->width);
            h = (subimage ? (subimage->height) / frames : info->height / frames);
            image = zu4_img_create(w, h * frames);


            //info->image->alphaOff();

            /* draw the tile from the image we found to our tile image */
            if (subimage) {
                Image *tiles = info->image;
                zu4_img_draw_subrect_on(image, tiles, 0, 0, subimage->x, subimage->y, subimage->width, subimage->height);
            }
            else zu4_img_draw_on(image, info->image, 0, 0);
        }

        if (animationRule.size() > 0) {
            extern TileAnimSet *tileanims;

            anim = NULL;
            if (tileanims)
                anim = tileanims->getByName(animationRule);
            if (anim == NULL)
                zu4_error(ZU4_LOG_WRN, "Warning: animation style '%s' not found", animationRule.c_str());
        }

        /* if we have animations, we always used 'animated' to draw from */
        //if (anim)
        //    image->alphaOff();


    }
}

void Tile::deleteImage()
{
    if(image) {
        delete image;
        image = NULL;
    }
    scale = 1;
}

/**
 * MapTile Class Implementation
 */
Direction MapTile::getDirection() const {
    return getTileType()->directionForFrame(frame);
}

bool MapTile::setDirection(Direction d) {
    /* if we're already pointing the right direction, do nothing! */
    if (getDirection() == d)
        return false;

    const Tile *type = getTileType();

    int new_frame = type->frameForDirection(d);
    if (new_frame != -1) {
        frame = new_frame;
        return true;
    }
    return false;
}

bool Tile::isDungeonFloor() const {
    Tile *floor = tileset->getByName("brick_floor");
    if (id == floor->id)
        return true;
    return false;
}

bool Tile::isOpaque() const {
    extern Context *c;
    return c->opacity ? opaque : false;
}

/**
 * Is tile a foreground tile (i.e. has transparent parts).
 * Deprecated? Never used in XML. Other mechanisms exist, though this could help?
 */
bool Tile::isForeground() const {
    return (rule->mask & MASK_FOREGROUND);
}

Direction Tile::directionForFrame(int frame) const {
    if (static_cast<unsigned>(frame) >= directions.size())
        return DIR_NONE;
    else
        return directions[frame];
}

int Tile::frameForDirection(Direction d) const {
    for (int i = 0; (unsigned) i < directions.size() && i < frames; i++) {
        if (directions[i] == d)
            return i;
    }
    return -1;
}


const Tile *MapTile::getTileType() const {
    return Tileset::findTileById(id);
}
