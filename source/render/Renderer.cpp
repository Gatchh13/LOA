//-----------------------------------------------------------------------------
// Renderer.cpp  (Milestone 1)
//-----------------------------------------------------------------------------

#include "Renderer.h"
#include "../core/Logger.h"
#include "../../include/types.h"

#include <cstdio>
#include <cstring>
#include <algorithm>

//-----------------------------------------------------------------------------
// Fallback color table (used when no sprite sheet is loaded)
// Indexed by tile ID. IDs not listed default to a magenta error color.
//-----------------------------------------------------------------------------
u32 Renderer::fallbackColorForTile(u8 tileId) {
    switch (tileId) {
        case TILE_GRASS:        return C2D_Color32( 80, 140,  60, 255); // green
        case TILE_DIRT:         return C2D_Color32(140, 110,  70, 255); // brown
        case TILE_FOREST_FLOOR: return C2D_Color32( 50,  90,  40, 255); // dark green
        case TILE_STONE_FLOOR:  return C2D_Color32( 80,  80,  90, 255); // grey
        case TILE_WATER:        return C2D_Color32( 50,  80, 160, 255); // blue
        case TILE_WALL:         return C2D_Color32(100,  80,  60, 255); // tan
        case TILE_TREE:         return C2D_Color32( 30,  70,  20, 255); // dark green
        case TILE_STONE_WALL:   return C2D_Color32( 55,  55,  65, 255); // dark grey
        case TILE_FENCE:        return C2D_Color32(130, 100,  50, 255); // wood
        default:                return C2D_Color32(255,   0, 255, 255); // error magenta
    }
}

Renderer::Renderer()
    : m_ready(false)
    , m_topTarget(nullptr)
    , m_botTarget(nullptr)
    , m_spriteSheet(nullptr)
    , m_textBuf(nullptr)
    , m_useFallbackColors(true)
{}

Renderer::~Renderer() {
    if (m_textBuf)     C2D_TextBufDelete(m_textBuf);
    if (m_spriteSheet) C2D_SpriteSheetFree(m_spriteSheet);
    C2D_Fini();
    C3D_Fini();
    gfxExit();
}

bool Renderer::init() {
    gfxInitDefault();
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    m_topTarget = C2D_CreateScreenTarget(GFX_TOP,    GFX_LEFT);
    m_botTarget = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);

    if (!m_topTarget || !m_botTarget) {
        ERR("Failed to create render targets");
        return false;
    }

    // Larger text buffer for zone name (zone name + FPS)
    m_textBuf = C2D_TextBufNew(128);
    if (!m_textBuf) {
        ERR("Failed to create text buffer");
        return false;
    }

    m_spriteSheet = C2D_SpriteSheetLoad("romfs:/gfx/tiles.t3x");
    if (m_spriteSheet) {
        size_t count = C2D_SpriteSheetCount(m_spriteSheet);
        if (count >= 3) {
            m_imgGrass  = C2D_SpriteSheetGetImage(m_spriteSheet, 0);
            m_imgWall   = C2D_SpriteSheetGetImage(m_spriteSheet, 1);
            m_imgPlayer = C2D_SpriteSheetGetImage(m_spriteSheet, 2);
            m_useFallbackColors = false;
            LOG("Sprite sheet loaded (%zu sprites)", count);
        } else {
            WARN("Sprite sheet too small (%zu sprites), using fallback colors", count);
        }
    } else {
        WARN("tiles.t3x not found — using fallback colors");
        m_useFallbackColors = true;
    }

    m_ready = true;
    LOG("Renderer initialized");
    return true;
}

void Renderer::beginFrame(u32 bgColor) {
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C2D_TargetClear(m_topTarget, bgColor);
    C2D_SceneBegin(m_topTarget);
}

void Renderer::drawColorRect(float sx, float sy, float w, float h, u32 color) const {
    C2D_DrawRectSolid(sx, sy, 0.5f, w, h, color);
}

