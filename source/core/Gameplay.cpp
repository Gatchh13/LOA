//-----------------------------------------------------------------------------
// Gameplay.cpp  (Milestone 12 — Engine Cleanup & Architecture Consolidation)
//
// Every line of logic here is moved verbatim from main.cpp's
// startGame lambda and the gameplay portion of the main loop — only
// the owning scope changed (local variable -> member field, e.g.
// `playerState` -> `m_playerState`). No constant, branch, or comment
// was altered beyond updating identifiers to match.
//-----------------------------------------------------------------------------

#include "Gameplay.h"
#include "../core/Logger.h"
#include "../core/GameState.h"
#include "../save/SaveManager.h"

Gameplay::Gameplay()
    : m_player(0.0f, 0.0f)
    , m_worldClock(8 * 60)   // default start: 08:00
{
}

void Gameplay::startGame(bool loadExisting) {
    bool loadedSave = false;

    // Pre-initialize zones so the TileMap exists before loadGame writes to it.
    m_zones.loadZone(ZoneID::TOWN, 0);
    m_worldObjects.init(m_zones.getTileMap());
    m_gatherNodes.init();
    m_enemies.init();
    m_questMgr.init();

    // Determine start position
    const ZoneDef&    startDef = getZoneDef(ZoneID::TOWN);
    const SpawnPoint& startSp  = startDef.spawns[0];
    m_player.setPosition(startSp.spawn_px, startSp.spawn_py);

    if (loadExisting && SaveManager::hasSave()) {
        loadedSave = SaveManager::loadGame(
            m_player, m_zones, m_worldClock, m_playerState,
            m_questMgr, m_worldObjects, m_zones.getTileMap());
        if (loadedSave) {
            // SaveManager::apply() already applied world object overrides
            // for the loaded zone (load zone -> restore states -> onZoneLoaded).
            m_npcs.init(m_worldClock.getTotalMinutes());
            LOG("Save loaded successfully");
        } else {
            WARN("Save validation failed — starting new game");
        }
    }

    if (!loadedSave) {
        // Fresh new game
        m_playerState.init();
        m_npcs.init(m_worldClock.getTotalMinutes());
        LOG("New game started");
    }

    m_camera.update(m_player.getCenterX(), m_player.getCenterY(),
                  m_zones.getTileMap().getWidthPixels(),
                  m_zones.getTileMap().getHeightPixels());

    m_dayNight.update(m_worldClock.getTimeAsFloat());
}

