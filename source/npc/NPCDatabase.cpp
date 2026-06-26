//-----------------------------------------------------------------------------
// NPCDatabase.cpp  (Milestone 10 — Data-Driven Content Pipeline)
//
// The actual NPC table. This is the ONE place to edit when adding,
// removing, or changing an NPC — schedules, dialogue, home zone, shop.
// Data extracted verbatim from NPCManager.cpp's old initBlacksmith()/
// initMerchant()/initGuard() functions; same names, same schedules,
// same dialogue text, same positions. Only the storage changed: from
// imperative strncpy()/assignment calls run once at startup, to a
// static const table that simply exists at compile time.
//
// MEMORY: schedules and the NPCDef table are static const — .rodata,
// 0 bytes RAM. 3 NPCs * (schedule array + one NPCDef row) — negligible
// against the 64MB ceiling; exact figures in the Milestone 10 design
// doc's memory-impact section.
//-----------------------------------------------------------------------------

#include "NPCDatabase.h"

//-----------------------------------------------------------------------------
// Aldric — the blacksmith. npc_id 0.
//-----------------------------------------------------------------------------
static const ScheduleEntry s_aldricSchedule[] = {
    {  6 * 60, 4,  2, "Wake"      },
    {  8 * 60, 4,  3, "Forge"     },
    { 12 * 60, 5,  9, "Tavern"    },
    { 13 * 60, 4,  3, "Forge"     },
    { 18 * 60, 4,  2, "Wind down" },
    { 20 * 60, 4,  9, "Home"      },
    {  0 * 60, 4,  9, "Sleep"     },
};

//-----------------------------------------------------------------------------
// Mira — the merchant. npc_id 1. Runs the one shop in the game.
//-----------------------------------------------------------------------------
static const ScheduleEntry s_miraSchedule[] = {
    {  7 * 60, 10,  6, "Open stall"  },
    {  8 * 60, 12,  8, "Market"      },
    { 13 * 60, 10,  9, "Lunch break" },
    { 14 * 60, 14,  7, "Afternoon"   },
    { 18 * 60, 10,  6, "Close stall" },
    { 19 * 60, 21,  2, "Home"        },
    { 22 * 60, 21,  3, "Sleep"       },
    {  0 * 60, 21,  3, "Night"       },
};

//-----------------------------------------------------------------------------
// Brennan — the guard. npc_id 2.
//-----------------------------------------------------------------------------
static const ScheduleEntry s_brennanSchedule[] = {
    {  0 * 60,  1,  1, "Patrol A1" },
    {  3 * 60, 13, 18, "Patrol A2" },
    {  6 * 60,  1,  1, "Patrol A3" },
    {  8 * 60, 26,  1, "Patrol B1" },
    { 12 * 60,  1, 20, "Patrol B2" },
    { 15 * 60, 13, 18, "Patrol B3" },
    { 18 * 60, 26, 20, "Patrol B4" },
    { 21 * 60,  1,  1, "Night post"},
};

static const NPCDef s_npcDefs[] = {
    {
        /*npc_id=*/   0,
        /*name=*/     "Aldric",
        /*dialogue=*/ "The forge never rests. Neither do I.",
        /*home_zone=*/ZoneID::TOWN,
        /*schedule=*/ s_aldricSchedule,
        /*schedule_count=*/static_cast<u8>(sizeof(s_aldricSchedule) / sizeof(s_aldricSchedule[0])),
        /*shop_id=*/  NO_SHOP,
    },
    {
        /*npc_id=*/   1,
        /*name=*/     "Mira",
        /*dialogue=*/ "Finest goods this side of the mountains!\n(Y: Browse)",
        /*home_zone=*/ZoneID::TOWN,
        /*schedule=*/ s_miraSchedule,
        /*schedule_count=*/static_cast<u8>(sizeof(s_miraSchedule) / sizeof(s_miraSchedule[0])),
        /*shop_id=*/  SHOP_MIRA,
    },
    {
        /*npc_id=*/   2,
        /*name=*/     "Brennan",
        /*dialogue=*/ "Move along. Nothing to see here.",
        /*home_zone=*/ZoneID::TOWN,
        /*schedule=*/ s_brennanSchedule,
        /*schedule_count=*/static_cast<u8>(sizeof(s_brennanSchedule) / sizeof(s_brennanSchedule[0])),
        /*shop_id=*/  NO_SHOP,
    },
};

static constexpr int NPC_DEF_COUNT = sizeof(s_npcDefs) / sizeof(s_npcDefs[0]);

const NPCDef& getNPCDef(u8 npc_def_index) {
    return s_npcDefs[npc_def_index];
}

int getNPCDefCount() {
    return NPC_DEF_COUNT;
}

const NPCDef& getNPCDefByIndex(int index) {
    return s_npcDefs[index];
}
