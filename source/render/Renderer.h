#pragma once

//-----------------------------------------------------------------------------
// Renderer.h  (Milestone 3)
//
// Additions over Milestone 2:
//   drawQuestMarker(...)    — pulsing diamond marker on the top screen
//   drawQuestHUD(...)       — objective text on bottom screen (no dialogue)
//
// Render order (top screen):
//   1. Tile map
//   2. NPCs
//   3. Quest marker (if active in current zone)
//   4. Player
//   5. FPS + clock debug
//   6. Day/night tint
//   7. Zone transition fade
//   8. Zone name banner
//
// Bottom screen:
//   Dialogue box (when active), otherwise quest HUD.
//-----------------------------------------------------------------------------

#include "../../include/types.h"
#include "../world/TileMap.h"
#include "../world/WorldObject.h"
#include "../npc/NPC.h"
#include "../npc/NPCManager.h"
#include "Camera.h"

#include <citro2d.h>

class Renderer {
public:
    Renderer();
    ~Renderer();

    bool init();

    // beginFrame clears the top screen with the zone's background color.
    void beginFrame(u32 bgColor);

    // Draw all visible tiles.
    void drawTileMap(const TileMap& map, const Camera& cam);

    // Draw all active NPCs in the current zone.
    void drawNPCs(const NPCManager& npcs, ZoneID currentZone, const Camera& cam);

    // Draw the player sprite.
    void drawPlayer(float worldX, float worldY, const Camera& cam);

    // Draw FPS counter and world clock (HH:MM + phase) in debug area.
    void drawClockDebug(float fps, int hour, int minute, const char* phase);

    // Draw day/night tint overlay. color.alpha drives intensity.
    // Call after world geometry, before fade and UI.
    void drawTint(u32 color);

    // Draw full-screen black fade [0,1].
    void drawFade(float alpha);

    // Draw zone name banner.
    void drawZoneName(const char* name, float alpha);

    // Draw dialogue box on bottom screen.
    // npc: the NPC currently speaking (must not be null).
    void drawDialogue(const NPC* npc);

    // Draw world objects (bridges, ladders, obstacles) in the current zone.
    // objects/count from WorldObjectManager::getObjects() / getObjectCount().
    void drawWorldObjects(const WorldObject* objects, int count,
                          ZoneID currentZone, const Camera& cam);

    // Draw quest objective text, gold, and resources on bottom screen.
    // objectiveText: nullptr = show "No active quest".
    void drawQuestHUD(const char* objectiveText, u32 gold, u8 wood, u8 rope);

    // Draw a centered status message on the top screen (e.g. "Game Saved").
    // alpha [0,1] for fade in/out. Call after world geometry, before endFrame.
    void drawStatusMessage(const char* text, float alpha);

    // Draw a pulsing diamond marker at a world position.
    // Used to show REACH_MARKER objective locations.
    // timeAccum: running total real seconds (for pulse animation).
    void drawQuestMarker(float worldX, float worldY,
                         const Camera& cam, float timeAccum);

    void endFrame();

    bool isReady() const { return m_ready; }

private:
    bool              m_ready;
    C3D_RenderTarget* m_topTarget;
    C3D_RenderTarget* m_botTarget;

    C2D_SpriteSheet m_spriteSheet;
    C2D_Image       m_imgGrass;
    C2D_Image       m_imgWall;
    C2D_Image       m_imgPlayer;

    // Larger text buffer: FPS(12) + clock(10) + phase(10) + zone name(24)
    // + dialogue(80) + NPC names(20) + padding = 256 glyphs
    C2D_TextBuf m_textBuf;

    bool m_useFallbackColors;
    bool m_dialogueDrawnThisFrame; // guards bottom screen draw

    static u32  fallbackColorForTile(u8 tileId);
    void        drawColorRect(float sx, float sy, float w, float h, u32 color) const;
    void        drawNPCSprite(float sx, float sy) const;
};

