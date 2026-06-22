#pragma once
#include <3ds.h>

class Clock {
public:
    Clock()
        : seconds_per_game_minute(2.0f),
          accumulator(0.0f),
          game_day(1),
          hour(8),
          minute(0),
          hour_changed(false),
          last_tick(0)
    {
        last_tick = svcGetSystemTick();
    }

    // Call once per frame. Returns real delta time in seconds.
    float update() {
        u64 now = svcGetSystemTick();
        u64 diff = now - last_tick;
        last_tick = now;

        float dt = (float)diff / (float)SYSCLOCK_ARM11;
        // Clamp to avoid huge dt spikes (e.g. first frame, or stall)
        if (dt > 0.1f) dt = 0.1f;

        tickGameTime(dt);
        return dt;
    }

    u8   getHour() const   { return hour; }
    u8   getMinute() const { return minute; }
    u16  getDay() const    { return game_day; }
    bool isNight() const   { return hour >= 20 || hour < 6; }
    bool hourChanged() const { return hour_changed; }

private:
    void tickGameTime(float dt) {
        hour_changed = false;
        accumulator += dt / seconds_per_game_minute;

        while (accumulator >= 1.0f) {
            accumulator -= 1.0f;
            minute++;
            if (minute >= 60) {
                minute = 0;
                hour++;
                hour_changed = true;
                if (hour >= 24) {
                    hour = 0;
                    game_day++;
                }
            }
        }
    }

    float seconds_per_game_minute; // 1 real second = (1 / this) game minutes
    float accumulator;
    u16   game_day;
    u8    hour, minute;
    bool  hour_changed;
    u64   last_tick;
};
