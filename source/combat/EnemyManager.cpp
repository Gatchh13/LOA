//-----------------------------------------------------------------------------
// EnemyManager.cpp
//-----------------------------------------------------------------------------

#include "EnemyManager.h"
#include "../items/ItemDef.h"
#include "../core/Logger.h"
#include <cstring>
#include <cstdio>
#include <cmath>
#include <algorithm>

//=============================================================================
// ENEMY DEFINITIONS
//
// One Forest Wolf, per the Milestone 8 assignment's explicit "one enemy
// type only" scope rule. Placed at tile (6,19) in Forest — checked
// against s_forestTiles in ZoneData.h to land on plain TILE_FOREST_FLOOR
// (2), clear of all four GatherNodeManager nodes ((5,5), (24,6), (8,16),
// (20,18)), the rope-ladder WorldObject (0,3)-(0,5), the fallen-tree
// WorldObject (22,11)-(22,12), the Forest spawn points, and the north-
// south dirt path (columns 13-14). Far enough from the south entrance
// (player typically arrives near row 21) that the wolf isn't an instant
// ambush on zone entry, but well within a short walk.
//=============================================================================

static void defineEnemies(Enemy* enemies) {
    enemies[0].id            = 0;
    enemies[0].zone          = ZoneID::FOREST;
    enemies[0].spawn_x       = 6 * TILE_SIZE + 8.0f;   // 104
    enemies[0].spawn_y       = 19 * TILE_SIZE + 8.0f;  // 312
    enemies[0].pos_x         = enemies[0].spawn_x;
    enemies[0].pos_y         = enemies[0].spawn_y;
    enemies[0].maxHp         = 40;
    enemies[0].hp            = enemies[0].maxHp;
    enemies[0].contactDamage = 10;
    enemies[0].lootItemId    = static_cast<u8>(ItemID::WOLF_PELT);
    enemies[0].state         = EnemyAIState::IDLE;
    enemies[0].attackCooldown = 0.0f;
    enemies[0].anim           = AnimState{};
    enemies[0].active         = true;

    // Remaining MAX_ENEMIES-1 slots stay inactive — headroom for a
    // future second enemy instance/type, not used in Milestone 8.
    for (int i = 1; i < MAX_ENEMIES; i++) {
        enemies[i].active = false;
    }
}

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
    defineEnemies(m_enemies);
    LOG("EnemyManager: initialized 1 enemy (Forest Wolf)");
}

//-----------------------------------------------------------------------------
// moveToward — single-axis-at-a-time step toward a target, with a tile-
// solid check. This is NPCManager::moveNPC's exact pattern (horizontal
// first, then vertical, clamped step, isSolid check) reused verbatim per
// the assignment's instruction to reuse existing patterns rather than
// invent movement logic. Returns true if the enemy actually moved.
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
// No pathfinding, no obstacle avoidance, no behavior trees — exactly as
// scoped. State transitions are pure distance checks against the
// player's current position.
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
                          float dt, const TileMap& map, PlayerState& playerState) {
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
                playerState.damage(e.contactDamage);
                LOG("Enemy %d bit the player for %u damage. Player HP: %u/%u",
                    e.id, e.contactDamage, playerState.hp, playerState.maxHp);
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
    u16 dmg = PLAYER_ATTACK_DAMAGE;
    e.hp = (dmg >= e.hp) ? 0 : static_cast<u16>(e.hp - dmg);

    LOG("Player hit enemy %d for %u damage. Enemy HP: %u/%u",
        e.id, dmg, e.hp, e.maxHp);

    if (e.hp == 0) {
        // Death: no corpse, no death animation — deactivate immediately.
        e.active = false;

        bool added = playerState.inventory.addItem(e.lootItemId, 1);
        const ItemDef& lootDef = getItemDef(e.lootItemId);
        if (added) {
            snprintf(m_messageBuf, sizeof(m_messageBuf), "Wolf defeated! +1 %s", lootDef.name);
        } else {
            // Inventory full — same "don't block the game state change,
            // just lose the item and say so" choice as
            // QuestManager::applyReward's full-inventory case.
            snprintf(m_messageBuf, sizeof(m_messageBuf), "Wolf defeated! (inventory full)");
            WARN("Enemy %d loot lost — inventory full (item_id=%u)", e.id, e.lootItemId);
        }
        m_lastMessage  = m_messageBuf;
        m_messageTimer = MESSAGE_DURATION;

        LOG("Enemy %d defeated. Loot: %s (added=%d)", e.id, lootDef.name, added);
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
    for (int i = 0; i < MAX_ENEMIES; i++) {
        Enemy& e = m_enemies[i];
        if (i == 0) {
            // Slot 0 is always the Wolf in Milestone 8 — re-activate it
            // even though death set active=false (every other field is
            // still intact; death only flips the one flag).
            e.active = true;
        }
        if (!e.active) continue;
        e.hp     = e.maxHp;
        e.pos_x  = e.spawn_x;
        e.pos_y  = e.spawn_y;
        e.state  = EnemyAIState::IDLE;
        e.attackCooldown = 0.0f;
    }
    LOG("EnemyManager: all enemies respawned at full HP");
}
