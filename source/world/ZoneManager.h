#pragma once

//-----------------------------------------------------------------------------
// ZoneManager.h
//
// Owns the active TileMap and handles:
//   1. Loading a zone by ID (sets up TileMap, records spawn)
//   2. Checking transition triggers every frame
//   3. Managing the fade-to-black transition animation
//   4. Exposing a pending transition so main.cpp can move the player
//
// Usage in main loop:
//
//   ZoneManager zones;
//   zones.loadZone(ZoneID::TOWN, 0);   // load town, spawn point 0
//
//   // each frame:
//   zones.update(playerCenterTileX, playerCenterTileY, dt);
//
//   if (zones.transitionReady()) {
//       const SpawnPoint& sp = zones.getPendingSpawn();
//       player.setPosition(sp.spawn_px, sp.spawn_py);
//       zones.commitTransition();   // actually swaps the zone
//   }
//
// Fade state machine:
//   NONE → FADE_OUT (triggered by player stepping on transition tile)
//        → LOADING  (zone + player position swapped at full black)
//        → FADE_IN  → NONE
//-----------------------------------------------------------------------------

#include "../../include/types.h"
#include "TileMap.h"
#include "ZoneData.h"

class ZoneManager {
public:
    ZoneManager();

    // Load a zone immediately (no fade). Used for initial game start.
    void loadZone(ZoneID id, u8 spawnIndex);

    // Call every frame. tileX/tileY = player's current center tile.
    // dt used to advance the fade animation.
    void update(int playerTileX, int playerTileY, float dt);

    // True when fade-out is complete and the zone + spawn point are ready
    // to be committed. Main loop should reposition the player then call
    // commitTransition().
    bool transitionReady() const;

    // The spawn point the player should be moved to before commitTransition().
    const SpawnPoint& getPendingSpawn() const;

    // Finalizes the transition: loads the new zone, begins fade-in.
    void commitTransition();

    // The currently active tile map (read by Renderer and Collision).
    const TileMap& getTileMap() const { return m_tileMap; }

    // Current zone definition (for metadata like name).
    const ZoneDef& getCurrentZoneDef() const { return getZoneDef(m_currentZone); }

    // Fade overlay alpha [0.0, 1.0]. 0 = fully visible, 1 = fully black.
    float getFadeAlpha() const { return m_fadeAlpha; }

    // True if a zone name banner should be shown (briefly after entering).
    bool  showZoneName()  const { return m_nameTimer > 0.0f; }
    float getNameAlpha()  const;

private:
    enum class FadeState : u8 { NONE, FADE_OUT, LOADING, FADE_IN };

    TileMap   m_tileMap;
    ZoneID    m_currentZone;

    FadeState m_fadeState;
    float     m_fadeAlpha;    // 0.0 = clear, 1.0 = black
    float     m_nameTimer;    // countdown for zone name display

    // Pending transition info (set when trigger fires)
    ZoneID    m_pendingZone;
    u8        m_pendingSpawnIndex;
    bool      m_transitionReady;

    static constexpr float FADE_SPEED  = 3.0f;  // alpha units per second
    static constexpr float NAME_DISPLAY_TIME = 3.0f; // seconds to show zone name
};
