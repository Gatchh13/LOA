#pragma once

//-----------------------------------------------------------------------------
// SpawnDatabase.h  (Milestone 10 — Data-Driven Content Pipeline)
//
// Declarations only — mirrors every other *Database.h's split (the
// actual const data table lives in exactly one .cpp). See SpawnTable.h
// for the struct itself.
//
// Adding a new spawn point: one row in SpawnDatabase.cpp — EnemyType,
// zone, position, respawn_enabled. EnemyManager::init() and
// respawnAll() already iterate this table generically; neither needs
// to change to support a new spawn entry, only MAX_ENEMIES needs
// enough headroom (currently 8, 1 used) for however many total
// instances exist across every zone combined.
//-----------------------------------------------------------------------------

#include "SpawnTable.h"
