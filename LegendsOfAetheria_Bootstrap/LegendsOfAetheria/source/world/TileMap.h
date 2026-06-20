#pragma once

//-----------------------------------------------------------------------------
// TileMap.h
// Static tile map with ground layer and collision layer.
// 25 x 20 tiles, 16x16 pixels each = 400x320 pixel world
// (slightly taller than the viewport so the camera can scroll).
//-----------------------------------------------------------------------------

#include "../../include/types.h"

class TileMap {
public:
    TileMap();

    // Returns tile ID at tile coordinate (tx, ty).
    // Returns TILE_WALL for out-of-bounds queries.
    u8 getTile(int tx, int ty) const;

    // Returns true if tile (tx, ty) blocks movement.
    bool isSolid(int tx, int ty) const;

    // Width and height in tiles.
    int getWidthTiles()  const { return TILEMAP_W; }
    int getHeightTiles() const { return TILEMAP_H; }

    // Width and height in pixels.
    int getWidthPixels()  const { return TILEMAP_W * TILE_SIZE; }
    int getHeightPixels() const { return TILEMAP_H * TILE_SIZE; }

private:
    // Tile data: 0 = grass (passable), 1 = wall (solid)
    // Layout: [row][col]  row 0 = top
    u8 m_tiles[TILEMAP_H][TILEMAP_W];
};
