#pragma once

//-----------------------------------------------------------------------------
// PlayerState.h  (Milestone 7 — Economy Loop: adds Inventory)
//
// Additions:
//   wood  — used to repair bridges and clear fallen trees
//   rope  — used to lower ladders
//   inventory — fixed 8-slot bag for shop/quest items (see Inventory.h).
//               Deliberately separate from wood/rope: those stay dedicated
//               counters used by the repair/gathering systems untouched
//               since Milestone 4/6. Unifying them into Inventory would
//               touch working code for no benefit toward Milestone 7's
//               goal (prove Gather → Store → Spend → Reward → Save → Load
//               for shop/quest items specifically).
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
// Save-friendly: struct embeds into SaveData verbatim. All fields are
// plain integer types or the POD Inventory struct. No pointers. No
// dynamic allocation.
//
// Memory: 24 bytes (8 bytes original fields + 16 bytes inventory).
//-----------------------------------------------------------------------------

#include "../../include/types.h"
#include "Inventory.h"

struct PlayerState {
    u32       gold;
    u8        wood;
    u8        rope;
    u8        pad[2];   // align to 4 bytes
    Inventory inventory;

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
};
