#pragma once

//-----------------------------------------------------------------------------
// PlayerState.h
// Minimal persistent player data for Milestone 3.
//
// Deliberately tiny. This is NOT the full save system (that comes later).
// It holds only what the quest system needs right now: gold.
//
// When the save system is built, this struct will be embedded inside
// SaveData verbatim — no migration needed.
//
// Memory: 8 bytes.
//-----------------------------------------------------------------------------

#include "../../include/types.h"

struct PlayerState {
    u32 gold;       // current gold total
    u32 reserved;   // pad to 8 bytes; available for future use

    PlayerState() : gold(0), reserved(0) {}

    void addGold(u32 amount) { gold += amount; }
};
