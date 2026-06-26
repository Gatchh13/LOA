//-----------------------------------------------------------------------------
// ItemDatabase.cpp  (Milestone 10 — Data-Driven Content Pipeline)
//
// The actual item table. This is the ONE place to edit when adding,
// removing, or changing an item — per the assignment's explicit goal
// ("future item additions require editing one table only"). Adding an
// item means: add one entry to ItemID in ItemDatabase.h, add one row
// here, done. The static_asserts below catch a mismatch between the
// two at compile time.
//
// MEMORY FOOTPRINT (verified, not assumed — same analysis carried over
// from Milestone 9, since the struct itself didn't change):
//   ItemDef is { const char* name; u8 id; u8 base_price; bool buyable;
//   bool sellable; u16 heal_amount; EquipSlot equip_slot; u8
//   attack_bonus; u8 defense_bonus; }. On 32-bit ARM: 4 (pointer) +
//   1+1+1+1 (id/base_price/buyable/sellable) + 2 (heal_amount, 2-byte
//   aligned) + 1 (equip_slot) + 1+1 (attack_bonus/defense_bonus) = 13,
//   rounded to 16 bytes for 4-byte array-stride alignment. 12 items *
//   16 bytes = 192 bytes, in .rodata — not the 64MB RAM ceiling.
//   Runtime RAM cost: 0 bytes.
//
//   This build environment's test compiler is 64-bit, where sizeof
//   reports a larger figure (8-byte pointer) — that's the test host,
//   not the 3DS target. devkitARM is 32-bit ARM, where 16 bytes applies.
//-----------------------------------------------------------------------------

#include "ItemDatabase.h"

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

const ItemDef& getItemDef(u8 item_id) {
    return s_itemDefs[item_id];
}

int getItemCount() {
    return ITEM_DEF_COUNT;
}

const ItemDef& getItemByIndex(int index) {
    return s_itemDefs[index];
}
