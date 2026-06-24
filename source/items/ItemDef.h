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
// MEMORY FOOTPRINT:
//   ItemDef is { const char* name; u8 id; u8 base_price; bool buyable; }.
//   Field order matters here: with the pointer first, there's no leading
//   alignment padding, so sizeof(ItemDef) == 8 bytes on a 32-bit ARM
//   target (4-byte pointer + 3 single-byte fields + 1 byte trailing pad
//   to keep the array's stride a multiple of 4). Putting `id` first
//   instead would cost 4 extra padding bytes per entry (12 bytes, not 8)
//   to keep the pointer aligned — verified by simulating the ARM32
//   layout, not assumed. 8 items * 8 bytes = 64 bytes total, in .rodata
//   (compiled into the executable, not the 64MB RAM ceiling — same as
//   QuestDef/ZoneData). Runtime RAM cost for this file: 0 bytes. It is
//   referenced by index/pointer, never copied.
//
//   Note: this build environment's test compiler is 64-bit, where
//   sizeof(ItemDef) reports 16 bytes (8-byte pointer) instead of 8 —
//   that is a property of the test host, not the 3DS target. devkitARM
//   compiles for 32-bit ARM, where the 8-byte figure above applies.
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
    COUNT  // keep last — number of items currently defined
};

static constexpr int MAX_ITEMS = 8;

//-----------------------------------------------------------------------------
// ItemDef — one item's static data. Pointer field first — see memory
// footprint note above for why field order is deliberate here.
//-----------------------------------------------------------------------------
struct ItemDef {
    const char* name;
    u8          id;
    u8          base_price;  // gold cost in the shop; ignored if !buyable
    bool        buyable;     // true = appears in Mira's stock list
};

static const ItemDef s_itemDefs[] = {
    { "Healing Herb",  static_cast<u8>(ItemID::HEALING_HERB),   8, true  },
    { "Torch",         static_cast<u8>(ItemID::TORCH),          5, true  },
    { "Simple Potion", static_cast<u8>(ItemID::SIMPLE_POTION), 15, true  },
    { "Trail Ration",  static_cast<u8>(ItemID::TRAIL_RATION),   6, true  },
    { "Lantern Oil",   static_cast<u8>(ItemID::LANTERN_OIL),   10, true  },
    { "Spare Cord",    static_cast<u8>(ItemID::SPARE_CORD),     4, true  },
    { "Mira's Token",  static_cast<u8>(ItemID::MIRAS_TOKEN),    0, false },
    { "Iron Nail",     static_cast<u8>(ItemID::IRON_NAIL),      3, true  },
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
