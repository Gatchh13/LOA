//-----------------------------------------------------------------------------
// main.cpp  (Milestone 8 — Combat Foundation)
//
// New over Milestone 7:
//   - Player HP (PlayerState.hp/maxHp, 100/100 start). Displayed on the
//     bottom-screen HUD as simple "HP: 100/100" text.
//   - Forest Wolf (EnemyManager, see source/combat/) — one enemy, an
//     Idle/Chase/Attack/Return state machine, contact damage on the
//     player, melee combat from the player (X), guaranteed Wolf Pelt
//     loot on death.
//   - Consumables: L uses a Healing Herb (+20 HP), R uses a Simple
//     Potion (+50 HP). Dedicated keys, no menu screen — the simplest
//     implementation that satisfies "no inventory menus if avoidable."
//   - Death/respawn: HP reaching 0 teleports the player to the Town
//     spawn point at full HP, costs a small flat amount of gold, and
//     respawns all enemies. No death screen.
//
// Button map (gameplay):
//   D-Pad / Circle Pad : move
//   A                  : interact (NPC, world object, gather node) / buy (in shop)
//   B                  : close dialogue / leave shop
//   X                  : melee attack
//   L                  : use Healing Herb (+20 HP)
//   R                  : use Simple Potion (+50 HP)
//   Y                  : open shop (only while talking to Mira)
//   SELECT             : save
//   START              : load
//   HOME               : quit (handled by aptMainLoop)
//
// Button map (title screen):
//   D-Pad / Circle Pad Up/Down : navigate
//   A / START                  : confirm
//   B / A (on credits panel)   : return to menu
//-----------------------------------------------------------------------------

#include <3ds.h>
#include <citro2d.h>

#include "core/Logger.h"
#include "core/Clock.h"
#include "core/WorldClock.h"
#include "core/GameState.h"
#include "core/TitleScreen.h"
#include "input/InputManager.h"
#include "world/ZoneManager.h"
#include "world/DayNight.h"
#include "world/WorldObjectManager.h"
#include "world/GatherNodeManager.h"
#include "world/Shop.h"
#include "combat/EnemyManager.h"
#include "render/Camera.h"
#include "render/Renderer.h"
#include "entities/Player.h"
#include "npc/NPCManager.h"
#include "quest/QuestManager.h"
#include "quest/PlayerState.h"
#include "save/SaveManager.h"

