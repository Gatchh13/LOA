#pragma once

//-----------------------------------------------------------------------------
// ZoneData.h
// Static zone definitions: map tile data, transition triggers, spawn points,
// and zone metadata for all three Milestone 1 zones.
//
// Design:
//   All data is const arrays in ROM — zero heap allocation.
//   ZoneManager reads these tables by ZoneID index at load time.
//
// Map coordinates:
//   Tile (0,0) is top-left.
//   Tile (W-1, H-1) is bottom-right.
//   Pixel position = tile * TILE_SIZE.
//
// Transition triggers:
//   A transition fires when the player's center tile matches trigger_tx/ty.
//   The player is then placed at dest_spawn_index in the destination zone.
//
// Spawn points:
//   Index 0 is always the default spawn (used for new game / first entry).
//   Higher indices are used as arrival points from specific transitions.
//-----------------------------------------------------------------------------

#include "../../include/types.h"

//-----------------------------------------------------------------------------
// TransitionDef
// One entry per zone exit. When the player stands on (trigger_tx, trigger_ty)
// the ZoneManager transitions to dest_zone, placing the player at
// dest_zone's spawn point [dest_spawn_index].
//-----------------------------------------------------------------------------
struct TransitionDef {
    u8    trigger_tx;       // tile X that triggers the transition
    u8    trigger_ty;       // tile Y that triggers the transition
    ZoneID dest_zone;       // which zone to load
    u8    dest_spawn_index; // which spawn point in dest_zone to arrive at
};

//-----------------------------------------------------------------------------
// SpawnPoint
// Pixel position where the player appears when entering this zone.
// spawn_px / spawn_py are the TOP-LEFT of the player rect (not center).
//-----------------------------------------------------------------------------
struct SpawnPoint {
    float spawn_px;
    float spawn_py;
};

//-----------------------------------------------------------------------------
// ZoneDef
// Everything ZoneManager needs to load a zone.
//-----------------------------------------------------------------------------
struct ZoneDef {
    ZoneID      id;
    const char* name;           // shown briefly on transition (max 24 chars)

    // Map dimensions in tiles (must be <= MAX_ZONE_W / MAX_ZONE_H)
    u8 width;
    u8 height;

    // Tile data: row-major, [row * width + col]
    // Tile IDs defined in types.h
    const u8* tiles;

    // Background clear color for this zone (visible if map is smaller than screen)
    u32 bgColor;

    // Spawn points array + count
    const SpawnPoint* spawns;
    u8                spawnCount;

    // Transition triggers array + count
    const TransitionDef* transitions;
    u8                   transitionCount;
};

//=============================================================================
// ZONE 0 — TOWN
// 28 x 22 tiles.  Cobblestone village with a central plaza.
// Exits:
//   South road (bottom centre) → Forest (spawn 1 = north entry)
//=============================================================================

// 28 columns × 22 rows
// Tile key:
//   0 = TILE_GRASS        (green ground)
//   1 = TILE_DIRT         (brown path)
//   16 = TILE_WALL        (building wall)
//   19 = TILE_FENCE       (fence)
static const u8 s_townTiles[22 * 28] = {
//   0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17  18  19  20  21  22  23  24  25  26  27
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,  // row  0
    16,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 16,  // row  1
    16,  0, 16, 16,  0,  0, 16, 16,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 16, 16,  0,  0, 16, 16,  0,  0,  0, 16,  // row  2
    16,  0, 16, 16,  0,  0, 16, 16,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 16, 16,  0,  0, 16, 16,  0,  0,  0, 16,  // row  3
    16,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 16,  // row  4
    16,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0, 16,  // row  5
    16,  0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0, 16,  // row  6
    16,  0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0, 16,  // row  7
    16,  0,  0, 19, 19, 19, 19,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0, 19, 19, 19, 19,  0,  0,  0, 16,  // row  8
    16,  0,  0, 19,  0,  0, 19,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0, 19,  0,  0, 19,  0,  0,  0, 16,  // row  9
    16,  0,  0, 19,  0,  0, 19,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0, 19,  0,  0, 19,  0,  0,  0, 16,  // row 10
    16,  0,  0, 19, 19, 19, 19,  0,  0,  1,  1,  1,  1,  1,  1,  1,  1,  1,  0,  0, 19, 19, 19, 19,  0,  0,  0, 16,  // row 11
    16,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 16,  // row 12
    16,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 16,  // row 13
    16,  0, 16, 16,  0,  0,  0,  0,  0,  0,  0,  0,  1,  0,  0,  1,  0,  0,  0,  0,  0,  0, 16, 16,  0,  0,  0, 16,  // row 14
    16,  0, 16, 16,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  0,  0,  0,  0,  0,  0,  0, 16, 16,  0,  0,  0, 16,  // row 15
    16,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 16,  // row 16
    16,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 16,  // row 17
    16,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 16,  // row 18
    16,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 16,  // row 19
    16,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 16,  // row 20
    16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,  0,  0, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16, 16,  // row 21
};
// Transition: row 21 cols 13–14 are open — player walks into them to go south.
// We trigger at row 20 (one tile before the gap) so they don't clip the wall.

