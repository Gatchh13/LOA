#pragma once

//-----------------------------------------------------------------------------
// Enemy.h  (Milestone 8 — Combat Foundation)
//
// Plain data struct for the first combat enemy (Forest Wolf). Modeled
// closely on NPC.h and GatherNode.h's conventions, per the assignment's
// explicit instruction to reuse those patterns rather than invent a new
// one: fixed-size array storage, no inheritance, no vtables, no heap.
//
// AI state machine (see EnemyManager.cpp for the actual transitions):
//   IDLE   — waiting at spawn_x/spawn_y.
//   CHASE  — player is within DETECT_RANGE; move toward the player's
//            current position every frame (re-targeted continuously,
//            same horizontal-then-vertical step pattern as
//            NPCManager::moveNPC — no pathfinding, exactly as scoped).
//   ATTACK — player is within ATTACK_RANGE; deal damage on a cooldown,
//            do not move (a wolf standing still to bite is simpler than
//            also chasing while attacking, and matches "no complex
//            combat logic").
//   RETURN — player left DETECT_RANGE while the wolf was away from
//            spawn; move back to spawn_x/spawn_y, then go IDLE.
//
// Only one enemy type exists in Milestone 8 (the assignment explicitly
// rules out multiple enemy types), so there is no EnemyType enum the
// way WorldObjectType exists for three object kinds — adding one now
// would be exactly the kind of speculative architecture the project
// philosophy says to avoid. If a second enemy type is added later, an
// EnemyType field can be added the same way WorldObjectType already
// demonstrates the pattern for this codebase.
//
// Death: no corpse persistence, no death animation. The dead enemy's
// `active` flag is set false and it is simply not drawn or updated
// again until EnemyManager::respawnAll() (called only on player death/
// respawn, see EnemyManager.h) brings it back at full HP at spawn.
//-----------------------------------------------------------------------------

#include "../../include/types.h"
#include "../entities/AnimState.h"

static constexpr int MAX_ENEMIES = 8;  // only 1 used in M8; headroom for
                                        // the next enemy without another
                                        // array-size change.

enum class EnemyAIState : u8 {
    IDLE   = 0,
    CHASE  = 1,
    ATTACK = 2,
    RETURN = 3,
};

struct Enemy {
    u8     id;
    ZoneID zone;

    // Current position (pixel, top-left of sprite — same convention as NPC).
    float pos_x;
    float pos_y;

    // Spawn position — Return state moves the enemy back here.
    float spawn_x;
    float spawn_y;

    // Combat stats
    u16 hp;
    u16 maxHp;
    u16 contactDamage;   // damage dealt to the player on ATTACK contact
    u8  lootItemId;      // ItemID granted on death (see ItemDef.h)

    EnemyAIState state;
    float        attackCooldown;  // seconds remaining until next attack tick

    AnimState anim;   // reused as-is from Milestone 6 — same facing/walk-
                       // frame struct already used by Player and NPC.

    bool active;       // false = this slot is unused, OR this enemy is dead
                        // (no separate "dead" flag — matches the existing
                        // convention of one flag meaning "not meaningfully
                        // present", e.g. InventorySlot.quantity == 0)
};

// Result of EnemyManager::tryAttack(). Mirrors InteractResult/GatherResult's
// existing pattern (a small result enum returned by the one verb this
// system exposes to main.cpp) rather than inventing a different shape.
enum class AttackResult : u8 {
    NONE,    // no enemy was in range, or the attack is on cooldown
    HIT,     // an enemy was damaged
    KILLED,  // an enemy was damaged and died from this hit
};
