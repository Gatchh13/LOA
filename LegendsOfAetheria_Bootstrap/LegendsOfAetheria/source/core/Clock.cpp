//-----------------------------------------------------------------------------
// Clock.cpp
//-----------------------------------------------------------------------------

#include "Clock.h"
#include <3ds.h>
#include <algorithm>

// The 3DS CPU tick runs at 268,111,856 Hz on Old 3DS.
// We use this to compute sub-millisecond delta times.
static constexpr double CPU_TICKS_PER_SEC = 268111856.0;

Clock::Clock()
    : m_lastTick(0)
    , m_delta(0.0f)
    , m_fps(0.0f)
    , m_fpsAccum(0.0f)
    , m_fpsFrames(0)
{
    m_lastTick = svcGetSystemTick();
}

void Clock::tick() {
    u64 now    = svcGetSystemTick();
    u64 diff   = now - m_lastTick;
    m_lastTick = now;

    float raw  = static_cast<float>(diff / CPU_TICKS_PER_SEC);
    m_delta    = std::min(raw, MAX_DELTA);

    // FPS: count frames over 1-second windows
    m_fpsAccum  += m_delta;
    m_fpsFrames += 1;

    if (m_fpsAccum >= 1.0f) {
        m_fps       = static_cast<float>(m_fpsFrames) / m_fpsAccum;
        m_fpsAccum  = 0.0f;
        m_fpsFrames = 0;
    }
}
