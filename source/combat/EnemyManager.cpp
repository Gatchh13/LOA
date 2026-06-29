//-----------------------------------------------------------------------------
// EnemyManager.cpp  (Milestone 10 — consumes EnemyDatabase + SpawnDatabase)
//-----------------------------------------------------------------------------

#include "EnemyManager.h"
#include "../core/Movement.h"
#include "../data/ItemDatabase.h"
#include "../world/SpawnDatabase.h"
#include "../core/Logger.h"
#include <cstring>
#include <cstdio>
#include <cmath>
#include <algorithm>

//=============================================================================
// EnemyManager
//
// Milestone 10 change: defineEnemies() (which hardcoded the one Forest
// Wolf's position AND its type-level stats inline) is gone. init() now
// populates m_enemies generically from SpawnDatabase (where, what type)
// and reads each instance's starting hp from EnemyDatabase (how much
// HP that type has) — the same split GatherNodeManager/WorldObjectManager
// don't yet have, but enemies now do, per the Milestone 10 assignment.
//
// A side effect noticed while migrating, not introduced by it:
// respawnAll() previously hardcoded "slot 0 is always the Wolf" to
// work around enemies not tracking which spawn entry they came from.
// Now that every Enemy instance carries spawn_x/spawn_y AND a `type`
// (read from SpawnEntry at init() time), respawnAll() no longer needs
// that special case — it can just check `active || was-ever-populated`
// generically. This was flagged as a known risk in the Milestone 8
// design doc specifically because it wouldn't generalize past one
// enemy; Milestone 10's spawn-table split is what removes the need
// for the special case, not a deliberate fix attempted on its own.
//=============================================================================

EnemyManager::EnemyManager()
    : m_lastMessage(nullptr)
    , m_messageTimer(0.0f)
    , m_attackCooldownRemaining(0.0f)
{
    memset(m_enemies, 0, sizeof(m_enemies));
    m_messageBuf[0] = '\0';
}

void EnemyManager::init() {
    int spawnCount = getSpawnCount();
    for (int i = 0; i < MAX_ENEMIES; i++) {
        m_enemies[i].active = false;
    }

    for (int i = 0; i < spawnCount && i < MAX_ENEMIES; i++) {
        const SpawnEntry& spawn = getSpawnEntry(i);
        const EnemyDef&   def   = getEnemyDef(spawn.type);

        Enemy& e = m_enemies[i];
        e.id              = static_cast<u8>(i);
        e.type            = spawn.type;
        e.zone            = spawn.zone;
        e.spawn_x         = spawn.spawn_x;
        e.spawn_y         = spawn.spawn_y;
        e.pos_x           = spawn.spawn_x;
        e.pos_y           = spawn.spawn_y;
        e.hp              = def.maxHp;
        e.state           = EnemyAIState::IDLE;
        e.attackCooldown  = 0.0f;
        e.hitInvulnTimer  = 0.0f;
        e.hitFlashTimer   = 0.0f;
        e.knockbackVelX   = 0.0f;
        e.knockbackVelY   = 0.0f;
        e.anim            = AnimState{};
        e.active          = true;
    }

    LOG("EnemyManager: initialized %d enemies from %d spawn entries",
        spawnCount < MAX_ENEMIES ? spawnCount : MAX_ENEMIES, spawnCount);
}

//-----------------------------------------------------------------------------
// moveToward — single-axis-at-a-time step toward a target, with a tile-
// solid check. This is NPCManager::moveNPC's exact pattern (horizontal
// first, then vertical, clamped step, isSolid check) reused verbatim per
// the Milestone 8 assignment's instruction to reuse existing patterns
// rather than invent movement logic. Returns true if the enemy actually
// moved.
//-----------------------------------------------------------------------------
// moveToward() was removed in Milestone 12 — its logic is now the
// shared seekTowardTarget() in source/core/Movement.cpp, called
// directly from updateAI() below with ENEMY_SPEED/ENEMY_RETURN_THRESHOLD
// passed through unchanged.

