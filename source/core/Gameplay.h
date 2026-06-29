#pragma once

//-----------------------------------------------------------------------------
// Gameplay.h  (Milestone 12 — Engine Cleanup & Architecture Consolidation)
//
// Owns every system that only exists once gameplay has actually
// started (Player, ZoneManager, NPCManager, QuestManager, combat,
// shop, inventory, save state, message-timer banners) and the
// per-frame update/render logic that drives them. Extracted verbatim
// from main.cpp's while(aptMainLoop()) body — every line of logic
// moved here unchanged; only the owning scope changed from "locals in
// main()" to "members of Gameplay, methods on Gameplay."
//
// main.cpp keeps: Clock, InputManager, Renderer, StateManager,
// TitleScreen — none of these are gameplay-specific; the title screen
// needs Clock/InputManager/Renderer too, and StateManager/TitleScreen
// are what DECIDE whether Gameplay should be running at all. main.cpp
// becomes: construct everything, dispatch between TitleScreen and
// Gameplay based on StateManager's current state, shut down. Exactly
// the "keep main.cpp focused on initialization, the main loop, and
// shutdown" shape this milestone's brief asks for.
//
// Lifecycle:
//   Gameplay gameplay;                  // constructed once, before the loop
//   ...
//   gameplay.startGame(loadExisting);   // called when the player picks
//                                       // New Game / Continue on the title
//                                       // screen — was a lambda in main(),
//                                       // now a method.
//   ...
//   gameplay.update(input, dt);         // called every frame while in
//   gameplay.render(renderer, timeAccum); // GameState::GAMEPLAY.
//
// No behavior changes. Every constant, every branch, every comment
// explaining WHY a piece of logic exists was preserved verbatim — this
// milestone is about WHERE the code lives, not what it does.
//-----------------------------------------------------------------------------

#include "../../include/types.h"
#include "../core/WorldClock.h"
#include "../world/ZoneManager.h"
#include "../world/DayNight.h"
#include "../world/WorldObjectManager.h"
#include "../world/GatherNodeManager.h"
#include "../world/Shop.h"
#include "../combat/EnemyManager.h"
#include "../player/InventoryScreen.h"
#include "../render/Camera.h"
#include "../render/Renderer.h"
#include "../entities/Player.h"
#include "../npc/NPCManager.h"
#include "../quest/QuestManager.h"
#include "../player/PlayerState.h"
#include "../input/InputManager.h"

class Gameplay {
public:
    Gameplay();

    // Runs once, when the player picks New Game or Continue on the
    // title screen. Was main()'s startGame lambda — same logic,
    // same name, now a method instead of a closure.
    void startGame(bool loadExisting);

    // Runs every frame while GameState::GAMEPLAY is active. Contains
    // everything that used to be the second half of main()'s while
    // loop body, after the TITLE_SCREEN branch's `continue`.
    void update(const InputManager& input, float dt);

    // Draws everything gameplay-related — was the "Render" section of
    // main()'s loop body, after the title-screen `continue`. fps comes
    // from main.cpp's Clock (not owned here — Clock is needed by the
    // title screen too, so it stays in main.cpp; this is the one piece
    // of external data Gameplay's render needs but doesn't own).
    void render(Renderer& renderer, float timeAccum, float fps);

private:
    //--------------------------------------------------------------------------
    // Systems — constructed in dependency order, same order and same
    // reasoning as the original main()'s locals: Player has no default
    // constructor (position is required), so it's constructed here with
    // a placeholder that startGame() always overwrites before gameplay
    // actually begins — same pattern as ZoneManager/NPCManager/
    // WorldObjectManager, which are default-constructed and only
    // meaningfully set up via their own init() calls.
    //--------------------------------------------------------------------------
    PlayerState        m_playerState;
    QuestManager       m_questMgr;

    ZoneManager        m_zones;
    NPCManager         m_npcs;
    WorldObjectManager m_worldObjects;
    GatherNodeManager  m_gatherNodes;
    EnemyManager       m_enemies;
    ShopUI             m_shop;
    InventoryScreen    m_invScreen;
    Player             m_player;
    Camera             m_camera;

    WorldClock         m_worldClock;
    DayNight           m_dayNight;

    // Status message state (save/load notification)
    const char* m_statusMessage = nullptr;
    float       m_statusTimer   = 0.0f;
    static constexpr float STATUS_DURATION = 3.0f;

    // Shop feedback message state (purchase result) — same pattern as
    // m_statusMessage above, scoped separately since it's shown inside
    // the shop panel rather than as a top-screen banner.
    const char* m_shopMessage      = nullptr;
    float       m_shopMessageTimer = 0.0f;
    static constexpr float SHOP_MESSAGE_DURATION = 2.0f;

    // Combat feedback message state (attack/consumable results,
    // Milestone 8) — same message+timer pattern as everything else in
    // this file, shown via drawStatusMessage like the save/load banner.
    const char* m_combatMessage      = nullptr;
    float       m_combatMessageTimer = 0.0f;
    static constexpr float COMBAT_MESSAGE_DURATION = 2.0f;

    // Inventory screen feedback message (equip/unequip results,
    // Milestone 9) — same message+timer pattern, shown as a top-screen
    // banner via drawStatusMessage rather than inside the inventory
    // panel itself.
    const char* m_invMessage      = nullptr;
    float       m_invMessageTimer = 0.0f;
    static constexpr float INV_MESSAGE_DURATION = 2.0f;
};
