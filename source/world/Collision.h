#pragma once

//-----------------------------------------------------------------------------
// Collision.h  (unchanged interface from bootstrap)
// Axis-separated tile AABB collision resolution.
//-----------------------------------------------------------------------------

#include "../../include/types.h"
#include "TileMap.h"

class Collision {
public:
    // Resolves movement for a rectangular entity.
    // Returns the final position after collision response.
    static Vec2 resolve(const TileMap& map,
                        Vec2 pos,
                        Vec2 velocity,
                        int  width,
                        int  height);

private:
    static bool rectOverlapsSolid(const TileMap& map,
                                  float px, float py,
                                  int   width, int height);
};

