#pragma once

//-----------------------------------------------------------------------------
// ItemDef.h  (Milestone 7 — Economy Loop)
//
// Static item database. Compile-time constants, zero heap, zero runtime
// loading — same philosophy as QuestDef.h.
//
// Architecture:
//   ItemID enum is the single registry of item identifiers (mirrors
//   QuestDef.h's QuestID pattern — exists so a new item can't accidentally
//   collide with an existing one). s_itemDefs[] holds the actual data,
//   indexed by ItemID. A static_assert ties the two together so adding
//   one without the other fails to compile rather than silently
//   desyncing (see QUEST_DEF_COUNT for precedent).
//
// Scope (Milestone 7): 8 items, flavor-matched to the existing Wayfinder/
// repair theme. Mira's Token is quest-reward-only (buyable == false) —
// it exists specifically to prove the item-reward path works for a
// non-purchasable item, not just gold-equivalent consumables.
//
// Milestone 8 adds a 9th item, Wolf Pelt — guaranteed loot from the
// Forest Wolf (see EnemyManager.h). Also not buyable, same pattern as
// Mira's Token: it proves the loot-drop path grants an item the same
// way the shop and quest-reward paths already do, without needing a
// separate "loot" code path through Inventory.
//
// MEMORY FOOTPRINT:
//   ItemDef is { const char* name; u8 id; u8 base_price; bool buyable;
//   u16 heal_amount; }. On a 32-bit ARM target: 4 (pointer) + 1 + 1 + 1
//   (three single-byte fields) + 1 pad + 2 (u16, naturally 2-byte
//   aligned) = 10, rounded up to 12 bytes to keep the array's stride a
//   multiple of the pointer's 4-byte alignment. This grew from the
//   Milestone 7 figure of 8 bytes when heal_amount (u16) was added in
//   Milestone 8 — verified by trying several field orderings, none of
//   which recovers the original 8 bytes once a u16 is present alongside
//   the pointer and three u8s; the extra 4 bytes/item is unavoidable,
//   not a missed optimization. 9 items * 12 bytes = 108 bytes total, in
//   .rodata (compiled into the executable, not the 64MB RAM ceiling —
//   same as QuestDef/ZoneData). Runtime RAM cost for this file: 0 bytes.
//   It is referenced by index/pointer, never copied.
//
//   Note: this build environment's test compiler is 64-bit, where
//   sizeof(ItemDef) reports a larger figure (8-byte pointer) than the
//   12-byte ARM32 figure above — that is a property of the test host,
//   not the 3DS target. devkitARM compiles for 32-bit ARM, where the
//   12-byte figure applies.
//-----------------------------------------------------------------------------

#include "../../include/types.h"

//-----------------------------------------------------------------------------
// ItemID registry — see header comment above.
//-----------------------------------------------------------------------------
enum class ItemID : u8 {
    HEALING_HERB  = 0,
    TORCH         = 1,
    SIMPLE_POTION = 2,
    TRAIL_RATION  = 3,
    LANTERN_OIL   = 4,
    SPARE_CORD    = 5,
    MIRAS_TOKEN   = 6,  // quest-reward only, not sold in the shop
    IRON_NAIL     = 7,
    WOLF_PELT     = 8,  // Milestone 8 — loot-only, not sold in the shop
    COUNT  // keep last — number of items currently defined
};

static constexpr int MAX_ITEMS = 9;

//-----------------------------------------------------------------------------
// ItemDef — one item's static data. Pointer field first — see memory
// footprint note above for why field order is deliberate here.
//
// heal_amount: HP restored on use, 0 = not a healing consumable (the
// same zero-means-absent convention as InventorySlot.quantity and
// QuestReward.item_qty, rather than a separate "is_consumable" bool).
//-----------------------------------------------------------------------------
struct ItemDef {
    const char* name;
    u8          id;
    u8          base_price;   // gold cost in the shop; ignored if !buyable
    bool        buyable;      // true = appears in Mira's stock list
    u16         heal_amount;  // HP restored on use; 0 = not healing-usable
};

static const ItemDef s_itemDefs[] = {
    { "Healing Herb",  static_cast<u8>(ItemID::HEALING_HERB),   8, true,   20 },
    { "Torch",         static_cast<u8>(ItemID::TORCH),          5, true,    0 },
    { "Simple Potion", static_cast<u8>(ItemID::SIMPLE_POTION), 15, true,   50 },
    { "Trail Ration",  static_cast<u8>(ItemID::TRAIL_RATION),   6, true,    0 },
    { "Lantern Oil",   static_cast<u8>(ItemID::LANTERN_OIL),   10, true,    0 },
    { "Spare Cord",    static_cast<u8>(ItemID::SPARE_CORD),     4, true,    0 },
    { "Mira's Token",  static_cast<u8>(ItemID::MIRAS_TOKEN),    0, false,   0 },
    { "Iron Nail",     static_cast<u8>(ItemID::IRON_NAIL),      3, true,    0 },
    { "Wolf Pelt",     static_cast<u8>(ItemID::WOLF_PELT),      0, false,   0 },
};

static constexpr int ITEM_DEF_COUNT = sizeof(s_itemDefs) / sizeof(s_itemDefs[0]);
static_assert(ITEM_DEF_COUNT == static_cast<int>(ItemID::COUNT),
              "s_itemDefs and ItemID registry are out of sync — "
              "every entry added to one must be added to the other");
static_assert(static_cast<int>(ItemID::COUNT) <= MAX_ITEMS,
              "More items defined than MAX_ITEMS allows");

// Accessor. item_id must be < ITEM_DEF_COUNT.
inline const ItemDef& getItemDef(u8 item_id) {
    return s_itemDefs[item_id];
}
