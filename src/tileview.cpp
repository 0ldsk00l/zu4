/*
 * $Id: tileview.cpp 3019 2012-03-18 11:31:13Z daniel_santos $
 */

#include <stdint.h>

#include "image.h"
#include "imagemgr.h"
#include "settings.h"
#include "screen.h"
#include "tile.h"
#include "tileanim.h"
#include "tileset.h"
#include "tileview.h"
#include "u4.h"
#include "error.h"

TileView::TileView(int x, int y, int columns, int rows) : View(x, y, columns * TILE_WIDTH, rows * TILE_HEIGHT) {
    this->columns = columns;
    this->rows = rows;
    this->tileWidth = TILE_WIDTH;
    this->tileHeight = TILE_HEIGHT;
    this->tileset = Tileset::get("base");
    animated = zu4_img_create(tileWidth, tileHeight);
}

TileView::TileView(int x, int y, int columns, int rows, const std::string &tileset) : View(x, y, columns * TILE_WIDTH, rows * TILE_HEIGHT) {
    this->columns = columns;
    this->rows = rows;
    this->tileWidth = TILE_WIDTH;
    this->tileHeight = TILE_HEIGHT;
    this->tileset = Tileset::get(tileset);
    animated = zu4_img_create(tileWidth, tileHeight);
}

TileView::~TileView() {
    delete animated;
}

void TileView::reinit() {
    View::reinit();
    tileset = Tileset::get("base");

    //Scratchpad needs to be re-inited if we rescale...
    if (animated)
    {
    	delete animated;
    	animated = NULL;
    }
    animated = zu4_img_create(tileWidth, tileHeight);
}

void TileView::loadTile(MapTile &mapTile)
{
	//This attempts to preload tiles in advance
    Tile *tile = tileset->get(mapTile.id);
    if (tile)
    {
    	tile->getImage();
    }
    //But may fail if the tiles don't exist directly in the expected imagesets
}

void TileView::drawTile(MapTile &mapTile, bool focus, int x, int y) {
    Tile *tile = tileset->get(mapTile.id);
    Image *image = tile->getImage();

    zu4_assert(x < columns, "x value of %d out of range", x);
    zu4_assert(y < rows, "y value of %d out of range", y);

    //Blank scratch pad
	zu4_img_fill(animated, 0,0,tileWidth,tileHeight,0,0,0,255);
	//Draw blackness on the tile.
	zu4_img_draw_subrect(animated, x * tileWidth + this->x,
						  y * tileHeight + this->y,
						  0,
						  0,
						  tileWidth,
						  tileHeight);

    // draw the tile to the screen
    if (tile->getAnim()) {
        // First, create our animated version of the tile
        tile->getAnim()->draw(animated, tile, mapTile, DIR_NONE);

        // Then draw it to the screen
        zu4_img_draw_subrect(animated, x * tileWidth + this->x,
                              y * tileHeight + this->y,
                              0,
                              0,
                              tileWidth,
                              tileHeight);
    }
    else {
        zu4_img_draw_subrect(image, x * tileWidth + this->x,
                           y * tileHeight + this->y,
                           0,
                           tileHeight * mapTile.frame,
                           tileWidth,
                           tileHeight);
    }

    // draw the focus around the tile if it has the focus
    if (focus)
        drawFocus(x, y);
}

void TileView::drawTile(std::vector<MapTile> &tiles, bool focus, int x, int y) {
	zu4_assert(x < columns, "x value of %d out of range", x);
	zu4_assert(y < rows, "y value of %d out of range", y);

	zu4_img_fill(animated, 0,0,tileWidth,tileHeight,0,0,0,255);
	zu4_img_draw_subrect(animated, x * tileWidth + this->x,
						  y * tileHeight + this->y,
						  0,
						  0,
						  tileWidth,
						  tileHeight);

	//int layer = 0;

	for (std::vector<MapTile>::reverse_iterator t = tiles.rbegin();
			t != tiles.rend();
			++t)
	{
		MapTile& frontTile = *t;
		Tile *frontTileType = tileset->get(frontTile.id);

		if (!frontTileType)
		{
			//TODO, this leads to an error. It happens after graphics mode changes.
			return;
		}

		Image *image = frontTileType->getImage();

		// draw the tile to the screen
		if (frontTileType->getAnim()) {
			// First, create our animated version of the tile
			frontTileType->getAnim()->draw(animated, frontTileType, frontTile, DIR_NONE);
		}
		else {
            if (!image)
                return; //This is a problem //FIXME, error message it.
			zu4_img_draw_subrect_on(animated, image,
								0, 0,
								0, tileHeight * frontTile.frame,
								tileWidth,  tileHeight);
		}

		// Then draw it to the screen
		zu4_img_draw_subrect(animated, x * tileWidth + this->x,
							  y * tileHeight + this->y,
							  0,
							  0,
							  tileWidth,
							  tileHeight);
	}

	// draw the focus around the tile if it has the focus
	if (focus)
        drawFocus(x, y);
}

/**
 * Draw a focus rectangle around the tile
 */
void TileView::drawFocus(int x, int y) {
    zu4_assert(x < columns, "x value of %d out of range", x);
    zu4_assert(y < rows, "y value of %d out of range", y);

    /*
     * draw the focus rectangle around the tile
     */
    if ((screenCurrentCycle * 4 / SCR_CYCLE_PER_SECOND) % 2) {
        /* left edge */
        zu4_img_fill(screen, x * tileWidth + this->x,
                         y * tileHeight + this->y,
                         2,
                         tileHeight,
                         0xff, 0xff, 0xff, 0xff);

        /* top edge */
        zu4_img_fill(screen, x * tileWidth + this->x,
                         y * tileHeight + this->y,
                         tileWidth,
                         2,
                         0xff, 0xff, 0xff, 0xff);

        /* right edge */
        zu4_img_fill(screen, (x + 1) * tileWidth + this->x - 2,
                         y * tileHeight + this->y,
                         2,
                         tileHeight,
                         0xff, 0xff, 0xff, 0xff);

        /* bottom edge */
        zu4_img_fill(screen, x * tileWidth + this->x,
                         (y + 1) * tileHeight + this->y - 2,
                         tileWidth,
                         2,
                         0xff, 0xff, 0xff, 0xff);
    }
}

void TileView::setTileset(Tileset *tileset) {
    this->tileset = tileset;
}
