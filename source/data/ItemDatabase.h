#pragma once

//-----------------------------------------------------------------------------
// ItemDatabase.h  (Milestone 10 — Data-Driven Content Pipeline)
//
// Central item registry. Moved here from source/items/ItemDef.h as part
// of Milestone 10's content-pipeline cleanup — same data, same layout,
// new home. The old source/items/ directory is removed; every file
// that referenced ItemDef.h now includes this instead.
//
// What actually changed, and what didn't:
//   - ItemID enum, EquipSlot enum, ItemDef struct: UNCHANGED. Same
//     fields, same order, same memory layout. No save-format impact —
//     see the Milestone 10 design doc's save-impact section.
//   - The data table (s_itemDefs[]) moved from this header into
//     ItemDatabase.cpp. Previously it was a `static const` array
//     sitting directly in the header, meaning every translation unit
//     that included ItemDef.h got its own copy of the array in its
//     object file (harmless for an array this small, but not how a
//     "central registry" should be structured once the table is meant
//     to be edited in one place by name, per the assignment's explicit
//     goal). Now there is exactly one definition, in exactly one .cpp.
//   - NEW: iteration helpers (getItemCount(), getItemByIndex()) — the
//     assignment asked for these explicitly ("provide getItem(id) and
//     iteration helpers"). Nothing in the existing codebase needed to
//     iterate "all items" before now (every call site already knew
//     which specific item it wanted), so this genuinely didn't exist.
//
// Still true, unchanged from Milestone 7-9:
//   - Compile-time static data. No JSON. No reflection. No dynamic
//     loading. Adding an item means adding one ItemID enum entry and
//     one row to s_itemDefs[] in ItemDatabase.cpp — exactly one table,
//     exactly as the assignment requires.
//   - The static_assert consistency checks (count matches the enum,
//     count fits MAX_ITEMS) are unchanged and still compile-time.
//
// MEMORY FOOTPRINT: unchanged from Milestone 9 — see ItemDef struct
// layout below. 12 items * 16 bytes (ARM32) = 192 bytes .rodata, 0
// bytes RAM. Moving the array into a .cpp doesn't change its size or
// its storage class (still `static const`, still compiled into
// .rodata) — it only changes which translation unit defines it.
//-----------------------------------------------------------------------------

#include "../../include/types.h"

//-----------------------------------------------------------------------------
// ItemID registry — single source of truth for item identifiers.
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
    WOLF_PELT      = 8,  // loot-only, not bought, but sellable
    WOODEN_SWORD   = 9,  // equipment: weapon
    LEATHER_ARMOR  = 10, // equipment: armor
    IRON_SWORD     = 11, // equipment: weapon
    COUNT  // keep last — number of items currently defined
};

static constexpr int MAX_ITEMS = 12;

// Which equipment slot an item occupies. NONE = not equipment.
enum class EquipSlot : u8 {
    NONE   = 0,
    WEAPON = 1,
    ARMOR  = 2,
};

//-----------------------------------------------------------------------------
// ItemDef — one item's static data. Field order is deliberate (pointer
// first avoids leading alignment padding) — see ItemDatabase.cpp for
// the verified ARM32 size analysis.
//
// heal_amount: HP restored on use, 0 = not a healing consumable.
// equip_slot/attack_bonus/defense_bonus: equipment data. Only items
// with equip_slot != EquipSlot::NONE use the bonus fields.
// base_price: gold cost to buy (if buyable) and the basis for sell
// price (if sellable) — see ItemDatabase.cpp's getSellPrice() callers
// in Shop.h. Set even for non-buyable-but-sellable items like Wolf
// Pelt, where it only ever feeds sell price.
//-----------------------------------------------------------------------------
struct ItemDef {
    const char* name;
    u8          id;
    u8          base_price;
    bool        buyable;
    bool        sellable;
    u16         heal_amount;
    EquipSlot   equip_slot;
    u8          attack_bonus;
    u8          defense_bonus;
};

// Accessor. item_id must be < getItemCount().
const ItemDef& getItemDef(u8 item_id);

//-----------------------------------------------------------------------------
// Iteration helpers (Milestone 10 — new).
//
// Nothing before Milestone 10 needed to walk "every item" — every call
// site already knew which specific ItemID it wanted (a quest reward, a
// shop purchase, a consumable use). These exist now because the
// assignment asks for them explicitly, and because a real content
// pipeline needs a way to enumerate its own content (e.g. a debug menu
// that lists every item, or a future "give all items" dev command) —
// that capability didn't exist anywhere in this codebase before.
//-----------------------------------------------------------------------------

// Total number of defined items. Same value as ITEM_DEF_COUNT used to
// expose via the static_assert in the old ItemDef.h, now also reachable
// as a runtime call for code that doesn't want to depend on the
// compile-time constant name directly.
int getItemCount();

// Index-based access for iteration: for (int i = 0; i < getItemCount(); i++)
// getItemByIndex(i). Distinct from getItemDef(item_id) — for every item
// currently defined, index i and ItemID value i happen to coincide
// (s_itemDefs[] is built in ItemID order), but getItemByIndex() doesn't
// assume that always holds, so reordering the table later wouldn't
// silently break iteration.
const ItemDef& getItemByIndex(int index);
