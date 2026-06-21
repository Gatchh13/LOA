//-----------------------------------------------------------------------------
// Renderer.cpp  (Milestone 2)
//-----------------------------------------------------------------------------

#include "Renderer.h"
#include "../core/Logger.h"

#include <cstdio>
#include <cstring>
#include <algorithm>

//-----------------------------------------------------------------------------
// Fallback colors (no sprite sheet)
//-----------------------------------------------------------------------------
u32 Renderer::fallbackColorForTile(u8 tileId) {
    switch (tileId) {
        case TILE_GRASS:        return C2D_Color32( 80, 140,  60, 255);
        case TILE_DIRT:         return C2D_Color32(140, 110,  70, 255);
        case TILE_FOREST_FLOOR: return C2D_Color32( 50,  90,  40, 255);
        case TILE_STONE_FLOOR:  return C2D_Color32( 80,  80,  90, 255);
        case TILE_WATER:        return C2D_Color32( 50,  80, 160, 255);
        case TILE_WALL:         return C2D_Color32(100,  80,  60, 255);
        case TILE_TREE:         return C2D_Color32( 30,  70,  20, 255);
        case TILE_STONE_WALL:   return C2D_Color32( 55,  55,  65, 255);
        case TILE_FENCE:        return C2D_Color32(130, 100,  50, 255);
        default:                return C2D_Color32(255,   0, 255, 255);
    }
}

//-----------------------------------------------------------------------------
Renderer::Renderer()
    : m_ready(false)
    , m_topTarget(nullptr)
    , m_botTarget(nullptr)
    , m_spriteSheet(nullptr)
    , m_textBuf(nullptr)
    , m_useFallbackColors(true)
    , m_dialogueDrawnThisFrame(false)
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

    // 256 glyph buffer: clock + FPS + zone name + dialogue + NPC names
    m_textBuf = C2D_TextBufNew(256);
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
            WARN("Sprite sheet too small, using fallback colors");
        }
    } else {
        WARN("tiles.t3x not found — using fallback colors");
    }

    m_ready = true;
    LOG("Renderer initialized (Milestone 2)");
    return true;
}

//-----------------------------------------------------------------------------
void Renderer::beginFrame(u32 bgColor) {
    m_dialogueDrawnThisFrame = false;
    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C2D_TargetClear(m_topTarget, bgColor);
    C2D_SceneBegin(m_topTarget);
    // Clear the text buffer once per frame here — all text calls append to it.
    C2D_TextBufClear(m_textBuf);
}

//-----------------------------------------------------------------------------
void Renderer::drawColorRect(float sx, float sy, float w, float h, u32 color) const {
    C2D_DrawRectSolid(sx, sy, 0.5f, w, h, color);
}

//-----------------------------------------------------------------------------
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
            float sx   = static_cast<float>(tx * TILE_SIZE - cam.getX());
            float sy   = static_cast<float>(ty * TILE_SIZE - cam.getY());
            u8    tile = map.getTile(tx, ty);

            if (m_useFallbackColors) {
                drawColorRect(sx, sy, TILE_SIZE, TILE_SIZE, fallbackColorForTile(tile));
            } else {
                if (tile < TILE_FIRST_SOLID && tile != TILE_WATER) {
                    C2D_DrawImageAt(m_imgGrass, sx, sy, 0.5f);
                } else if (tile >= TILE_FIRST_SOLID) {
                    C2D_DrawImageAt(m_imgWall,  sx, sy, 0.5f);
                } else {
                    drawColorRect(sx, sy, TILE_SIZE, TILE_SIZE, fallbackColorForTile(tile));
                }
            }
        }
    }
}

//-----------------------------------------------------------------------------
// NPC sprite: 14×14 blue square with a white center dot.
// Distinct from the player (red) so they're easy to tell apart.
//-----------------------------------------------------------------------------
void Renderer::drawNPCSprite(float sx, float sy) const {
    drawColorRect(sx,     sy,     14, 14, C2D_Color32( 60, 100, 200, 255));
    drawColorRect(sx + 5, sy + 5,  4,  4, C2D_Color32(220, 230, 255, 255));
}

