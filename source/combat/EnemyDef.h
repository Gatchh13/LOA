#pragma once

//-----------------------------------------------------------------------------
// EnemyDef.h  (Milestone 10 — Data-Driven Content Pipeline)
//
// Static, type-level enemy data — separated from Enemy.h's per-instance
// runtime state. Before Milestone 10, maxHp/contactDamage/lootItemId
// lived directly on the Enemy struct and were assigned once per
// instance inside EnemyManager.cpp's defineEnemies() — type-level facts
// ("every Forest Wolf has 40 max HP") duplicated into instance storage,
// which only stayed harmless because exactly one instance existed. The
// Milestone 9 assignment's own scope rule ("no multiple enemy types")
// is what kept this invisible; Milestone 10 explicitly asks to prepare
// for multiple types, which is exactly the case where the duplication
// would start to matter (two Wolf instances would each carry their own
// copy of facts that are true of "Wolf" as a type, not of either
// instance specifically).
//
// EnemyType (new): one entry per enemy type. Only FOREST_WOLF exists
// today — Milestone 9's explicit "one enemy type only" rule is still
// honored; this enum just gives that one type a name in the same place
// WorldObjectType already names BRIDGE/LADDER/OBSTACLE, rather than
// leaving "what kind of enemy is this" implicit.
//
// MEMORY: EnemyDef is static const data — 0 bytes RAM, same .rodata
// placement as every other *Def table in this project.
//-----------------------------------------------------------------------------

#include "../../include/types.h"

enum class EnemyType : u8 {
    FOREST_WOLF = 0,
    COUNT  // keep last — number of enemy types currently defined
};

//-----------------------------------------------------------------------------
// EnemyDef — one enemy type's static combat stats.
//-----------------------------------------------------------------------------
struct EnemyDef {
    EnemyType   type;
    const char* name;          // for logging/debug, not currently rendered
    u16         maxHp;
    u16         contactDamage; // damage dealt to the player on ATTACK contact
    u8          lootItemId;    // ItemID granted on death (see ItemDatabase.h)
};

// Accessor. type must be < getEnemyDefCount().
const EnemyDef& getEnemyDef(EnemyType type);

// Iteration helpers (Milestone 10 — new, same rationale as the other
// *Database.h files' getXCount()/getXByIndex()).
int getEnemyDefCount();
const EnemyDef& getEnemyDefByIndex(int index);
