#pragma once

//-----------------------------------------------------------------------------
// Renderer.h
// Wraps citro2d initialization and draw calls.
//
// Usage per frame:
//   renderer.beginFrame();
//   renderer.drawTileMap(...);
//   renderer.drawSprite(...);
//   renderer.drawFPS(...);
//   renderer.endFrame();
//-----------------------------------------------------------------------------

#include "../../include/types.h"
#include "../world/TileMap.h"
#include "Camera.h"

#include <citro2d.h>

class Renderer {
public:
    Renderer();
    ~Renderer();

    // Initialize citro2d and load textures.
    // Returns false on failure.
    bool init();

    // Call at the start of each frame.
    void beginFrame();

    // Draw the tile map relative to the camera.
    void drawTileMap(const TileMap& map, const Camera& cam);

    // Draw the player sprite at a world position.
    void drawPlayer(float worldX, float worldY, const Camera& cam);

    // Draw the FPS counter in the top-left corner.
    void drawFPS(float fps);

    // Flush and swap buffers. Call at the end of each frame.
    void endFrame();

    // True once init() succeeds.
    bool isReady() const { return m_ready; }

private:
    bool            m_ready;
    C3D_RenderTarget* m_topTarget;
    C3D_RenderTarget* m_botTarget;

    // Sprite sheet texture (loaded from romfs)
    C2D_SpriteSheet m_spriteSheet;

    // Sprite image handles (indices into the sprite sheet)
    C2D_Image m_imgGrass;
    C2D_Image m_imgWall;
    C2D_Image m_imgPlayer;

    // Text / FPS rendering
    C2D_TextBuf m_textBuf;
    C2D_Font    m_font;       // nullptr = use built-in system font

    // Solid-color tile fallback (used if textures fail to load)
    bool m_useFallbackColors;

    // Draw a solid 16x16 colored rect at screen position (sx, sy)
    void drawColorRect(int sx, int sy, int w, int h, u32 color) const;
};
