#pragma once

//-----------------------------------------------------------------------------
// PlayerState.h  (Milestone 9 — Equipment & Character Progression)
//
// Additions:
//   wood  — used to repair bridges and clear fallen trees
//   rope  — used to lower ladders
//   inventory — fixed 8-slot bag for shop/quest items (see Inventory.h).
//               Deliberately separate from wood/rope: those stay dedicated
//               counters used by the repair/gathering systems untouched
//               since Milestone 4/6.
//   hp / maxHp — player health (Milestone 8). u16 per the assignment's
//               recommendation, even though a single u8 (0-255) would
//               comfortably cover 100/100 — u16 leaves room for a future
//               leveling system to raise maxHp past 255 without another
//               field-width change. Two extra bytes is not worth
//               optimizing away for that headroom.
//   equippedWeapon / equippedArmor — Milestone 9. u8 ItemID of the
//               currently-equipped item in each slot, or NO_EQUIPMENT
//               (0xFF) if the slot is empty. 0xFF rather than 0 because
//               0 is a valid real ItemID (Healing Herb) — using 0 as
//               "empty" would be ambiguous the moment a real item with
//               ID 0 could ever be equipped. There are only two slots
//               (weapon, armor) per the assignment's explicit scope —
//               no array, no slot-index abstraction, just two named
//               fields, since "two of them, forever" doesn't benefit
//               from being generalized into a loop over N slots.
//
// PlayerState is a plain struct: no custom constructor. New-game starting
// values are set explicitly via init() rather than baked into a constructor,
// so "what does a fresh save start with" is one obvious call site instead of
// hidden default-construction logic.
//
// Starting resources are 0 as of Milestone 6 — GatherNodeManager (see
// source/world/GatherNodeManager.h) now provides wood/rope via harvestable
// nodes in Forest, so a fresh game requires gathering before repairing the
// bridge (5 wood), ladder (3 rope), or fallen tree (8 wood).
//
// Starting HP is 100/100 per the Milestone 8 assignment. Starting
// equipment is empty (both slots NO_EQUIPMENT) — the player earns their
// first weapon via the Missing Package quest reward (Milestone 9,
// Feature 8), not by starting with one.
//
// Save-friendly: struct embeds into SaveData verbatim. All fields are
// plain integer types or the POD Inventory struct. No pointers. No
// dynamic allocation.
//
// Memory: 32 bytes (28 bytes from Milestone 8 + 2 bytes for the two
// equipment slot fields + 2 bytes trailing padding to keep the struct's
// total size a multiple of 4, required since PlayerState starts with a
// u32 and is embedded by value in other structs — verified via
// offsetof/sizeof, not assumed: equippedArmor lands at offset 29, and
// the struct rounds up to 32).
//-----------------------------------------------------------------------------

#include "../../include/types.h"
#include "Inventory.h"
#include "../data/ItemDatabase.h"

// Sentinel for "no item equipped in this slot" — see header comment for
// why 0xFF rather than 0.
static constexpr u8 NO_EQUIPMENT = 0xFF;

// Base stats before equipment bonuses. Named here (not buried as a
// magic number in EnemyManager.cpp) since both the combat formula and
// the stats-panel HUD need the same starting point.
static constexpr u16 PLAYER_BASE_ATTACK  = 15;  // matches Milestone 8's
                                                  // flat PLAYER_ATTACK_DAMAGE
static constexpr u16 PLAYER_BASE_DEFENSE = 0;   // no innate defense —
                                                  // armor is the only source

struct PlayerState {
    u32       gold;
    u8        wood;
    u8        rope;
    u8        pad[2];   // align to 4 bytes
    Inventory inventory;
    u16       hp;
    u16       maxHp;
    u8        equippedWeapon;  // ItemID, or NO_EQUIPMENT
    u8        equippedArmor;   // ItemID, or NO_EQUIPMENT

    // Set new-game starting values. Call once when starting a fresh game
    // (not on load — SaveManager::apply() overwrites these fields directly
    // from save data, so init() is never called on a loaded game).
    void init() {
        gold   = 0;
        wood   = 0;
        rope   = 0;
        pad[0] = 0;
        pad[1] = 0;
        inventory.init();
        maxHp  = 100;
        hp     = maxHp;
        equippedWeapon = NO_EQUIPMENT;
        equippedArmor  = NO_EQUIPMENT;
    }

    void addGold(u32 amount)  { gold += amount; }

    // Clamped, not wrapped: wood/rope are now earned repeatedly via
    // gathering nodes (Milestone 6 — see GatherNodeManager), not just
    // granted once at game start, so silent u8 wraparound past 255 is a
    // real risk here, not a theoretical one.
    void addWood(u8 amount) {
        int sum = static_cast<int>(wood) + static_cast<int>(amount);
        wood = static_cast<u8>(sum > 255 ? 255 : sum);
    }
    void addRope(u8 amount) {
        int sum = static_cast<int>(rope) + static_cast<int>(amount);
        rope = static_cast<u8>(sum > 255 ? 255 : sum);
    }

