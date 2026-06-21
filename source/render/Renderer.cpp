//-----------------------------------------------------------------------------
// Renderer.cpp
//
// citro2d coordinate system:
//   Origin (0,0) is TOP-LEFT of the screen.
//   X increases rightward, Y increases downward.
//   Z depth for 2D: use 0.5f for all sprites (ignored by citro2d DrawImage).
//
// Sprite sheet layout (tiles.t3x):
//   Each image in the sheet corresponds to one sprite.
//   Index 0 = grass tile (16x16)
//   Index 1 = wall  tile (16x16)
//   Index 2 = player    (16x16 for bootstrap; 32x32 in final art)
//
// If the sprite sheet cannot be loaded (no romfs assets yet), the renderer
// falls back to drawing solid-color rectangles so the bootstrap is always
// runnable even before any art exists.
//-----------------------------------------------------------------------------

#include "Renderer.h"
#include "../core/Logger.h"

#include <cstdio>
#include <cstring>

// Background clear color (dark grey)
static constexpr u32 COLOR_CLEAR  = C2D_Color32(30,  30,  30,  255);
// Fallback tile colors
static constexpr u32 COLOR_GRASS  = C2D_Color32(80,  140, 60,  255);
static constexpr u32 COLOR_WALL   = C2D_Color32(100, 80,  60,  255);
static constexpr u32 COLOR_PLAYER = C2D_Color32(200, 80,  80,  255);
static constexpr u32 COLOR_WHITE  = C2D_Color32(255, 255, 255, 255);

Renderer::Renderer()
    : m_ready(false)
    , m_topTarget(nullptr)
    , m_botTarget(nullptr)
    , m_spriteSheet(nullptr)
    , m_textBuf(nullptr)
    , m_font(nullptr)
    , m_useFallbackColors(true)
{}

Renderer::~Renderer() {
    if (m_textBuf)    C2D_TextBufDelete(m_textBuf);
    if (m_spriteSheet) C2D_SpriteSheetFree(m_spriteSheet);
    C2D_Fini();
    C3D_Fini();
    gfxExit();
}

bool Renderer::init() {
    // Initialize 3DS graphics hardware
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    // Create render targets for top and bottom screens
    m_topTarget = C2D_CreateScreenTarget(GFX_TOP,    GFX_LEFT);
    m_botTarget = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    if (!m_topTarget || !m_botTarget) {
        ERR("Failed to create render targets");
        return false;
    }

    // Allocate text buffer (holds up to 32 glyphs — enough for the FPS string)
    m_textBuf = C2D_TextBufNew(64);
    if (!m_textBuf) {
        ERR("Failed to create text buffer");
        return false;
    }

    // Attempt to load sprite sheet from romfs.
    // If it fails we run in fallback color mode — this is intentional so the
    // bootstrap works before any art assets exist.
    m_spriteSheet = C2D_SpriteSheetLoad("romfs:/gfx/tiles.t3x");
    if (m_spriteSheet) {
        // Sprite sheet loaded — retrieve individual images by index.
        // These indices match the order sprites were packed by tex3ds.
        size_t count = C2D_SpriteSheetCount(m_spriteSheet);
        if (count >= 3) {
            m_imgGrass  = C2D_SpriteSheetGetImage(m_spriteSheet, 0);
            m_imgWall   = C2D_SpriteSheetGetImage(m_spriteSheet, 1);
            m_imgPlayer = C2D_SpriteSheetGetImage(m_spriteSheet, 2);
            m_useFallbackColors = false;
            LOG("Sprite sheet loaded (%zu sprites)", count);
        } else {
            WARN("Sprite sheet has fewer than 3 sprites (%zu), using fallback colors", count);
        }
    } else {
        WARN("tiles.t3x not found — using fallback colors");
        m_useFallbackColors = true;
    }

    m_ready = true;
    LOG("Renderer initialized");
    return true;
}

