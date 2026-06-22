//-----------------------------------------------------------------------------
// TileMap.cpp  (Milestone 4 — adds override layer)
//-----------------------------------------------------------------------------

#include "TileMap.h"
#include <cstring>

TileMap::TileMap()
    : m_width(0)
    , m_height(0)
    , m_bgColor(C2D_Color32(20, 20, 20, 255))
{
    memset(m_tiles, TILE_WALL, sizeof(m_tiles));
    clearOverrides();
}

void TileMap::loadFromData(const ZoneDef& def) {
    m_width   = def.width;
    m_height  = def.height;
    m_bgColor = def.bgColor;

    for (int row = 0; row < m_height; row++) {
        for (int col = 0; col < m_width; col++) {
            m_tiles[row][col] = def.tiles[row * m_width + col];
        }
    }
    // Overrides are NOT cleared here — WorldObjectManager calls clearOverrides()
    // then re-applies the correct state via onZoneLoaded(). This ordering
    // ensures WorldObjectManager controls the override lifecycle.
}

void TileMap::clearOverrides() {
    for (int i = 0; i < MAX_TILE_OVERRIDES; i++) {
        m_overrides[i].tx      = 0;
        m_overrides[i].ty      = 0;
        m_overrides[i].tile_id = 0xFF;  // 0xFF = unused
    }
}

void TileMap::setTileOverride(int tx, int ty, u8 tileId) {
    // Find existing entry for this coordinate and update it
    for (int i = 0; i < MAX_TILE_OVERRIDES; i++) {
        if (m_overrides[i].tile_id != 0xFF &&
            m_overrides[i].tx == static_cast<u8>(tx) &&
            m_overrides[i].ty == static_cast<u8>(ty))
        {
            m_overrides[i].tile_id = tileId;
            return;
        }
    }
    // Find empty slot
    for (int i = 0; i < MAX_TILE_OVERRIDES; i++) {
        if (m_overrides[i].tile_id == 0xFF) {
            m_overrides[i].tx      = static_cast<u8>(tx);
            m_overrides[i].ty      = static_cast<u8>(ty);
            m_overrides[i].tile_id = tileId;
            return;
        }
    }
    // Override table full — this should not happen with MAX_TILE_OVERRIDES=16
    // and only 3 objects with at most 3 tiles each (9 overrides max).
}

u8 TileMap::getTile(int tx, int ty) const {
    if (tx < 0 || ty < 0 || tx >= m_width || ty >= m_height) {
        return TILE_FIRST_SOLID;
    }
    // Check override layer first
    for (int i = 0; i < MAX_TILE_OVERRIDES; i++) {
        if (m_overrides[i].tile_id != 0xFF &&
            m_overrides[i].tx == static_cast<u8>(tx) &&
            m_overrides[i].ty == static_cast<u8>(ty))
        {
            return m_overrides[i].tile_id;
        }
    }
    return m_tiles[ty][tx];
}

bool TileMap::isSolid(int tx, int ty) const {
    return getTile(tx, ty) >= TILE_FIRST_SOLID;
}
