#pragma once

//-----------------------------------------------------------------------------
// QuestDef.h
// Static quest data definitions — compile-time constants, zero heap.
//
// Architecture:
//   A quest progresses through a linear sequence of QuestSteps.
//   Each step has one objective type. When the objective is met,
//   QuestManager advances to the next step automatically.
//
//   Step types:
//     TALK_TO_NPC    : player speaks to a specific NPC (by npc_id)
//     REACH_MARKER   : player walks within MARKER_RADIUS pixels of a world position
//     RETURN_TO_NPC  : player speaks to a specific NPC again (logically distinct
//                      from TALK_TO_NPC so UI can show "return to X" language)
//
//   Reward:
//     Applied once when the final step is completed.
//     Currently only gold. Expandable to items later without changing the struct.
//
// "The Missing Package" quest steps:
//   Step 0: TALK_TO_NPC   (Mira, npc_id=1) — accept quest
//   Step 1: REACH_MARKER  (Forest marker at tile 13,11)
//   Step 2: RETURN_TO_NPC (Mira, npc_id=1) — hand in
//
// Memory: all data is static const in ROM — 0 bytes RAM.
//-----------------------------------------------------------------------------

#include "../../include/types.h"

//-----------------------------------------------------------------------------
// QuestStepType
//-----------------------------------------------------------------------------
enum class QuestStepType : u8 {
    TALK_TO_NPC,    // objective: speak to npc with npc_id == target_id
    REACH_MARKER,   // objective: enter MARKER_RADIUS of (marker_x, marker_y)
    RETURN_TO_NPC,  // objective: speak to npc with npc_id == target_id again
};

//-----------------------------------------------------------------------------
// QuestStep
// One objective within a quest.
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
//-----------------------------------------------------------------------------
struct QuestReward {
    u32 gold;
};

//-----------------------------------------------------------------------------
// QuestDef
// One complete quest definition.
//-----------------------------------------------------------------------------
static constexpr int MAX_QUEST_STEPS = 8;
static constexpr int MAX_QUESTS      = 16;

struct QuestDef {
    u8          quest_id;
    const char* title;

    const QuestStep* steps;
    u8               step_count;

    QuestReward reward;
};

//-----------------------------------------------------------------------------
// Quest ID registry
//
// All quest IDs are defined here, once. When adding a new quest, add it to
// this enum rather than declaring a standalone constant elsewhere — this is
// what prevents ID collisions once the quest count grows past one.
//-----------------------------------------------------------------------------
enum class QuestID : u8 {
    MISSING_PACKAGE = 0,
    COUNT  // keep last — number of quests currently defined
};

// Legacy alias — existing code references QUEST_MISSING_PACKAGE as a u8.
// New code should prefer QuestID::MISSING_PACKAGE.
static constexpr u8 QUEST_MISSING_PACKAGE = static_cast<u8>(QuestID::MISSING_PACKAGE);

//-----------------------------------------------------------------------------
// Proximity radius for REACH_MARKER objectives (pixels).
// 24px = 1.5 tiles — forgiving enough to feel natural.
//-----------------------------------------------------------------------------
static constexpr float MARKER_RADIUS = 24.0f;

//=============================================================================
// "The Missing Package" — quest data
//
// Forest marker position: tile (13, 11) center = pixel (13*16+8, 11*16+8)
// = (216, 184). This is on the main forest path, T-junction area.
//=============================================================================

static const QuestStep s_missingPackageSteps[] = {
    {   // Step 0: Talk to Mira to accept
        QuestStepType::TALK_TO_NPC,
        /*target_npc_id=*/ 1,          // Mira is NPC index 1 in NPCManager
        /*marker_x=*/ 0.0f,
        /*marker_y=*/ 0.0f,
        /*marker_zone=*/ ZoneID::TOWN,
        /*objective_text=*/ "Talk to Mira in the market",
        /*npc_dialogue_override=*/
            "A package was left somewhere in the forest.\n"
            "Could you find it? (A: Accept)"
    },
    {   // Step 1: Reach forest marker
        QuestStepType::REACH_MARKER,
        /*target_npc_id=*/ 0,
        /*marker_x=*/ 13 * TILE_SIZE + 8.0f,   // tile (13,11) center
        /*marker_y=*/ 11 * TILE_SIZE + 8.0f,
        /*marker_zone=*/ ZoneID::FOREST,
        /*objective_text=*/ "Find the package in the forest",
        /*npc_dialogue_override=*/ nullptr
    },
    {   // Step 2: Return to Mira
        QuestStepType::RETURN_TO_NPC,
        /*target_npc_id=*/ 1,
        /*marker_x=*/ 0.0f,
        /*marker_y=*/ 0.0f,
        /*marker_zone=*/ ZoneID::TOWN,
        /*objective_text=*/ "Return the package to Mira",
        /*npc_dialogue_override=*/
            "You found it! Here is your reward.\n"
            "(A: Complete)"
    },
};

static const QuestDef s_questDefs[] = {
    {
        QUEST_MISSING_PACKAGE,
        "The Missing Package",
        s_missingPackageSteps,
        3,
        { /*gold=*/ 50 }
    },
};

// Number of quests actually defined in s_questDefs. Kept separate from
// MAX_QUESTS (the SaveData/runtime array capacity) intentionally — this
// catches the case where QuestID gains an entry but s_questDefs doesn't
// (or vice versa).
static constexpr int QUEST_DEF_COUNT = sizeof(s_questDefs) / sizeof(s_questDefs[0]);
static_assert(QUEST_DEF_COUNT == static_cast<int>(QuestID::COUNT),
              "s_questDefs and QuestID registry are out of sync — "
              "every entry added to one must be added to the other");
static_assert(static_cast<int>(QuestID::COUNT) <= MAX_QUESTS,
              "More quests defined than MAX_QUESTS allows — "
              "grow MAX_QUESTS and the SaveData quest arrays first");

// Accessor. quest_id must be < QUEST_DEF_COUNT.
inline const QuestDef& getQuestDef(u8 quest_id) {
    return s_questDefs[quest_id];
}