//-----------------------------------------------------------------------------
// updateAI — Idle -> Chase -> Attack -> Return state machine.
// No pathfinding, no obstacle avoidance, no behavior trees. State
// transitions are pure distance checks against the player's current
// position. Generic over EnemyType — nothing here special-cases the
// Wolf specifically, so a second enemy type needs no changes here.
//-----------------------------------------------------------------------------
void EnemyManager::updateAI(Enemy& e, float playerX, float playerY,
                            float dt, const TileMap& map) {
    // Milestone 12, Task 7: knockback application + decay, and hit-
    // invuln/hit-flash timer countdown. Runs unconditionally, before
    // the state-machine switch below, so it applies regardless of
    // which AI state the enemy is in (a knocked-back enemy mid-CHASE
    // still gets pushed; an enemy that's just been hit is still briefly
    // invulnerable whether it's chasing, attacking, or idling).
    if (e.hitInvulnTimer > 0.0f) {
        e.hitInvulnTimer -= dt;
        if (e.hitInvulnTimer < 0.0f) e.hitInvulnTimer = 0.0f;
    }
    if (e.hitFlashTimer > 0.0f) {
        e.hitFlashTimer -= dt;
        if (e.hitFlashTimer < 0.0f) e.hitFlashTimer = 0.0f;
    }
    if (e.knockbackVelX != 0.0f || e.knockbackVelY != 0.0f) {
        // Apply this frame's displacement directly — knockback is an
        // external push, not something seekTowardTarget() needs to
        // know about (see Movement.h's header comment on why this
        // separation was the design from the start, not an
        // afterthought). No tile-collision check on the knockback
        // displacement itself: with KNOCKBACK_DECAY_PER_SEC this fast
        // and no caller currently setting a nonzero velocity, a real
        // knockback effect's tuning (and whether it should respect
        // walls) is exactly the kind of decision deferred to whichever
        // future milestone actually triggers this hook for the first
        // time — see the Milestone 12 design doc's "Document Future
        // Refactors" section.
        e.pos_x += e.knockbackVelX * dt;
        e.pos_y += e.knockbackVelY * dt;

        float decay = KNOCKBACK_DECAY_PER_SEC * dt;
        if (fabsf(e.knockbackVelX) <= decay) e.knockbackVelX = 0.0f;
        else e.knockbackVelX -= (e.knockbackVelX > 0.0f ? decay : -decay);
        if (fabsf(e.knockbackVelY) <= decay) e.knockbackVelY = 0.0f;
        else e.knockbackVelY -= (e.knockbackVelY > 0.0f ? decay : -decay);
    }

    float dx   = playerX - e.pos_x;
    float dy   = playerY - e.pos_y;
    float distToPlayer = sqrtf(dx * dx + dy * dy);

    switch (e.state) {
        case EnemyAIState::IDLE:
            e.anim.update(0.0f, 0.0f, dt);
            if (distToPlayer <= ENEMY_DETECT_RANGE_PX) {
                e.state = EnemyAIState::CHASE;
            }
            break;

        case EnemyAIState::CHASE:
            if (distToPlayer <= ENEMY_ATTACK_RANGE_PX) {
                e.state = EnemyAIState::ATTACK;
                e.attackCooldown = 0.0f; // attack immediately on entering range
            } else if (distToPlayer > ENEMY_DETECT_RANGE_PX) {
                e.state = EnemyAIState::RETURN;
            } else {
                seekTowardTarget(e.pos_x, e.pos_y, playerX, playerY,
                                 ENEMY_SPEED, ENEMY_RETURN_THRESHOLD, dt, map, e.anim);
            }
            break;

        case EnemyAIState::ATTACK:
            e.anim.update(0.0f, 0.0f, dt); // stand still while attacking — no
                                            // combined chase+attack, per scope
            if (distToPlayer > ENEMY_ATTACK_RANGE_PX) {
                // Player stepped out of bite range — resume chasing if
                // still detected, otherwise head home.
                e.state = (distToPlayer <= ENEMY_DETECT_RANGE_PX)
                         ? EnemyAIState::CHASE
                         : EnemyAIState::RETURN;
            }
            // Contact damage itself is applied in update(), not here —
            // see the header comment on update() for why that's a
            // separate pass rather than folded into this switch.
            break;

        case EnemyAIState::RETURN: {
            bool stillMoving = seekTowardTarget(e.pos_x, e.pos_y, e.spawn_x, e.spawn_y,
                                                ENEMY_SPEED, ENEMY_RETURN_THRESHOLD, dt, map, e.anim);
            if (distToPlayer <= ENEMY_DETECT_RANGE_PX) {
                // Player re-entered range while heading home — chase again.
                e.state = EnemyAIState::CHASE;
            } else if (!stillMoving) {
                e.state = EnemyAIState::IDLE;
            }
            break;
        }
    }
}

