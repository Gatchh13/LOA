//-----------------------------------------------------------------------------
// EnemyDatabase.cpp  (Milestone 10 — Data-Driven Content Pipeline)
//
// The actual enemy-type table. One row per EnemyType. Data extracted
// verbatim from EnemyManager.cpp's old defineEnemies() function — same
// values (40 max HP, 10 contact damage, Wolf Pelt loot), just moved
// from per-instance assignment into a type-level table referenced by
// EnemyType instead.
//-----------------------------------------------------------------------------

#include "EnemyDatabase.h"
#include "../data/ItemDatabase.h"

static const EnemyDef s_enemyDefs[] = {
    {
        /*type=*/          EnemyType::FOREST_WOLF,
        /*name=*/          "Forest Wolf",
        /*maxHp=*/         40,
        /*contactDamage=*/ 10,
        /*lootItemId=*/    static_cast<u8>(ItemID::WOLF_PELT),
    },
};

static constexpr int ENEMY_DEF_COUNT = sizeof(s_enemyDefs) / sizeof(s_enemyDefs[0]);
static_assert(ENEMY_DEF_COUNT == static_cast<int>(EnemyType::COUNT),
              "s_enemyDefs and EnemyType registry are out of sync — "
              "every entry added to one must be added to the other");

const EnemyDef& getEnemyDef(EnemyType type) {
    return s_enemyDefs[static_cast<int>(type)];
}

int getEnemyDefCount() {
    return ENEMY_DEF_COUNT;
}

const EnemyDef& getEnemyDefByIndex(int index) {
    return s_enemyDefs[index];
}
