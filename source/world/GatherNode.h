#pragma once

//-----------------------------------------------------------------------------
// GatherNode.h  (Milestone 6 — Foundation of Feel)
//
// A fixed-position resource node the player can harvest with A when in
// range. Unlike WorldObject (repair bridges/ladders/obstacles, one-time
// state change), a GatherNode is repeatable: harvest it, it goes on
// cooldown, it becomes available again after COOLDOWN_SECONDS.
//
// This is what finally removes the need for PlayerState's
// PLACEHOLDER_STARTING_WOOD/_ROPE hack — see PlayerState.h. Starting
// resources can eventually drop to 0 once nodes exist to earn more.
//
// Modeled closely on WorldObject's plain-data, fixed-array conventions:
// no inheritance, no heap, POD-friendly layout, save-restorable by index.
//-----------------------------------------------------------------------------

#include "../../include/types.h"

static constexpr int MAX_GATHER_NODES = 16;

// How long a harvested node stays empty before it can be harvested again.
// Generous on purpose for M6 — the loop is "explore, gather a little,
// repair something" not "grind nodes on a timer". Tune later once there's
// actual playtesting to inform it.
static constexpr float GATHER_COOLDOWN_SECONDS = 60.0f;

// Max interaction distance, matching OBJECT_INTERACT_RANGE's role for
// WorldObject (see WorldObject.h) — same value, kept as its own constant
// since gather nodes and world objects are conceptually separate systems
// that happen to agree on this number today.
static constexpr float GATHER_INTERACT_RANGE = 28.0f;

enum class GatherResource : u8 {
    WOOD = 0,
    ROPE = 1,
};

struct GatherNode {
    u8             id;            // unique ID within this manager
    ZoneID         zone;          // which zone this node lives in
    float          pos_x;         // pixel position (interaction point)
    float          pos_y;
    GatherResource resource;
    u8             amount;        // how much is granted per harvest
    float          cooldown;      // seconds remaining until harvestable again; 0 = ready
    bool           active;        // false = this slot is unused
};

// Result of GatherNodeManager::tryHarvest(). Deliberately separate from
// WorldObject's InteractResult (see WorldObject.h) — REPAIRED/NEED_RESOURCES
// don't fit gathering's semantics (a node grants resources, it doesn't
// consume them, and "on cooldown" has no WorldObject equivalent).
enum class GatherResult : u8 {
    NONE,        // not in range of any node, or A not pressed
    HARVESTED,   // resource granted, node now on cooldown
    ON_COOLDOWN, // in range, but the closest node hasn't respawned yet
};
