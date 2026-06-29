#pragma once

//-----------------------------------------------------------------------------
// Inventory.h  (Milestone 7 — Economy Loop)
//
// The simplest inventory that can prove the economy loop: a fixed array
// of 8 slots, no sorting, no categories, no stack splitting, no drag and
// drop. Three operations: add, remove, check.
//
// Design:
//   InventorySlot { item_id, quantity }. quantity == 0 means the slot is
//   empty — no separate "active" bool needed, matching the existing
//   convention elsewhere in this codebase (e.g. WorldObject uses `state`
//   alone, not state + a redundant active flag, to mean "is this object
//   meaningfully present").
//
//   addItem() stacks onto an existing slot of the same item_id if one
//   exists, otherwise uses the first empty slot. This is "no stack
//   splitting" (an existing stack is never partially divided across two
//   slots) rather than "no stacking" — without any stacking at all,
//   buying two torches would consume two of the only eight slots, which
//   defeats the point of having slots at all for a shop-driven economy.
//
//   removeItem() is all-or-nothing: if the player doesn't have at least
//   `qty`, nothing is removed and it returns false. There is no partial
//   removal to reconcile, which is what "no stack splitting" rules out.
//
// Embedding: this struct is meant to be a member of PlayerState (see
// PlayerState.h), not a standalone system threaded through every
// function signature. Every function that already takes PlayerState&
// (QuestManager::applyReward, SaveManager::gather/apply, the HUD draw
// call) gets inventory access for free — this is the single decision
// that keeps Milestone 7 from touching the existing quest/save/gather
// call chains at all.
//
// SAVE IMPLICATIONS:
//   Inventory is POD (two u8 fields per slot, no pointers) — it
//   serializes byte-for-byte. 8 slots * 2 bytes = 16 bytes, written
//   directly into SaveData's existing reserved_inventory block (see
//   SaveData.h), which was sized at 64 bytes specifically for this.
//   No new save infrastructure is needed; SaveManager::gather()/apply()
//   gain a short loop each, not new code paths.
//
// MEMORY:
//   sizeof(InventorySlot) == 2 bytes. 8 slots == 16 bytes RAM total,
//   embedded directly in PlayerState (which goes from 8 to 24 bytes —
//   still trivial against the 64MB ceiling).
//-----------------------------------------------------------------------------

#include "../../include/types.h"

static constexpr int INVENTORY_SLOTS = 8;

struct InventorySlot {
    u8 item_id;   // valid only when quantity > 0
    u8 quantity;  // 0 = empty slot
};

struct Inventory {
    InventorySlot slots[INVENTORY_SLOTS];

    void init() {
        for (int i = 0; i < INVENTORY_SLOTS; i++) {
            slots[i].item_id  = 0;
            slots[i].quantity = 0;
        }
    }

    // Add `qty` of `item_id`. Stacks onto an existing slot of the same
    // item first; otherwise uses the first empty slot. Returns false
    // (no partial add) if there's no existing stack and no empty slot.
    bool addItem(u8 item_id, u8 qty) {
        if (qty == 0) return true; // nothing to do, not an error

        for (int i = 0; i < INVENTORY_SLOTS; i++) {
            if (slots[i].quantity > 0 && slots[i].item_id == item_id) {
                int sum = static_cast<int>(slots[i].quantity) + static_cast<int>(qty);
                slots[i].quantity = static_cast<u8>(sum > 255 ? 255 : sum);
                return true;
            }
        }
        for (int i = 0; i < INVENTORY_SLOTS; i++) {
            if (slots[i].quantity == 0) {
                slots[i].item_id  = item_id;
                slots[i].quantity = qty;
                return true;
            }
        }
        return false; // full
    }

    // Remove `qty` of `item_id`. All-or-nothing: returns false and
    // changes nothing if the player has fewer than `qty`.
    bool removeItem(u8 item_id, u8 qty) {
        if (qty == 0) return true;

        for (int i = 0; i < INVENTORY_SLOTS; i++) {
            if (slots[i].quantity > 0 && slots[i].item_id == item_id) {
                if (slots[i].quantity < qty) return false;
                slots[i].quantity -= qty;
                if (slots[i].quantity == 0) slots[i].item_id = 0;
                return true;
            }
        }
        return false; // don't have any
    }

    // How many of `item_id` the player currently holds (0 if none).
    u8 getQuantity(u8 item_id) const {
        for (int i = 0; i < INVENTORY_SLOTS; i++) {
            if (slots[i].quantity > 0 && slots[i].item_id == item_id) {
                return slots[i].quantity;
            }
        }
        return 0;
    }
};
