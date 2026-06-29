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

// Milestone 12, Task 7: combat robustness additions — hit
// invulnerability, hit-flash preparation, and a minimal knockback hook.
// All three default to "no effect" (zero timers / zero velocity), so an
// Enemy that's never been hit behaves identically to before this
// milestone; these fields only do anything once EnemyManager::tryAttack
// (or a future caller) sets them.
static constexpr float HIT_INVULN_DURATION_SEC = 0.2f; // matches the
    // existing 0.5s player attack cooldown's intent loosely — short
    // enough that a legitimate fast follow-up hit still lands, long
    // enough to make a single swing register as exactly one hit even
    // if tryAttack() were ever called more than once in a frame (it
    // currently isn't, but this is the kind of invariant worth making
    // explicit and enforced rather than implicit and merely true today
    // — see Movement.h's header comment for the same philosophy
    // applied to knockback below).
static constexpr float HIT_FLASH_DURATION_SEC  = 0.1f; // how long
    // hitFlashTimer stays nonzero after a hit, for a future renderer
    // change to read and tint the sprite — no rendering code reads
    // this field yet (see Renderer::drawEnemies, unchanged this
    // milestone); it exists so the data is ready the moment a flash
    // effect is implemented, without another pass through EnemyManager.
static constexpr float KNOCKBACK_DECAY_PER_SEC = 600.0f; // pixels/sec^2
    // equivalent — how fast knockbackVelX/Y bleeds off toward zero.
    // Not currently applied by anything (no caller sets a nonzero
    // knockback velocity yet — this is the "hook," not a triggered
    // effect), but EnemyManager::updateAI() decays and applies it
    // unconditionally so the field is live infrastructure, not dead
    // weight waiting for a second pass to wire in.

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

    // Milestone 12, Task 7 — see the constants above for what each of
    // these is for. All zero-initialized, all "no effect" until set.
    float hitInvulnTimer;  // seconds remaining where this enemy can't be hit again
    float hitFlashTimer;   // seconds remaining to show a hit-flash (rendering not yet wired)
    float knockbackVelX;   // pixels/second, decays toward 0 via KNOCKBACK_DECAY_PER_SEC
    float knockbackVelY;

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
