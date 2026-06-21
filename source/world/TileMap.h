#pragma once

//-----------------------------------------------------------------------------
// TileMap.h  (Milestone 1 — replaces bootstrap version)
//
// Changes from bootstrap:
//   - Variable width/height (up to MAX_ZONE_W x MAX_ZONE_H)
//   - loadFromData(): populates from a ZoneDef's tile array
//   - isSolid() uses TILE_FIRST_SOLID threshold (not a hardcoded tile ID)
//   - bgColor exposed for renderer clear call
//-----------------------------------------------------------------------------

#include "../../include/types.h"
#include "ZoneData.h"

class TileMap {
public:
    TileMap();

    // Load tile data from a ZoneDef. Call this every time a new zone is entered.
    void loadFromData(const ZoneDef& def);

    // Returns tile ID at tile coordinate (tx, ty).
    // Returns TILE_FIRST_SOLID for out-of-bounds (treat edge as solid).
    u8 getTile(int tx, int ty) const;

    // Returns true if the tile blocks movement.
    bool isSolid(int tx, int ty) const;

    // Current map dimensions in tiles.
    int getWidthTiles()  const { return m_width; }
    int getHeightTiles() const { return m_height; }

    // Current map dimensions in pixels.
    int getWidthPixels()  const { return m_width  * TILE_SIZE; }
    int getHeightPixels() const { return m_height * TILE_SIZE; }

    // Background clear color for this zone (set by loadFromData).
    u32 getBgColor() const { return m_bgColor; }

private:
    u8  m_tiles[MAX_ZONE_H][MAX_ZONE_W];
    int m_width;
    int m_height;
    u32 m_bgColor;
};
