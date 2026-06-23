#pragma once

//-----------------------------------------------------------------------------
// PlayerState.h  (Milestone 4 — adds resources)
//
// Additions:
//   wood  — used to repair bridges and clear fallen trees
//   rope  — used to lower ladders
//
// PlayerState is a plain struct: no custom constructor. New-game starting
// values are set explicitly via init() rather than baked into a constructor,
// so "what does a fresh save start with" is one obvious call site instead of
// hidden default-construction logic.
//
// PLACEHOLDER STARTING RESOURCES:
//   There is no gathering system yet (planned for Milestone 6). Until it
//   exists, init() grants a fixed amount of wood/rope so the three world
//   objects (bridge: 5 wood, ladder: 3 rope, fallen tree: 8 wood — see
//   WorldObjectManager.cpp) are all reachable for testing. Remove
//   PLACEHOLDER_STARTING_WOOD / _ROPE the moment gathering nodes exist.
//
// Save-friendly: struct embeds into SaveData verbatim. All fields are
// plain integer types. No pointers. No dynamic allocation.
//
// Memory: 20 bytes.
//-----------------------------------------------------------------------------

#include "../../include/types.h"

// TODO(Milestone 6): delete once resource gathering exists.
static constexpr u8 PLACEHOLDER_STARTING_WOOD = 20;
static constexpr u8 PLACEHOLDER_STARTING_ROPE = 10;

struct PlayerState {
    u32 gold;
    u8  wood;
    u8  rope;
    u8  pad[2];   // align to 4 bytes

    // Set new-game starting values. Call once when starting a fresh game
    // (not on load — SaveManager::apply() overwrites these fields directly
    // from save data, so init() is never called on a loaded game).
    void init() {
        gold   = 0;
        wood   = PLACEHOLDER_STARTING_WOOD;
        rope   = PLACEHOLDER_STARTING_ROPE;
        pad[0] = 0;
        pad[1] = 0;
    }

    void addGold(u32 amount)  { gold += amount; }
    void addWood(u8  amount)  { wood = static_cast<u8>(wood + amount); }
    void addRope(u8  amount)  { rope = static_cast<u8>(rope + amount); }
};
