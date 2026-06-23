#pragma once

//-----------------------------------------------------------------------------
// WorldObjectManager.h
// Owns all WorldObject instances. Handles interaction, tile override
// application, and the quest hook for future integration.
//
// Lifecycle:
//   1. init()          — populate objects, apply INACTIVE overrides to TileMap
//   2. onZoneLoaded()  — re-apply overrides for the new zone's objects
//   3. update()        — nothing per-frame (objects are static; no animation yet)
//   4. tryInteract()   — called when A is pressed, returns InteractResult
//   5. drawObjects()   — called by Renderer (placeholder visuals)
//
// TileMap override protocol:
//   WorldObjectManager calls TileMap::setTileOverride() to modify individual
//   tiles at runtime. TileMap::isSolid() checks overrides before the base layer.
//   This keeps Collision unchanged — it still just calls map.isSolid().
//
// Quest hook:
//   onWorldObjectRepaired(id) is called immediately after repair.
//   QuestManager does not use it yet, but the signature is established.
//-----------------------------------------------------------------------------

#include "../../include/types.h"
#include "WorldObject.h"
#include "TileMap.h"
#include "../quest/PlayerState.h"

// Forward declaration for quest hook
class QuestManager;

class WorldObjectManager {
public:
    WorldObjectManager();

    // Populate all objects and apply initial tile overrides to the current map.
    // Call once at startup, then call onZoneLoaded() on every zone transition.
    void init(TileMap& map);

    // Call after every zone transition. Re-applies overrides for the new zone.
    // ZoneManager loads the map fresh on each transition — overrides must be
    // re-applied every time the zone is entered.
    void onZoneLoaded(ZoneID zone, TileMap& map);

    // Check if the player is within range of any object and A was pressed.
    // Returns the interaction result and updates state if repaired.
    // playerX/Y: player center in pixels.
    // aPressed: true only on the frame A is pressed.
    // map: updated in-place on repair.
    // playerState: resources consumed on repair.
    // questMgr: receives onWorldObjectRepaired() hook (pass by pointer, may be null).
    InteractResult tryInteract(float playerX, float playerY, bool aPressed,
                               TileMap& map, PlayerState& playerState,
                               QuestManager* questMgr);

    // The message from the last tryInteract call (for HUD display).
    // Valid until the next tryInteract call.
    // nullptr if no message.
    const char* getLastMessage() const { return m_lastMessage; }

    // True if a message should currently be shown (cleared after MESSAGE_DURATION).
    bool hasMessage() const { return m_messageTimer > 0.0f; }

    // Advance message timer. Call once per frame with real dt.
    void updateMessageTimer(float dt);

    // Access objects for rendering.
    const WorldObject* getObjects()    const { return m_objects; }
    int                getObjectCount() const { return MAX_WORLD_OBJECTS; }

    // World object state for future save system.
    // Caller writes: for (int i=0; i<MAX_WORLD_OBJECTS; i++) states[i]=getState(i)
    WorldObjectState getState(u8 id) const;
    void             setState(u8 id, WorldObjectState s, TileMap& map);

    // Restore all world object states from save data (slot-indexed, not id-indexed).
    // Called only by SaveManager::apply(). Restores state values ONLY — does
    // NOT touch tile overrides. The caller MUST call onZoneLoaded(zone, map)
    // immediately after this, so that only the correctly-loaded zone's
    // overrides get applied (applying every zone's overrides regardless of
    // which TileMap is loaded was the cause of a cross-zone tile corruption
    // bug — see WorldObjectManager.cpp).
    void setStatesFromSave(const u8* states, int count, TileMap& map);

    static constexpr float MESSAGE_DURATION = 3.0f;  // seconds to show message

private:
    WorldObject m_objects[MAX_WORLD_OBJECTS];
    const char* m_lastMessage;
    float       m_messageTimer;

    // Apply the correct tile overrides for one object given its current state.
    void applyOverrides(const WorldObject& obj, TileMap& map);

    // Repair one object: consume resources, change state, apply overrides, fire hook.
    void repair(WorldObject& obj, TileMap& map, PlayerState& ps, QuestManager* qm);

    // Quest hook — called after every successful repair.
    void onWorldObjectRepaired(u8 id, QuestManager* questMgr);
};
