//-----------------------------------------------------------------------------
// DayNight.cpp
//-----------------------------------------------------------------------------

#include "DayNight.h"
#include <algorithm>
#include <cmath>

// C2D_Color32 packs as RGBA8: (r<<24)|(g<<16)|(b<<8)|a
// We define tint colors as full-screen overlays.
// Alpha 0 = invisible, 255 = fully opaque.

// No tint during full day
static constexpr u32 COLOR_NONE    = C2D_Color32(  0,   0,   0,   0);
// Dawn: warm pink-orange, light
static constexpr u32 COLOR_DAWN    = C2D_Color32(255, 140,  60,  60);
// Evening: amber, moderate
static constexpr u32 COLOR_EVENING = C2D_Color32(200,  90,  10,  90);
// Night: cool deep blue, heavy
static constexpr u32 COLOR_NIGHT   = C2D_Color32( 10,  20,  80, 160);

DayNight::DayNight()
    : m_tintColor(COLOR_NONE)
    , m_phase(DayPhase::DAY)
{}

u32 DayNight::lerpColor(u32 a, u32 b, float t) {
    t = std::max(0.0f, std::min(1.0f, t));
    u8 ra = static_cast<u8>(r(a) + (r(b) - r(a)) * t);
    u8 ga = static_cast<u8>(g(a) + (g(b) - g(a)) * t);
    u8 ba = static_cast<u8>(DayNight::b(a) + (DayNight::b(b) - DayNight::b(a)) * t);
    u8 aa = static_cast<u8>(DayNight::a(a) + (DayNight::a(b) - DayNight::a(a)) * t);
    return C2D_Color32(ra, ga, ba, aa);
}

void DayNight::update(float timeFloat) {
    // Wrap to [0, 24)
    while (timeFloat >= 24.0f) timeFloat -= 24.0f;
    while (timeFloat <   0.0f) timeFloat += 24.0f;

    //--------------------------------------------------------------------------
    // Phase boundaries (hours):
    //   20:00 → 05:00  NIGHT
    //   05:00 → 07:00  DAWN  (transition: night → day)
    //   07:00 → 18:00  DAY
    //   18:00 → 20:00  EVENING (transition: day → night)
    //--------------------------------------------------------------------------

    if (timeFloat >= 7.0f && timeFloat < 18.0f) {
        // Full day — no tint
        m_phase     = DayPhase::DAY;
        m_tintColor = COLOR_NONE;
    }
    else if (timeFloat >= 18.0f && timeFloat < 20.0f) {
        // Evening: fade from no tint to full evening over 2 hours
        m_phase       = DayPhase::EVENING;
        float t       = (timeFloat - 18.0f) / 2.0f;
        m_tintColor   = lerpColor(COLOR_NONE, COLOR_EVENING, t);
    }
    else if (timeFloat >= 20.0f || timeFloat < 5.0f) {
        // Night — held at full night tint
        // Blend evening→night at the 20:00 boundary (15-min transition)
        m_phase = DayPhase::NIGHT;
        if (timeFloat >= 20.0f && timeFloat < 20.25f) {
            float t     = (timeFloat - 20.0f) / 0.25f;
            m_tintColor = lerpColor(COLOR_EVENING, COLOR_NIGHT, t);
        } else {
            m_tintColor = COLOR_NIGHT;
        }
    }
    else {
        // Dawn: 05:00–07:00, fade from night tint to no tint
        m_phase     = DayPhase::DAWN;
        float t     = (timeFloat - 5.0f) / 2.0f;
        m_tintColor = lerpColor(COLOR_NIGHT, COLOR_DAWN, t * 0.5f + 0.0f);
        // Second half of dawn (06:00–07:00): dawn color fades to nothing
        if (timeFloat >= 6.0f) {
            float t2    = (timeFloat - 6.0f) / 1.0f;
            m_tintColor = lerpColor(COLOR_DAWN, COLOR_NONE, t2);
        }
    }
}

const char* DayNight::getPhaseName() const {
    switch (m_phase) {
        case DayPhase::DAWN:    return "Dawn";
        case DayPhase::DAY:     return "Day";
        case DayPhase::EVENING: return "Evening";
        case DayPhase::NIGHT:   return "Night";
        default:                return "?";
    }
}
