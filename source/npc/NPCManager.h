#pragma once

//-----------------------------------------------------------------------------
// NPCManager.h
// Owns all NPC instances. Updates schedules, movement, and interaction.
//
// Responsibilities:
//   1. Populate NPC data at startup (hardcoded for Milestone 2)
//   2. Evaluate schedule entries when the hour changes
//   3. Move NPCs toward their current target each frame
//   4. Check A-button interaction with the nearest NPC
//   5. Expose NPC list for the renderer
//
// Memory:
//   MAX_NPCS = 16. Each NPC ~200 bytes. Total: ~3.2 KB.
//   All stack/BSS allocated — no heap.
//
// Zone filtering:
//   Only NPCs whose home_zone matches the current zone are updated/drawn.
//   When entering a zone, NPCs are teleported to their scheduled position
//   for the current time so they don't appear walking from spawn.
//-----------------------------------------------------------------------------

#include "../../include/types.h"
#include "NPC.h"
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

    // True if the currently-open dialogue belongs to Mira (npc_id == 1,
    // the shopkeeper). Used by main.cpp to gate the Y-button "browse
    // wares" shop trigger (Milestone 7) — checked instead of hardcoding
    // npc_id == 1 at the call site so the "who is the shopkeeper" fact
    // lives in exactly one place.
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

    // Move NPC toward target_x/target_y. Stops at ARRIVAL_THRESHOLD.
    // Returns true if still moving.
    bool moveNPC(NPC& npc, const TileMap& map, float dt);

    // Helper: populate one NPC slot.
    NPC& slot(int i) { return m_npcs[i]; }
};

