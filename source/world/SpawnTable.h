#pragma once

//-----------------------------------------------------------------------------
// SpawnTable.h  (Milestone 10 — Data-Driven Content Pipeline)
//
// Zone-driven enemy spawn definitions. Separated from EnemyDef.h
// (type-level combat stats) and from Enemy.h (runtime instance state):
// a SpawnEntry says WHERE an enemy of a given EnemyType appears and
// WHETHER it respawns — it does not duplicate the type's stats (those
// live on EnemyDef, looked up via the type field) and it is not itself
// runtime state (EnemyManager copies a SpawnEntry's position/type into
// a fresh Enemy instance at init() time; the SpawnEntry itself never
// changes).
//
// Before Milestone 10, this same information (Forest Wolf, spawn tile
// (6,19), implicitly always respawns since EnemyManager::respawnAll()
// unconditionally revives every enemy) was inline in
// EnemyManager.cpp's defineEnemies() function, mixed with the runtime
// initialization code that actually constructs the Enemy struct.
// SpawnTable.h/SpawnDatabase.{h,cpp} hold only the data; EnemyManager's
// init()/respawnAll() consume it.
//
// respawn_enabled (Milestone 10 — new field): whether this spawn point
// brings its enemy back after death. Every current spawn has this set
// true, matching the existing (and only) behavior — respawnAll()
// already revives every enemy unconditionally, so true is not a
// behavior change, just making an assumption that was previously
// implicit (and therefore unable to ever be anything else) into an
// explicit, per-spawn-point value. A future "this enemy guards a
// one-time treasure room and shouldn't respawn" spawn point can now
// set this to false without EnemyManager's respawn logic needing to
// change shape — it would just skip spawns where this is false.
//
// MEMORY: SpawnEntry table is static const — 0 bytes RAM.
//-----------------------------------------------------------------------------

#include "../../include/types.h"
#include "../combat/EnemyDef.h"

struct SpawnEntry {
    EnemyType type;
    ZoneID    zone;
    float     spawn_x;
    float     spawn_y;
    bool      respawn_enabled;
};

// Accessor. index must be < getSpawnCount().
const SpawnEntry& getSpawnEntry(int index);

int getSpawnCount();