void Gameplay::update(const InputManager& input, float dt) {
    //----------------------------------------------------------------------
    // Gameplay input
    //----------------------------------------------------------------------
    bool aPressed      = input.isPressed(KEY_A);
    bool bPressed      = input.isPressed(KEY_B);
    bool yPressed      = input.isPressed(KEY_Y);
    bool xPressed      = input.isPressed(KEY_X);
    bool lPressed      = input.isPressed(KEY_L);
    bool rPressed      = input.isPressed(KEY_R);
    bool selectPressed = input.isPressed(KEY_SELECT);
    bool startPressed  = input.isPressed(KEY_START);

    //----------------------------------------------------------------------
    // Save (SELECT). START opens the inventory screen — see below,
    // after shop/dialogue state is known, since opening it is itself
    // a state change that should be checked alongside those.
    //
    // Milestone 8 had an in-gameplay "press START to reload your
    // last save without quitting" feature here. It's removed in
    // Milestone 9: every other button is now claimed (X=attack,
    // L/R=consumables, Y=shop-trigger, A=interact/buy, B=close/
    // leave), and START is the assignment's explicit choice for
    // opening the inventory screen. Continue from the title screen
    // remains the only load path — a deliberate scope decision, not
    // an oversight (see the Milestone 9 design doc's risk section).
    //----------------------------------------------------------------------
    if (selectPressed) {
        bool ok = SaveManager::saveGame(
            m_player, m_zones, m_worldClock,
            m_playerState, m_questMgr, m_worldObjects);
        m_statusMessage = ok ? "Game Saved." : "Save Failed!";
        m_statusTimer   = STATUS_DURATION;
        LOG("Save triggered: %s", m_statusMessage);
    }

    //----------------------------------------------------------------------
    // Update
    //----------------------------------------------------------------------
    m_statusTimer      = m_statusTimer > 0.0f ? m_statusTimer - dt : 0.0f;
    m_shopMessageTimer = m_shopMessageTimer > 0.0f ? m_shopMessageTimer - dt : 0.0f;
    m_invMessageTimer  = m_invMessageTimer > 0.0f ? m_invMessageTimer - dt : 0.0f;

    m_worldClock.update(dt);
    m_dayNight.update(m_worldClock.getTimeAsFloat());
    m_worldObjects.updateMessageTimer(dt);
    m_gatherNodes.update(dt);
    m_gatherNodes.updateMessageTimer(dt);

    ZoneID currentZone = m_zones.getCurrentZoneDef().id;

    // Open the shop: Y while talking to Mira specifically. Checked
    // before the B-close-dialogue handling below so the same frame
    // that opens the shop doesn't also immediately process a B press
    // meant for something else.
    if (yPressed && !m_shop.isOpen() && m_npcs.isDialogueOpen() &&
        m_npcs.isTalkingToMerchant())
    {
        m_npcs.closeDialogue();
        m_shop.open();
        m_shopMessage      = nullptr;
        m_shopMessageTimer = 0.0f;
        LOG("Shop opened");
    }

    if (m_shop.isOpen()) {
        // Shop has exclusive input while open: no movement, no
        // dialogue, no world object/gather/NPC interaction this
        // frame. Mirrors how dialogue already suspends movement via
        // `canMove` below, just for the shop's own input set.
        if (input.isPressed(KEY_DOWN)) m_shop.moveCursor(+1);
        if (input.isPressed(KEY_UP))   m_shop.moveCursor(-1);

        if (aPressed) {
            ShopResult result = m_shop.tryBuy(m_playerState);
            switch (result) {
                case ShopResult::PURCHASED:
                    m_shopMessage = "Purchased!";
                    break;
                case ShopResult::NOT_ENOUGH_GOLD:
                    m_shopMessage = "Not enough gold.";
                    break;
                case ShopResult::INVENTORY_FULL:
                    m_shopMessage = "Inventory full.";
                    break;
                case ShopResult::NOT_BUYABLE:
                    m_shopMessage = "Not for sale.";
                    break;
                case ShopResult::NONE:
                case ShopResult::SOLD:
                case ShopResult::NOT_SELLABLE:
                case ShopResult::NOTHING_TO_SELL:
                default:
                    m_shopMessage = nullptr;
                    break;
            }
            m_shopMessageTimer = SHOP_MESSAGE_DURATION;
        }

        // Sell (Milestone 9, Feature 1). X is the player's attack
        // button during normal gameplay, but attack input is
        // already gated off while the shop is open (see
        // `interactionSuspended` below), so there's no conflict
        // reusing it here for sell, exactly as the assignment
        // prefers ("A = Buy, X = Sell while inside the shop").
        if (xPressed) {
            ShopResult result = m_shop.trySell(m_playerState);
            switch (result) {
                case ShopResult::SOLD:
                    m_shopMessage = "Sold!";
                    break;
                case ShopResult::NOTHING_TO_SELL:
                    m_shopMessage = "You don't have one.";
                    break;
                case ShopResult::NOT_SELLABLE:
                    m_shopMessage = "Mira won't buy that.";
                    break;
                case ShopResult::NONE:
                case ShopResult::PURCHASED:
                case ShopResult::NOT_ENOUGH_GOLD:
                case ShopResult::INVENTORY_FULL:
                case ShopResult::NOT_BUYABLE:
                default:
                    m_shopMessage = nullptr;
                    break;
            }
            m_shopMessageTimer = SHOP_MESSAGE_DURATION;
        }

        if (bPressed) {
            m_shop.close();
            LOG("Shop closed");
        }
    }

    // Open the inventory/equip screen: START, from normal gameplay
    // (Milestone 9, Feature 4). Suspended while talking, shopping,
    // or already in the inventory screen — the same "one panel at a
    // time" rule the shop already follows relative to dialogue.
    if (startPressed && !m_invScreen.isOpen() && !m_shop.isOpen() && !m_npcs.isDialogueOpen()) {
        m_invScreen.open();
        m_invMessage      = nullptr;
        m_invMessageTimer = 0.0f;
        LOG("Inventory screen opened");
    }

    if (m_invScreen.isOpen()) {
        if (input.isPressed(KEY_DOWN)) m_invScreen.moveCursor(+1);
        if (input.isPressed(KEY_UP))   m_invScreen.moveCursor(-1);

        if (aPressed) {
            InventoryAction result = m_invScreen.tryActivate(m_playerState);
            switch (result) {
                case InventoryAction::EQUIPPED:
                    m_invMessage = "Equipped.";
                    break;
                case InventoryAction::UNEQUIPPED:
                    m_invMessage = "Unequipped.";
                    break;
                case InventoryAction::NONE:
                default:
                    m_invMessage = nullptr;
                    break;
            }
            m_invMessageTimer = INV_MESSAGE_DURATION;
        }

        if (bPressed) {
            m_invScreen.close();
            LOG("Inventory screen closed");
        }
    }

    if (bPressed && m_npcs.isDialogueOpen()) {
        m_npcs.closeDialogue();
    }

    bool canMove = (m_zones.getFadeAlpha() < 1.0f) && !m_npcs.isDialogueOpen()
                  && !m_shop.isOpen() && !m_invScreen.isOpen();
    if (canMove) {
        Vec2 axis = input.getMovementAxis();
        m_player.update(axis, dt, m_zones.getTileMap());
    }

    // A-button priority: dialogue close → world object → gather node → NPC
    // Skipped entirely while the shop or inventory screen is open —
    // their A press was already consumed by shop.tryBuy() or
    // invScreen.tryActivate() above.
    bool aConsumed = m_shop.isOpen() || m_invScreen.isOpen();

    if (!aConsumed && m_npcs.isDialogueOpen() && aPressed) {
        m_npcs.tryInteract(m_player.getCenterX(), m_player.getCenterY(),
                         aPressed, m_questMgr, m_playerState);
        aConsumed = true;
    }

    if (!aConsumed) {
        InteractResult result = m_worldObjects.tryInteract(
            m_player.getCenterX(), m_player.getCenterY(),
            aPressed, m_zones.getTileMap(), m_playerState, &m_questMgr);
        if (result != InteractResult::NONE) aConsumed = true;
    }

    if (!aConsumed) {
        GatherResult gatherResult = m_gatherNodes.tryHarvest(
            m_player.getCenterX(), m_player.getCenterY(),
            currentZone, aPressed, m_playerState);
        if (gatherResult != GatherResult::NONE) aConsumed = true;
    }

    if (!aConsumed) {
        m_npcs.tryInteract(m_player.getCenterX(), m_player.getCenterY(),
                         aPressed, m_questMgr, m_playerState);
    }

    //----------------------------------------------------------------------
    // Combat (Milestone 8) — suspended while talking or shopping,
    // exactly like movement (`canMove` above already excludes both).
    //----------------------------------------------------------------------
    m_combatMessageTimer = m_combatMessageTimer > 0.0f ? m_combatMessageTimer - dt : 0.0f;

    bool interactionSuspended = m_npcs.isDialogueOpen() || m_shop.isOpen() || m_invScreen.isOpen();

    // Enemy AI/contact-damage always ticks — an enemy mid-bite
    // shouldn't freeze just because a menu opened elsewhere. This is
    // harmless even while suspended: the player's position is also
    // frozen via `canMove` above, so a suspended enemy's distance to
    // the player isn't changing either; this just keeps its animation
    // timer and any in-progress bite-interval honest.
    m_enemies.update(currentZone, m_player.getCenterX(), m_player.getCenterY(),
                  dt, m_zones.getTileMap(), m_playerState, interactionSuspended);
    m_enemies.updateMessageTimer(dt);

    if (!interactionSuspended) {
        AttackResult atkResult = m_enemies.tryAttack(
            m_player.getCenterX(), m_player.getCenterY(),
            m_player.getFacing(), xPressed, dt, m_playerState);
        if (atkResult == AttackResult::KILLED) {
            m_combatMessage      = m_enemies.getLastMessage();
            m_combatMessageTimer = COMBAT_MESSAGE_DURATION;
        }

        // Consumables: L = Healing Herb, R = Simple Potion. Dedicated
        // keys, no menu screen at all — the simplest implementation
        // that satisfies "no inventory menus if avoidable" literally.
        if (lPressed) {
            bool used = m_playerState.useConsumable(static_cast<u8>(ItemID::HEALING_HERB));
            m_combatMessage      = used ? "Used Healing Herb. +20 HP" : "No Healing Herb.";
            m_combatMessageTimer = COMBAT_MESSAGE_DURATION;
        }
        if (rPressed) {
            bool used = m_playerState.useConsumable(static_cast<u8>(ItemID::SIMPLE_POTION));
            m_combatMessage      = used ? "Used Simple Potion. +50 HP" : "No Simple Potion.";
            m_combatMessageTimer = COMBAT_MESSAGE_DURATION;
        }
    }

    // Death/respawn (Feature 8). Checked once per frame, after all
    // of this frame's damage sources (enemy contact damage above)
    // have been applied — so a killing blow and the respawn it
    // triggers always happen within the same frame, never delayed.
    if (m_playerState.isDead()) {
        const ZoneDef&    townDef = getZoneDef(ZoneID::TOWN);
        const SpawnPoint& townSp  = townDef.spawns[0];

        // Milestone 11: close any open menu before respawning.
        // Previously, dying while talking to an NPC or browsing the
        // shop/inventory left that screen stuck open after the
        // respawn teleport — the player would land in Town still
        // staring at a dialogue box or shop list from wherever they
        // died. Combined with the contact-damage-suppression fix
        // above, this should rarely trigger from enemy damage alone
        // now, but it's the correct standalone behavior regardless
        // of what caused death.
        if (m_npcs.isDialogueOpen()) m_npcs.closeDialogue();
        if (m_shop.isOpen())         m_shop.close();
        if (m_invScreen.isOpen())    m_invScreen.close();

        if (currentZone != ZoneID::TOWN) {
            m_zones.forceLoadZone(ZoneID::TOWN, 0);
            m_worldObjects.onZoneLoaded(ZoneID::TOWN, m_zones.getTileMap());
            currentZone = ZoneID::TOWN;
        }
        m_player.setPosition(townSp.spawn_px, townSp.spawn_py);
        m_playerState.hp = m_playerState.maxHp;

        // Small gold loss on death, per the assignment's optional
        // suggestion — discourages treating death as a non-event
        // without it being punishing enough to need a real penalty
        // system (no equipment durability, no item loss — just a
        // flat, small, clamped-at-0 gold cost).
        constexpr u32 DEATH_GOLD_LOSS = 5;
        m_playerState.gold = (m_playerState.gold > DEATH_GOLD_LOSS)
                          ? m_playerState.gold - DEATH_GOLD_LOSS : 0;

        m_enemies.respawnAll();

        m_combatMessage      = "You were defeated. Returned to Town.";
        m_combatMessageTimer = COMBAT_MESSAGE_DURATION;
        LOG("Player died — respawned at Town, lost %u gold, all enemies respawned",
            DEATH_GOLD_LOSS);
    }

    m_questMgr.update(currentZone, m_player.getCenterX(), m_player.getCenterY());

    m_npcs.update(currentZone,
                m_worldClock.getTotalMinutes(),
                m_worldClock.hourJustChanged(),
                m_zones.getTileMap(),
                dt);

    if (!m_npcs.isDialogueOpen()) {
        m_zones.update(m_player.getCenterTileX(), m_player.getCenterTileY(), dt);
    }

    if (m_zones.transitionReady()) {
        const SpawnPoint& sp = m_zones.getPendingSpawn();
        m_player.setPosition(sp.spawn_px, sp.spawn_py);
        m_zones.commitTransition();
        m_worldObjects.onZoneLoaded(m_zones.getCurrentZoneDef().id,
                                  m_zones.getTileMap());
    }

    m_camera.update(m_player.getCenterX(), m_player.getCenterY(),
                  m_zones.getTileMap().getWidthPixels(),
                  m_zones.getTileMap().getHeightPixels());
}

