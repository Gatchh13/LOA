#pragma once

//-----------------------------------------------------------------------------
// TileMap.h  (Milestone 4 — adds runtime tile override layer)
//
// New in Milestone 4:
//   setTileOverride(tx, ty, tileId) — write a runtime tile value
//   clearOverrides()                — reset all overrides (call on zone load)
//   isSolid() checks override layer BEFORE the base layer
//
// Override layer design:
//   Fixed array of MAX_TILE_OVERRIDES entries.
//   Linear scan — at most 16 entries, scanned at most 4 times per collision
//   rect check. Negligible CPU cost.
//   0xFF in the override tile field = entry unused.
//-----------------------------------------------------------------------------

#include "../../include/types.h"
#include "ZoneData.h"

static constexpr int MAX_TILE_OVERRIDES = 16;

struct TileOverrideEntry {
    u8 tx;
    u8 ty;
    u8 tile_id;   // 0xFF = unused slot
};

class TileMap {
public:
    TileMap();

    // Load tile data from a ZoneDef. Clears overrides.
    void loadFromData(const ZoneDef& def);

    // Runtime override: set a specific tile to a different ID.
    // Overwrites any existing override for the same (tx,ty).
    void setTileOverride(int tx, int ty, u8 tileId);

    // Remove all runtime overrides (call after loadFromData).
    void clearOverrides();

    // Returns tile ID at (tx, ty). Override layer checked first.
    u8 getTile(int tx, int ty) const;

    // Returns true if tile is impassable.
    bool isSolid(int tx, int ty) const;

    int getWidthTiles()   const { return m_width; }
    int getHeightTiles()  const { return m_height; }
    int getWidthPixels()  const { return m_width  * TILE_SIZE; }
    int getHeightPixels() const { return m_height * TILE_SIZE; }
    u32 getBgColor()      const { return m_bgColor; }

private:
    u8  m_tiles[MAX_ZONE_H][MAX_ZONE_W];
    int m_width;
    int m_height;
    u32 m_bgColor;

    TileOverrideEntry m_overrides[MAX_TILE_OVERRIDES];
};
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
