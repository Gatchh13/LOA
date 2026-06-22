//-----------------------------------------------------------------------------
// main.cpp  (Milestone 4 — Infrastructure & Shortcut System)
//
// New over Milestone 3:
//   - WorldObjectManager : bridges, ladders, obstacles with tile collision
//   - TileMap override layer : runtime tile modifications
//   - PlayerState: wood + rope resources (starts at 20 wood, 10 rope)
//   - Interaction priority: dialogue close → world object → NPC
//   - Object message display on bottom screen
//   - onZoneLoaded() re-applies world object overrides on zone transition
//
// Render order (top screen):
//   tiles → world objects → NPCs → quest marker → player → clock debug →
//   day/night tint → fade → zone banner
//
// Bottom screen:
//   dialogue OR quest HUD (with resources)
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

int main() {
    romfsInit();

    Logger::init();
    LOG("Legends of Aetheria — Milestone 4: Infrastructure & Shortcuts");

    Renderer renderer;
    if (!renderer.init()) {
        ERR("Renderer failed to initialize. Aborting.");
        romfsExit();
        return 1;
    }

    Clock        clock;
    WorldClock   worldClock(8 * 60);
    DayNight     dayNight;
    InputManager input;

    // Zone setup
    ZoneManager zones;
    const ZoneDef&    startDef = getZoneDef(ZoneID::TOWN);
    const SpawnPoint& startSp  = startDef.spawns[0];
    zones.loadZone(ZoneID::TOWN, 0);

    Player player(startSp.spawn_px, startSp.spawn_py);

    Camera camera;
    camera.update(player.getCenterX(), player.getCenterY(),
                  zones.getTileMap().getWidthPixels(),
                  zones.getTileMap().getHeightPixels());

    // Systems
    PlayerState        playerState;
    QuestManager       questMgr;
    questMgr.init();

    NPCManager         npcs;
    npcs.init(worldClock.getTotalMinutes());

    // WorldObjectManager — init applies initial tile overrides to town map
    WorldObjectManager worldObjects;
    worldObjects.init(zones.getTileMap());

    dayNight.update(worldClock.getTimeAsFloat());

    float timeAccum = 0.0f;

    LOG("World loaded — entering main loop");

    while (aptMainLoop()) {
        //----------------------------------------------------------------------
        // Input
        //----------------------------------------------------------------------
        input.update();

        if (input.startPressed()) {
            LOG("START pressed — exiting");
            break;
        }

        bool aPressed = input.isPressed(KEY_A);
        bool bPressed = input.isPressed(KEY_B);

        //----------------------------------------------------------------------
        // Update
        //----------------------------------------------------------------------
        clock.tick();
        float dt = clock.getDelta();
        timeAccum += dt;

        worldClock.update(dt);
        dayNight.update(worldClock.getTimeAsFloat());
        worldObjects.updateMessageTimer(dt);

        ZoneID currentZone = zones.getCurrentZoneDef().id;

        // B closes dialogue (no quest progression on B)
        if (bPressed && npcs.isDialogueOpen()) {
            npcs.closeDialogue();
        }

        bool canMove = (zones.getFadeAlpha() < 1.0f) && !npcs.isDialogueOpen();
        if (canMove) {
            Vec2 axis = input.getMovementAxis();
            player.update(axis, dt, zones.getTileMap());
        }

        //----------------------------------------------------------------------
        // A-button interaction priority:
        //   1. Close dialogue (if open)
        //   2. World object interaction (if not in dialogue)
        //   3. NPC interaction (if not in dialogue and no world object claimed A)
        //----------------------------------------------------------------------
        bool aConsumed = false;

        if (npcs.isDialogueOpen() && aPressed) {
            // NPC interaction handles its own A-close
            npcs.tryInteract(player.getCenterX(), player.getCenterY(),
                             aPressed, questMgr, playerState);
            aConsumed = true;
        }

        if (!aConsumed) {
            InteractResult result = worldObjects.tryInteract(
                player.getCenterX(), player.getCenterY(),
                aPressed,
                zones.getTileMap(),
                playerState,
                &questMgr);

            if (result != InteractResult::NONE) {
                aConsumed = true;
            }
        }

        if (!aConsumed) {
            npcs.tryInteract(player.getCenterX(), player.getCenterY(),
                             aPressed, questMgr, playerState);
        }

        // Quest proximity (REACH_MARKER)
        questMgr.update(currentZone, player.getCenterX(), player.getCenterY());

        // NPC schedules
        npcs.update(currentZone,
                    worldClock.getTotalMinutes(),
                    worldClock.hourJustChanged(),
                    zones.getTileMap(),
                    dt);

        // Zone transitions
        if (!npcs.isDialogueOpen()) {
            zones.update(player.getCenterTileX(), player.getCenterTileY(), dt);
        }

        if (zones.transitionReady()) {
            const SpawnPoint& sp = zones.getPendingSpawn();
            player.setPosition(sp.spawn_px, sp.spawn_py);
            zones.commitTransition();
            // Re-apply world object overrides for the newly loaded zone
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

        // World objects drawn above tiles, below NPCs
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

        // Bottom screen
        if (npcs.isDialogueOpen()) {
            renderer.drawDialogue(npcs.getActiveDialogueNPC());
        } else {
            // Show world object message if active, otherwise quest objective
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
