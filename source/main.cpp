//-----------------------------------------------------------------------------
// main.cpp  (Milestone 2 — Living Town Prototype)
//
// New over Milestone 1:
//   - WorldClock   : in-game 24-hour time
//   - DayNight     : time → tint color
//   - NPCManager   : NPC state, schedules, movement, dialogue
//
// Render order (top screen):
//   tiles → NPCs → player → clock debug → day/night tint → fade → zone banner
//
// Input additions:
//   A button : interact with NPC / close dialogue
//   B button : close dialogue (alternate)
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

int main() {
    romfsInit();

    Logger::init();
    LOG("Legends of Aetheria — Milestone 2: Living Town");

    Renderer renderer;
    if (!renderer.init()) {
        ERR("Renderer failed to initialize. Aborting.");
        romfsExit();
        return 1;
    }

    Clock        clock;
    WorldClock   worldClock(8 * 60);  // Start at 08:00
    DayNight     dayNight;
    InputManager input;

    // Zone setup — start in Town
    ZoneManager zones;
    const ZoneDef&    startDef = getZoneDef(ZoneID::TOWN);
    const SpawnPoint& startSp  = startDef.spawns[0];
    zones.loadZone(ZoneID::TOWN, 0);

    Player player(startSp.spawn_px, startSp.spawn_py);

    Camera camera;
    camera.update(player.getCenterX(), player.getCenterY(),
                  zones.getTileMap().getWidthPixels(),
                  zones.getTileMap().getHeightPixels());

    // NPC setup — init after world clock so NPCs snap to correct positions
    NPCManager npcs;
    npcs.init(worldClock.getTotalMinutes());

    // Initial day/night state
    dayNight.update(worldClock.getTimeAsFloat());

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

        // World clock always advances (even during fade)
        worldClock.update(dt);
        dayNight.update(worldClock.getTimeAsFloat());

        // Determine current zone for NPC filtering
        ZoneID currentZone = zones.getCurrentZoneDef().id;

        // Close dialogue with B
        if (bPressed && npcs.isDialogueOpen()) {
            npcs.closeDialogue();
        }

        // Player movement blocked during zone fade or open dialogue
        bool canMove = (zones.getFadeAlpha() < 1.0f) && !npcs.isDialogueOpen();

        if (canMove) {
            Vec2 axis = input.getMovementAxis();
            player.update(axis, dt, zones.getTileMap());
        }

        // NPC interaction (A button)
        // tryInteract handles both open and close
        npcs.tryInteract(player.getCenterX(), player.getCenterY(), aPressed);

        // NPC schedule + movement update
        npcs.update(currentZone,
                    worldClock.getTotalMinutes(),
                    worldClock.hourJustChanged(),
                    zones.getTileMap(),
                    dt);

        // Zone transitions (not while dialogue open)
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

        // World geometry
        renderer.drawTileMap(zones.getTileMap(), camera);

        // NPCs (drawn before player so player appears on top)
        renderer.drawNPCs(npcs, currentZone, camera);

        // Player
        renderer.drawPlayer(player.getX(), player.getY(), camera);

        // HUD: FPS + clock
        renderer.drawClockDebug(clock.getFPS(),
                                worldClock.getHour(),
                                worldClock.getMinute(),
                                dayNight.getPhaseName());

        // Day/night tint (after world, before fade)
        renderer.drawTint(dayNight.getTintColor());

        // Zone transition fade
        renderer.drawFade(zones.getFadeAlpha());

        // Zone name banner (visible through fade-in)
        if (zones.showZoneName()) {
            renderer.drawZoneName(zones.getCurrentZoneDef().name,
                                  zones.getNameAlpha());
        }

        // Dialogue box on bottom screen (if open)
        if (npcs.isDialogueOpen()) {
            renderer.drawDialogue(npcs.getActiveDialogueNPC());
        }

        renderer.endFrame();
    }

    LOG("Clean shutdown");
    romfsExit();
    return 0;
}