static const SpawnPoint s_townSpawns[] = {
    { 13 * TILE_SIZE + 1.0f, 18 * TILE_SIZE + 1.0f }, // [0] default: south road, facing north
    { 13 * TILE_SIZE + 1.0f,  1 * TILE_SIZE + 1.0f }, // [1] arrival from forest: north
};

static const TransitionDef s_townTransitions[] = {
    { 13, 21, ZoneID::FOREST, 1 },  // walk south off bottom → forest spawn[1]
    { 14, 21, ZoneID::FOREST, 1 },  // (two tiles wide path)
};

//=============================================================================
// ZONE 1 — FOREST
// 30 x 24 tiles.  Dense trees with a winding path.
// Exits:
//   North (row 0) → Town (spawn 1 = south road arrival)
//   East  (col 29, rows 11–12) → Dungeon Entrance (spawn 1)
//=============================================================================

static const u8 s_forestTiles[24 * 30] = {
//   0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17  18  19  20  21  22  23  24  25  26  27  28  29
    17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,  2,  2, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,  // row  0
    17, 17,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, 17, 17,  // row  1
    17,  2,  2,  2, 17, 17,  2,  2,  2,  2,  2,  2,  2,  1,  1,  2,  2,  2,  2,  2, 17, 17,  2,  2,  2,  2,  2,  2,  2, 17,  // row  2
    17,  2,  2, 17, 17, 17,  2,  2,  2,  2,  2,  2,  2,  1,  1,  2,  2,  2,  2, 17, 17, 17,  2,  2,  2,  2,  2,  2,  2, 17,  // row  3
    17,  2, 17, 17,  2,  2,  2,  2,  2,  2,  2,  2,  2,  1,  1,  2,  2,  2,  2,  2,  2, 17, 17,  2,  2,  2,  2,  2,  2, 17,  // row  4
    17,  2, 17,  2,  2,  2, 17, 17,  2,  2,  2,  2,  2,  1,  1,  2,  2,  2, 17, 17,  2,  2, 17,  2,  2,  2,  2,  2,  2, 17,  // row  5
    17,  2,  2,  2,  2, 17, 17,  2,  2,  2,  2,  2,  2,  1,  1,  2,  2,  2,  2, 17, 17,  2,  2,  2,  2,  2,  2,  2,  2, 17,  // row  6
    17,  2,  2,  2,  2,  2,  2,  2,  2, 17, 17,  2,  2,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  2, 17, 17,  2,  2,  2, 17,  // row  7
    17,  2,  2, 17,  2,  2,  2,  2, 17, 17,  2,  2,  2,  1,  1,  2,  2, 17, 17,  2,  2,  2,  2, 17,  2,  2,  2,  2,  2, 17,  // row  8
    17,  2,  2, 17, 17,  2,  2,  2,  2,  2,  2,  2,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2, 17, 17,  2,  2,  2,  2,  2, 17,  // row  9
    17,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, 17,  // row 10
    17,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  1,  0,  // row 11 — east exit
    17,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  1,  0,  // row 12 — east exit
    17,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  1,  1,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, 17,  // row 13
    17,  2,  2, 17, 17,  2,  2,  2,  2,  2,  2,  2,  2,  1,  1,  2,  2,  2,  2,  2,  2,  2, 17, 17,  2,  2,  2,  2,  2, 17,  // row 14
    17,  2,  2, 17,  2,  2,  2,  2,  2,  2,  2,  2,  2,  1,  1,  2,  2,  2,  2,  2,  2, 17,  2,  2,  2,  2,  2,  2,  2, 17,  // row 15
    17,  2,  2,  2,  2,  2, 17, 17,  2,  2,  2,  2,  2,  1,  1,  2,  2,  2,  2, 17, 17,  2,  2,  2,  2,  2,  2,  2,  2, 17,  // row 16
    17,  2, 17,  2,  2, 17, 17,  2,  2,  2,  2,  2,  2,  1,  1,  2,  2,  2, 17, 17,  2,  2, 17,  2,  2,  2,  2,  2,  2, 17,  // row 17
    17,  2, 17, 17,  2,  2,  2,  2,  2,  2,  2,  2,  2,  1,  1,  2,  2,  2,  2,  2,  2, 17, 17,  2,  2,  2,  2,  2,  2, 17,  // row 18
    17,  2,  2,  2, 17, 17,  2,  2,  2,  2,  2,  2,  2,  1,  1,  2,  2,  2,  2,  2, 17, 17,  2,  2,  2,  2,  2,  2,  2, 17,  // row 19
    17,  2,  2,  2,  2, 17, 17,  2,  2,  2,  2,  2,  2,  1,  1,  2,  2,  2,  2, 17, 17,  2,  2,  2,  2,  2,  2,  2,  2, 17,  // row 20
    17,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  1,  1,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, 17,  // row 21
    17, 17,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2,  2, 17, 17,  // row 22
    17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17, 17,  // row 23
};

