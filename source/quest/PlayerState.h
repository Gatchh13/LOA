#pragma once

//-----------------------------------------------------------------------------
// PlayerState.h  (Milestone 8 — Combat Foundation: adds HP)
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
// Starting HP is 100/100 per the Milestone 8 assignment.
//
// Save-friendly: struct embeds into SaveData verbatim. All fields are
// plain integer types or the POD Inventory struct. No pointers. No
// dynamic allocation.
//
// Memory: 28 bytes (24 bytes from Milestone 7 + 4 bytes for hp/maxHp).
//-----------------------------------------------------------------------------

#include "../../include/types.h"
#include "Inventory.h"
#include "../items/ItemDef.h"

struct PlayerState {
    u32       gold;
    u8        wood;
    u8        rope;
    u8        pad[2];   // align to 4 bytes
    Inventory inventory;
    u16       hp;
    u16       maxHp;

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
};