void Gameplay::render(Renderer& renderer, float timeAccum, float fps) {
    ZoneID currentZone = m_zones.getCurrentZoneDef().id;

    renderer.beginFrame(m_zones.getTileMap().getBgColor());

    //==========================================================================
    // WORLD LAYER (Milestone 12, Task 4)
    //
    // Static/semi-static zone content: the tile map itself, plus
    // anything that's effectively part of the terrain (repairable
    // objects, gather nodes). None of this needs Y-sorting against
    // entities — it's drawn once, in a fixed back-to-front order,
    // before any entity touches the screen.
    //==========================================================================
    renderer.drawTileMap(m_zones.getTileMap(), m_camera);

    renderer.drawWorldObjects(m_worldObjects.getObjects(),
                              m_worldObjects.getObjectCount(),
                              currentZone, m_camera);

    renderer.drawGatherNodes(m_gatherNodes.getNodes(),
                             m_gatherNodes.getNodeCount(),
                             currentZone, m_camera);

    //==========================================================================
    // ENTITY LAYER (Milestone 12, Task 4)
    //
    // Everything that moves and has a Y position that should determine
    // draw order once real sprites with vertical extent exist (a
    // player standing "in front of" a tall NPC sprite should draw on
    // top of it; standing "behind" it should draw underneath). Today
    // every entity is a small flat colored rect with no real height, so
    // this fixed call order (enemies, then NPCs, then the player) is
    // not visibly wrong — but it IS the exact place a future Y-sort
    // pass belongs: replace this fixed sequence with "gather every
    // entity's (sprite, y) into one list, sort by y, draw in that
    // order" without touching the WORLD LAYER above or the OVERLAY
    // LAYER below, since neither of those participates in Y-sorting at
    // all. This is intentionally NOT implemented yet (Task 4's brief:
    // "do NOT fully rewrite the renderer... prepare it") — only this
    // clearly-bounded layer's existence is established by this comment
    // block and the lack of any draw-order logic mixed into the
    // WORLD/OVERLAY layers.
    //==========================================================================
    renderer.drawEnemies(m_enemies.getEnemies(),
                         m_enemies.getEnemyCount(),
                         currentZone, m_camera);

    renderer.drawNPCs(m_npcs, currentZone, m_camera);

    if (m_questMgr.markerVisible(currentZone)) {
        renderer.drawQuestMarker(m_questMgr.getMarkerX(), m_questMgr.getMarkerY(),
                                 m_camera, timeAccum);
    }

    renderer.drawPlayer(m_player.getX(), m_player.getY(), m_camera,
                        m_player.getFacing(), m_player.getAnimFrame());

    //==========================================================================
    // OVERLAY LAYER (Milestone 12, Task 4)
    //
    // Screen-space UI and full-screen effects — never world-positioned,
    // never Y-sorted, always drawn last so it sits on top of everything
    // in the WORLD/ENTITY layers above. Debug text, day/night tint,
    // zone transition fade, zone name banner, status/combat/inventory
    // message banners, and the bottom-screen panel (inventory/shop/
    // dialogue/HUD, in that mutual-exclusion priority order).
    //==========================================================================
    renderer.drawClockDebug(fps,
                            m_worldClock.getHour(),
                            m_worldClock.getMinute(),
                            m_dayNight.getPhaseName());

    renderer.drawTint(m_dayNight.getTintColor());
    renderer.drawFade(m_zones.getFadeAlpha());

    if (m_zones.showZoneName()) {
        renderer.drawZoneName(m_zones.getCurrentZoneDef().name,
                              m_zones.getNameAlpha());
    }

    // Save/load status message, combat feedback, or inventory
    // equip/unequip feedback if more recent — reuses the existing
    // message/timer banner rather than adding a second on-screen
    // text system, per the Milestone 8 assignment's explicit
    // instruction to reuse the project's message/timer pattern
    // instead of building new UI (Milestone 9 follows the same rule
    // for the new inventory screen's feedback).
    if (m_invMessageTimer > 0.0f && m_invMessage != nullptr) {
        float alpha = (m_invMessageTimer < 0.5f) ? m_invMessageTimer / 0.5f : 1.0f;
        renderer.drawStatusMessage(m_invMessage, alpha);
    } else if (m_combatMessageTimer > 0.0f && m_combatMessage != nullptr) {
        float alpha = (m_combatMessageTimer < 0.5f) ? m_combatMessageTimer / 0.5f : 1.0f;
        renderer.drawStatusMessage(m_combatMessage, alpha);
    } else if (m_statusTimer > 0.0f && m_statusMessage != nullptr) {
        float alpha = (m_statusTimer < 0.5f) ? m_statusTimer / 0.5f : 1.0f;
        renderer.drawStatusMessage(m_statusMessage, alpha);
    }

    // Bottom screen: inventory screen → shop → dialogue → quest HUD
    if (m_invScreen.isOpen()) {
        renderer.drawInventoryScreen(m_invScreen.getCursor(), m_playerState);
    } else if (m_shop.isOpen()) {
        renderer.drawShop(m_shop.getCursor(), m_playerState.gold,
                          m_shopMessage, m_shopMessageTimer);
    } else if (m_npcs.isDialogueOpen()) {
        renderer.drawDialogue(m_npcs.getActiveDialogueNPC());
    } else {
        const char* displayText = m_questMgr.getActiveObjectiveText();
        if (m_worldObjects.hasMessage()) {
            displayText = m_worldObjects.getLastMessage();
        } else if (m_gatherNodes.hasMessage()) {
            displayText = m_gatherNodes.getLastMessage();
        } else if (m_enemies.hasMessage()) {
            displayText = m_enemies.getLastMessage();
        }
        renderer.drawQuestHUD(displayText, m_playerState.hp, m_playerState.maxHp,
                              m_playerState.gold, m_playerState.wood, m_playerState.rope,
                              m_playerState.getAttack(), m_playerState.getDefense());
    }
}
