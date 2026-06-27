#pragma once

//-----------------------------------------------------------------------------
// EnemyManager.h  (Milestone 8 — Combat Foundation)
//
// Owns all Enemy instances. Handles the Idle/Chase/Attack/Return state
// machine, player melee attacks against enemies, and contact damage from
// enemies against the player.
//
// Lifecycle (mirrors GatherNodeManager's, per the assignment's explicit
// instruction to reuse existing patterns):
//   1. init()           — populate enemies (once, at game start)
//   2. update()          — run AI + contact damage every frame with real dt
//   3. tryAttack()        — called when X is pressed, returns AttackResult
//   4. getEnemies()       — used by Renderer for placeholder visuals
//   5. respawnAll()       — called on player death (see main.cpp); resets
//                           every enemy to full HP at its spawn position
//
// Not save-persisted in Milestone 8: enemy HP/position/AI state reset on
// load, exactly like GatherNodeManager's cooldowns reset on load (see
// GatherNodeManager.h's header comment for the same reasoning). A dead
// wolf does not stay dead across a save/load cycle — this is a
// deliberate scope cut, not an oversight. See the Milestone 8 design
// doc's save-impact section for why persisting enemy state was judged
// not worth the complexity for one enemy instance.
//-----------------------------------------------------------------------------

#include "../../include/types.h"
#include "Enemy.h"
#include "../quest/PlayerState.h"
#include "../world/TileMap.h"

// Player-facing combat tuning constants. Kept here (not in Enemy.h)
// since they describe the player's attack, not enemy data.
static constexpr float ATTACK_RANGE_PX     = 20.0f;  // melee reach from player center
static constexpr float ATTACK_COOLDOWN_SEC = 0.5f;   // time between player attacks
// Milestone 9: player damage is now PlayerState::getAttack() (base +
// weapon bonus), not a flat constant — see EnemyManager.cpp::tryAttack().
// The old PLAYER_ATTACK_DAMAGE constant is gone rather than left unused
// alongside the new formula, which would invite using the stale one by
// accident.

// Enemy AI tuning constants.
static constexpr float ENEMY_DETECT_RANGE_PX = 64.0f;  // Idle -> Chase threshold
static constexpr float ENEMY_ATTACK_RANGE_PX = 14.0f;  // Chase -> Attack threshold
static constexpr float ENEMY_SPEED           = 40.0f;  // pixels per second, chase/return
static constexpr float ENEMY_ATTACK_INTERVAL = 1.0f;   // seconds between contact-damage ticks
static constexpr float ENEMY_RETURN_THRESHOLD = 2.0f;  // "close enough to spawn" — same
                                                         // role as NPC's ARRIVAL_THRESHOLD

class EnemyManager {
public:
    EnemyManager();

    // Populate enemies. Call once at game start (mirrors
    // GatherNodeManager::init() / WorldObjectManager::init()).
    void init();

    // Run AI for every active enemy in the current zone, and apply
    // contact damage to the player if any enemy is in its Attack state
    // and in range. Call once per frame with real dt.
    // playerX/Y: player center in pixels. map: for movement collision
    // (Chase/Return use the same tile-solid check as NPCManager::moveNPC).
    // playerInvulnerable (Milestone 11): true while the player can't
    // respond — dialogue/shop/inventory screen open. The AI state
    // machine and the bite-interval cooldown still tick normally (so
    // nothing desyncs and the wolf doesn't get a free instant bite the
    // moment the player closes the menu), but the damage itself is
    // suppressed. Fixes a real fairness bug: previously contact damage
    // applied unconditionally, so a player standing near an attacking
    // enemy while talking to a different NPC or browsing the shop took
    // free damage — and could die — with no way to fight back, heal,
    // or even see it coming until the menu closed.
    void update(ZoneID currentZone, float playerX, float playerY,
               float dt, const TileMap& map, PlayerState& playerState,
               bool playerInvulnerable);

    // Player melee attack. Call when X is pressed. Damages the closest
    // enemy within ATTACK_RANGE_PX of the player's facing direction, on
    // a cooldown. Grants loot directly to playerState's inventory if the
    // hit kills the enemy (see Enemy::lootItemId) — same "the function
    // that causes the state change also grants the consequence" pattern
    // as WorldObjectManager::repair() consuming resources inline.
    AttackResult tryAttack(float playerX, float playerY, Facing playerFacing,
                           bool xPressed, float dt, PlayerState& playerState);

    // The message from the last tryAttack/contact-damage event (for HUD
    // display) — same message+timer pattern as WorldObjectManager/
    // GatherNodeManager.
    const char* getLastMessage() const { return m_lastMessage; }
    bool        hasMessage() const { return m_messageTimer > 0.0f; }
    void        updateMessageTimer(float dt);

    // Reset every enemy to full HP at its spawn position and IDLE state.
    // Called by main.cpp on player death/respawn (Feature 8) — a dead
    // wolf should not stay dead forever just because the player died
    // once, and re-fighting a half-dead wolf after respawning would be
    // a strange, undertuned edge case not worth supporting instead.
    void respawnAll();

    // Access enemies for rendering.
    const Enemy* getEnemies()     const { return m_enemies; }
    int          getEnemyCount()  const { return MAX_ENEMIES; }

    static constexpr float MESSAGE_DURATION = 2.0f;

private:
    Enemy       m_enemies[MAX_ENEMIES];
    const char* m_lastMessage;
    float       m_messageTimer;
    float       m_attackCooldownRemaining;  // player's own attack cooldown

    char m_messageBuf[40];

    // AI helpers — internal, mirror NPCManager::moveNPC's step pattern.
    void updateAI(Enemy& e, float playerX, float playerY, float dt, const TileMap& map);
    bool moveToward(Enemy& e, float targetX, float targetY, float dt, const TileMap& map);
};