void Renderer::drawNPCs(const NPCManager& mgr, ZoneID currentZone, const Camera& cam) {
    const NPC* npcs = mgr.getNPCs();
    int count       = mgr.getNPCCount();

    for (int i = 0; i < count; i++) {
        const NPC& npc = npcs[i];
        if (!npc.active)                  continue;
        if (npc.home_zone != currentZone)  continue;

        float sx = npc.pos_x - static_cast<float>(cam.getX());
        float sy = npc.pos_y - static_cast<float>(cam.getY());

        // Cull off-screen NPCs (with 1-tile margin)
        if (sx < -TILE_SIZE || sx > SCREEN_TOP_W + TILE_SIZE) continue;
        if (sy < -TILE_SIZE || sy > SCREEN_TOP_H + TILE_SIZE) continue;

        drawNPCSprite(sx, sy);

        // Draw NPC name above sprite when in dialogue range (optional debug aid)
        // We skip this to keep the text buffer usage low and avoid clutter.
    }
}

//-----------------------------------------------------------------------------
void Renderer::drawPlayer(float worldX, float worldY, const Camera& cam) {
    float sx = static_cast<float>(cam.worldToScreenX(worldX));
    float sy = static_cast<float>(cam.worldToScreenY(worldY));

    if (m_useFallbackColors) {
        drawColorRect(sx,     sy,     16, 16, C2D_Color32(200,  80,  80, 255));
        drawColorRect(sx + 6, sy + 6,  4,  4, C2D_Color32(255, 255, 255, 255));
    } else {
        C2D_DrawImageAt(m_imgPlayer, sx, sy, 0.5f);
    }
}

//-----------------------------------------------------------------------------
void Renderer::drawClockDebug(float fps, int hour, int minute, const char* phase) {
    // Format: "FPS:60.0  08:30 Dawn"
    char buf[64];
    snprintf(buf, sizeof(buf), "FPS:%.0f  %02d:%02d %s", fps, hour, minute, phase);

    C2D_Text text;
    C2D_TextParse(&text, m_textBuf, buf);
    C2D_TextOptimize(&text);

    // Shadow
    C2D_DrawText(&text, C2D_WithColor | C2D_AtBaseline,
                 3.0f, 13.0f, 0.6f, 0.55f, 0.55f,
                 C2D_Color32(0, 0, 0, 180));
    // Text
    C2D_DrawText(&text, C2D_WithColor | C2D_AtBaseline,
                 2.0f, 12.0f, 0.7f, 0.55f, 0.55f,
                 C2D_Color32(255, 255, 80, 255));
}

//-----------------------------------------------------------------------------
void Renderer::drawTint(u32 color) {
    // Alpha is the lowest byte in C2D_Color32(r,g,b,a)
    u8 alpha = color & 0xFF;
    if (alpha == 0) return;
    drawColorRect(0.0f, 0.0f,
                  static_cast<float>(SCREEN_TOP_W),
                  static_cast<float>(SCREEN_TOP_H),
                  color);
}

//-----------------------------------------------------------------------------
void Renderer::drawFade(float alpha) {
    if (alpha <= 0.0f) return;
    u8 a = static_cast<u8>(std::min(alpha, 1.0f) * 255.0f);
    drawColorRect(0.0f, 0.0f,
                  static_cast<float>(SCREEN_TOP_W),
                  static_cast<float>(SCREEN_TOP_H),
                  C2D_Color32(0, 0, 0, a));
}

