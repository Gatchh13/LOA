#pragma once

//-----------------------------------------------------------------------------
// Collision.h
// Tile-based AABB collision using axis separation.
//
// The approach:
//   1. Try moving on X axis only.  If that position is solid, cancel X.
//   2. Try moving on Y axis only.  If that position is solid, cancel Y.
//
// This gives smooth wall-sliding without tunnelling at normal speeds.
//-----------------------------------------------------------------------------

#include "../../include/types.h"
#include "TileMap.h"

class Collision {
public:
    // Resolves movement for a rectangular entity.
    //
    // Parameters:
    //   map       - the tile map to test against
    //   pos       - current pixel position (top-left of entity rect)
    //   velocity  - proposed movement in pixels (delta * speed)
    //   width     - entity width  in pixels
    //   height    - entity height in pixels
    //
    // Returns the resolved position after collision.
    static Vec2 resolve(const TileMap& map,
                        Vec2 pos,
                        Vec2 velocity,
                        int  width,
                        int  height);

private:
    // Tests whether a pixel-space rect overlaps any solid tile.
    static bool rectOverlapsSolid(const TileMap& map,
                                  float px, float py,
                                  int   width, int height);
};
