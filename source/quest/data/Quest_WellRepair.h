#pragma once

//-----------------------------------------------------------------------------
// Quest_WellRepair.h  (Milestone 10 — Data-Driven Content Pipeline)
//
// "The Town Well" — second quest, added in Milestone 10 specifically to
// prove the QuestDatabase scales past a single entry. Reuses existing
// step types and existing NPCs only — no new world object type, no new
// mechanic, per the assignment's explicit constraint ("reuse existing
// step types/world objects").
//
// Premise: the town well's pump has jammed. Aldric (the blacksmith,
// npc_id=0) asks the player to go take a look and report back — a
// REACH_MARKER step at the well's tile position stands in for "the
// player investigated the well," the same way Missing Package's
// REACH_MARKER stands in for "the player found the package," without
// needing a new interactable object.
//
// Steps:
//   0: TALK_TO_NPC   (Aldric, npc_id=0) — accept quest
//   1: REACH_MARKER  (Town well at tile 7,16)
//   2: RETURN_TO_NPC (Aldric, npc_id=0) — report back
// Reward: 20 gold + 2x Iron Nail (thematically fitting — Aldric pays
// in the same nails the player just helped him account for; also
// proves a quest reward can grant a buyable item, not just a
// loot/quest-exclusive one like Missing Package's Wooden Sword).
//
// Well marker position: tile (7, 16) center = pixel (7*16+8, 16*16+8)
// = (120, 264). Checked against s_townTiles in ZoneData.h to confirm
// it's open floor (tile value 0), clear of the fence clusters (rows
// 8-11, columns 3-6 and 20-23), the central building (columns 9-17,
// rows 5-13), and the north-south dirt path (columns 13-14).
//
// Memory: static const in ROM — 0 bytes RAM.
//-----------------------------------------------------------------------------

#include "../../../include/types.h"
#include "../QuestStep.h"
#include "../../data/ItemDatabase.h"

static const QuestStep s_wellRepairSteps[] = {
    {   // Step 0: Talk to Aldric to accept
        QuestStepType::TALK_TO_NPC,
        /*target_npc_id=*/ 0,          // Aldric is NPC index 0 in NPCManager
        /*marker_x=*/ 0.0f,
        /*marker_y=*/ 0.0f,
        /*marker_zone=*/ ZoneID::TOWN,
        /*objective_text=*/ "Talk to Aldric about the well",
        /*npc_dialogue_override=*/
            "The well's pump has jammed again. Could you take a\n"
            "look and tell me what's wrong? (A: Accept)"
    },
    {   // Step 1: Reach the well marker
        QuestStepType::REACH_MARKER,
        /*target_npc_id=*/ 0,
        /*marker_x=*/ 7 * TILE_SIZE + 8.0f,    // tile (7,16) center
        /*marker_y=*/ 16 * TILE_SIZE + 8.0f,
        /*marker_zone=*/ ZoneID::TOWN,
        /*objective_text=*/ "Check the town well",
        /*npc_dialogue_override=*/ nullptr
    },
    {   // Step 2: Report back to Aldric
        QuestStepType::RETURN_TO_NPC,
        /*target_npc_id=*/ 0,
        /*marker_x=*/ 0.0f,
        /*marker_y=*/ 0.0f,
        /*marker_zone=*/ ZoneID::TOWN,
        /*objective_text=*/ "Tell Aldric what you found",
        /*npc_dialogue_override=*/
            "Jammed gears, just as I thought. Here — for your\n"
            "trouble. (A: Complete)"
    },
};

static constexpr QuestDef s_wellRepairDef = {
    /*quest_id=*/  static_cast<u8>(QuestID::WELL_REPAIR),
    /*title=*/     "The Town Well",
    /*steps=*/     s_wellRepairSteps,
    /*step_count=*/3,
    /*reward=*/    { /*gold=*/ 20, /*item_id=*/ static_cast<u8>(ItemID::IRON_NAIL), /*item_qty=*/ 2 },
    /*post_complete_dialogue=*/
        "Well's running smooth again, thanks to you.\n"
        "Couldn't have fixed it without those nails."
};
