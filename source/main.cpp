//-----------------------------------------------------------------------------
// main.cpp  (Milestone 6 — Foundation of Feel)
//
// New over Milestone 5:
//   - Boots into a TITLE_SCREEN state instead of straight into gameplay.
//     New Game / Continue (disabled with no save) / Credits.
//   - GameState/StateManager (source/core/GameState.h) now actually used —
//     previously declared but never wired into main().
//   - Gameplay setup (zone load, player construction, save/load) is now
//     deferred until the player actually chooses New Game or Continue on
//     the title screen, rather than running unconditionally before the
//     loop even starts.
//   - Player/NPC sprites now animate (facing + walk frame) via AnimState
//     — see source/entities/AnimState.h. Still fallback colored-rect art;
//     animation shows as a small facing notch + step bob until a real
//     sprite sheet exists.
//
// Button map (gameplay):
//   D-Pad / Circle Pad : move
//   A                  : interact (NPC, world object)
//   B                  : close dialogue
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
    Player             player(0.0f, 0.0f);
    Camera              camera;

    StateManager state;       // starts at GameState::BOOT
    TitleScreen  titleScreen;

    float timeAccum   = 0.0f;

    // Status message state (save/load notification)
    const char* statusMessage = nullptr;
    float       statusTimer   = 0.0f;
    static constexpr float STATUS_DURATION = 3.0f;

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
        statusTimer  = statusTimer > 0.0f ? statusTimer - dt : 0.0f;

        worldClock.update(dt);
        dayNight.update(worldClock.getTimeAsFloat());
        worldObjects.updateMessageTimer(dt);
        gatherNodes.update(dt);
        gatherNodes.updateMessageTimer(dt);

        ZoneID currentZone = zones.getCurrentZoneDef().id;

        if (bPressed && npcs.isDialogueOpen()) {
            npcs.closeDialogue();
        }

        bool canMove = (zones.getFadeAlpha() < 1.0f) && !npcs.isDialogueOpen();
        if (canMove) {
            Vec2 axis = input.getMovementAxis();
            player.update(axis, dt, zones.getTileMap());
        }

        // A-button priority: dialogue close → world object → gather node → NPC
        bool aConsumed = false;

        if (npcs.isDialogueOpen() && aPressed) {
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

        // Bottom screen
        if (npcs.isDialogueOpen()) {
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
