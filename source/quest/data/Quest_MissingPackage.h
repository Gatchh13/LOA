#pragma once

//-----------------------------------------------------------------------------
// Quest_MissingPackage.h  (Milestone 11 — Gameplay Integration: dialogue updated)
//
// "The Missing Package" — moved here unchanged from QuestDef.h in
// Milestone 10. Milestone 11 updates Mira's accept-dialogue only.
//
// Steps:
//   0: TALK_TO_NPC   (Mira, npc_id=1) — accept quest
//   1: REACH_MARKER  (Forest marker at tile 13,11)
//   2: RETURN_TO_NPC (Mira, npc_id=1) — hand in
// Reward: 50 gold + 1x Wooden Sword (changed from Mira's Token in
// Milestone 9 to prove the quest -> equipment path).
//
// Forest marker position: tile (13, 11) center = pixel (13*16+8, 11*16+8)
// = (216, 184). This sits directly on the main north-south forest path
// — reaching it requires no detour at all, since the path is fully
// open from the Forest spawn point straight up to the marker.
//
// Milestone 11 finding: the Forest Wolf (tile 6,19) and the fallen-tree
// WorldObject (tile 22,11-22,12) are NOT on this direct route — both
// sit well off to either side, and a player who walks straight up the
// path never encounters either. Making them mandatory would require
// walling off the path's wide-open flanks across an entire row (the
// map has no natural single-tile chokepoint), which is a bigger tile-
// data change than this milestone's "smallest code changes" scope
// supports. Fixed instead at the dialogue level: Mira's accept line
// now explicitly mentions both, so a player who skips them is making
// an informed choice to take the safe direct route, not missing
// content the game implied was mandatory. This is the smallest fix
// that resolves the actual problem the assignment names — "dialogue
// contradicts world state" — without reshaping the zone.
//
// Milestone 10 also added post_complete_dialogue: Mira now comments on
// the delivered package/sword when talked to again after completion,
// instead of repeating her generic shop line forever.
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
            "A package was left somewhere in the forest. Mind the\n"
            "wolf, and the fallen tree! (A: Accept)"
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
    /*reward=*/    { /*gold=*/ 50, /*item_id=*/ static_cast<u8>(ItemID::WOODEN_SWORD), /*item_qty=*/ 1 },
    /*post_complete_dialogue=*/
        "Thank you again for finding that package. That sword\n"
        "suit you?"
};