//-----------------------------------------------------------------------------
void Renderer::drawZoneName(const char* name, float alpha) {
    if (alpha <= 0.0f || !name) return;
    u8 a = static_cast<u8>(std::min(alpha, 1.0f) * 255.0f);

    float bannerY = static_cast<float>(SCREEN_TOP_H) - 36.0f;
    drawColorRect(0.0f, bannerY,
                  static_cast<float>(SCREEN_TOP_W), 28.0f,
                  C2D_Color32(0, 0, 0, static_cast<u8>(a * 0.7f)));

    C2D_Text text;
    C2D_TextParse(&text, m_textBuf, name);
    C2D_TextOptimize(&text);

    float tw = 0.0f, th = 0.0f;
    C2D_TextGetDimensions(&text, 0.7f, 0.7f, &tw, &th);
    float tx = (static_cast<float>(SCREEN_TOP_W) - tw) * 0.5f;
    float ty = bannerY + (28.0f + th) * 0.5f;

    C2D_DrawText(&text, C2D_WithColor | C2D_AtBaseline,
                 tx + 1.0f, ty + 1.0f, 0.7f, 0.7f, 0.7f,
                 C2D_Color32(0, 0, 0, a));
    C2D_DrawText(&text, C2D_WithColor | C2D_AtBaseline,
                 tx, ty, 0.8f, 0.7f, 0.7f,
                 C2D_Color32(255, 230, 150, a));
}

//-----------------------------------------------------------------------------
// drawDialogue — drawn on the BOTTOM screen (320×240)
//
// Layout:
//   Dark panel covering the top ~80px of the bottom screen.
//   NPC name in bold yellow at top-left.
//   Dialogue text in white below.
//   "A: Close" hint at bottom-right.
//
// Switching to the bottom screen target happens here.
// endFrame switches back implicitly (it clears/begins botTarget).
// To avoid double-clearing, we track m_dialogueDrawnThisFrame.
//-----------------------------------------------------------------------------
void Renderer::drawDialogue(const NPC* npc) {
    if (!npc) return;

    // Switch to bottom screen
    C2D_SceneBegin(m_botTarget);
    C2D_TargetClear(m_botTarget, C2D_Color32(20, 20, 20, 255));

    // Dark dialogue panel
    drawColorRect(0.0f, 0.0f,
                  static_cast<float>(SCREEN_BOT_W), 90.0f,
                  C2D_Color32(10, 10, 20, 230));

    // Border line
    drawColorRect(0.0f, 88.0f,
                  static_cast<float>(SCREEN_BOT_W), 2.0f,
                  C2D_Color32(100, 100, 160, 255));

    // NPC name
    C2D_Text nameText;
    C2D_TextParse(&nameText, m_textBuf, npc->name);
    C2D_TextOptimize(&nameText);
    C2D_DrawText(&nameText, C2D_WithColor | C2D_AtBaseline,
                 10.0f, 22.0f, 0.8f, 0.65f, 0.65f,
                 C2D_Color32(255, 220, 80, 255));

    // Dialogue text (word-wrap not implemented — keep dialogue lines short)
    C2D_Text dlgText;
    C2D_TextParse(&dlgText, m_textBuf, npc->dialogue);
    C2D_TextOptimize(&dlgText);
    C2D_DrawText(&dlgText, C2D_WithColor | C2D_AtBaseline,
                 10.0f, 52.0f, 0.7f, 0.55f, 0.55f,
                 C2D_Color32(230, 230, 230, 255));

    // Close hint
    C2D_Text hintText;
    C2D_TextParse(&hintText, m_textBuf, "A: Close");
    C2D_TextOptimize(&hintText);
    C2D_DrawText(&hintText, C2D_WithColor | C2D_AtBaseline,
                 260.0f, 82.0f, 0.6f, 0.5f, 0.5f,
                 C2D_Color32(140, 140, 140, 255));

    m_dialogueDrawnThisFrame = true;

    // Switch back to top screen for remaining draw calls
    C2D_SceneBegin(m_topTarget);
}

//-----------------------------------------------------------------------------
void Renderer::endFrame() {
    // Draw bottom screen (if dialogue was already drawn, skip re-clearing)
    if (!m_dialogueDrawnThisFrame) {
        C2D_TargetClear(m_botTarget, C2D_Color32(20, 20, 20, 255));
        C2D_SceneBegin(m_botTarget);
    }
    C3D_FrameEnd(0);
}
