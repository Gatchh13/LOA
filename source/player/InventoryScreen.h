#pragma once

//-----------------------------------------------------------------------------
// InventoryScreen.h  (Milestone 9 — Equipment & Character Progression)
//
// The minimal equip/unequip UI the assignment asks for: a bottom-screen
// text list of the player's actual inventory slots, cursor navigation,
// A to equip/unequip, B to exit. No drag-and-drop, no icons, no separate
// "equipment screen", no tabs, no categories — just the 8 inventory
// slots, in slot order.
//
// Architecture:
//   Same shape as ShopUI (cursor index + open/closed bool) rather than a
//   new kind of UI state object — this project already has one proven
//   pattern for "a simple cursor-driven bottom-screen list with an
//   action button," and this is exactly that pattern applied to a
//   different list (inventory slots instead of shop stock).
//
//   Unlike ShopUI, this cursor walks PlayerState::inventory.slots[]
//   directly rather than a separate static item list — there's no
//   equivalent of "shop stock" for your own bag; the 8 real inventory
//   slots ARE the list. Empty slots are skippable-but-shown (drawn as
//   "-- empty --", not hidden) so the slot positions stay stable while
//   navigating, rather than the list reflowing as items are added/
//   removed mid-session.
//
// Equip/unequip semantics (per the assignment):
//   A on a slot holding equipment that's NOT currently equipped: equip
//   it (swaps out whatever was in that slot, which returns to the
//   inventory — see PlayerState::equipItem()).
//   A on a slot holding the item that's CURRENTLY equipped: unequip it
//   (see PlayerState::unequipItem()).
//   A on a slot holding a non-equipment item, or an empty slot: no-op.
//
// Trigger: START opens this screen from normal gameplay (per the
// assignment's explicit "Open inventory with START"). This repurposes
// START, which previously triggered an in-gameplay reload — that
// feature is removed in Milestone 9 (Continue from the title screen
// remains the only load path; see the Milestone 9 design doc's risk
// section for why every other button was already claimed).
//
// MEMORY: InventoryScreen is 2 small fields (~8 bytes), same as ShopUI.
//
// CPU: equip/unequip attempts are O(1) (PlayerState::equipItem/
// unequipItem are themselves O(INVENTORY_SLOTS) at most) and only run
// on a button press, not every frame.
//-----------------------------------------------------------------------------

#include "../../include/types.h"
#include "PlayerState.h"
#include "../data/ItemDatabase.h"

enum class InventoryAction : u8 {
    NONE,      // no action attempted this call, or the slot under the
                // cursor doesn't support one (empty, or non-equipment)
    EQUIPPED,
    UNEQUIPPED,
};

class InventoryScreen {
public:
    void open() {
        m_isOpen = true;
        m_cursor = 0;
    }

    void close() {
        m_isOpen = false;
    }

    bool isOpen() const { return m_isOpen; }

    // Move the cursor by `dir` (+1 down, -1 up), clamped — same
    // hard-stop-not-wrap convention as ShopUI::moveCursor.
    void moveCursor(int dir) {
        int next = m_cursor + dir;
        if (next < 0) next = 0;
        if (next >= INVENTORY_SLOTS) next = INVENTORY_SLOTS - 1;
        m_cursor = next;
    }

    int getCursor() const { return m_cursor; }

    // Equip or unequip whatever is in the inventory slot under the
    // cursor, per the semantics in the header comment above. Returns
    // NONE (no change) if the slot is empty or holds a non-equipment
    // item.
    InventoryAction tryActivate(PlayerState& playerState) {
        const InventorySlot& slot = playerState.inventory.slots[m_cursor];
        if (slot.quantity == 0) return InventoryAction::NONE; // empty slot

        const ItemDef& def = getItemDef(slot.item_id);
        if (def.equip_slot == EquipSlot::NONE) return InventoryAction::NONE; // not equipment

        bool isCurrentlyEquipped =
            (def.equip_slot == EquipSlot::WEAPON && playerState.equippedWeapon == slot.item_id) ||
            (def.equip_slot == EquipSlot::ARMOR  && playerState.equippedArmor  == slot.item_id);

        if (isCurrentlyEquipped) {
            bool ok = playerState.unequipItem(def.equip_slot);
            return ok ? InventoryAction::UNEQUIPPED : InventoryAction::NONE;
        } else {
            bool ok = playerState.equipItem(slot.item_id);
            return ok ? InventoryAction::EQUIPPED : InventoryAction::NONE;
        }
    }

private:
    bool m_isOpen = false;
    int  m_cursor = 0;
};
