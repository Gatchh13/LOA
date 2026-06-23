//-----------------------------------------------------------------------------
// ZoneManager.cpp
//-----------------------------------------------------------------------------

#include "ZoneManager.h"
#include "../core/Logger.h"
#include <algorithm>

ZoneManager::ZoneManager()
    : m_currentZone(ZoneID::TOWN)
    , m_fadeState(FadeState::NONE)
    , m_fadeAlpha(0.0f)
    , m_nameTimer(0.0f)
    , m_pendingZone(ZoneID::TOWN)
    , m_pendingSpawnIndex(0)
    , m_transitionReady(false)
{}

void ZoneManager::loadZone(ZoneID id, u8 spawnIndex) {
    m_currentZone = id;
    const ZoneDef& def = getZoneDef(id);
    m_tileMap.loadFromData(def);
    m_nameTimer = NAME_DISPLAY_TIME;
    LOG("Zone loaded: %s (spawn %d)", def.name, spawnIndex);
    (void)spawnIndex; // spawn point applied by caller
}

void ZoneManager::update(int playerTileX, int playerTileY, float dt) {
    // Advance name banner timer
    if (m_nameTimer > 0.0f) {
        m_nameTimer -= dt;
        if (m_nameTimer < 0.0f) m_nameTimer = 0.0f;
    }

    // Fade state machine
    switch (m_fadeState) {

        case FadeState::NONE: {
            // Check if the player is standing on any transition tile.
            // Only fire if we're not already in a transition.
            const ZoneDef& def = getZoneDef(m_currentZone);
            for (u8 i = 0; i < def.transitionCount; i++) {
                const TransitionDef& t = def.transitions[i];
                if (playerTileX == t.trigger_tx && playerTileY == t.trigger_ty) {
                    m_pendingZone        = t.dest_zone;
                    m_pendingSpawnIndex  = t.dest_spawn_index;
                    m_transitionReady    = false;
                    m_fadeState          = FadeState::FADE_OUT;
                    LOG("Transition triggered: -> zone %d spawn %d",
                        (int)t.dest_zone, t.dest_spawn_index);
                    break;
                }
            }
            break;
        }

        case FadeState::FADE_OUT: {
            m_fadeAlpha += FADE_SPEED * dt;
            if (m_fadeAlpha >= 1.0f) {
                m_fadeAlpha       = 1.0f;
                m_fadeState       = FadeState::LOADING;
                m_transitionReady = true;
                // Main loop will call commitTransition() this frame.
            }
            break;
        }

        case FadeState::LOADING:
            // Waiting for main loop to call commitTransition().
            break;

        case FadeState::FADE_IN: {
            m_fadeAlpha -= FADE_SPEED * dt;
            if (m_fadeAlpha <= 0.0f) {
                m_fadeAlpha = 0.0f;
                m_fadeState = FadeState::NONE;
            }
            break;
        }
    }
}

bool ZoneManager::transitionReady() const {
    return m_transitionReady;
}

const SpawnPoint& ZoneManager::getPendingSpawn() const {
    const ZoneDef& def = getZoneDef(m_pendingZone);
    // Clamp index to valid range
    u8 idx = m_pendingSpawnIndex;
    if (idx >= def.spawnCount) idx = 0;
    return def.spawns[idx];
}

void ZoneManager::commitTransition() {
    m_transitionReady = false;
    // Load the new zone (player position is set by the caller using getPendingSpawn())
    loadZone(m_pendingZone, m_pendingSpawnIndex);
    m_fadeState = FadeState::FADE_IN;
}

float ZoneManager::getNameAlpha() const {
    if (m_nameTimer <= 0.0f) return 0.0f;
    // Fade in for first 0.3s, hold, fade out for last 0.5s
    if (m_nameTimer > NAME_DISPLAY_TIME - 0.3f) {
        return 1.0f - (NAME_DISPLAY_TIME - m_nameTimer) / 0.3f;
    }
    if (m_nameTimer < 0.5f) {
        return m_nameTimer / 0.5f;
    }
    return 1.0f;
}

