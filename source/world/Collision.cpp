//-----------------------------------------------------------------------------
// Collision.cpp
//-----------------------------------------------------------------------------

#include "Collision.h"
#include <cmath>

bool Collision::rectOverlapsSolid(const TileMap& map,
                                   float px, float py,
                                   int   width, int height)
{
    // Convert the four corners of the rect to tile coordinates and test each.
    // Using the corners (rather than the center) correctly handles entities
    // that span more than one tile.

    int x0 = static_cast<int>(floorf(px))         / TILE_SIZE;
    int y0 = static_cast<int>(floorf(py))         / TILE_SIZE;
    int x1 = static_cast<int>(floorf(px + width  - 1)) / TILE_SIZE;
    int y1 = static_cast<int>(floorf(py + height - 1)) / TILE_SIZE;

    for (int ty = y0; ty <= y1; ty++) {
        for (int tx = x0; tx <= x1; tx++) {
            if (map.isSolid(tx, ty)) return true;
        }
    }
    return false;
}

Vec2 Collision::resolve(const TileMap& map,
                         Vec2 pos,
                         Vec2 velocity,
                         int  width,
                         int  height)
{
    Vec2 resolved = pos;

    // --- X axis ---
    float newX = pos.x + velocity.x;
    if (!rectOverlapsSolid(map, newX, pos.y, width, height)) {
        resolved.x = newX;
    }

    // --- Y axis ---
    float newY = resolved.y + velocity.y;
    if (!rectOverlapsSolid(map, resolved.x, newY, width, height)) {
        resolved.y = newY;
    }

    return resolved;
}