void Renderer::beginFrame() {
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);

    // Clear top screen
    C2D_TargetClear(m_topTarget, COLOR_CLEAR);
    C2D_SceneBegin(m_topTarget);
}

void Renderer::drawColorRect(int sx, int sy, int w, int h, u32 color) const {
    C2D_DrawRectSolid(
        static_cast<float>(sx),
        static_cast<float>(sy),
        0.5f,
        static_cast<float>(w),
        static_cast<float>(h),
        color);
}

void Renderer::drawTileMap(const TileMap& map, const Camera& cam) {
    // Visible tile range
    int tx0 = cam.getX() / TILE_SIZE;
    int ty0 = cam.getY() / TILE_SIZE;
    int tx1 = tx0 + (SCREEN_TOP_W / TILE_SIZE) + 1;
    int ty1 = ty0 + (SCREEN_TOP_H / TILE_SIZE) + 1;

    // Clamp to map bounds
    if (tx0 < 0) tx0 = 0;
    if (ty0 < 0) ty0 = 0;
    if (tx1 > map.getWidthTiles())  tx1 = map.getWidthTiles();
    if (ty1 > map.getHeightTiles()) ty1 = map.getHeightTiles();

    for (int ty = ty0; ty < ty1; ty++) {
        for (int tx = tx0; tx < tx1; tx++) {
            int sx = tx * TILE_SIZE - cam.getX();
            int sy = ty * TILE_SIZE - cam.getY();
            u8  tile = map.getTile(tx, ty);

            if (m_useFallbackColors) {
                u32 color = (tile == TILE_WALL) ? COLOR_WALL : COLOR_GRASS;
                drawColorRect(sx, sy, TILE_SIZE, TILE_SIZE, color);
            } else {
                C2D_Image& img = (tile == TILE_WALL) ? m_imgWall : m_imgGrass;
                C2D_DrawImageAt(img,
                                static_cast<float>(sx),
                                static_cast<float>(sy),
                                0.5f);
            }
        }
    }
}

void Renderer::drawPlayer(float worldX, float worldY, const Camera& cam) {
    int sx = cam.worldToScreenX(worldX);
    int sy = cam.worldToScreenY(worldY);

    if (m_useFallbackColors) {
        // 16x16 red square (player)
        drawColorRect(sx, sy, 16, 16, COLOR_PLAYER);
        // Small white dot in the center to indicate facing (cosmetic)
        drawColorRect(sx + 6, sy + 6, 4, 4, COLOR_WHITE);
    } else {
        C2D_DrawImageAt(m_imgPlayer,
                        static_cast<float>(sx),
                        static_cast<float>(sy),
                        0.5f);
    }
}

void Renderer::drawFPS(float fps) {
    // Render to top screen (we're already in the top screen scene)
    char buf[32];
    snprintf(buf, sizeof(buf), "FPS: %.1f", fps);

    C2D_Text text;
    C2D_TextBufClear(m_textBuf);
    C2D_TextParse(&text, m_textBuf, buf);
    C2D_TextOptimize(&text);

    // Draw at top-left with a dark shadow for readability
    C2D_DrawText(&text, C2D_WithColor | C2D_AtBaseline,
                 3.0f, 13.0f, 0.6f,    // x, y, z
                 0.6f, 0.6f,            // scaleX, scaleY
                 C2D_Color32(0, 0, 0, 180));

    C2D_DrawText(&text, C2D_WithColor | C2D_AtBaseline,
                 2.0f, 12.0f, 0.7f,
                 0.6f, 0.6f,
                 C2D_Color32(255, 255, 80, 255));
}

void Renderer::endFrame() {
    // Draw bottom screen (black for now)
    C2D_TargetClear(m_botTarget, C2D_Color32(20, 20, 20, 255));
    C2D_SceneBegin(m_botTarget);
    // (bottom screen UI will be added in future milestones)

    C3D_FrameEnd(0);
}
