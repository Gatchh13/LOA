#pragma once

//-----------------------------------------------------------------------------
// Quest_MissingPackage.h  (Milestone 10 — Data-Driven Content Pipeline)
//
// "The Missing Package" — moved here unchanged from QuestDef.h as part
// of Milestone 10's split into one file per quest. Same steps, same
// reward, same memory layout — only the file location changed.
//
// Steps:
//   0: TALK_TO_NPC   (Mira, npc_id=1) — accept quest
//   1: REACH_MARKER  (Forest marker at tile 13,11)
//   2: RETURN_TO_NPC (Mira, npc_id=1) — hand in
// Reward: 50 gold + 1x Wooden Sword (changed from Mira's Token in
// Milestone 9 to prove the quest -> equipment path).
//
// Forest marker position: tile (13, 11) center = pixel (13*16+8, 11*16+8)
// = (216, 184). This is on the main forest path, T-junction area.
//
// Memory: static const in ROM — 0 bytes RAM.
//-----------------------------------------------------------------------------

#include "../../../include/types.h"
#include "../QuestStep.h"
#include "../../data/ItemDatabase.h"

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

static constexpr QuestDef s_missingPackageDef = {
    /*quest_id=*/  static_cast<u8>(QuestID::MISSING_PACKAGE),
    /*title=*/     "The Missing Package",
    /*steps=*/     s_missingPackageSteps,
    /*step_count=*/3,
    /*reward=*/    { /*gold=*/ 50, /*item_id=*/ static_cast<u8>(ItemID::WOODEN_SWORD), /*item_qty=*/ 1 }
};
