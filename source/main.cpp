//-----------------------------------------------------------------------------
// main.cpp  (Milestone 1 — World Exploration)
//
// Changes from bootstrap:
//   - TileMap replaced by ZoneManager (owns the active map)
//   - Zone transitions: detect → fade out → reposition player → fade in
//   - Camera updated after every player move
//   - Renderer::beginFrame takes bgColor from current zone
//   - drawFade and drawZoneName called each frame
//-----------------------------------------------------------------------------

#include <3ds.h>
#include <citro2d.h>

#include "core/Logger.h"
#include "core/Clock.h"
#include "input/InputManager.h"
#include "world/ZoneManager.h"
#include "render/Camera.h"
#include "render/Renderer.h"
#include "entities/Player.h"

int main() {
    romfsInit();

    Logger::init();
    LOG("Legends of Aetheria — Milestone 1: World Exploration");

    Renderer renderer;
    if (!renderer.init()) {
        ERR("Renderer failed to initialize. Aborting.");
        romfsExit();
        return 1;
    }

    Clock        clock;
    InputManager input;

    // Start in Town at default spawn [0].
    ZoneManager  zones;
    const ZoneDef& startDef   = getZoneDef(ZoneID::TOWN);
    const SpawnPoint& startSp = startDef.spawns[0];
    zones.loadZone(ZoneID::TOWN, 0);

    Player player(startSp.spawn_px, startSp.spawn_py);

    Camera camera;
    camera.update(player.getCenterX(), player.getCenterY(),
                  zones.getTileMap().getWidthPixels(),
                  zones.getTileMap().getHeightPixels());

    LOG("World loaded — entering main loop");

    while (aptMainLoop()) {
        // --- Input ---
        input.update();
        if (input.startPressed()) {
            LOG("START pressed — exiting");
            break;
        }

        // --- Update ---
        clock.tick();
        float dt = clock.getDelta();

        // Only move the player when no transition is in progress.
        // (Fade state NONE or FADE_IN — player is visible and in control.)
        if (zones.getFadeAlpha() < 1.0f) {
            Vec2 axis = input.getMovementAxis();
            player.update(axis, dt, zones.getTileMap());
        }

        // Advance zone fade / check transition triggers.
        zones.update(player.getCenterTileX(), player.getCenterTileY(), dt);

        // If fade-out is complete, reposition the player and commit.
        if (zones.transitionReady()) {
            const SpawnPoint& sp = zones.getPendingSpawn();
            player.setPosition(sp.spawn_px, sp.spawn_py);
            zones.commitTransition();   // loads new zone, starts fade-in
        }

        // Update camera every frame — handles both normal movement and the
        // hard snap when the player is repositioned on zone entry.
        camera.update(player.getCenterX(), player.getCenterY(),
                      zones.getTileMap().getWidthPixels(),
                      zones.getTileMap().getHeightPixels());

        // --- Render ---
        renderer.beginFrame(zones.getTileMap().getBgColor());
        renderer.drawTileMap(zones.getTileMap(), camera);
        renderer.drawPlayer(player.getX(), player.getY(), camera);
        renderer.drawFPS(clock.getFPS());

        // Fade overlay (drawn on top of world, beneath zone name)
        renderer.drawFade(zones.getFadeAlpha());

        // Zone name banner (drawn on top of fade so it's visible during fade-in)
        if (zones.showZoneName()) {
            renderer.drawZoneName(
                zones.getCurrentZoneDef().name,
                zones.getNameAlpha());
        }

        renderer.endFrame();
    }

    LOG("Clean shutdown");
    romfsExit();
    return 0;
}
