#pragma once

//-----------------------------------------------------------------------------
// QuestManager.h
// Runtime quest state. Tracks which step each quest is on and drives
// objective completion.
//
// Usage pattern:
//
//   // In main loop, every frame:
//   questMgr.update(currentZone, playerCenterX, playerCenterY);
//
//   // When NPC dialogue opens, check if NPC triggers a quest step:
//   questMgr.onTalkToNPC(npc_id);   // called by NPCManager::tryInteract
//
//   // Renderer queries for HUD:
//   questMgr.getActiveObjectiveText()
//   questMgr.hasActiveQuest()
//
// Quest lifecycle:
//   NOT_STARTED → (player talks to quest giver) → IN_PROGRESS
//   IN_PROGRESS → (all steps completed)         → COMPLETE
//
// Step advancement:
//   TALK_TO_NPC / RETURN_TO_NPC: advanced by onTalkToNPC().
//   REACH_MARKER:                advanced by update() proximity check.
//
//   The step does NOT advance the moment the trigger fires — it advances
//   when the player *closes* the dialogue (for TALK steps) or immediately
//   on proximity (for REACH steps). This prevents the dialogue from
//   jumping to the next NPC line before the player reads the current one.
//
// Memory: ~40 bytes runtime state per quest × MAX_QUESTS = ~640 bytes.
//-----------------------------------------------------------------------------

#include "../../include/types.h"
#include "QuestDef.h"
#include "PlayerState.h"

enum class QuestStatus : u8 {
    NOT_STARTED = 0,
    IN_PROGRESS = 1,
    COMPLETE    = 2,
};

struct QuestState {
    QuestStatus status;
    u8          current_step;  // index into QuestDef::steps[]
    bool        step_just_completed; // true for one frame when a step finishes
};

class QuestManager {
public:
    QuestManager();

    // Call once at startup.
    void init();

    // Call every frame.
    // Checks REACH_MARKER objectives for the active quest.
    void update(ZoneID currentZone, float playerX, float playerY);

    // Called by NPCManager when the player opens dialogue with an NPC.
    // npc_id: the NPC's id field.
    // Returns the dialogue override string if the NPC has quest context,
    // or nullptr if the NPC should show their default dialogue.
    const char* onTalkToNPC(u8 npc_id);

    // Called by NPCManager when the player closes dialogue with an NPC.
    // Advances TALK_TO_NPC / RETURN_TO_NPC steps if conditions are met.
    void onDialogueClosed(u8 npc_id, PlayerState& playerState);

    // True if any quest is IN_PROGRESS.
    bool hasActiveQuest() const;

    // Objective text for the current active step (shown in HUD).
    // Returns nullptr if no quest is active.
    const char* getActiveObjectiveText() const;

    // Status of a specific quest.
    QuestStatus getStatus(u8 quest_id) const;

    // True if the marker should be drawn in the current zone.
    bool markerVisible(ZoneID currentZone) const;

    // Marker world-pixel position (only valid when markerVisible() is true).
    float getMarkerX() const;
    float getMarkerY() const;

    // True for exactly one frame after a quest completes (for fanfare, etc.)
    bool questJustCompleted() const { return m_questJustCompleted; }

private:
    QuestState m_states[MAX_QUESTS];
    bool       m_questJustCompleted;

    // Pending NPC talk state: set by onTalkToNPC, consumed by onDialogueClosed.
    u8   m_pendingTalkNpcId;
    bool m_pendingTalkActive;

    // Advance to the next step (or complete the quest if it was the last).
    void advanceStep(u8 quest_id, PlayerState& playerState);

    // Apply quest reward to player state.
    void applyReward(const QuestDef& def, PlayerState& playerState);

    // Find the first IN_PROGRESS quest (v1 supports one active quest at a time).
    int findActiveQuest() const;
};
