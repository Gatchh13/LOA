//-----------------------------------------------------------------------------
// SpawnDatabase.cpp  (Milestone 10 — Data-Driven Content Pipeline)
//
// The actual spawn table. One Forest Wolf spawn, extracted verbatim
// from EnemyManager.cpp's old defineEnemies() — same position (tile
// (6,19) in Forest, checked against s_forestTiles in ZoneData.h to
// confirm it's clear of every gather node, world object, and the dirt
// path — see the original comment, preserved below), same type,
// respawn_enabled = true (matching EnemyManager::respawnAll()'s
// existing unconditional-revive behavior).
//-----------------------------------------------------------------------------

#include "SpawnDatabase.h"

// Placed at tile (6,19) in Forest — clear of all four GatherNodeManager
// nodes ((5,5), (24,6), (8,16), (20,18)), the rope-ladder WorldObject
// (0,3)-(0,5), the fallen-tree WorldObject (22,11)-(22,12), the Forest
// spawn points, and the north-south dirt path (columns 13-14). Far
// enough from the south entrance that it isn't an instant ambush on
// zone entry, but well within a short walk.
static const SpawnEntry s_spawnTable[] = {
    {
        /*type=*/            EnemyType::FOREST_WOLF,
        /*zone=*/             ZoneID::FOREST,
        /*spawn_x=*/          6 * TILE_SIZE + 8.0f,   // 104
        /*spawn_y=*/          19 * TILE_SIZE + 8.0f,  // 312
        /*respawn_enabled=*/  true,
    },
};

static constexpr int SPAWN_COUNT = sizeof(s_spawnTable) / sizeof(s_spawnTable[0]);

const SpawnEntry& getSpawnEntry(int index) {
    return s_spawnTable[index];
}

int getSpawnCount() {
    return SPAWN_COUNT;
}