void Renderer::drawTileMap(const TileMap& map, const Camera& cam) {
    int tx0 = cam.getX() / TILE_SIZE;
    int ty0 = cam.getY() / TILE_SIZE;
    int tx1 = tx0 + (SCREEN_TOP_W / TILE_SIZE) + 2;
    int ty1 = ty0 + (SCREEN_TOP_H / TILE_SIZE) + 2;

    if (tx0 < 0) tx0 = 0;
    if (ty0 < 0) ty0 = 0;
    if (tx1 > map.getWidthTiles())  tx1 = map.getWidthTiles();
    if (ty1 > map.getHeightTiles()) ty1 = map.getHeightTiles();

    for (int ty = ty0; ty < ty1; ty++) {
        for (int tx = tx0; tx < tx1; tx++) {
            float sx = static_cast<float>(tx * TILE_SIZE - cam.getX());
            float sy = static_cast<float>(ty * TILE_SIZE - cam.getY());
            u8    tile = map.getTile(tx, ty);

            if (m_useFallbackColors) {
                drawColorRect(sx, sy, TILE_SIZE, TILE_SIZE, fallbackColorForTile(tile));
            } else {
                // With a real sprite sheet, select image by tile ID.
                // For bootstrap art (only grass/wall/player), fall through to
                // color for extended tile types.
                if (tile == TILE_GRASS || tile == TILE_DIRT ||
                    tile == TILE_FOREST_FLOOR || tile == TILE_STONE_FLOOR)
                {
                    C2D_DrawImageAt(m_imgGrass,
                                    sx, sy, 0.5f);
                } else if (tile >= TILE_FIRST_SOLID) {
                    C2D_DrawImageAt(m_imgWall,
                                    sx, sy, 0.5f);
                } else {
                    // Water and other special ground tiles — fallback color
                    drawColorRect(sx, sy, TILE_SIZE, TILE_SIZE, fallbackColorForTile(tile));
                }
            }
        }
    }
}

void Renderer::drawPlayer(float worldX, float worldY, const Camera& cam) {
    float sx = static_cast<float>(cam.worldToScreenX(worldX));
    float sy = static_cast<float>(cam.worldToScreenY(worldY));

    if (m_useFallbackColors) {
        drawColorRect(sx, sy, 16, 16, C2D_Color32(200, 80, 80, 255));
        drawColorRect(sx + 6, sy + 6, 4, 4, C2D_Color32(255, 255, 255, 255));
    } else {
        C2D_DrawImageAt(m_imgPlayer, sx, sy, 0.5f);
    }
}

void Renderer::drawFPS(float fps) {
    char buf[32];
    snprintf(buf, sizeof(buf), "FPS: %.1f", fps);

    C2D_Text text;
    C2D_TextBufClear(m_textBuf);
    C2D_TextParse(&text, m_textBuf, buf);
    C2D_TextOptimize(&text);

    C2D_DrawText(&text, C2D_WithColor | C2D_AtBaseline,
                 3.0f, 13.0f, 0.6f, 0.6f, 0.6f,
                 C2D_Color32(0, 0, 0, 180));
    C2D_DrawText(&text, C2D_WithColor | C2D_AtBaseline,
                 2.0f, 12.0f, 0.7f, 0.6f, 0.6f,
                 C2D_Color32(255, 255, 80, 255));
}

void Renderer::drawFade(float alpha) {
    if (alpha <= 0.0f) return;
    u8 a = static_cast<u8>(std::min(alpha, 1.0f) * 255.0f);
    drawColorRect(0.0f, 0.0f,
                  static_cast<float>(SCREEN_TOP_W),
                  static_cast<float>(SCREEN_TOP_H),
                  C2D_Color32(0, 0, 0, a));
}

void Renderer::drawZoneName(const char* name, float alpha) {
    if (alpha <= 0.0f || !name) return;

    u8 a = static_cast<u8>(std::min(alpha, 1.0f) * 255.0f);

    // Dark background banner
    float bannerY = static_cast<float>(SCREEN_TOP_H) - 36.0f;
    drawColorRect(0.0f, bannerY,
                  static_cast<float>(SCREEN_TOP_W), 28.0f,
                  C2D_Color32(0, 0, 0, static_cast<u8>(a * 0.7f)));

    // Zone name text, centered
    C2D_Text text;
    // Use a fresh buffer section — FPS may have been drawn already this frame.
    // TextBuf is cleared at the start of drawFPS; here we just append.
    C2D_TextParse(&text, m_textBuf, name);
    C2D_TextOptimize(&text);

    // Measure width to center
    float tw = 0.0f, th = 0.0f;
    C2D_TextGetDimensions(&text, 0.7f, 0.7f, &tw, &th);

    float tx = (static_cast<float>(SCREEN_TOP_W) - tw) * 0.5f;
    float ty = bannerY + (28.0f + th) * 0.5f;

    // Shadow
    C2D_DrawText(&text, C2D_WithColor | C2D_AtBaseline,
                 tx + 1.0f, ty + 1.0f, 0.7f, 0.7f, 0.7f,
                 C2D_Color32(0, 0, 0, a));
    // Main text
    C2D_DrawText(&text, C2D_WithColor | C2D_AtBaseline,
                 tx, ty, 0.8f, 0.7f, 0.7f,
                 C2D_Color32(255, 230, 150, a));
}

void Renderer::endFrame() {
    C2D_TargetClear(m_botTarget, C2D_Color32(20, 20, 20, 255));
    C2D_SceneBegin(m_botTarget);
    C3D_FrameEnd(0);
}
