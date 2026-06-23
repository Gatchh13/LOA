//-----------------------------------------------------------------------------
// main.cpp  (Milestone 5 — Save / Load Foundation)
//
// New over Milestone 4:
//   - SaveManager   : save to / load from sdmc:/loa_save.bin
//   - On startup: load save if exists, else new game
//   - SELECT = save game
//   - START  = load game  (changed from quit to load; HOME still quits)
//   - Status message "Game Saved" / "Game Loaded" displayed for 3 seconds
//   - statusMessage / statusTimer drive the notification
//
// Button map:
//   D-Pad / Circle Pad : move
//   A                  : interact (NPC, world object)
//   B                  : close dialogue
//   SELECT             : save
//   START              : load
//   HOME               : quit (handled by aptMainLoop)
//-----------------------------------------------------------------------------

#include <3ds.h>
#include <citro2d.h>

#include "core/Logger.h"
#include "core/Clock.h"
#include "core/WorldClock.h"
#include "input/InputManager.h"
#include "world/ZoneManager.h"
#include "world/DayNight.h"
#include "world/WorldObjectManager.h"
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
    LOG("Legends of Aetheria — Milestone 5: Save / Load Foundation");

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
    // Systems — constructed in dependency order
    //--------------------------------------------------------------------------
    PlayerState        playerState;
    QuestManager       questMgr;

    ZoneManager        zones;
    NPCManager         npcs;
    WorldObjectManager worldObjects;

    //--------------------------------------------------------------------------
    // Startup: load save OR new game
    //--------------------------------------------------------------------------
    bool loadedSave = false;

    // Pre-initialize zones so the TileMap exists before loadGame writes to it.
    zones.loadZone(ZoneID::TOWN, 0);
    worldObjects.init(zones.getTileMap());
    questMgr.init();



    // Determine start position
    const ZoneDef&    startDef = getZoneDef(ZoneID::TOWN);
    const SpawnPoint& startSp  = startDef.spawns[0];
    Player player(startSp.spawn_px, startSp.spawn_py);

    // Deferred load: now that Player exists, attempt the real load
    if (SaveManager::hasSave()) {
        loadedSave = SaveManager::loadGame(
            player, zones, worldClock, playerState,
            questMgr, worldObjects, zones.getTileMap());
        if (loadedSave) {
            // Re-apply world object overrides for the loaded zone
            worldObjects.onZoneLoaded(zones.getCurrentZoneDef().id,
                                      zones.getTileMap());
            npcs.init(worldClock.getTotalMinutes());
            LOG("Save loaded successfully");
        } else {
            WARN("Save validation failed — starting new game");
        }
    }

    if (!loadedSave) {
        // Fresh new game
        npcs.init(worldClock.getTotalMinutes());
        LOG("New game started");
    }

    Camera camera;
    camera.update(player.getCenterX(), player.getCenterY(),
                  zones.getTileMap().getWidthPixels(),
                  zones.getTileMap().getHeightPixels());

    dayNight.update(worldClock.getTimeAsFloat());

    float timeAccum   = 0.0f;

    // Status message state (save/load notification)
    const char* statusMessage = nullptr;
    float       statusTimer   = 0.0f;
    static constexpr float STATUS_DURATION = 3.0f;

    LOG("Entering main loop");

    while (aptMainLoop()) {
        //----------------------------------------------------------------------
        // Input
        //----------------------------------------------------------------------
        input.update();

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
                    worldObjects.onZoneLoaded(zones.getCurrentZoneDef().id,
                                              zones.getTileMap());
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
        clock.tick();
        float dt = clock.getDelta();
        timeAccum   += dt;
        statusTimer  = statusTimer > 0.0f ? statusTimer - dt : 0.0f;

        worldClock.update(dt);
        dayNight.update(worldClock.getTimeAsFloat());
        worldObjects.updateMessageTimer(dt);

        ZoneID currentZone = zones.getCurrentZoneDef().id;

        if (bPressed && npcs.isDialogueOpen()) {
            npcs.closeDialogue();
        }

        bool canMove = (zones.getFadeAlpha() < 1.0f) && !npcs.isDialogueOpen();
        if (canMove) {
            Vec2 axis = input.getMovementAxis();
            player.update(axis, dt, zones.getTileMap());
        }

        // A-button priority: dialogue close → world object → NPC
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

        renderer.drawNPCs(npcs, currentZone, camera);

        if (questMgr.markerVisible(currentZone)) {
            renderer.drawQuestMarker(questMgr.getMarkerX(), questMgr.getMarkerY(),
                                     camera, timeAccum);
        }

        renderer.drawPlayer(player.getX(), player.getY(), camera);

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
            const char* displayText = worldObjects.hasMessage()
                                      ? worldObjects.getLastMessage()
                                      : questMgr.getActiveObjectiveText();
            renderer.drawQuestHUD(displayText, playerState.gold,
                                  playerState.wood, playerState.rope);
        }

        renderer.endFrame();
    }

    LOG("Clean shutdown");
    romfsExit();
    return 0;
}