void EnemyManager::update(ZoneID currentZone, float playerX, float playerY,
                          float dt, const TileMap& map, PlayerState& playerState,
                          bool playerInvulnerable) {
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy& e = m_enemies[i];
        if (!e.active)             continue;
        if (e.zone != currentZone) continue;

        updateAI(e, playerX, playerY, dt, map);

        // Contact damage: only while in ATTACK state, on its own
        // interval (ENEMY_ATTACK_INTERVAL), independent of the player's
        // own attack cooldown (a wolf biting and the player swinging are
        // unrelated timers — conflating them would make the AI's pacing
        // depend on player input timing, which is the kind of coupling
        // this project's "loud, explicit invariants" philosophy argues
        // against, e.g. QuestManager::startQuest's single-active-quest
        // guard).
        if (e.state == EnemyAIState::ATTACK) {
            e.attackCooldown -= dt;
            if (e.attackCooldown <= 0.0f) {
                e.attackCooldown = ENEMY_ATTACK_INTERVAL;

                // Milestone 11: skip the damage itself while the player
                // can't respond (a menu is open). The cooldown above
                // still resets normally — closing the menu doesn't
                // grant a free instant bite the moment contact damage
                // resumes, it just means the wolf's next *real* tick is
                // a full ENEMY_ATTACK_INTERVAL away, exactly as if this
                // tick had landed normally.
                if (!playerInvulnerable) {
                    const EnemyDef& def = getEnemyDef(e.type);

                    // Milestone 9 combat formula: enemy_attack - armor_bonus,
                    // minimum 1. Computed in int to avoid u16 underflow if
                    // defense ever exceeded attack (clamped explicitly below
                    // rather than relying on unsigned wraparound never
                    // happening to line up correctly).
                    int rawDamage = static_cast<int>(def.contactDamage)
                                  - static_cast<int>(playerState.getDefense());
                    u16 dmg = static_cast<u16>(rawDamage < 1 ? 1 : rawDamage);

                    playerState.damage(dmg);
                    LOG("Enemy %d (%s) bit the player for %u damage (atk=%u def=%u). Player HP: %u/%u",
                        e.id, def.name, dmg, def.contactDamage, playerState.getDefense(),
                        playerState.hp, playerState.maxHp);
                }
            }
        }
    }
}

