#pragma once

//-----------------------------------------------------------------------------
// AnimState.h  (Milestone 6 — sprite animation foundation)
//
// Shared by Player and NPC. Tracks which way an entity is facing and which
// animation frame it's currently on, driven by movement velocity each frame.
//
// Design:
//   - Facing is 4-directional (DOWN/UP/LEFT/RIGHT), matching the D-Pad/Circle
//     Pad input model — no 8-direction diagonal sprites needed.
//   - "Walking" vs "idle" is derived from whether the entity moved this
//     frame, not from a separate flag — one less thing that can desync.
//   - Frame advancement is time-based (anim_timer counts down in seconds),
//     not call-count-based, so animation speed is independent of frame rate.
//   - This struct is deliberately NOT in SaveData. Animation state is
//     visual-only and always derived correctly from a fresh idle pose on
//     load — there is nothing here worth persisting.
//
// Usage:
//   AnimState anim;
//   ...
//   anim.update(velocityX, velocityY, dt);
//   // then read anim.facing / anim.frame for drawing.
//
// Sprite sheet mapping (once real art exists):
//   Each (facing, frame) pair maps to one sprite index. With FRAMES_PER_ANIM
//   frames × 4 facings, a walk-cycle sheet needs FRAMES_PER_ANIM * 4 sprites.
//   Until a sprite sheet exists, Renderer uses facing/frame to vary the
//   fallback colored-rect drawing instead (see Renderer::drawDirectionalRect).
//-----------------------------------------------------------------------------

#include "../../include/types.h"

enum class Facing : u8 {
    DOWN  = 0,
    UP    = 1,
    LEFT  = 2,
    RIGHT = 3,
};

// How many animation frames make up one full walk cycle.
// Frame 0 is always the idle/standing pose for that facing.
static constexpr int FRAMES_PER_ANIM = 4;

// Seconds each walking frame is held before advancing. Tuned for a 16px
// tile at Player::SPEED == 80px/s — about 5 frames to cross one tile.
static constexpr float ANIM_FRAME_DURATION = 0.12f;

// Minimum speed (pixels/second, either axis) to count as "moving" for
// animation purposes. Below this, treat as idle even if velocity is
// technically nonzero (avoids jitter from floating point movement at
// very low dt or near-zero analog input).
static constexpr float ANIM_MOVE_THRESHOLD = 1.0f;

struct AnimState {
    Facing facing      = Facing::DOWN;
    u8     frame        = 0;     // 0..FRAMES_PER_ANIM-1; 0 = idle pose
    bool   isMoving     = false;
    float  frameTimer   = 0.0f;  // counts down; advances frame at 0

    // Call once per frame with the entity's velocity for this frame
    // (pixels/second, pre- or post-collision — either is fine, this only
    // reads direction and magnitude) and the real frame delta time.
    void update(float velX, float velY, float dt) {
        float absX = velX < 0.0f ? -velX : velX;
        float absY = velY < 0.0f ? -velY : velY;

        isMoving = (absX > ANIM_MOVE_THRESHOLD) || (absY > ANIM_MOVE_THRESHOLD);

        if (isMoving) {
            // Horizontal takes priority on diagonal movement — matches the
            // existing horizontal-then-vertical collision/movement
            // convention used elsewhere (NPCManager::moveNPC).
            if (absX >= absY) {
                facing = (velX < 0.0f) ? Facing::LEFT : Facing::RIGHT;
            } else {
                facing = (velY < 0.0f) ? Facing::UP : Facing::DOWN;
            }

            frameTimer -= dt;
            if (frameTimer <= 0.0f) {
                frameTimer = ANIM_FRAME_DURATION;
                frame = static_cast<u8>((frame + 1) % FRAMES_PER_ANIM);
                // Frame 0 is the idle pose — skip it during walk cycles so
                // movement always shows a "stepping" frame, not a standing one.
                if (frame == 0) frame = 1;
            }
        } else {
            // Idle: reset to standing pose, facing unchanged (keeps facing
            // the last direction moved, which reads naturally — NPCs and
            // the player don't snap to face down the instant they stop).
            frame      = 0;
            frameTimer = 0.0f;
        }
    }
};
