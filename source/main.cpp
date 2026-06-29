//-----------------------------------------------------------------------------
// main.cpp  (Milestone 12 — Engine Cleanup & Architecture Consolidation)
//
// Milestone 12 extracted all gameplay-specific state and update/render
// logic into source/core/Gameplay.{h,cpp} — every line of logic that
// used to live in this file's while(aptMainLoop()) body moved there
// verbatim (see Gameplay.cpp's header comment). main.cpp now owns only
// what genuinely isn't gameplay-specific: Clock (needed by the title
// screen too), InputManager, Renderer, StateManager, TitleScreen — and
// its job is reduced to initialization, the main loop's dispatch
// between TitleScreen and Gameplay, and shutdown. No behavior changed;
// only where the code lives.
//
// Previous milestones' changelog (kept for history — see each
// milestone's own design doc for full detail):
//   M11: NPCs acknowledge completed quests; fixed a fairness bug where
//        contact damage applied while a menu was open with no way to
//        respond; death now closes any open menu before respawning.
//   M10: item/quest/NPC/enemy definitions split into databases
//        (source/data/, source/quest/data/, etc.); added a second
//        quest, "The Town Well."
//   M9:  Selling, equipment (weapon/armor slots, 3 new items), the
//        inventory/equip screen (START), attack/defense HUD line.
//   M8:  HP, the Forest Wolf (Idle/Chase/Attack/Return AI), melee
//        attack (X), consumables (L/R), death/respawn.
//   M7:  Inventory, the shop (Y while talking to Mira), quest item
//        rewards.
//
// Button map (gameplay):
//   D-Pad / Circle Pad : move
//   A                  : interact (NPC, world object, gather node) / buy (shop) / equip-unequip (inventory)
//   B                  : close dialogue / leave shop / exit inventory
//   X                  : melee attack / sell (in shop)
//   L                  : use Healing Herb (+20 HP)
//   R                  : use Simple Potion (+50 HP)
//   Y                  : open shop (only while talking to Mira)
//   START              : open inventory/equip screen
//   SELECT             : save
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
#include "core/GameState.h"
#include "core/TitleScreen.h"
#include "core/Gameplay.h"
#include "input/InputManager.h"
#include "render/Renderer.h"
#include "save/SaveManager.h"

int main() {
    romfsInit();

    Logger::init();
    LOG("Legends of Aetheria — Milestone 12: Engine Cleanup & Architecture Consolidation");

    Renderer renderer;
    if (!renderer.init()) {
        ERR("Renderer failed to initialize. Aborting.");
        romfsExit();
        return 1;
    }

    Clock        clock;
    InputManager input;

    Gameplay     gameplay;
    StateManager state;       // starts at GameState::BOOT
    TitleScreen  titleScreen;

    float timeAccum = 0.0f;

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
                gameplay.startGame(/*loadExisting=*/false);
                state.push(GameState::GAMEPLAY);
            } else if (result == TitleResult::START_LOAD) {
                gameplay.startGame(/*loadExisting=*/true);
                state.push(GameState::GAMEPLAY);
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
        gameplay.update(input, dt);
        gameplay.render(renderer, timeAccum, clock.getFPS());
        renderer.endFrame();
    }

    LOG("Clean shutdown");
    romfsExit();
    return 0;
}
