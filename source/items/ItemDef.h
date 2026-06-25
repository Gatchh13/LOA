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
// Milestone 9 adds:
//   - sellable: true unless explicitly excluded. Wolf Pelt is sellable
//     (the whole point of Feature 1); Mira's Token is the one exclusion,
//     matching the assignment's explicit "Mira's Token cannot be sold."
//     This is a general default with one carve-out, not an enumerated
//     whitelist — selling is "can sell unless told otherwise," which
//     reads more naturally against the assignment's wording than a
//     second parallel "what's sellable" list that could drift out of
//     sync with this one.
//   - equip_slot / attack_bonus / defense_bonus: equipment data. Only
//     items with equip_slot != EquipSlot::NONE use attack_bonus/
//     defense_bonus; every consumable/material leaves both at 0, which
//     is also their natural "no bonus" value, so no separate "is this
//     equipment" check is needed beyond reading equip_slot itself.
//   - 3 new equipment items: Wooden Sword (+5 attack), Leather Armor
//     (+3 defense), Iron Sword (+10 attack). MAX_ITEMS 9 -> 12.
//
// MEMORY FOOTPRINT:
//   ItemDef is { const char* name; u8 id; u8 base_price; bool buyable;
//   bool sellable; u16 heal_amount; EquipSlot equip_slot; u8 attack_bonus;
//   u8 defense_bonus; }. On a 32-bit ARM target: 4 (pointer) + 1+1+1+1
//   (id, base_price, buyable, sellable) + 2 (heal_amount, 2-byte aligned)
//   + 1 (equip_slot, u8 enum) + 1 + 1 (attack_bonus, defense_bonus) = 13,
//   rounded up to 16 bytes to keep the array's stride a multiple of the
//   pointer's 4-byte alignment. 12 items * 16 bytes = 192 bytes total,
//   in .rodata (compiled into the executable, not the 64MB RAM ceiling
//   — same as QuestDef/ZoneData). Runtime RAM cost for this file: 0
//   bytes. It is referenced by index/pointer, never copied.
//
//   Note: this build environment's test compiler is 64-bit, where
//   sizeof(ItemDef) reports a larger figure (8-byte pointer) than the
//   16-byte ARM32 figure above — that is a property of the test host,
//   not the 3DS target. devkitARM compiles for 32-bit ARM, where the
//   16-byte figure applies.
//-----------------------------------------------------------------------------

#include "../../include/types.h"

//-----------------------------------------------------------------------------
// ItemID registry — see header comment above.
//-----------------------------------------------------------------------------
enum class ItemID : u8 {
    HEALING_HERB   = 0,
    TORCH          = 1,
    SIMPLE_POTION  = 2,
    TRAIL_RATION   = 3,
    LANTERN_OIL    = 4,
    SPARE_CORD     = 5,
    MIRAS_TOKEN    = 6,  // quest-reward only, not sold in the shop
    IRON_NAIL      = 7,
    WOLF_PELT      = 8,  // Milestone 8 — loot-only, not bought, but sellable
    WOODEN_SWORD   = 9,  // Milestone 9 — equipment: weapon
    LEATHER_ARMOR  = 10, // Milestone 9 — equipment: armor
    IRON_SWORD     = 11, // Milestone 9 — equipment: weapon
    COUNT  // keep last — number of items currently defined
};

static constexpr int MAX_ITEMS = 12;

// Which equipment slot an item occupies. NONE = not equipment (the
// default for every consumable/material item). Only WEAPON and ARMOR
// exist per the Milestone 9 assignment's explicit "only two slots,
// nothing else" scope rule.
enum class EquipSlot : u8 {
    NONE   = 0,
    WEAPON = 1,
    ARMOR  = 2,
};

//-----------------------------------------------------------------------------
// ItemDef — one item's static data. Pointer field first — see memory
// footprint note above for why field order is deliberate here.
//
// heal_amount: HP restored on use, 0 = not a healing consumable (the
// same zero-means-absent convention as InventorySlot.quantity and
// QuestReward.item_qty, rather than a separate "is_consumable" bool).
//
// equip_slot/attack_bonus/defense_bonus: equipment data (Milestone 9).
// Only items with equip_slot != EquipSlot::NONE use the bonus fields —
// every other item leaves them at their natural zero value, so reading
// equip_slot is the one check needed to know whether an item is
// equipment at all.
//-----------------------------------------------------------------------------
struct ItemDef {
    const char* name;
    u8          id;
    u8          base_price;    // gold cost to buy (if buyable) and the basis
                                // for sell price (if sellable) — see
                                // Shop.h's getSellPrice(). Set even for
                                // non-buyable-but-sellable items like Wolf
                                // Pelt, where it only ever feeds sell price.
    bool        buyable;       // true = appears in Mira's buy stock
    bool        sellable;      // true = Mira will buy this from the player
    u16         heal_amount;   // HP restored on use; 0 = not healing-usable
    EquipSlot   equip_slot;    // which slot this occupies; NONE = not equipment
    u8          attack_bonus;  // added to player's base attack when equipped
    u8          defense_bonus; // subtracted from incoming enemy damage when equipped
};

static const ItemDef s_itemDefs[] = {
    { "Healing Herb",  static_cast<u8>(ItemID::HEALING_HERB),    8, true,  true,  20, EquipSlot::NONE,   0,  0 },
    { "Torch",         static_cast<u8>(ItemID::TORCH),           5, true,  true,   0, EquipSlot::NONE,   0,  0 },
    { "Simple Potion", static_cast<u8>(ItemID::SIMPLE_POTION),  15, true,  true,  50, EquipSlot::NONE,   0,  0 },
    { "Trail Ration",  static_cast<u8>(ItemID::TRAIL_RATION),    6, true,  true,   0, EquipSlot::NONE,   0,  0 },
    { "Lantern Oil",   static_cast<u8>(ItemID::LANTERN_OIL),    10, true,  true,   0, EquipSlot::NONE,   0,  0 },
    { "Spare Cord",    static_cast<u8>(ItemID::SPARE_CORD),      4, true,  true,   0, EquipSlot::NONE,   0,  0 },
    { "Mira's Token",  static_cast<u8>(ItemID::MIRAS_TOKEN),     0, false, false,  0, EquipSlot::NONE,   0,  0 },
    { "Iron Nail",     static_cast<u8>(ItemID::IRON_NAIL),       3, true,  true,   0, EquipSlot::NONE,   0,  0 },
    { "Wolf Pelt",     static_cast<u8>(ItemID::WOLF_PELT),      10, false, true,   0, EquipSlot::NONE,   0,  0 },
    { "Wooden Sword",  static_cast<u8>(ItemID::WOODEN_SWORD),   12, true,  true,   0, EquipSlot::WEAPON, 5,  0 },
    { "Leather Armor", static_cast<u8>(ItemID::LEATHER_ARMOR),  14, true,  true,   0, EquipSlot::ARMOR,  0,  3 },
    { "Iron Sword",    static_cast<u8>(ItemID::IRON_SWORD),     30, true,  true,   0, EquipSlot::WEAPON, 10, 0 },
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
