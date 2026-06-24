//-----------------------------------------------------------------------------
// Renderer.cpp  (Milestone 2)
//-----------------------------------------------------------------------------

#include "Renderer.h"
#include "../world/WorldObject.h"
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
// drawDirectionalIndicator  (Milestone 6 — animation foundation)
//
// Fallback-mode visual for facing + walk animation, used until a real
// sprite sheet exists. Draws a small notch on the edge of the entity's
// bounding box in the direction it's facing, and applies a tiny offset
// keyed off animFrame so movement is visibly distinct from standing still
// even with placeholder colored-rect art.
//
// animFrame is 0 (idle) or 1..FRAMES_PER_ANIM-1 (walking). The offset
// alternates by frame parity — cheap and reads as "stepping" at normal
// walk speed.
//-----------------------------------------------------------------------------
void Renderer::drawDirectionalIndicator(float sx, float sy, float w, float h,
                                        Facing facing, u8 animFrame, u32 color) const {
    constexpr float NOTCH = 3.0f;
    float cx = sx + w * 0.5f;
    float cy = sy + h * 0.5f;

    // Walking bob: shift the notch slightly on odd frames.
    // Idle (frame 0) always uses the resting offset.
    float bob = (animFrame != 0 && (animFrame % 2) == 1) ? 1.0f : 0.0f;

    switch (facing) {
        case Facing::DOWN:
            drawColorRect(cx - NOTCH * 0.5f, sy + h - NOTCH - bob, NOTCH, NOTCH, color);
            break;
        case Facing::UP:
            drawColorRect(cx - NOTCH * 0.5f, sy + bob, NOTCH, NOTCH, color);
            break;
        case Facing::LEFT:
            drawColorRect(sx + bob, cy - NOTCH * 0.5f, NOTCH, NOTCH, color);
            break;
        case Facing::RIGHT:
            drawColorRect(sx + w - NOTCH - bob, cy - NOTCH * 0.5f, NOTCH, NOTCH, color);
            break;
    }
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
// NPC sprite: 14×14 blue square with a white center dot, plus a small
// dark notch showing facing direction (Milestone 6).
// Distinct from the player (red) so they're easy to tell apart.
//-----------------------------------------------------------------------------
void Renderer::drawNPCSprite(float sx, float sy, Facing facing, u8 animFrame) const {
    drawColorRect(sx,     sy,     14, 14, C2D_Color32( 60, 100, 200, 255));
    drawColorRect(sx + 5, sy + 5,  4,  4, C2D_Color32(220, 230, 255, 255));
    drawDirectionalIndicator(sx, sy, 14, 14, facing, animFrame,
                             C2D_Color32(20, 30, 70, 255));
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

        drawNPCSprite(sx, sy, npc.anim.facing, npc.anim.frame);

        // Draw NPC name above sprite when in dialogue range (optional debug aid)
        // We skip this to keep the text buffer usage low and avoid clutter.
    }
}

