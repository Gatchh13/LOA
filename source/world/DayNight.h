#pragma once

//-----------------------------------------------------------------------------
// DayNight.h
// Maps world clock time to a screen tint color for day/night atmosphere.
//
// Phases:
//   DAWN     05:00–07:00   warm orange-pink tint, brightening
//   DAY      07:00–18:00   no tint (full color)
//   EVENING  18:00–20:00   warm amber tint, dimming
//   NIGHT    20:00–05:00   deep blue overlay, dark
//
// Implementation:
//   Returns a u32 color (RGBA8) to pass to Renderer::drawTint().
//   The renderer draws a full-screen rect at that color — identical to
//   drawFade() but with a colored overlay instead of black.
//   Alpha 0 = no tint. Alpha 180 = heavy night overlay.
//
// Performance: pure arithmetic, zero allocations, called once per frame.
//-----------------------------------------------------------------------------

#include "../../include/types.h"

enum class DayPhase : u8 {
    DAWN    = 0,
    DAY     = 1,
    EVENING = 2,
    NIGHT   = 3
};

class DayNight {
public:
    DayNight();

    // Update tint based on current world time.
    // timeFloat: current time as float hours (e.g. 14.5 = 14:30).
    void update(float timeFloat);

    // RGBA8 tint color to overlay on the screen.
    // Draw this after world geometry, before UI.
    u32 getTintColor() const { return m_tintColor; }

    // Current phase for external logic (NPC schedules check this).
    DayPhase getPhase() const { return m_phase; }

    // Human-readable phase name for debug display.
    const char* getPhaseName() const;

private:
    u32      m_tintColor;
    DayPhase m_phase;

    // Linearly interpolate between two RGBA colors by t [0,1].
    static u32 lerpColor(u32 a, u32 b, float t);

    // Extract/pack RGBA components.
    static u8 r(u32 c) { return (c >> 24) & 0xFF; }
    static u8 g(u32 c) { return (c >> 16) & 0xFF; }
    static u8 b(u32 c) { return (c >>  8) & 0xFF; }
    static u8 a(u32 c) { return  c        & 0xFF; }
};

