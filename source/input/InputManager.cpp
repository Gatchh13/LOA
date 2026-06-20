//-----------------------------------------------------------------------------
// InputManager.cpp
//-----------------------------------------------------------------------------

#include "InputManager.h"
#include <3ds.h>
#include <algorithm>

InputManager::InputManager()
    : m_keysHeld(0)
    , m_keysDown(0)
{
    m_circle = {0, 0};
}

void InputManager::update() {
    hidScanInput();
    m_keysHeld = hidKeysHeld();
    m_keysDown = hidKeysDown();
    hidCircleRead(&m_circle);
}

bool InputManager::isHeld(u32 key) const {
    return (m_keysHeld & key) != 0;
}

bool InputManager::isPressed(u32 key) const {
    return (m_keysDown & key) != 0;
}

bool InputManager::startPressed() const {
    return (m_keysDown & KEY_START) != 0;
}

Vec2 InputManager::getMovementAxis() const {
    float dx = 0.0f;
    float dy = 0.0f;

    // --- D-Pad ---
    if (m_keysHeld & KEY_LEFT)  dx -= 1.0f;
    if (m_keysHeld & KEY_RIGHT) dx += 1.0f;
    if (m_keysHeld & KEY_UP)    dy -= 1.0f;
    if (m_keysHeld & KEY_DOWN)  dy += 1.0f;

    // --- Circle Pad ---
    // Only apply if above dead zone
    if (m_circle.dx > DEADZONE || m_circle.dx < -DEADZONE ||
        m_circle.dy > DEADZONE || m_circle.dy < -DEADZONE)
    {
        // Circle Pad reports +Y as up; our screen-space Y is +down.
        // Normalize from raw [-154, 154] to [-1, 1].
        float cx = static_cast<float>(m_circle.dx) / 154.0f;
        float cy = static_cast<float>(m_circle.dy) / 154.0f;  // flip Y

        // Clamp to [-1, 1]
        cx = std::max(-1.0f, std::min(1.0f, cx));
        cy = std::max(-1.0f, std::min(1.0f, cy));

        // Only override D-Pad if circle pad has actual input
        if (cx != 0.0f) dx = cx;
        if (cy != 0.0f) dy = -cy;  // flip: Circle Pad up = negative screen Y
    }

    return Vec2(dx, dy);
}
