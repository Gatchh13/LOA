//-----------------------------------------------------------------------------
// Camera.cpp
//-----------------------------------------------------------------------------

#include "Camera.h"
#include <algorithm>

Camera::Camera() : m_x(0), m_y(0) {}

void Camera::update(float targetX, float targetY,
                    int   mapPixelW, int mapPixelH)
{
    // Center the viewport on the target position.
    int cx = static_cast<int>(targetX) - SCREEN_TOP_W / 2;
    int cy = static_cast<int>(targetY) - SCREEN_TOP_H / 2;

    // Clamp so we never show outside the map.
    int maxX = mapPixelW - SCREEN_TOP_W;
    int maxY = mapPixelH - SCREEN_TOP_H;

    m_x = std::max(0, std::min(cx, maxX));
    m_y = std::max(0, std::min(cy, maxY));
}