    // Heal, clamped to maxHp — same clamp-not-wrap precedent as
    // addWood/addRope above. Used by both consumable items (Healing
    // Herb, Simple Potion) and the Milestone 8 death/respawn flow
    // (restores full HP).
    void heal(u16 amount) {
        u32 sum = static_cast<u32>(hp) + static_cast<u32>(amount);
        hp = static_cast<u16>(sum > maxHp ? maxHp : sum);
    }

    // Apply damage, clamped at 0 (u16 would otherwise wrap below 0 the
    // same way u8 wood/rope could wrap above 255 — same risk, same fix).
    void damage(u16 amount) {
        hp = (amount >= hp) ? 0 : static_cast<u16>(hp - amount);
    }

    bool isDead() const { return hp == 0; }

    // Use one of `item_id` as a healing consumable: removes it from the
    // inventory and applies its ItemDef::heal_amount via heal(). All-or-
    // nothing — if the player doesn't have the item, or the item has no
    // heal_amount (it isn't a healing consumable), nothing changes and
    // this returns false. This is deliberately just inventory.removeItem()
    // + heal() composed together rather than a separate "consumable use"
    // system — Healing Herb and Simple Potion are the only two usable
    // items in Milestone 8, and a dedicated system for two items would
    // be exactly the kind of speculative architecture the project
    // philosophy argues against.
    bool useConsumable(u8 item_id) {
        const ItemDef& def = getItemDef(item_id);
        if (def.heal_amount == 0) return false; // not a healing item
        if (!inventory.removeItem(item_id, 1))  return false; // don't have one
        heal(def.heal_amount);
        return true;
    }

    // Total attack: base + equipped weapon's bonus (0 if no weapon
    // equipped). Used by EnemyManager::tryAttack() for the combat
    // formula and by the Milestone 9 stats-panel HUD.
    u16 getAttack() const {
        u16 bonus = (equippedWeapon != NO_EQUIPMENT)
                   ? getItemDef(equippedWeapon).attack_bonus : 0;
        return PLAYER_BASE_ATTACK + bonus;
    }

    // Total defense: base + equipped armor's bonus (0 if no armor
    // equipped). Subtracted from incoming enemy damage in
    // EnemyManager::update()'s contact-damage formula.
    u16 getDefense() const {
        u16 bonus = (equippedArmor != NO_EQUIPMENT)
                   ? getItemDef(equippedArmor).defense_bonus : 0;
        return PLAYER_BASE_DEFENSE + bonus;
    }

    // Equip an item from the inventory into its ItemDef::equip_slot.
    // If something is already equipped in that slot, it's returned to
    // the inventory first (swap, not stack — the assignment's "old item
    // returns to inventory" requirement). Returns false (no change) if
    // the item isn't equipment, isn't held, or the swap-back would fail
    // because the inventory has no room for the previously-equipped item.
    bool equipItem(u8 item_id) {
        const ItemDef& def = getItemDef(item_id);
        if (def.equip_slot == EquipSlot::NONE) return false; // not equipment
        if (inventory.getQuantity(item_id) == 0) return false; // don't have it

        u8& slot = (def.equip_slot == EquipSlot::WEAPON) ? equippedWeapon : equippedArmor;

        if (slot == item_id) return false; // already equipped — use
                                            // unequipItem() for that, not this

        // Pull the new item out of the inventory before doing anything
        // else, so a failure below leaves nothing half-applied.
        if (!inventory.removeItem(item_id, 1)) return false;

        if (slot != NO_EQUIPMENT) {
            // Return the previously-equipped item to the inventory. If
            // this fails (inventory completely full of unrelated items),
            // undo the removeItem above and refuse the whole operation —
            // equipping should never destroy the old item as a side effect.
            if (!inventory.addItem(slot, 1)) {
                inventory.addItem(item_id, 1); // put the new item back
                return false;
            }
        }

        slot = item_id;
        return true;
    }

    // Unequip whatever is in `slot_type`, returning it to the inventory.
    // Returns false (no change) if that slot is already empty, or if
    // the inventory has no room for the returned item.
    bool unequipItem(EquipSlot slot_type) {
        u8& slot = (slot_type == EquipSlot::WEAPON) ? equippedWeapon : equippedArmor;
        if (slot_type == EquipSlot::NONE)  return false;
        if (slot == NO_EQUIPMENT)          return false;

        if (!inventory.addItem(slot, 1)) return false; // no room — refuse
        slot = NO_EQUIPMENT;
        return true;
    }
};