static const SpawnPoint s_forestSpawns[] = {
    { 13 * TILE_SIZE + 1.0f, 21 * TILE_SIZE + 1.0f }, // [0] default: south of forest
    { 13 * TILE_SIZE + 1.0f,  1 * TILE_SIZE + 1.0f }, // [1] arrival from town (north)
    { 27 * TILE_SIZE + 1.0f, 11 * TILE_SIZE + 1.0f }, // [2] arrival from dungeon (east)
};

static const TransitionDef s_forestTransitions[] = {
    { 13, 0, ZoneID::TOWN,             1 }, // walk north off top  → town spawn[1]
    { 14, 0, ZoneID::TOWN,             1 }, // (two-tile-wide path)
    { 29, 11, ZoneID::DUNGEON_ENTRANCE, 1 }, // walk east off right → dungeon spawn[1]
    { 29, 12, ZoneID::DUNGEON_ENTRANCE, 1 },
};

//=============================================================================
// ZONE 2 — DUNGEON ENTRANCE
// 20 x 18 tiles. Stone corridor leading into darkness.
// Exits:
//   West (col 0, rows 8–9) → Forest (spawn 2 = east arrival)
//=============================================================================

static const u8 s_dungeonTiles[18 * 20] = {
//   0   1   2   3   4   5   6   7   8   9  10  11  12  13  14  15  16  17  18  19
    18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,  // row  0
    18,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 18,  // row  1
    18,  3, 18, 18,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 18, 18,  3,  // row  2 (intentional — open corners for visual)
    18,  3, 18, 18,  3,  3, 18, 18,  3,  3,  3,  3, 18, 18,  3,  3,  3, 18, 18,  3,  // row  3
    18,  3,  3,  3,  3,  3, 18, 18,  3,  3,  3,  3, 18, 18,  3,  3,  3,  3,  3, 18,  // row  4
    18,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 18,  // row  5
    18, 18, 18, 18, 18, 18, 18, 18,  3,  3,  3,  3, 18, 18, 18, 18, 18, 18, 18, 18,  // row  6
    18,  3,  3,  3,  3,  3,  3, 18,  3,  3,  3,  3, 18,  3,  3,  3,  3,  3,  3, 18,  // row  7
     3,  3,  3,  3,  3,  3,  3, 18,  3,  3,  3,  3, 18,  3,  3,  3,  3,  3,  3, 18,  // row  8 — west exit (col 0 open)
     3,  3,  3,  3,  3,  3,  3, 18,  3,  3,  3,  3, 18,  3,  3,  3,  3,  3,  3, 18,  // row  9 — west exit (col 0 open)
    18,  3,  3,  3,  3,  3,  3, 18,  3,  3,  3,  3, 18,  3,  3,  3,  3,  3,  3, 18,  // row 10
    18, 18, 18, 18, 18, 18, 18, 18,  3,  3,  3,  3, 18, 18, 18, 18, 18, 18, 18, 18,  // row 11
    18,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 18,  // row 12
    18,  3,  3, 18, 18,  3,  3,  3,  3,  3,  3,  3,  3,  3, 18, 18,  3,  3,  3, 18,  // row 13
    18,  3,  3, 18, 18,  3,  3,  3,  3,  3,  3,  3,  3,  3, 18, 18,  3,  3,  3, 18,  // row 14
    18,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 18,  // row 15
    18,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3,  3, 18,  // row 16
    18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18, 18,  // row 17
};

static const SpawnPoint s_dungeonSpawns[] = {
    {  2 * TILE_SIZE + 1.0f,  8 * TILE_SIZE + 1.0f }, // [0] default: west corridor
    {  1 * TILE_SIZE + 1.0f,  8 * TILE_SIZE + 1.0f }, // [1] arrival from forest (east→west)
};

static const TransitionDef s_dungeonTransitions[] = {
    { 0, 8, ZoneID::FOREST, 2 }, // walk west off left → forest spawn[2]
    { 0, 9, ZoneID::FOREST, 2 },
};

//=============================================================================
// Master zone table — indexed by ZoneID
//=============================================================================
static const ZoneDef s_zoneDefs[ZONE_COUNT] = {
    {   // ZoneID::TOWN
        ZoneID::TOWN,
        "Town",
        28, 22,
        s_townTiles,
        C2D_Color32(20, 20, 20, 255),
        s_townSpawns,    2,
        s_townTransitions, 2,
    },
    {   // ZoneID::FOREST
        ZoneID::FOREST,
        "Forest",
        30, 24,
        s_forestTiles,
        C2D_Color32(10, 20, 10, 255),
        s_forestSpawns,    3,
        s_forestTransitions, 4,
    },
    {   // ZoneID::DUNGEON_ENTRANCE
        ZoneID::DUNGEON_ENTRANCE,
        "Dungeon Entrance",
        20, 18,
        s_dungeonTiles,
        C2D_Color32(5, 5, 10, 255),
        s_dungeonSpawns,    2,
        s_dungeonTransitions, 2,
    },
};

// Retrieve a zone definition by ID.
inline const ZoneDef& getZoneDef(ZoneID id) {
    return s_zoneDefs[static_cast<u8>(id)];
}
