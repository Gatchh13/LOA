#pragma once

//-----------------------------------------------------------------------------
// WorldObject.h
// Data types for the world object / shortcut system.
//
// A WorldObject is a piece of world infrastructure that starts broken and
// can be permanently repaired by the player. Repairing it modifies tile
// collision, creating new passages or shortcuts.
//
// Save-friendly design:
//   The entire runtime state of all world objects can be serialized as:
//     u8 states[MAX_WORLD_OBJECTS]   (one byte per object)
//   A future SaveData embeds this array directly. No migration needed.
//
// Tile override design:
//   Each object owns a list of (tx, ty, broken_tile, repaired_tile) entries.
//   WorldObjectManager writes these overrides into TileMap on load/repair.
//   INACTIVE objects apply their broken_tile overrides (blocking).
//   REPAIRED objects apply their repaired_tile overrides (open).
//   Objects where broken_tile == the ZoneData tile need no INACTIVE override.
//-----------------------------------------------------------------------------

#include "../../include/types.h"

//-----------------------------------------------------------------------------
// Enums
//-----------------------------------------------------------------------------
enum class WorldObjectType : u8 {
    BRIDGE   = 0,
    LADDER   = 1,
    OBSTACLE = 2,
};

enum class WorldObjectState : u8 {
    INACTIVE = 0,   // broken / blocked — not yet repaired
    REPAIRED = 1,   // passable / functional
};

//-----------------------------------------------------------------------------
// Resource costs
//-----------------------------------------------------------------------------
struct WorldObjectCost {
    u8 wood;
    u8 rope;
};

//-----------------------------------------------------------------------------
// TileOverride
// One tile changed by this object's state.
// broken_tile: tile ID when INACTIVE  (applied on zone load if INACTIVE)
// repaired_tile: tile ID when REPAIRED (applied on repair or zone load if REPAIRED)
//
// If broken_tile == 0xFF, no override is applied in INACTIVE state
// (the ZoneData tile already represents the broken state).
//-----------------------------------------------------------------------------
struct TileOverride {
    u8 tx;
    u8 ty;
    u8 broken_tile;    // 0xFF = no override (zone data is already correct)
    u8 repaired_tile;  // tile ID after repair (typically passable)
};

//-----------------------------------------------------------------------------
// WorldObject
// Runtime instance. Stored in WorldObjectManager's fixed array.
//-----------------------------------------------------------------------------
static constexpr int MAX_WORLD_OBJECTS    = 16;
static constexpr int MAX_OBJECT_TILES     = 4;   // tiles affected per object
static constexpr float OBJECT_INTERACT_RANGE = 28.0f;  // pixels

struct WorldObject {
    u8               id;
    WorldObjectType  type;
    WorldObjectState state;
    ZoneID           zone;

    // Interaction point — player must be within OBJECT_INTERACT_RANGE of this.
    // Stored in pixel coordinates (center of the primary affected tile).
    float            interact_x;
    float            interact_y;

    // Resource cost to repair
    WorldObjectCost  cost;

    // Tile overrides this object controls
    TileOverride     tiles[MAX_OBJECT_TILES];
    u8               tile_count;

    // Human-readable label for HUD messages
    const char*      label;           // e.g. "Bridge", "Ladder", "Fallen Tree"
    const char*      repair_message;  // e.g. "Bridge repaired."
    const char*      clear_message;   // e.g. "Obstacle cleared." (may equal repair_message)

    bool active;   // false = slot unused
};

//-----------------------------------------------------------------------------
// Interaction result — returned by WorldObjectManager::tryInteract
//-----------------------------------------------------------------------------
enum class InteractResult : u8 {
    NONE,               // player not near any object, or A not pressed
    REPAIRED,           // object was just repaired
    ALREADY_REPAIRED,   // player pressed A on a repaired object
    NEED_RESOURCES,     // in range but can't afford
};
