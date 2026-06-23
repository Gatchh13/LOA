#pragma once

//-----------------------------------------------------------------------------
// InputManager.h
// Wraps libctru's hidScan/hidKeysDown/hidKeysHeld and Circle Pad.
// Call update() once at the top of the game loop before any input queries.
//-----------------------------------------------------------------------------

#include "../../include/types.h"

class InputManager {
public:
    InputManager();

    // Must be called once per frame before isHeld / isPressed.
    void update();

    // True every frame the button is held.
    bool isHeld(u32 key) const;

    // True only on the frame the button was first pressed.
    bool isPressed(u32 key) const;

    // Normalized movement direction combining D-Pad and Circle Pad.
    // Returns Vec2 with components in [-1, 1]. May not be unit-length.
    Vec2 getMovementAxis() const;

    // True if the player pressed START (used for clean shutdown).
    bool startPressed() const;

private:
    u32  m_keysHeld;
    u32  m_keysDown;
    circlePosition m_circle;

    // Circle Pad dead zone (raw value range is roughly -154 to 154)
    static constexpr s16 DEADZONE = 20;
};