AttackResult EnemyManager::tryAttack(float playerX, float playerY, Facing playerFacing,
                                     bool xPressed, float dt, PlayerState& playerState) {
    if (m_attackCooldownRemaining > 0.0f) {
        m_attackCooldownRemaining -= dt;
        if (m_attackCooldownRemaining < 0.0f) m_attackCooldownRemaining = 0.0f;
    }

    if (!xPressed) return AttackResult::NONE;
    if (m_attackCooldownRemaining > 0.0f) return AttackResult::NONE;

    // Milestone 12, Task 7: directional attack cone, replacing the
    // previous point-offset+radius check. An enemy must be within
    // ATTACK_RANGE_PX of the player's actual center AND within
    // ATTACK_CONE_HALF_ANGLE_DEG of the player's facing direction.
    // Same closest-wins pattern as before (WorldObjectManager::
    // tryInteract / GatherNodeManager::tryHarvest) for picking among
    // multiple candidates in range.
    float facingDirX = 0.0f, facingDirY = 0.0f;
    switch (playerFacing) {
        case Facing::UP:    facingDirX =  0.0f; facingDirY = -1.0f; break;
        case Facing::DOWN:  facingDirX =  0.0f; facingDirY =  1.0f; break;
        case Facing::LEFT:  facingDirX = -1.0f; facingDirY =  0.0f; break;
        case Facing::RIGHT: facingDirX =  1.0f; facingDirY =  0.0f; break;
    }
    float coneHalfAngleRad = ATTACK_CONE_HALF_ANGLE_DEG * (3.14159265f / 180.0f);
    float cosConeHalfAngle = cosf(coneHalfAngleRad);

    float closest = ATTACK_RANGE_PX;
    int   bestIdx  = -1;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy& e = m_enemies[i];
        if (!e.active)            continue;
        if (e.hitInvulnTimer > 0.0f) continue; // Task 7: can't be hit again yet

        float dx   = e.pos_x - playerX;
        float dy   = e.pos_y - playerY;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist > closest) continue;
        if (dist < 0.001f) {
            // Enemy is essentially on top of the player — no direction
            // to compare against the facing cone, treat as in-cone
            // rather than dividing by ~zero.
        } else {
            float dirX = dx / dist;
            float dirY = dy / dist;
            float dot  = dirX * facingDirX + dirY * facingDirY; // cos(angle between)
            if (dot < cosConeHalfAngle) continue; // outside the cone
        }

        closest = dist;
        bestIdx = i;
    }

    m_attackCooldownRemaining = ATTACK_COOLDOWN_SEC;

    if (bestIdx < 0) return AttackResult::NONE;

    Enemy& e = m_enemies[bestIdx];
    const EnemyDef& def = getEnemyDef(e.type);

    // Milestone 9 combat formula: base_attack + weapon_bonus, via
    // PlayerState::getAttack() (see PlayerState.h). No minimum-1 clamp
    // needed here — unlike enemy attack minus armor, there's no
    // defense-side subtraction on the player's outgoing damage, so it
    // can never go below PLAYER_BASE_ATTACK.
    u16 dmg = playerState.getAttack();
    e.hp = (dmg >= e.hp) ? 0 : static_cast<u16>(e.hp - dmg);

    // Milestone 12, Task 7: start the hit-invuln and hit-flash windows.
    // Set unconditionally on any successful hit (including a killing
    // blow — the enemy deactivates immediately below regardless, so
    // these timers are simply irrelevant for a dead enemy rather than
    // needing a special case to skip them).
    e.hitInvulnTimer = HIT_INVULN_DURATION_SEC;
    e.hitFlashTimer  = HIT_FLASH_DURATION_SEC;

    LOG("Player hit enemy %d (%s) for %u damage (atk=%u). Enemy HP: %u/%u",
        e.id, def.name, dmg, playerState.getAttack(), e.hp, def.maxHp);

    if (e.hp == 0) {
        // Death: no corpse, no death animation — deactivate immediately.
        e.active = false;

        bool added = playerState.inventory.addItem(def.lootItemId, 1);
        const ItemDef& lootDef = getItemDef(def.lootItemId);
        if (added) {
            snprintf(m_messageBuf, sizeof(m_messageBuf), "%s defeated! +1 %s", def.name, lootDef.name);
        } else {
            // Inventory full — same "don't block the game state change,
            // just lose the item and say so" choice as
            // QuestManager::applyReward's full-inventory case.
            snprintf(m_messageBuf, sizeof(m_messageBuf), "%s defeated! (inventory full)", def.name);
            WARN("Enemy %d loot lost — inventory full (item_id=%u)", e.id, def.lootItemId);
        }
        m_lastMessage  = m_messageBuf;
        m_messageTimer = MESSAGE_DURATION;

        LOG("Enemy %d (%s) defeated. Loot: %s (added=%d)", e.id, def.name, lootDef.name, added);
        return AttackResult::KILLED;
    }

    m_lastMessage  = "Hit!";
    m_messageTimer = MESSAGE_DURATION;
    return AttackResult::HIT;
}

void EnemyManager::updateMessageTimer(float dt) {
    if (m_messageTimer > 0.0f) {
        m_messageTimer -= dt;
        if (m_messageTimer < 0.0f) m_messageTimer = 0.0f;
    }
}

void EnemyManager::respawnAll() {
    // Milestone 10: no longer hardcodes "slot 0 is the Wolf" — every
    // slot that init() populated from SpawnDatabase (spawn_x/spawn_y
    // are only ever set by init(), never zeroed except by the
    // constructor's memset) gets revived, regardless of which index it
    // is or whether it died. A genuinely unused slot has spawn_x ==
    // spawn_y == 0 AND was never set active by init(), so checking
    // spawn position alone isn't quite enough on its own — but combined
    // with the fact that init() always sets every spawned slot's `type`
    // field too, and a never-populated slot's `type` defaults to
    // EnemyType::FOREST_WOLF (0) from the constructor's memset, the
    // cleanest unambiguous signal is whether this slot index is within
    // the spawn table's actual count, which init() already established.
    int spawnCount = getSpawnCount();
    for (int i = 0; i < spawnCount && i < MAX_ENEMIES; i++) {
        Enemy& e = m_enemies[i];
        const EnemyDef& def = getEnemyDef(e.type);
        e.active = true;
        e.hp     = def.maxHp;
        e.pos_x  = e.spawn_x;
        e.pos_y  = e.spawn_y;
        e.state  = EnemyAIState::IDLE;
        e.attackCooldown = 0.0f;
        e.hitInvulnTimer = 0.0f;
        e.hitFlashTimer  = 0.0f;
        e.knockbackVelX  = 0.0f;
        e.knockbackVelY  = 0.0f;
    }
    LOG("EnemyManager: all enemies respawned at full HP");
}
