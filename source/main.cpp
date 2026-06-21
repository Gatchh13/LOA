//-----------------------------------------------------------------------------
// main.cpp
// Legends of Aetheria — Bootstrap Entry Point
//
// Responsible for:
//   1. System initialization (3DS services)
//   2. Subsystem construction
//   3. Main game loop (input → update → render)
//   4. Clean shutdown
//-----------------------------------------------------------------------------

#include <3ds.h>
#include <citro2d.h>

#include "core/Logger.h"
#include "core/Clock.h"
#include "input/InputManager.h"
#include "world/TileMap.h"
#include "render/Camera.h"
#include "render/Renderer.h"
#include "entities/Player.h"

int main() {
    //-------------------------------------------------------------------------
    // 3DS service initialization
    // romfsInit() mounts the romfs archive so we can read "romfs:/..." paths.
    //-------------------------------------------------------------------------
    romfsInit();

    //-------------------------------------------------------------------------
    // Subsystem initialization
    //-------------------------------------------------------------------------
    Logger::init();
    LOG("Legends of Aetheria — Bootstrap Build");

    Renderer renderer;
    if (!renderer.init()) {
        ERR("Renderer failed to initialize. Aborting.");
        romfsExit();
        return 1;
    }

    Clock        clock;
    InputManager input;
    TileMap      map;

    // Spawn the player at tile (2,2) = pixel (32, 32) — inside the open area.
    Player       player(2 * TILE_SIZE + 1.0f, 2 * TILE_SIZE + 1.0f);

    Camera       camera;
    camera.update(player.getCenterX(), player.getCenterY(),
                  map.getWidthPixels(), map.getHeightPixels());

    LOG("Bootstrap initialized — entering main loop");

    //-------------------------------------------------------------------------
    // Main loop
    // aptMainLoop() returns false when the HOME button is pressed or the
    // system requests an exit (e.g. power-off, sleep mode).
    //-------------------------------------------------------------------------
    while (aptMainLoop()) {
        // --- Input ---
        input.update();

        // START button = intentional exit
        if (input.startPressed()) {
            LOG("START pressed — exiting");
            break;
        }

        // --- Update ---
        clock.tick();
        float dt = clock.getDelta();

        Vec2 axis = input.getMovementAxis();
        player.update(axis, dt, map);

        camera.update(player.getCenterX(), player.getCenterY(),
                      map.getWidthPixels(), map.getHeightPixels());

        // --- Render ---
        renderer.beginFrame();
        renderer.drawTileMap(map, camera);
        renderer.drawPlayer(player.getX(), player.getY(), camera);
        renderer.drawFPS(clock.getFPS());
        renderer.endFrame();
    }

    //-------------------------------------------------------------------------
    // Shutdown
    // Renderer destructor handles C2D/C3D/gfx cleanup.
    //-------------------------------------------------------------------------
    LOG("Clean shutdown");
    romfsExit();
    return 0;
}
