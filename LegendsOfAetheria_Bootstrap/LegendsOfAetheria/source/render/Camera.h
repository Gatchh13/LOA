#pragma once

//-----------------------------------------------------------------------------
// Camera.h
// Follows the player and clamps to map boundaries.
// Output is a pixel offset (cx, cy) that the renderer subtracts from world
// positions to get screen positions.
//-----------------------------------------------------------------------------

#include "../../include/types.h"

class Camera {
public:
    Camera();

    // Centers on the given world-pixel position and clamps to the map.
    // mapPixelW / mapPixelH: total map size in pixels.
    void update(float targetX, float targetY,
                int   mapPixelW, int mapPixelH);

    // Top-left corner of the viewport in world-pixel coordinates.
    int getX() const { return m_x; }
    int getY() const { return m_y; }

    // Convert a world pixel position to a screen pixel position.
    int worldToScreenX(float wx) const { return static_cast<int>(wx) - m_x; }
    int worldToScreenY(float wy) const { return static_cast<int>(wy) - m_y; }

private:
    int m_x;
    int m_y;
};
