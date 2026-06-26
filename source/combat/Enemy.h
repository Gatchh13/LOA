#pragma once

//-----------------------------------------------------------------------------
// Enemy.h  (Milestone 10 — type-level stats split out to EnemyDef.h)
//
// Plain data struct for an enemy RUNTIME instance. Modeled closely on
// NPC.h and GatherNode.h's conventions, per Milestone 8's original
// instruction to reuse those patterns rather than invent a new one:
// fixed-size array storage, no inheritance, no vtables, no heap.
//
// Milestone 10 split: maxHp/contactDamage/lootItemId moved to EnemyDef
// (see EnemyDef.h) — they're facts about the TYPE (every Forest Wolf
// has the same max HP), not the instance. Enemy keeps current hp
// (mutable — decreases in combat), position, spawn position, AI state,
// and a `type` field referencing which EnemyDef this instance is.
//
// AI state machine (see EnemyManager.cpp for the actual transitions):
//   IDLE   — waiting at spawn_x/spawn_y.
//   CHASE  — player is within DETECT_RANGE; move toward the player's
//            current position every frame (re-targeted continuously,
//            same horizontal-then-vertical step pattern as
//            NPCManager::moveNPC — no pathfinding).
//   ATTACK — player is within ATTACK_RANGE; deal damage on a cooldown,
//            do not move.
//   RETURN — player left DETECT_RANGE while away from spawn; move back,
//            then go IDLE.
//
// Death: no corpse persistence, no death animation. The dead enemy's
// `active` flag is set false and it is simply not drawn or updated
// again until EnemyManager::respawnAll() brings it back at full HP at
// spawn (reading maxHp fresh from its EnemyDef, not a stored copy).
//-----------------------------------------------------------------------------

#include "../../include/types.h"
#include "../entities/AnimState.h"
#include "EnemyDef.h"

static constexpr int MAX_ENEMIES = 8;  // only 1 used today; headroom for
                                        // more instances (of the same or
                                        // a future second type) without
                                        // another array-size change.

enum class EnemyAIState : u8 {
    IDLE   = 0,
    CHASE  = 1,
    ATTACK = 2,
    RETURN = 3,
};

struct Enemy {
    u8        id;
    EnemyType type;   // which EnemyDef this instance represents — see
                       // EnemyDef.h for maxHp/contactDamage/lootItemId
    ZoneID    zone;

    // Current position (pixel, top-left of sprite — same convention as NPC).
    float pos_x;
    float pos_y;

    // Spawn position — Return state moves the enemy back here.
    float spawn_x;
    float spawn_y;

    // Current HP — genuinely per-instance (decreases in combat).
    // maxHp lives on EnemyDef; compare against getEnemyDef(type).maxHp.
    u16 hp;

    EnemyAIState state;
    float        attackCooldown;  // seconds remaining until next attack tick

    AnimState anim;   // reused as-is from Milestone 6 — same facing/walk-
                       // frame struct already used by Player and NPC.

    bool active;       // false = this slot is unused, OR this enemy is dead
};

// Result of EnemyManager::tryAttack(). Mirrors InteractResult/GatherResult's
// existing pattern (a small result enum returned by the one verb this
// system exposes to main.cpp) rather than inventing a different shape.
enum class AttackResult : u8 {
    NONE,    // no enemy was in range, or the attack is on cooldown
    HIT,     // an enemy was damaged
    KILLED,  // an enemy was damaged and died from this hit
};
