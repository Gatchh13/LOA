#pragma once

//-----------------------------------------------------------------------------
// NPCManager.h  (Milestone 10 — consumes NPCDatabase instead of
// hardcoded init functions)
// Owns all NPC instances. Updates schedules, movement, and interaction.
//
// Responsibilities:
//   1. Populate NPC runtime state from NPCDatabase at startup
//   2. Evaluate schedule entries when the hour changes
//   3. Move NPCs toward their current target each frame
//   4. Check A-button interaction with the nearest NPC
//   5. Expose NPC list for the renderer
//
// Memory:
//   MAX_NPCS = 16. Each NPC ~40 bytes now (down from ~200 — name/
//   dialogue/schedule moved to NPCDef, which is static ROM data
//   referenced by defIndex, not duplicated per instance). Total: ~640
//   bytes runtime, same order of magnitude reduction Milestone 8/9's
//   PlayerState size-verification practice already established —
//   exact figures in the Milestone 10 design doc.
//   All stack/BSS allocated — no heap.
//
// Zone filtering:
//   Only NPCs whose home_zone (now read via getNPCDef(npc.defIndex))
//   matches the current zone are updated/drawn. When entering a zone,
//   NPCs are teleported to their scheduled position for the current
//   time so they don't appear walking from spawn.
//-----------------------------------------------------------------------------

#include "../../include/types.h"
#include "NPC.h"
#include "NPCDatabase.h"
#include "../world/TileMap.h"

// Forward declaration — avoids circular include with QuestManager
class QuestManager;
class PlayerState;

static constexpr int MAX_NPCS = 16;

class NPCManager {
public:
    NPCManager();

    // Populate all NPC data. Call once after WorldClock is constructed.
    // currentMinute: initial world clock minute, used to snap NPCs to position.
    void init(int currentMinute);

    // Call every frame.
    // currentZone:   only NPCs in this zone are updated.
    // currentMinute: total in-game minutes (0–1439), from WorldClock.
    // hourChanged:   pass WorldClock::hourJustChanged() — triggers schedule re-eval.
    // map:           used for NPC–tile collision (NPCs don't walk through walls).
    // dt:            real frame delta for movement.
    void update(ZoneID currentZone, int currentMinute,
                bool hourChanged, const TileMap& map, float dt);

    // Check if the player is close enough to any NPC and A is pressed.
    // playerX/Y: player center in pixels.
    // aPressed:  InputManager::isPressed(KEY_A).
    // questMgr:  used to get dialogue overrides and notify of talk events.
    // playerState: used by QuestManager to apply rewards on dialogue close.
    // Activates dialogue on the closest NPC within range.
    void tryInteract(float playerX, float playerY, bool aPressed,
                     QuestManager& questMgr, PlayerState& playerState);

    // Close any open dialogue. Call when B is pressed or A is pressed again.
    void closeDialogue();

    // True if any NPC dialogue is currently open.
    bool isDialogueOpen() const;

    // The NPC whose dialogue is open. Only valid when isDialogueOpen() == true.
    const NPC* getActiveDialogueNPC() const;

    // True if the currently-open dialogue belongs to an NPC that runs a
    // shop (NPCDef::shop_id != NO_SHOP) — Milestone 10 replaces the old
    // hardcoded "npc_id == 1" check with a data-driven one, so a second
    // shopkeeper needs one NPCDef field set, not a second special case
    // here. Used by main.cpp to gate the Y-button "browse wares" trigger.
    bool isTalkingToMerchant() const;

    // Access the NPC array for rendering.
    const NPC* getNPCs()    const { return m_npcs; }
    int        getNPCCount() const { return MAX_NPCS; }

private:
    NPC m_npcs[MAX_NPCS];
    int m_activeDialogueIndex; // -1 = none

    // Re-evaluate which schedule entry is active given the current minute.
    // Snaps the NPC's target to the scheduled tile position.
    void evaluateSchedule(NPC& npc, int currentMinute);

    // Snap NPC to scheduled position without movement (used on zone load).
    void snapToSchedule(NPC& npc, int currentMinute);

    // Move toward target_x/target_y is now handled by the shared
    // seekTowardTarget() (source/core/Movement.h) — see update()'s
    // call site. The old private moveNPC() method, which duplicated
    // EnemyManager::moveToward()'s logic exactly, was removed in
    // Milestone 12.
};

