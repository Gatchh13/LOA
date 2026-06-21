#pragma once

//-----------------------------------------------------------------------------
// NPC.h
// Plain data structs for NPCs. No inheritance. No vtables.
//
// Design:
//   NPCs are value types stored in a fixed-size array in NPCManager.
//   The schedule system uses total in-game minutes for comparison so
//   crossing midnight works correctly without special cases.
//
// Schedule:
//   Each NPC has up to MAX_SCHEDULE_ENTRIES schedule entries.
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
//   Each NPC has one static dialogue line (no branching).
//   The dialogue box is shown by the renderer when dialogue_active is true.
//
// Zone restriction:
//   home_zone is the zone where this NPC lives. NPCs in other zones are
//   simply not updated or drawn — they teleport to their scheduled position
//   when the player enters their home zone.
//-----------------------------------------------------------------------------

#include "../../include/types.h"

static constexpr int MAX_SCHEDULE_ENTRIES = 8;
static constexpr int MAX_NPC_NAME_LEN     = 20;
static constexpr int MAX_DIALOGUE_LEN     = 80;
static constexpr float NPC_SPEED          = 50.0f;  // pixels per second
static constexpr float ARRIVAL_THRESHOLD  = 2.0f;   // pixels — "close enough"
static constexpr float INTERACT_RANGE_PX  = 32.0f;  // max distance for A-button dialogue

//-----------------------------------------------------------------------------
// ScheduleEntry
// One entry in an NPC's daily schedule.
// start_minute: in-game minute of day (0–1439) when this entry becomes active.
// dest_tx/ty: destination tile coordinates in the NPC's home zone.
// label: short string shown in debug overlay (e.g. "Forge", "Home")
//-----------------------------------------------------------------------------
struct ScheduleEntry {
    int   start_minute; // 0–1439 (e.g. 8*60=480 for 08:00)
    u8    dest_tx;
    u8    dest_ty;
    const char* label;  // debug/logging only, points to string literal
};

//-----------------------------------------------------------------------------
// NPC
// All state for one NPC. Fits in ~200 bytes.
//-----------------------------------------------------------------------------
struct NPC {
    // Identity
    u8    npc_id;                     // unique ID, matches target_npc_id in QuestStep
    char  name[MAX_NPC_NAME_LEN];
    char  dialogue[MAX_DIALOGUE_LEN];
    ZoneID home_zone;                 // zone where this NPC lives

    // Position (pixel coordinates, top-left of 16×16 sprite)
    float pos_x;
    float pos_y;

    // Schedule
    ScheduleEntry schedule[MAX_SCHEDULE_ENTRIES];
    int           schedule_count;
    int           active_entry;       // index of current schedule entry
    float         target_x;          // pixel target derived from schedule
    float         target_y;

    // State flags
    bool        dialogue_active;            // true while player is reading dialogue
    bool        active;                     // false = this NPC slot is unused

    // Quest dialogue override — set by NPCManager when QuestManager provides
    // context-sensitive text. nullptr = use normal dialogue field.
    // Points to a string literal in QuestDef — never heap-allocated.
    const char* dialogue_override;
};
