#include <3ds.h>
#include <citro2d.h>
#include "core/GameState.h"
#include "core/Clock.h"
#include "core/InputManager.h"
#include "render/Renderer.h"

// --- Phase 0 placeholder player ---
// Real ECS (TransformComponent/SpriteComponent) comes in Phase 1.
// For now: a flat-color rectangle standing in for the sprite, just to
// prove the loop, input, and render pipeline all work end to end.
struct PlaceholderPlayer {
    float x = 200.0f - 8.0f; // center of 400px top screen, 16px wide box
    float y = 120.0f - 8.0f;
    static constexpr float SPEED = 60.0f; // px/sec
    u8 facing = 0; // 0=down 1=up 2=left 3=right

    void update(const InputManager& input, float dt) {
        s8 dx, dy;
        input.getMoveDir(dx, dy);

        if (dx != 0 || dy != 0) {
            x += dx * SPEED * dt;
            y += dy * SPEED * dt;

            if (dy > 0) facing = 0;
            else if (dy < 0) facing = 1;
            else if (dx < 0) facing = 2;
            else if (dx > 0) facing = 3;
        }

        // Clamp to top screen bounds (400x240) so it's always visible in Phase 0
        if (x < 0) x = 0;
        if (y < 0) y = 0;
        if (x > 400 - 16) x = 400 - 16;
        if (y > 240 - 16) y = 240 - 16;
    }

    // Facing-tinted placeholder colors until real sprite sheet lands (Phase 1/6)
    u32 colorForFacing() const {
        switch (facing) {
            case 0: return C2D_Color32(200, 170, 90, 255);  // down  - tan
            case 1: return C2D_Color32(110, 130, 80, 255);  // up    - olive
            case 2: return C2D_Color32(150, 110, 70, 255);  // left  - brown
            case 3: return C2D_Color32(170, 140, 100, 255); // right - light brown
        }
        return C2D_Color32(255, 255, 255, 255);
    }
};

static void renderGameplay(Renderer& renderer, const PlaceholderPlayer& player) {
    renderer.beginTopScreen(C2D_Color32(40, 60, 40, 255)); // dark green = stand-in "ground"

    // Draw a simple 16x16 box for the player. Real sprite draw call
    // (C2D_DrawImageAt with sprite sheet) replaces this in Phase 1.
    C2D_DrawRectSolid(player.x, player.y, 0.5f, 16.0f, 16.0f, player.colorForFacing());

    renderer.beginBottomScreen(C2D_Color32(20, 20, 20, 255));
    // Bottom screen UI is empty in Phase 0 — inventory/quest UI comes in Phase 5.
}

int main(int argc, char* argv[]) {
    Renderer renderer;
    if (!renderer.init()) {
        return 1;
    }

    StateManager stateManager;
    InputManager inputManager;
    Clock clock;
    PlaceholderPlayer player;

    stateManager.push(GameState::GAMEPLAY);

    consoleDebugInit(debugDevice_NULL); // keep stdout silent on release-style builds

    while (aptMainLoop()) {
        inputManager.update();
        float dt = clock.update();

        if (inputManager.isDown(KEY_START)) {
            break; // Start to quit, for dev convenience — remove before Phase 7 polish pass
        }

        switch (stateManager.current()) {
            case GameState::GAMEPLAY:
                player.update(inputManager, dt);
                break;
            default:
                break;
        }

        renderer.beginFrame();
        switch (stateManager.current()) {
            case GameState::GAMEPLAY:
                renderGameplay(renderer, player);
                break;
            default:
                break;
        }
        renderer.endFrame();
    }

    renderer.shutdown();
    return 0;
}