int main() {
    romfsInit();

    Logger::init();
    LOG("Legends of Aetheria — Milestone 6: Foundation of Feel");

    Renderer renderer;
    if (!renderer.init()) {
        ERR("Renderer failed to initialize. Aborting.");
        romfsExit();
        return 1;
    }

    Clock        clock;
    WorldClock   worldClock(8 * 60);   // default start: 08:00
    DayNight     dayNight;
    InputManager input;

    //--------------------------------------------------------------------------
    // Systems — constructed in dependency order.
    // Player has no default constructor (position is required), so it's
    // constructed here with a placeholder that startGame() below always
    // overwrites before gameplay actually begins — same pattern as
    // ZoneManager/NPCManager/WorldObjectManager, which are default-
    // constructed and only meaningfully set up via their own init() calls.
    //--------------------------------------------------------------------------
    PlayerState        playerState;
    QuestManager       questMgr;

    ZoneManager        zones;
    NPCManager         npcs;
    WorldObjectManager worldObjects;
    GatherNodeManager  gatherNodes;
    EnemyManager       enemies;
    ShopUI             shop;
    Player             player(0.0f, 0.0f);
    Camera              camera;

    StateManager state;       // starts at GameState::BOOT
    TitleScreen  titleScreen;

    float timeAccum   = 0.0f;

    // Status message state (save/load notification)
    const char* statusMessage = nullptr;
    float       statusTimer   = 0.0f;
    static constexpr float STATUS_DURATION = 3.0f;

    // Shop feedback message state (purchase result) — same pattern as
    // statusMessage above, scoped separately since it's shown inside the
    // shop panel rather than as a top-screen banner.
    const char* shopMessage      = nullptr;
    float       shopMessageTimer = 0.0f;
    static constexpr float SHOP_MESSAGE_DURATION = 2.0f;

    // Combat feedback message state (attack/consumable results,
    // Milestone 8) — same message+timer pattern as everything else in
    // this file, shown via drawStatusMessage like the save/load banner.
    const char* combatMessage      = nullptr;
    float       combatMessageTimer = 0.0f;
    static constexpr float COMBAT_MESSAGE_DURATION = 2.0f;

    //--------------------------------------------------------------------------
    // startGame — runs once, when the player picks New Game or Continue on
    // the title screen. Everything in here used to run unconditionally
    // before the main loop in Milestone 5; it's now deferred so the title
    // screen can be shown first without touching any gameplay system.
    //--------------------------------------------------------------------------
    auto startGame = [&](bool loadExisting) {
        bool loadedSave = false;

        // Pre-initialize zones so the TileMap exists before loadGame writes to it.
        zones.loadZone(ZoneID::TOWN, 0);
        worldObjects.init(zones.getTileMap());
        gatherNodes.init();
        enemies.init();
        questMgr.init();

        // Determine start position
        const ZoneDef&    startDef = getZoneDef(ZoneID::TOWN);
        const SpawnPoint& startSp  = startDef.spawns[0];
        player.setPosition(startSp.spawn_px, startSp.spawn_py);

        if (loadExisting && SaveManager::hasSave()) {
            loadedSave = SaveManager::loadGame(
                player, zones, worldClock, playerState,
                questMgr, worldObjects, zones.getTileMap());
            if (loadedSave) {
                // SaveManager::apply() already applied world object overrides
                // for the loaded zone (load zone -> restore states -> onZoneLoaded).
                npcs.init(worldClock.getTotalMinutes());
                LOG("Save loaded successfully");
            } else {
                WARN("Save validation failed — starting new game");
            }
        }

        if (!loadedSave) {
            // Fresh new game
            playerState.init();
            npcs.init(worldClock.getTotalMinutes());
            LOG("New game started");
        }

        camera.update(player.getCenterX(), player.getCenterY(),
                      zones.getTileMap().getWidthPixels(),
                      zones.getTileMap().getHeightPixels());

        dayNight.update(worldClock.getTimeAsFloat());

        state.push(GameState::GAMEPLAY);
    };

    titleScreen.enter(SaveManager::hasSave());
    state.push(GameState::TITLE_SCREEN);

    LOG("Entering main loop");

    while (aptMainLoop()) {
        //----------------------------------------------------------------------
        // Input
        //----------------------------------------------------------------------
        input.update();

        clock.tick();
        float dt = clock.getDelta();
        timeAccum += dt;

        //----------------------------------------------------------------------
        // Title screen
        //----------------------------------------------------------------------
        if (state.is(GameState::TITLE_SCREEN)) {
            TitleResult result = titleScreen.update(input);
            if (result == TitleResult::START_NEW) {
                startGame(/*loadExisting=*/false);
            } else if (result == TitleResult::START_LOAD) {
                startGame(/*loadExisting=*/true);
            }

            renderer.beginFrame(C2D_Color32(18, 22, 34, 255));
            renderer.drawTitleScreen(titleScreen.getSelected(),
                                     titleScreen.hasSave(),
                                     titleScreen.isShowingCredits(),
                                     timeAccum);
            renderer.drawTitleScreenBottom();
            renderer.endFrame();
            continue;
        }

        //----------------------------------------------------------------------
        // Gameplay
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
        // Save / Load (SELECT / START)
        //----------------------------------------------------------------------
        if (selectPressed) {
            bool ok = SaveManager::saveGame(
                player, zones, worldClock,
                playerState, questMgr, worldObjects);
            statusMessage = ok ? "Game Saved." : "Save Failed!";
            statusTimer   = STATUS_DURATION;
            LOG("Save triggered: %s", statusMessage);
        }

        if (startPressed) {
            if (SaveManager::hasSave()) {
                bool ok = SaveManager::loadGame(
                    player, zones, worldClock,
                    playerState, questMgr, worldObjects,
                    zones.getTileMap());
                if (ok) {
                    // SaveManager::apply() already applied world object
                    // overrides for the loaded zone.
                    npcs.init(worldClock.getTotalMinutes());
                    camera.update(player.getCenterX(), player.getCenterY(),
                                  zones.getTileMap().getWidthPixels(),
                                  zones.getTileMap().getHeightPixels());
                    statusMessage = "Game Loaded.";
                } else {
                    statusMessage = "Load Failed!";
                }
                statusTimer = STATUS_DURATION;
                LOG("Load triggered: %s", statusMessage);
            } else {
                statusMessage = "No Save Found.";
                statusTimer   = STATUS_DURATION;
            }
        }

        //----------------------------------------------------------------------
        // Update
        //----------------------------------------------------------------------
        statusTimer      = statusTimer > 0.0f ? statusTimer - dt : 0.0f;
        shopMessageTimer = shopMessageTimer > 0.0f ? shopMessageTimer - dt : 0.0f;

        worldClock.update(dt);
        dayNight.update(worldClock.getTimeAsFloat());
        worldObjects.updateMessageTimer(dt);
        gatherNodes.update(dt);
        gatherNodes.updateMessageTimer(dt);

        ZoneID currentZone = zones.getCurrentZoneDef().id;

        // Open the shop: Y while talking to Mira specifically. Checked
        // before the B-close-dialogue handling below so the same frame
        // that opens the shop doesn't also immediately process a B press
        // meant for something else.
        if (yPressed && !shop.isOpen() && npcs.isDialogueOpen() &&
            npcs.isTalkingToMerchant())
        {
            npcs.closeDialogue();
            shop.open();
            shopMessage      = nullptr;
            shopMessageTimer = 0.0f;
            LOG("Shop opened");
        }

        if (shop.isOpen()) {
            // Shop has exclusive input while open: no movement, no
            // dialogue, no world object/gather/NPC interaction this
            // frame. Mirrors how dialogue already suspends movement via
            // `canMove` below, just for the shop's own input set.
            if (input.isPressed(KEY_DOWN)) shop.moveCursor(+1);
            if (input.isPressed(KEY_UP))   shop.moveCursor(-1);

            if (aPressed) {
                ShopResult result = shop.tryBuy(playerState);
                switch (result) {
                    case ShopResult::PURCHASED:
                        shopMessage = "Purchased!";
                        break;
                    case ShopResult::NOT_ENOUGH_GOLD:
                        shopMessage = "Not enough gold.";
                        break;
                    case ShopResult::INVENTORY_FULL:
                        shopMessage = "Inventory full.";
                        break;
                    case ShopResult::NONE:
                    default:
                        shopMessage = nullptr;
                        break;
                }
                shopMessageTimer = SHOP_MESSAGE_DURATION;
            }

            if (bPressed) {
                shop.close();
                LOG("Shop closed");
            }
        }

        if (bPressed && npcs.isDialogueOpen()) {
            npcs.closeDialogue();
        }

        bool canMove = (zones.getFadeAlpha() < 1.0f) && !npcs.isDialogueOpen()
                      && !shop.isOpen();
        if (canMove) {
            Vec2 axis = input.getMovementAxis();
            player.update(axis, dt, zones.getTileMap());
        }

        // A-button priority: dialogue close → world object → gather node → NPC
        // Skipped entirely while the shop is open — its A press was
        // already consumed by shop.tryBuy() above.
        bool aConsumed = shop.isOpen();

        if (!aConsumed && npcs.isDialogueOpen() && aPressed) {
            npcs.tryInteract(player.getCenterX(), player.getCenterY(),
                             aPressed, questMgr, playerState);
            aConsumed = true;
        }

        if (!aConsumed) {
            InteractResult result = worldObjects.tryInteract(
                player.getCenterX(), player.getCenterY(),
                aPressed, zones.getTileMap(), playerState, &questMgr);
            if (result != InteractResult::NONE) aConsumed = true;
        }

        if (!aConsumed) {
            GatherResult gatherResult = gatherNodes.tryHarvest(
                player.getCenterX(), player.getCenterY(),
                currentZone, aPressed, playerState);
            if (gatherResult != GatherResult::NONE) aConsumed = true;
        }

        if (!aConsumed) {
            npcs.tryInteract(player.getCenterX(), player.getCenterY(),
                             aPressed, questMgr, playerState);
        }

        //----------------------------------------------------------------------
        // Combat (Milestone 8) — suspended while talking or shopping,
        // exactly like movement (`canMove` above already excludes both).
        //----------------------------------------------------------------------
        combatMessageTimer = combatMessageTimer > 0.0f ? combatMessageTimer - dt : 0.0f;

        bool interactionSuspended = npcs.isDialogueOpen() || shop.isOpen();

        // Enemy AI/contact-damage always ticks — an enemy mid-bite
        // shouldn't freeze just because a menu opened elsewhere. This is
        // harmless even while suspended: the player's position is also
        // frozen via `canMove` above, so a suspended enemy's distance to
        // the player isn't changing either; this just keeps its animation
        // timer and any in-progress bite-interval honest.
        enemies.update(currentZone, player.getCenterX(), player.getCenterY(),
                      dt, zones.getTileMap(), playerState);
        enemies.updateMessageTimer(dt);

        if (!interactionSuspended) {
            AttackResult atkResult = enemies.tryAttack(
                player.getCenterX(), player.getCenterY(),
                player.getFacing(), xPressed, dt, playerState);
            if (atkResult == AttackResult::KILLED) {
                combatMessage      = enemies.getLastMessage();
                combatMessageTimer = COMBAT_MESSAGE_DURATION;
            }

            // Consumables: L = Healing Herb, R = Simple Potion. Dedicated
            // keys, no menu screen at all — the simplest implementation
            // that satisfies "no inventory menus if avoidable" literally.
            if (lPressed) {
                bool used = playerState.useConsumable(static_cast<u8>(ItemID::HEALING_HERB));
                combatMessage      = used ? "Used Healing Herb. +20 HP" : "No Healing Herb.";
                combatMessageTimer = COMBAT_MESSAGE_DURATION;
            }
            if (rPressed) {
                bool used = playerState.useConsumable(static_cast<u8>(ItemID::SIMPLE_POTION));
                combatMessage      = used ? "Used Simple Potion. +50 HP" : "No Simple Potion.";
                combatMessageTimer = COMBAT_MESSAGE_DURATION;
            }
        }

        // Death/respawn (Feature 8). Checked once per frame, after all
        // of this frame's damage sources (enemy contact damage above)
        // have been applied — so a killing blow and the respawn it
        // triggers always happen within the same frame, never delayed.
        if (playerState.isDead()) {
            const ZoneDef&    townDef = getZoneDef(ZoneID::TOWN);
            const SpawnPoint& townSp  = townDef.spawns[0];

            if (currentZone != ZoneID::TOWN) {
                zones.forceLoadZone(ZoneID::TOWN, 0);
                worldObjects.onZoneLoaded(ZoneID::TOWN, zones.getTileMap());
                currentZone = ZoneID::TOWN;
            }
            player.setPosition(townSp.spawn_px, townSp.spawn_py);
            playerState.hp = playerState.maxHp;

            // Small gold loss on death, per the assignment's optional
            // suggestion — discourages treating death as a non-event
            // without it being punishing enough to need a real penalty
            // system (no equipment durability, no item loss — just a
            // flat, small, clamped-at-0 gold cost).
            constexpr u32 DEATH_GOLD_LOSS = 5;
            playerState.gold = (playerState.gold > DEATH_GOLD_LOSS)
                              ? playerState.gold - DEATH_GOLD_LOSS : 0;

            enemies.respawnAll();

            combatMessage      = "You were defeated. Returned to Town.";
            combatMessageTimer = COMBAT_MESSAGE_DURATION;
            LOG("Player died — respawned at Town, lost %u gold, all enemies respawned",
                DEATH_GOLD_LOSS);
        }

        questMgr.update(currentZone, player.getCenterX(), player.getCenterY());

        npcs.update(currentZone,
                    worldClock.getTotalMinutes(),
                    worldClock.hourJustChanged(),
                    zones.getTileMap(),
                    dt);

        if (!npcs.isDialogueOpen()) {
            zones.update(player.getCenterTileX(), player.getCenterTileY(), dt);
        }

        if (zones.transitionReady()) {
            const SpawnPoint& sp = zones.getPendingSpawn();
            player.setPosition(sp.spawn_px, sp.spawn_py);
            zones.commitTransition();
            worldObjects.onZoneLoaded(zones.getCurrentZoneDef().id,
                                      zones.getTileMap());
        }

        camera.update(player.getCenterX(), player.getCenterY(),
                      zones.getTileMap().getWidthPixels(),
                      zones.getTileMap().getHeightPixels());

        //----------------------------------------------------------------------
        // Render
        //----------------------------------------------------------------------
        renderer.beginFrame(zones.getTileMap().getBgColor());

        renderer.drawTileMap(zones.getTileMap(), camera);

        renderer.drawWorldObjects(worldObjects.getObjects(),
                                  worldObjects.getObjectCount(),
                                  currentZone, camera);

        renderer.drawGatherNodes(gatherNodes.getNodes(),
                                 gatherNodes.getNodeCount(),
                                 currentZone, camera);

        renderer.drawEnemies(enemies.getEnemies(),
                             enemies.getEnemyCount(),
                             currentZone, camera);

        renderer.drawNPCs(npcs, currentZone, camera);

        if (questMgr.markerVisible(currentZone)) {
            renderer.drawQuestMarker(questMgr.getMarkerX(), questMgr.getMarkerY(),
                                     camera, timeAccum);
        }

        renderer.drawPlayer(player.getX(), player.getY(), camera,
                            player.getFacing(), player.getAnimFrame());

        renderer.drawClockDebug(clock.getFPS(),
                                worldClock.getHour(),
                                worldClock.getMinute(),
                                dayNight.getPhaseName());

        renderer.drawTint(dayNight.getTintColor());
        renderer.drawFade(zones.getFadeAlpha());

        if (zones.showZoneName()) {
            renderer.drawZoneName(zones.getCurrentZoneDef().name,
                                  zones.getNameAlpha());
        }

        // Save/load status message, or combat feedback if more recent —
        // reuses the existing message/timer banner rather than adding a
        // second on-screen text system, per the Milestone 8 assignment's
        // explicit instruction to reuse the project's message/timer
        // pattern instead of building new UI.
        if (combatMessageTimer > 0.0f && combatMessage != nullptr) {
            float alpha = (combatMessageTimer < 0.5f) ? combatMessageTimer / 0.5f : 1.0f;
            renderer.drawStatusMessage(combatMessage, alpha);
        } else if (statusTimer > 0.0f && statusMessage != nullptr) {
            float alpha = (statusTimer < 0.5f) ? statusTimer / 0.5f : 1.0f;
            renderer.drawStatusMessage(statusMessage, alpha);
        }

        // Bottom screen: shop → dialogue → quest HUD
        if (shop.isOpen()) {
            renderer.drawShop(shop.getCursor(), playerState.gold,
                              shopMessage, shopMessageTimer);
        } else if (npcs.isDialogueOpen()) {
            renderer.drawDialogue(npcs.getActiveDialogueNPC());
        } else {
            const char* displayText = questMgr.getActiveObjectiveText();
            if (worldObjects.hasMessage()) {
                displayText = worldObjects.getLastMessage();
            } else if (gatherNodes.hasMessage()) {
                displayText = gatherNodes.getLastMessage();
            } else if (enemies.hasMessage()) {
                displayText = enemies.getLastMessage();
            }
            renderer.drawQuestHUD(displayText, playerState.hp, playerState.maxHp,
                                  playerState.gold, playerState.wood, playerState.rope);
        }

        renderer.endFrame();
    }

    LOG("Clean shutdown");
    romfsExit();
    return 0;
}
