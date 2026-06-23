#pragma once

//-----------------------------------------------------------------------------
// WorldClock.h
// 24-hour in-game world clock.
//
// Completely separate from Clock.h (which tracks real delta time / FPS).
// WorldClock converts real elapsed seconds into in-game time using a
// configurable acceleration multiplier.
//
// Time representation:
//   Stored as total in-game minutes since midnight (0–1439).
//   Getters expose hour (0–23) and minute (0–59) separately.
//
// Usage:
//   worldClock.update(dt);            // call once per frame
//   worldClock.getHour()              // 0–23
//   worldClock.getMinute()            // 0–59
//   worldClock.getTotalMinutes()      // 0–1439, wraps at midnight
//   worldClock.getTimeAsFloat()       // e.g. 14.5 = 14:30
//   worldClock.hourJustChanged()      // true for exactly one frame per hour
//-----------------------------------------------------------------------------

#include "../../include/types.h"

class WorldClock {
public:
    // TIME_SCALE: how many in-game minutes pass per real second.
    // 1.0  = real time (24 hrs per day)
    // 60.0 = 1 real second = 1 in-game minute  (24 real minutes per day)
    // 720.0 = 1 real second = 12 in-game minutes (2 real minutes per day)
    static constexpr float TIME_SCALE = 60.0f;

    // Start time in minutes from midnight (default 8:00 = 480).
    explicit WorldClock(int startMinutes = 480);

    // Advance the clock. Call once per frame with real delta time.
    void update(float dt);

    // Current hour of the day (0–23).
    int getHour() const;

    // Current minute of the hour (0–59).
    int getMinute() const;

    // Total in-game minutes since midnight (0–1439).
    int getTotalMinutes() const { return m_totalMinutes; }

    // Hour as a float for smooth interpolation (e.g. 14.5 = 14:30).
    float getTimeAsFloat() const;

    // Returns true for exactly one frame when the hour value changes.
    // Use this to trigger schedule re-evaluations cheaply.
    bool hourJustChanged() const { return m_hourChanged; }

    // Set the time directly (minutes from midnight, 0–1439).
    void setTime(int totalMinutes);

private:
    float m_accumulator;   // fractional in-game minutes accumulated
    int   m_totalMinutes;  // whole in-game minutes since midnight (0–1439)
    int   m_lastHour;      // previous frame's hour, for change detection
    bool  m_hourChanged;   // true for exactly one frame
};