//-----------------------------------------------------------------------------
void Renderer::drawPlayer(float worldX, float worldY, const Camera& cam,
                          Facing facing, u8 animFrame) {
    float sx = static_cast<float>(cam.worldToScreenX(worldX));
    float sy = static_cast<float>(cam.worldToScreenY(worldY));

    if (m_useFallbackColors) {
        drawColorRect(sx,     sy,     16, 16, C2D_Color32(200,  80,  80, 255));
        drawColorRect(sx + 6, sy + 6,  4,  4, C2D_Color32(255, 255, 255, 255));
        drawDirectionalIndicator(sx, sy, 16, 16, facing, animFrame,
                                 C2D_Color32(90, 20, 20, 255));
    } else {
        // TODO(sprite sheet): once tiles.t3x includes a walk-cycle strip,
        // select the image by (facing, animFrame) instead of always
        // drawing m_imgPlayer. See AnimState.h for the indexing scheme.
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
// drawTitleScreen  (Milestone 6 — Foundation of Feel)
//
// Pure top-screen draw — no tile map, no camera, nothing gameplay-related
// is touched here. Three rows (New Game / Continue / Credits) are drawn
// as plain text with a triangular cursor next to whichever is selected.
// Continue is drawn at reduced alpha when hasSave is false, matching how
// drawFade/drawFooter etc. already encode "dimmed" as a literal alpha
// value rather than a separate disabled-color palette.
//-----------------------------------------------------------------------------
void Renderer::drawTitleScreen(TitleOption selected, bool hasSave,
                               bool showingCredits, float timeAccum) {
    // No background rect needed here — beginFrame() already cleared the
    // top screen to this color via C2D_TargetClear before this is called.

    if (showingCredits) {
        C2D_Text title;
        C2D_TextParse(&title, m_textBuf, "Credits");
        C2D_TextOptimize(&title);
        float tw = 0.0f, th = 0.0f;
        C2D_TextGetDimensions(&title, 0.9f, 0.9f, &tw, &th);
        float tx = (static_cast<float>(SCREEN_TOP_W) - tw) * 0.5f;
        C2D_DrawText(&title, C2D_WithColor | C2D_AtBaseline,
                     tx, 40.0f, 0.7f, 0.9f, 0.9f,
                     C2D_Color32(255, 230, 150, 255));

        static const char* lines[] = {
            "Legends of Aetheria",
            "A solo homebrew project",
            "",
            "Press A or B to return",
        };
        float y = 90.0f;
        for (const char* line : lines) {
            C2D_Text t;
            C2D_TextParse(&t, m_textBuf, line);
            C2D_TextOptimize(&t);
            float lw = 0.0f, lh = 0.0f;
            C2D_TextGetDimensions(&t, 0.55f, 0.55f, &lw, &lh);
            float lx = (static_cast<float>(SCREEN_TOP_W) - lw) * 0.5f;
            C2D_DrawText(&t, C2D_WithColor | C2D_AtBaseline,
                         lx, y, 0.6f, 0.55f, 0.55f,
                         C2D_Color32(220, 220, 220, 255));
            y += 22.0f;
        }
        return;
    }

    // Title
    C2D_Text title;
    C2D_TextParse(&title, m_textBuf, "Legends of Aetheria");
    C2D_TextOptimize(&title);
    float tw = 0.0f, th = 0.0f;
    C2D_TextGetDimensions(&title, 1.1f, 1.1f, &tw, &th);
    float titleX = (static_cast<float>(SCREEN_TOP_W) - tw) * 0.5f;
    C2D_DrawText(&title, C2D_WithColor | C2D_AtBaseline,
                 titleX + 1.0f, 71.0f, 0.5f, 1.1f, 1.1f,
                 C2D_Color32(0, 0, 0, 180));
    C2D_DrawText(&title, C2D_WithColor | C2D_AtBaseline,
                 titleX, 70.0f, 0.6f, 1.1f, 1.1f,
                 C2D_Color32(255, 230, 150, 255));

    // Menu rows
    static constexpr const char* LABELS[3] = { "New Game", "Continue", "Credits" };
    constexpr float ROW_HEIGHT = 26.0f;
    constexpr float FIRST_ROW_Y = 130.0f;

    // Gentle pulse on the cursor so the selected row reads as "alive"
    // even with no sprite work done yet — sin wave, ~1.5s period.
    float pulse = 0.5f + 0.5f * sinf(timeAccum * 4.0f);

    for (int i = 0; i < 3; i++) {
        TitleOption opt = static_cast<TitleOption>(i);
        bool isSelected = (opt == selected);
        bool isDisabled = (opt == TitleOption::CONTINUE) && !hasSave;

        u8 alpha = isDisabled ? 100 : 255;
        u32 color = isSelected
            ? C2D_Color32(255, 240, 180, alpha)
            : C2D_Color32(200, 200, 200, alpha);

        float rowY = FIRST_ROW_Y + i * ROW_HEIGHT;

        C2D_Text label;
        C2D_TextParse(&label, m_textBuf, LABELS[i]);
        C2D_TextOptimize(&label);
        float lw = 0.0f, lh = 0.0f;
        C2D_TextGetDimensions(&label, 0.7f, 0.7f, &lw, &lh);
        float lx = (static_cast<float>(SCREEN_TOP_W) - lw) * 0.5f;

        C2D_DrawText(&label, C2D_WithColor | C2D_AtBaseline,
                     lx, rowY, 0.6f, 0.7f, 0.7f, color);

        if (isSelected && !isDisabled) {
            // Small triangular cursor to the left of the selected label,
            // nudged outward slightly by `pulse` for a subtle breathing effect.
            float cursorX = lx - 14.0f - pulse * 2.0f;
            float cy = rowY - lh * 0.35f;
            C2D_DrawTriangle(
                cursorX,      cy - 5.0f, color,
                cursorX,      cy + 5.0f, color,
                cursorX + 9.0f, cy,        color,
                0.6f);
        }
    }

    // Footer hint
    C2D_Text hint;
    const char* hintText = hasSave
        ? "Up/Down: Select   A/Start: Confirm"
        : "Up/Down: Select   A/Start: Confirm   (No save found)";
    C2D_TextParse(&hint, m_textBuf, hintText);
    C2D_TextOptimize(&hint);
    float hw = 0.0f, hh = 0.0f;
    C2D_TextGetDimensions(&hint, 0.45f, 0.45f, &hw, &hh);
    float hx = (static_cast<float>(SCREEN_TOP_W) - hw) * 0.5f;
    C2D_DrawText(&hint, C2D_WithColor | C2D_AtBaseline,
                 hx, static_cast<float>(SCREEN_TOP_H) - 14.0f, 0.6f, 0.45f, 0.45f,
                 C2D_Color32(160, 160, 160, 255));
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
// Shows either the NPC's quest dialogue override (if set by QuestManager)
// or their default dialogue string.
// Newline (\n) in the dialogue string splits into two visual lines.
//-----------------------------------------------------------------------------
void Renderer::drawDialogue(const NPC* npc) {
    if (!npc) return;

    C2D_SceneBegin(m_botTarget);
    C2D_TargetClear(m_botTarget, C2D_Color32(20, 20, 20, 255));

    // Dark dialogue panel
    drawColorRect(0.0f, 0.0f,
                  static_cast<float>(SCREEN_BOT_W), 100.0f,
                  C2D_Color32(10, 10, 20, 230));

    // Border line
    drawColorRect(0.0f, 98.0f,
                  static_cast<float>(SCREEN_BOT_W), 2.0f,
                  C2D_Color32(100, 100, 160, 255));

    // NPC name
    C2D_Text nameText;
    C2D_TextParse(&nameText, m_textBuf, npc->name);
    C2D_TextOptimize(&nameText);
    C2D_DrawText(&nameText, C2D_WithColor | C2D_AtBaseline,
                 10.0f, 22.0f, 0.8f, 0.65f, 0.65f,
                 C2D_Color32(255, 220, 80, 255));

    // Choose text: quest override takes priority
    const char* rawText = (npc->dialogue_override != nullptr)
                          ? npc->dialogue_override
                          : npc->dialogue;

    // Split on \n into at most two lines
    char line1[MAX_DIALOGUE_LEN] = {};
    char line2[MAX_DIALOGUE_LEN] = {};
    const char* nl = nullptr;
    for (const char* p = rawText; *p; p++) {
        if (*p == '\n') { nl = p; break; }
    }
    if (nl) {
        int len1 = static_cast<int>(nl - rawText);
        if (len1 >= MAX_DIALOGUE_LEN) len1 = MAX_DIALOGUE_LEN - 1;
        strncpy(line1, rawText, static_cast<size_t>(len1));
        line1[len1] = '\0';
        strncpy(line2, nl + 1, MAX_DIALOGUE_LEN - 1);
    } else {
        strncpy(line1, rawText, MAX_DIALOGUE_LEN - 1);
    }

    C2D_Text t1, t2;
    C2D_TextParse(&t1, m_textBuf, line1);
    C2D_TextOptimize(&t1);
    C2D_DrawText(&t1, C2D_WithColor | C2D_AtBaseline,
                 10.0f, 50.0f, 0.7f, 0.55f, 0.55f,
                 C2D_Color32(230, 230, 230, 255));

    if (line2[0] != '\0') {
        C2D_Text t2text;
        C2D_TextParse(&t2text, m_textBuf, line2);
        C2D_TextOptimize(&t2text);
        C2D_DrawText(&t2text, C2D_WithColor | C2D_AtBaseline,
                     10.0f, 72.0f, 0.7f, 0.55f, 0.55f,
                     C2D_Color32(200, 200, 200, 255));
    }

    // Close hint
    C2D_Text hintText;
    C2D_TextParse(&hintText, m_textBuf, "A: Close");
    C2D_TextOptimize(&hintText);
    C2D_DrawText(&hintText, C2D_WithColor | C2D_AtBaseline,
                 250.0f, 92.0f, 0.6f, 0.5f, 0.5f,
                 C2D_Color32(140, 140, 140, 255));

    m_dialogueDrawnThisFrame = true;
    C2D_SceneBegin(m_topTarget);
}

//-----------------------------------------------------------------------------
// drawGatherNodes  (Milestone 6 — Foundation of Feel)
//
// Placeholder visuals, same philosophy as drawWorldObjects: no sprites
// required. Wood nodes are a small stump (dark ring + lighter center).
// Rope nodes are a coiled tan loop. Both drawn at reduced alpha while on
// cooldown so harvested-vs-ready is readable without needing to walk up
// and try interacting.
//-----------------------------------------------------------------------------
void Renderer::drawGatherNodes(const GatherNode* nodes, int count,
                               ZoneID currentZone, const Camera& cam) {
    for (int i = 0; i < count; i++) {
        const GatherNode& node = nodes[i];
        if (!node.active)             continue;
        if (node.zone != currentZone) continue;

        float sx = node.pos_x - static_cast<float>(cam.getX()) - 6.0f;
        float sy = node.pos_y - static_cast<float>(cam.getY()) - 6.0f;

        if (sx < -16.0f || sx > SCREEN_TOP_W + 16.0f) continue;
        if (sy < -16.0f || sy > SCREEN_TOP_H + 16.0f) continue;

        bool onCooldown = node.cooldown > 0.0f;
        u8   alpha      = onCooldown ? 90 : 255;

        if (node.resource == GatherResource::WOOD) {
            // Stump: dark ring + lighter center
            drawColorRect(sx,     sy,     12, 12, C2D_Color32(90, 60, 30, alpha));
            drawColorRect(sx + 3, sy + 3,  6,  6, C2D_Color32(150, 110, 60, alpha));
        } else {
            // Rope coil: two offset tan rects suggesting a loop
            drawColorRect(sx,     sy + 3, 12,  6, C2D_Color32(200, 170, 110, alpha));
            drawColorRect(sx + 3, sy,      6, 12, C2D_Color32(170, 140, 90, alpha));
        }
    }
}

//-----------------------------------------------------------------------------
// drawWorldObjects
// Placeholder visuals for world objects. No sprites required.
//
// BRIDGE INACTIVE:   brown gap (two dark rects with a gap between)
// BRIDGE REPAIRED:   solid brown crossing (filled rect)
//
// LADDER INACTIVE:   dark vertical line
// LADDER REPAIRED:   brighter vertical band with horizontal rungs
//
// OBSTACLE INACTIVE: large dark green blocking rect
// OBSTACLE REPAIRED: nothing drawn (cleared)
//-----------------------------------------------------------------------------
void Renderer::drawWorldObjects(const WorldObject* objects, int count,
                                ZoneID currentZone, const Camera& cam) {
    for (int i = 0; i < count; i++) {
        const WorldObject& obj = objects[i];
        if (!obj.active)              continue;
        if (obj.zone != currentZone)  continue;

        // Position on screen: use the primary affected tile (tiles[0])
        if (obj.tile_count == 0) continue;
        float sx = obj.tiles[0].tx * TILE_SIZE - static_cast<float>(cam.getX());
        float sy = obj.tiles[0].ty * TILE_SIZE - static_cast<float>(cam.getY());

        // Cull off-screen
        if (sx < -32.0f || sx > SCREEN_TOP_W + 32.0f) continue;
        if (sy < -32.0f || sy > SCREEN_TOP_H + 32.0f) continue;

        bool repaired = (obj.state == WorldObjectState::REPAIRED);

        switch (obj.type) {
            case WorldObjectType::BRIDGE: {
                // Spans tile_count tiles horizontally
                float w = static_cast<float>(obj.tile_count * TILE_SIZE);
                if (repaired) {
                    // Solid brown crossing
                    drawColorRect(sx, sy, w, TILE_SIZE,
                                  C2D_Color32(139, 90, 43, 255));
                    // Plank lines
                    for (int p = 0; p < obj.tile_count; p++) {
                        drawColorRect(sx + p * TILE_SIZE + 1, sy + 4,
                                      TILE_SIZE - 2, 2,
                                      C2D_Color32(100, 60, 20, 200));
                        drawColorRect(sx + p * TILE_SIZE + 1, sy + 10,
                                      TILE_SIZE - 2, 2,
                                      C2D_Color32(100, 60, 20, 200));
                    }
                } else {
                    // Brown gap — show wall color with a dark gap
                    drawColorRect(sx, sy, w, TILE_SIZE,
                                  C2D_Color32(100, 80, 60, 255));
                    drawColorRect(sx + 4, sy + 4, w - 8, TILE_SIZE - 8,
                                  C2D_Color32(20, 10, 5, 255));
                }
                break;
            }

            case WorldObjectType::LADDER: {
                // Spans tile_count tiles vertically
                float h = static_cast<float>(obj.tile_count * TILE_SIZE);
                if (repaired) {
                    // Bright ladder — side rails + rungs
                    drawColorRect(sx + 3, sy, 3, h,
                                  C2D_Color32(180, 120, 60, 255));
                    drawColorRect(sx + 10, sy, 3, h,
                                  C2D_Color32(180, 120, 60, 255));
                    for (int r = 0; r < obj.tile_count * 2; r++) {
                        drawColorRect(sx + 3, sy + r * (TILE_SIZE / 2) + 6,
                                      10, 2,
                                      C2D_Color32(220, 160, 80, 255));
                    }
                } else {
                    // Dark vertical line — coiled rope suggestion
                    drawColorRect(sx + 6, sy, 4, h,
                                  C2D_Color32(60, 40, 20, 255));
                }
                break;
            }

            case WorldObjectType::OBSTACLE: {
                if (!repaired) {
                    // Large dark green blocking rect across tile_count tiles vertically
                    float h = static_cast<float>(obj.tile_count * TILE_SIZE);
                    drawColorRect(sx, sy, TILE_SIZE, h,
                                  C2D_Color32(40, 70, 20, 255));
                    // Tree suggestion lines
                    drawColorRect(sx + 6, sy, 4, h,
                                  C2D_Color32(80, 50, 20, 255));
                }
                // REPAIRED = nothing drawn (cleared)
                break;
            }
        }
    }
}

//-----------------------------------------------------------------------------
// drawQuestHUD  (Milestone 4 — adds wood/rope resource display)
//-----------------------------------------------------------------------------
void Renderer::drawTitleScreenBottom() {
    C2D_SceneBegin(m_botTarget);
    C2D_TargetClear(m_botTarget, C2D_Color32(18, 22, 34, 255));

    C2D_Text text;
    C2D_TextParse(&text, m_textBuf, "A Wayfinder's tale of road and lantern.");
    C2D_TextOptimize(&text);
    float tw = 0.0f, th = 0.0f;
    C2D_TextGetDimensions(&text, 0.55f, 0.55f, &tw, &th);
    float tx = (static_cast<float>(SCREEN_BOT_W) - tw) * 0.5f;
    float ty = (static_cast<float>(SCREEN_BOT_H) + th) * 0.5f;
    C2D_DrawText(&text, C2D_WithColor | C2D_AtBaseline,
                 tx, ty, 0.6f, 0.55f, 0.55f,
                 C2D_Color32(150, 150, 170, 255));

    m_dialogueDrawnThisFrame = true; // tells endFrame() not to re-clear this
    C2D_SceneBegin(m_topTarget);
}

//-----------------------------------------------------------------------------
void Renderer::drawQuestHUD(const char* objectiveText, u32 gold, u8 wood, u8 rope) {
    C2D_SceneBegin(m_botTarget);
    C2D_TargetClear(m_botTarget, C2D_Color32(20, 20, 20, 255));

    // Header bar
    drawColorRect(0.0f, 0.0f,
                  static_cast<float>(SCREEN_BOT_W), 18.0f,
                  C2D_Color32(10, 10, 30, 220));

    C2D_Text headerText;
    C2D_TextParse(&headerText, m_textBuf, "QUEST");
    C2D_TextOptimize(&headerText);
    C2D_DrawText(&headerText, C2D_WithColor | C2D_AtBaseline,
                 10.0f, 14.0f, 0.7f, 0.5f, 0.5f,
                 C2D_Color32(180, 180, 220, 255));

    // Objective
    if (objectiveText != nullptr) {
        C2D_Text objText;
        C2D_TextParse(&objText, m_textBuf, objectiveText);
        C2D_TextOptimize(&objText);
        C2D_DrawText(&objText, C2D_WithColor | C2D_AtBaseline,
                     10.0f, 38.0f, 0.7f, 0.58f, 0.58f,
                     C2D_Color32(255, 255, 180, 255));
    } else {
        C2D_Text noQText;
        C2D_TextParse(&noQText, m_textBuf, "No active quest");
        C2D_TextOptimize(&noQText);
        C2D_DrawText(&noQText, C2D_WithColor | C2D_AtBaseline,
                     10.0f, 38.0f, 0.7f, 0.55f, 0.55f,
                     C2D_Color32(120, 120, 120, 255));
    }

    // Divider
    drawColorRect(0.0f, 50.0f,
                  static_cast<float>(SCREEN_BOT_W), 1.0f,
                  C2D_Color32(60, 60, 80, 255));

    // Resources row: Gold  Wood  Rope
    char resBuf[64];
    snprintf(resBuf, sizeof(resBuf), "Gold:%u  Wood:%u  Rope:%u", gold, wood, rope);
    C2D_Text resText;
    C2D_TextParse(&resText, m_textBuf, resBuf);
    C2D_TextOptimize(&resText);
    C2D_DrawText(&resText, C2D_WithColor | C2D_AtBaseline,
                 10.0f, 70.0f, 0.7f, 0.55f, 0.55f,
                 C2D_Color32(220, 200, 120, 255));

    m_dialogueDrawnThisFrame = true;
    C2D_SceneBegin(m_topTarget);
}

//-----------------------------------------------------------------------------
// drawQuestMarker
// Draws a pulsing diamond (4 triangles) at the world position of a
// REACH_MARKER objective. Pulses between full and half alpha on a 1-second
// cycle so it's visible but not distracting.
//
// A diamond is drawn as four C2D_DrawTriangle calls sharing a center point.
// citro2d doesn't have a built-in diamond primitive, so we use colored
// rectangles rotated — simpler: just draw a small square rotated 45°.
// Since citro2d doesn't support rotation, we approximate with a diamond
// made from two overlapping rects.
//-----------------------------------------------------------------------------
void Renderer::drawQuestMarker(float worldX, float worldY,
                               const Camera& cam, float timeAccum) {
    float sx = worldX - static_cast<float>(cam.getX());
    float sy = worldY - static_cast<float>(cam.getY());

    // Cull if off screen
    if (sx < -20.0f || sx > SCREEN_TOP_W + 20.0f) return;
    if (sy < -20.0f || sy > SCREEN_TOP_H + 20.0f) return;

    // Pulse: sin wave [0,1] at 1 Hz
    float pulse = 0.5f + 0.5f * sinf(timeAccum * 6.283f);
    u8    alpha = static_cast<u8>(140.0f + 115.0f * pulse);  // 140–255

    u32 color      = C2D_Color32(255, 220, 50, alpha);
    u32 colorInner = C2D_Color32(255, 255, 180, static_cast<u8>(alpha / 2));

    // Diamond approximation: vertical + horizontal rect crossed at center
    // Vertical bar (tall, narrow)
    drawColorRect(sx - 2.0f, sy - 8.0f, 4.0f, 16.0f, color);
    // Horizontal bar (short, wide)
    drawColorRect(sx - 8.0f, sy - 2.0f, 16.0f, 4.0f, color);
    // Inner dot
    drawColorRect(sx - 2.0f, sy - 2.0f, 4.0f, 4.0f, colorInner);
}

//-----------------------------------------------------------------------------
// drawStatusMessage
// Semi-transparent dark pill centered on the top screen.
// Used for "Game Saved" / "Game Loaded" notifications.
//-----------------------------------------------------------------------------
void Renderer::drawStatusMessage(const char* text, float alpha) {
    if (alpha <= 0.0f || !text) return;
    u8 a = static_cast<u8>(std::min(alpha, 1.0f) * 255.0f);

    // Background pill
    drawColorRect(100.0f, 108.0f, 200.0f, 24.0f,
                  C2D_Color32(0, 0, 0, static_cast<u8>(a * 0.75f)));

    C2D_Text msgText;
    C2D_TextParse(&msgText, m_textBuf, text);
    C2D_TextOptimize(&msgText);

    float tw = 0.0f, th = 0.0f;
    C2D_TextGetDimensions(&msgText, 0.65f, 0.65f, &tw, &th);
    float tx = (static_cast<float>(SCREEN_TOP_W) - tw) * 0.5f;
    float ty = 108.0f + (24.0f + th) * 0.5f;

    C2D_DrawText(&msgText, C2D_WithColor | C2D_AtBaseline,
                 tx + 1.0f, ty + 1.0f, 0.75f, 0.65f, 0.65f,
                 C2D_Color32(0, 0, 0, a));
    C2D_DrawText(&msgText, C2D_WithColor | C2D_AtBaseline,
                 tx, ty, 0.8f, 0.65f, 0.65f,
                 C2D_Color32(180, 255, 180, a));
}

//-----------------------------------------------------------------------------
void Renderer::endFrame() {
    if (!m_dialogueDrawnThisFrame) {
        C2D_TargetClear(m_botTarget, C2D_Color32(20, 20, 20, 255));
        C2D_SceneBegin(m_botTarget);
    }
    C3D_FrameEnd(0);
}

