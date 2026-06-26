#pragma once

//-----------------------------------------------------------------------------
// NPC.h  (Milestone 10 — static data split out to NPCDef.h)
// Plain data struct for NPC RUNTIME state. No inheritance. No vtables.
//
// Design:
//   NPCs are value types stored in a fixed-size array in NPCManager.
//   Static data (name, dialogue, home zone, schedule contents) now
//   lives in NPCDef.h/NPCDatabase — this struct holds only what
//   genuinely changes during play: position, which schedule entry is
//   currently active, animation state, and dialogue-open status.
//
// Schedule:
//   ScheduleEntry stays defined here (not NPCDef.h) since NPCManager's
//   movement code (moveNPC, evaluateSchedule) operates on
//   ScheduleEntry directly and NPCDef.h already depends on NPC.h for
//   it — keeping the type in NPC.h avoids a circular header dependency
//   between NPC.h and NPCDef.h.
//   Each entry says: "at or after this time, go to this tile position."
//   The active entry is the one with the highest start_minute <= current time.
//   Entries must be sorted by start_minute ascending in the data.
//
// Movement:
//   NPCs move one axis at a time: horizontal first, then vertical.
//   Speed is in pixels per second. They stop when within ARRIVAL_THRESHOLD
//   pixels of their target.
//
// Dialogue:
//   The default dialogue line now lives on NPCDef (see NPCDef.h).
//   dialogue_override (here, runtime) takes priority when QuestManager
//   sets it; nullptr means "use NPCDef's default dialogue instead."
//-----------------------------------------------------------------------------

#include "../../include/types.h"
#include "../entities/AnimState.h"

static constexpr int MAX_SCHEDULE_ENTRIES = 8;
static constexpr int MAX_NPC_NAME_LEN     = 20;  // still used by NPCDatabase's
                                                   // static_assert-style sanity
                                                   // checks on name length, even
                                                   // though NPC no longer stores
                                                   // a fixed name buffer itself
static constexpr int MAX_DIALOGUE_LEN     = 80;  // same role for dialogue length
static constexpr float NPC_SPEED          = 50.0f;  // pixels per second
static constexpr float ARRIVAL_THRESHOLD  = 2.0f;   // pixels — "close enough"
static constexpr float INTERACT_RANGE_PX  = 32.0f;  // max distance for A-button dialogue

//-----------------------------------------------------------------------------
// ScheduleEntry
// One entry in an NPC's daily schedule. Defined here (not NPCDef.h) —
// see header comment above.
//-----------------------------------------------------------------------------
struct ScheduleEntry {
    int   start_minute; // 0–1439 (e.g. 8*60=480 for 08:00)
    u8    dest_tx;
    u8    dest_ty;
    const char* label;  // debug/logging only, points to string literal
};

//-----------------------------------------------------------------------------
// NPC — runtime state only. Identity/schedule-content/dialogue-text now
// live on NPCDef (see NPCDef.h), referenced here by defIndex.
//-----------------------------------------------------------------------------
struct NPC {
    // Identity reference — which NPCDef this instance represents.
    // NPCManager::init() sets this once; nothing else changes it.
    u8 defIndex;

    // Position (pixel coordinates, top-left of 16×16 sprite)
    float pos_x;
    float pos_y;

    // Schedule progress (the schedule's CONTENT is on NPCDef; this is
    // just "which entry is currently active" and the derived target).
    int   active_entry;       // index into NPCDef::schedule
    float target_x;          // pixel target derived from schedule
    float target_y;

    // Animation state — driven by moveNPC()'s per-axis movement each frame.
    // Not saved (see AnimState.h): always derives correctly from a fresh
    // idle pose after load/zone-entry.
    AnimState anim;

    // State flags
    bool        dialogue_active;            // true while player is reading dialogue
    bool        active;                     // false = this NPC slot is unused

    // Quest dialogue override — set by NPCManager when QuestManager provides
    // context-sensitive text. nullptr = use NPCDef's default dialogue.
    // Points to a string literal in QuestDef — never heap-allocated.
    const char* dialogue_override;
};
