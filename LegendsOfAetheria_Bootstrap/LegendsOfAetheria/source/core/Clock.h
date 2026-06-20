#pragma once

//-----------------------------------------------------------------------------
// Clock.h
// Delta time and FPS counter.
// Uses the 3DS CPU tick counter (svcGetSystemTick) for high-resolution timing.
//-----------------------------------------------------------------------------

#include "../../include/types.h"

class Clock {
public:
    Clock();

    // Call once per frame at the top of the game loop.
    // Updates delta time and FPS counter.
    void tick();

    // Seconds since last frame. Capped at 0.05 to avoid physics tunnelling
    // on frame spikes (e.g. zone loads).
    float getDelta() const { return m_delta; }

    // Smoothed frames per second (updated once per second).
    float getFPS() const { return m_fps; }

private:
    static constexpr float MAX_DELTA = 0.05f;  // 20 fps floor for physics

    u64   m_lastTick;
    float m_delta;
    float m_fps;
    float m_fpsAccum;
    int   m_fpsFrames;
};
