#pragma once

//-----------------------------------------------------------------------------
// PlayerState.h  (Milestone 4 — adds resources)
//
// Additions:
//   wood  — used to repair bridges and clear fallen trees
//   rope  — used to lower ladders
//
// Test values: player starts with wood=20, rope=10.
// These will be replaced by a gathering system in a future milestone.
//
// Save-friendly: struct embeds into SaveData verbatim. All fields are
// plain integer types. No pointers. No dynamic allocation.
//
// Memory: 20 bytes.
//-----------------------------------------------------------------------------

#include "../../include/types.h"

struct PlayerState {
    u32 gold;
    u8  wood;
    u8  rope;
    u8  pad[2];   // align to 4 bytes

    PlayerState()
        : gold(0)
        , wood(20)   // test starting values
        , rope(10)
    {
        pad[0] = 0;
        pad[1] = 0;
    }

    void addGold(u32 amount)  { gold += amount; }
    void addWood(u8  amount)  { wood = static_cast<u8>(wood + amount); }
    void addRope(u8  amount)  { rope = static_cast<u8>(rope + amount); }
};
