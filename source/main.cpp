//-----------------------------------------------------------------------------
// main.cpp  (Milestone 3 — Quest Framework)
//
// New over Milestone 2:
//   - QuestManager  : tracks quest state, objectives, rewards
//   - PlayerState   : gold counter
//   - Quest marker  : drawn in forest when REACH_MARKER step is active
//   - Quest HUD     : objective text on bottom screen (when no dialogue)
//   - timeAccum     : running real-time seconds for marker pulse animation
//
// Render order (top screen):
//   tiles → NPCs → quest marker → player → clock debug →
//   day/night tint → fade → zone banner
//
// Bottom screen:
//   dialogue (if open) OR quest HUD (always otherwise)
//-----------------------------------------------------------------------------

#include <3ds.h>
#include <citro2d.h>

#include "core/Logger.h"
#include "core/Clock.h"
#include "core/WorldClock.h"
#include "input/InputManager.h"
#include "world/ZoneManager.h"
#include "world/DayNight.h"
#include "render/Camera.h"
#include "render/Renderer.h"
#include "entities/Player.h"
#include "npc/NPCManager.h"
#include "quest/QuestManager.h"
#include "quest/PlayerState.h"

int main() {
    romfsInit();

    Logger::init();
    LOG("Legends of Aetheria — Milestone 3: Quest Framework");

    Renderer renderer;
    if (!renderer.init()) {
        ERR("Renderer failed to initialize. Aborting.");
        romfsExit();
        return 1;
    }

    Clock        clock;
    WorldClock   worldClock(8 * 60);   // Start at 08:00
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

    // NPC + quest setup
    PlayerState  playerState;
    QuestManager questMgr;
    questMgr.init();

    NPCManager npcs;
    npcs.init(worldClock.getTotalMinutes());

    dayNight.update(worldClock.getTimeAsFloat());

    // Running real-time accumulator for marker pulse animation
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

        ZoneID currentZone = zones.getCurrentZoneDef().id;

        // B closes dialogue
        if (bPressed && npcs.isDialogueOpen()) {
            // Must also notify quest manager that dialogue ended without advancing
            // (B is an escape — no quest step progression)
            npcs.closeDialogue();
        }

        bool canMove = (zones.getFadeAlpha() < 1.0f) && !npcs.isDialogueOpen();

        if (canMove) {
            Vec2 axis = input.getMovementAxis();
            player.update(axis, dt, zones.getTileMap());
        }

        // NPC interaction — passes QuestManager so tryInteract can get
        // dialogue overrides and report talk events
        npcs.tryInteract(player.getCenterX(), player.getCenterY(),
                         aPressed, questMgr, playerState);

        // Quest proximity checks (REACH_MARKER)
        questMgr.update(currentZone, player.getCenterX(), player.getCenterY());

        // NPC schedules and movement
        npcs.update(currentZone,
                    worldClock.getTotalMinutes(),
                    worldClock.hourJustChanged(),
                    zones.getTileMap(),
                    dt);

        // Zone transitions (suppressed during dialogue)
        if (!npcs.isDialogueOpen()) {
            zones.update(player.getCenterTileX(), player.getCenterTileY(), dt);
        }

        if (zones.transitionReady()) {
            const SpawnPoint& sp = zones.getPendingSpawn();
            player.setPosition(sp.spawn_px, sp.spawn_py);
            zones.commitTransition();
        }

        camera.update(player.getCenterX(), player.getCenterY(),
                      zones.getTileMap().getWidthPixels(),
                      zones.getTileMap().getHeightPixels());

        //----------------------------------------------------------------------
        // Render
        //----------------------------------------------------------------------
        renderer.beginFrame(zones.getTileMap().getBgColor());

        renderer.drawTileMap(zones.getTileMap(), camera);

        renderer.drawNPCs(npcs, currentZone, camera);

        // Quest marker — shown when a REACH_MARKER objective is active here
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

        // Bottom screen: dialogue if open, quest HUD otherwise
        if (npcs.isDialogueOpen()) {
            renderer.drawDialogue(npcs.getActiveDialogueNPC());
        } else {
            renderer.drawQuestHUD(questMgr.getActiveObjectiveText(),
                                  playerState.gold);
        }

        renderer.endFrame();
    }

    LOG("Clean shutdown");
    romfsExit();
    return 0;
}
