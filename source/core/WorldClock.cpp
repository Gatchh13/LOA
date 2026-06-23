//-----------------------------------------------------------------------------
// WorldClock.cpp
//-----------------------------------------------------------------------------

#include "WorldClock.h"

WorldClock::WorldClock(int startMinutes)
    : m_accumulator(0.0f)
    , m_totalMinutes(startMinutes % 1440)
    , m_lastHour(startMinutes / 60)
    , m_hourChanged(false)
{}

void WorldClock::update(float dt) {
    // Convert real seconds to in-game minutes
    m_accumulator += dt * TIME_SCALE;

    // Consume whole minutes
    int wholeMinutes = static_cast<int>(m_accumulator);
    if (wholeMinutes > 0) {
        m_accumulator   -= static_cast<float>(wholeMinutes);
        m_totalMinutes   = (m_totalMinutes + wholeMinutes) % 1440;
    }

    // Detect hour boundary
    int currentHour = m_totalMinutes / 60;
    m_hourChanged   = (currentHour != m_lastHour);
    m_lastHour      = currentHour;
}

int WorldClock::getHour() const {
    return m_totalMinutes / 60;
}

int WorldClock::getMinute() const {
    return m_totalMinutes % 60;
}

float WorldClock::getTimeAsFloat() const {
    return static_cast<float>(m_totalMinutes) / 60.0f;
}

void WorldClock::setTime(int totalMinutes) {
    m_totalMinutes = totalMinutes % 1440;
    m_lastHour     = m_totalMinutes / 60;
    m_hourChanged  = false;
    m_accumulator  = 0.0f;
}

