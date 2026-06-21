//-----------------------------------------------------------------------------
// TileMap.cpp  (Milestone 1)
//-----------------------------------------------------------------------------

#include "TileMap.h"
#include <cstring>

TileMap::TileMap()
    : m_width(0)
    , m_height(0)
    , m_bgColor(C2D_Color32(20, 20, 20, 255))
{
    memset(m_tiles, TILE_WALL, sizeof(m_tiles));
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
}

u8 TileMap::getTile(int tx, int ty) const {
    if (tx < 0 || ty < 0 || tx >= m_width || ty >= m_height) {
        return TILE_FIRST_SOLID;
    }
    return m_tiles[ty][tx];
}

bool TileMap::isSolid(int tx, int ty) const {
    return getTile(tx, ty) >= TILE_FIRST_SOLID;
}
