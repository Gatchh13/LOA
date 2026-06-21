#pragma once

//-----------------------------------------------------------------------------
// Renderer.h  (Milestone 1)
//
// New additions over bootstrap:
//   - drawFade(alpha)       — full-screen black overlay for zone transitions
//   - drawZoneName(name, alpha) — zone name banner on zone entry
//   - drawTileMap uses per-zone bgColor for the screen clear
//   - Extended fallback color table covers new tile IDs
//-----------------------------------------------------------------------------

#include "../../include/types.h"
#include "../world/TileMap.h"
#include "Camera.h"

#include <citro2d.h>

class Renderer {
public:
    Renderer();
    ~Renderer();

    bool init();

    // Call at the start of each frame. bgColor = current zone's background color.
    void beginFrame(u32 bgColor);

    // Draw all visible tiles in the map.
    void drawTileMap(const TileMap& map, const Camera& cam);

    // Draw the player sprite at world position.
    void drawPlayer(float worldX, float worldY, const Camera& cam);

    // Draw FPS counter (top-left of top screen).
    void drawFPS(float fps);

    // Draw a full-screen black rectangle at the given alpha [0,1].
    // Call after all world geometry and before endFrame.
    void drawFade(float alpha);

    // Draw the zone name banner centered near the bottom of the top screen.
    // alpha [0,1] controls transparency.
    void drawZoneName(const char* name, float alpha);

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

    C2D_TextBuf m_textBuf;

    bool m_useFallbackColors;

    // Returns a fallback color for a given tile ID.
    static u32 fallbackColorForTile(u8 tileId);

    void drawColorRect(float sx, float sy, float w, float h, u32 color) const;
};
