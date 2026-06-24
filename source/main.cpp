//-----------------------------------------------------------------------------
// main.cpp  (Milestone 7 — Economy Loop)
//
// New over Milestone 6:
//   - Shop system: pressing Y while talking to Mira opens her stock list
//     (ShopUI, see source/world/Shop.h). Up/Down selects, A buys
//     (deducts gold, adds to inventory), B leaves. Reuses the existing
//     dialogue-open/closed flow rather than adding a new GameState.
//   - Inventory: PlayerState now owns an 8-slot Inventory (see
//     source/quest/Inventory.h). Populated by shop purchases and quest
//     rewards. Persisted in SaveData (version bumped 5 -> 6).
//   - Quest rewards can now include an item alongside gold (see
//     QuestDef.h's QuestReward) — "The Missing Package" grants Mira's
//     Token on completion, proving the reward path for a non-purchasable
//     item.
//
// Button map (gameplay):
//   D-Pad / Circle Pad : move
//   A                  : interact (NPC, world object, gather node) / buy (in shop)
//   B                  : close dialogue / leave shop
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

        // Save/load status message
        if (statusTimer > 0.0f && statusMessage != nullptr) {
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
            }
            renderer.drawQuestHUD(displayText, playerState.gold,
                                  playerState.wood, playerState.rope);
        }

        renderer.endFrame();
    }

    LOG("Clean shutdown");
    romfsExit();
    return 0;
}
