//-----------------------------------------------------------------------------
// EnemyManager.cpp  (Milestone 10 — consumes EnemyDatabase + SpawnDatabase)
//-----------------------------------------------------------------------------

#include "EnemyManager.h"
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
bool EnemyManager::moveToward(Enemy& e, float targetX, float targetY,
                              float dt, const TileMap& map) {
    float dx   = targetX - e.pos_x;
    float dy   = targetY - e.pos_y;
    float dist = sqrtf(dx * dx + dy * dy);
    if (dist <= ENEMY_RETURN_THRESHOLD) {
        e.anim.update(0.0f, 0.0f, dt);
        return false;
    }

    float step = ENEMY_SPEED * dt;
    float velX = 0.0f;
    float velY = 0.0f;

    if (fabsf(dx) > ENEMY_RETURN_THRESHOLD) {
        float moveX = (dx > 0.0f ? 1.0f : -1.0f) * std::min(step, fabsf(dx));
        float newX  = e.pos_x + moveX;
        int tx = static_cast<int>(newX + 8) / TILE_SIZE;
        int ty = static_cast<int>(e.pos_y + 8) / TILE_SIZE;
        if (!map.isSolid(tx, ty)) {
            e.pos_x = newX;
            velX = (dx > 0.0f ? 1.0f : -1.0f) * ENEMY_SPEED;
        }
    } else if (fabsf(dy) > ENEMY_RETURN_THRESHOLD) {
        float moveY = (dy > 0.0f ? 1.0f : -1.0f) * std::min(step, fabsf(dy));
        float newY  = e.pos_y + moveY;
        int tx = static_cast<int>(e.pos_x + 8) / TILE_SIZE;
        int ty = static_cast<int>(newY + 8) / TILE_SIZE;
        if (!map.isSolid(tx, ty)) {
            e.pos_y = newY;
            velY = (dy > 0.0f ? 1.0f : -1.0f) * ENEMY_SPEED;
        }
    }

    e.anim.update(velX, velY, dt);
    return true;
}

//-----------------------------------------------------------------------------
// updateAI — Idle -> Chase -> Attack -> Return state machine.
// No pathfinding, no obstacle avoidance, no behavior trees. State
// transitions are pure distance checks against the player's current
// position. Generic over EnemyType — nothing here special-cases the
// Wolf specifically, so a second enemy type needs no changes here.
//-----------------------------------------------------------------------------
void EnemyManager::updateAI(Enemy& e, float playerX, float playerY,
                            float dt, const TileMap& map) {
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
                moveToward(e, playerX, playerY, dt, map);
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
            bool stillMoving = moveToward(e, e.spawn_x, e.spawn_y, dt, map);
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

    // Facing-direction reach: the attack hitbox is a point offset from
    // the player's center in their facing direction, then a closest-
    // enemy-in-range check — same closest-wins pattern as
    // WorldObjectManager::tryInteract / GatherNodeManager::tryHarvest.
    float reachX = playerX;
    float reachY = playerY;
    switch (playerFacing) {
        case Facing::UP:    reachY -= ATTACK_RANGE_PX; break;
        case Facing::DOWN:  reachY += ATTACK_RANGE_PX; break;
        case Facing::LEFT:  reachX -= ATTACK_RANGE_PX; break;
        case Facing::RIGHT: reachX += ATTACK_RANGE_PX; break;
    }

    float closest = ATTACK_RANGE_PX;
    int   bestIdx  = -1;
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy& e = m_enemies[i];
        if (!e.active) continue;
        float dx   = reachX - e.pos_x;
        float dy   = reachY - e.pos_y;
        float dist = sqrtf(dx * dx + dy * dy);
        if (dist <= closest) {
            closest = dist;
            bestIdx = i;
        }
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
    }
    LOG("EnemyManager: all enemies respawned at full HP");
}
