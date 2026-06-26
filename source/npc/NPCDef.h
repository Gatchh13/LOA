#pragma once

//-----------------------------------------------------------------------------
// NPCDef.h  (Milestone 10 — Data-Driven Content Pipeline)
//
// Static NPC data, separated from NPC's runtime/mutable state (NPC.h).
// Before Milestone 10, name/dialogue/home_zone/schedule lived directly
// on the NPC struct, written once via strncpy() inside per-NPC init
// functions in NPCManager.cpp (initBlacksmith(), initMerchant(),
// initGuard()) and never mutated again after that — genuinely static
// data, just stored in a mutable-looking struct and assigned via
// imperative code instead of being data.
//
// What actually changed, and what didn't:
//   - The data itself (names, dialogue text, schedules, positions) is
//     UNCHANGED — copied verbatim from NPCManager.cpp's init functions
//     into NPCDatabase.cpp's table below.
//   - NPC.h's struct shrinks: name/dialogue/home_zone/schedule/
//     schedule_count move here. NPC keeps a `u8 defIndex` referencing
//     this table, plus genuine runtime state (position, current
//     schedule target, animation, dialogue_active flag).
//   - dialogue changes from a mutable char[MAX_DIALOGUE_LEN] buffer to
//     a `const char*` pointing at a string literal — it was always
//     write-once at init time (dialogue_override, a separate field,
//     already handles the one case where displayed text actually
//     changes at runtime), so there was never a reason for it to be a
//     mutable buffer; it just hadn't been split out from the runtime
//     struct before now.
//
// shop_id (Milestone 10 — new field): which Shop this NPC offers, or
// NO_SHOP if they don't run one. Replaces NPCManager's old
// MERCHANT_NPC_ID hardcoded constant — isTalkingToMerchant() becomes
// "does the NPC currently in dialogue have shop_id != NO_SHOP" instead
// of "is npc_id == 1". With one shop (Mira's) this is the same
// behavior either way; the difference is a second shopkeeper now needs
// one field set, not a second hardcoded constant and a second special
// case in NPCManager.
//
// MEMORY: NPCDef is static const data — 0 bytes RAM, same .rodata
// placement as every other *Def table in this project. Schedules are
// referenced by pointer (npc.h's existing ScheduleEntry array
// convention), not duplicated.
//-----------------------------------------------------------------------------

#include "../../include/types.h"
#include "NPC.h"

// Sentinel for "this NPC doesn't run a shop" — 0xFF rather than 0,
// since 0 could be a valid future shop ID the moment a second
// shopkeeper exists. Same sentinel convention as PlayerState.h's
// NO_EQUIPMENT.
static constexpr u8 NO_SHOP = 0xFF;

// Shop ID for Mira's shop — there is exactly one Shop (see Shop.h),
// so this is currently the only non-NO_SHOP value any NPC's shop_id
// will ever hold. Named here so NPCManager.cpp's table can reference
// it by name instead of a bare 0.
static constexpr u8 SHOP_MIRA = 0;

//-----------------------------------------------------------------------------
// NPCDef — one NPC's static data.
//-----------------------------------------------------------------------------
struct NPCDef {
    u8          npc_id;
    const char* name;
    const char* dialogue;       // default dialogue line (see NPC.h header
                                 // comment — dialogue_override, a separate
                                 // runtime field, takes priority when set)
    ZoneID      home_zone;
    const ScheduleEntry* schedule;
    u8                    schedule_count;
    u8          shop_id;        // NO_SHOP if this NPC doesn't run a shop
};

// Accessor. npc_def_index must be < getNPCDefCount().
const NPCDef& getNPCDef(u8 npc_def_index);

// Iteration helpers (Milestone 10 — new, same rationale as
// ItemDatabase.h/QuestDatabase.h's: nothing previously needed to
// enumerate "every NPC defined," but a real content pipeline should
// support it).
int getNPCDefCount();
const NPCDef& getNPCDefByIndex(int index);
