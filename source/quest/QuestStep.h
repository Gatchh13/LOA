#pragma once

//-----------------------------------------------------------------------------
// QuestStep.h  (Milestone 10 — Data-Driven Content Pipeline)
//
// QuestStepType, QuestStep, and QuestReward — the shared building blocks
// every quest file (source/quest/data/Quest_*.h) is built from. Split
// out of the old QuestDef.h so each per-quest file only needs to
// include this, not the full QuestDatabase aggregator (which would
// otherwise create an include cycle: QuestDatabase.h includes every
// Quest_*.h, and if those in turn included QuestDatabase.h for
// QuestStep's definition, that's circular).
//
// Step types:
//   TALK_TO_NPC    : player speaks to a specific NPC (by npc_id)
//   REACH_MARKER   : player walks within MARKER_RADIUS pixels of a world position
//   RETURN_TO_NPC  : player speaks to a specific NPC again (logically distinct
//                    from TALK_TO_NPC so UI can show "return to X" language)
//
// Reward:
//   Applied once when the final step is completed. Gold and/or item.
//   item_qty == 0 means "no item reward" — same empty-means-absent
//   convention as InventorySlot, so no separate "has item reward" bool
//   is needed.
//-----------------------------------------------------------------------------

#include "../../include/types.h"

enum class QuestStepType : u8 {
    TALK_TO_NPC,    // objective: speak to npc with npc_id == target_id
    REACH_MARKER,   // objective: enter MARKER_RADIUS of (marker_x, marker_y)
    RETURN_TO_NPC,  // objective: speak to npc with npc_id == target_id again
};

//-----------------------------------------------------------------------------
// Proximity radius for REACH_MARKER objectives (pixels).
// 24px = 1.5 tiles — forgiving enough to feel natural.
//-----------------------------------------------------------------------------
static constexpr float MARKER_RADIUS = 24.0f;

//-----------------------------------------------------------------------------
// QuestStep — one objective within a quest.
//-----------------------------------------------------------------------------
struct QuestStep {
    QuestStepType type;

    // For TALK_TO_NPC / RETURN_TO_NPC: which NPC triggers this step.
    u8 target_npc_id;

    // For REACH_MARKER: world-pixel position of the marker.
    float marker_x;
    float marker_y;
    ZoneID marker_zone;     // marker only checks in this zone

    // Text shown in the HUD status line while this step is active.
    const char* objective_text;

    // Dialogue override for the target NPC while this step is current.
    // nullptr = use the NPC's default dialogue.
    const char* npc_dialogue_override;
};

//-----------------------------------------------------------------------------
// QuestReward
// item_id/item_qty: only valid when item_qty > 0.
//-----------------------------------------------------------------------------
struct QuestReward {
    u32 gold;
    u8  item_id;   // ItemID — only valid when item_qty > 0
    u8  item_qty;  // 0 = no item reward
};

//-----------------------------------------------------------------------------
// QuestDef — one complete quest definition. quest_id/title/steps/
// step_count/reward, unchanged shape from Milestone 7-9's QuestDef.h.
//-----------------------------------------------------------------------------
static constexpr int MAX_QUEST_STEPS = 8;
static constexpr int MAX_QUESTS      = 16;

struct QuestDef {
    u8          quest_id;
    const char* title;

    const QuestStep* steps;
    u8                step_count;

    QuestReward reward;
};

//-----------------------------------------------------------------------------
// Quest ID registry — single source of truth for quest identifiers.
// Every per-quest file (Quest_*.h) references an entry from here.
//-----------------------------------------------------------------------------
enum class QuestID : u8 {
    MISSING_PACKAGE = 0,
    WELL_REPAIR     = 1,
    COUNT  // keep last — number of quests currently defined
};

// Legacy alias — existing code references QUEST_MISSING_PACKAGE as a u8.
static constexpr u8 QUEST_MISSING_PACKAGE = static_cast<u8>(QuestID::MISSING_PACKAGE);
static constexpr u8 QUEST_WELL_REPAIR     = static_cast<u8>(QuestID::WELL_REPAIR);
