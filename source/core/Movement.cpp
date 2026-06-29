//-----------------------------------------------------------------------------
// Movement.cpp  (Milestone 12 — Engine Cleanup & Architecture Consolidation)
//
// Logic moved verbatim from NPCManager::moveNPC() and
// EnemyManager::moveToward() — both deleted from their respective
// files, replaced with a call to seekTowardTarget() below. No numeric
// constant, no branch, no comparison operator changed; only the
// owning-struct-specific field names (npc.pos_x -> posX, e.anim -> anim,
// etc.) were generalized into parameters.
//-----------------------------------------------------------------------------

#include "Movement.h"
#include <cmath>
#include <algorithm>

bool seekTowardTarget(float& posX, float& posY,
                      float targetX, float targetY,
                      float speed, float arrivalThreshold,
                      float dt, const TileMap& map,
                      AnimState& anim) {
    float dx   = targetX - posX;
    float dy   = targetY - posY;
    float dist = sqrtf(dx * dx + dy * dy);
    if (dist <= arrivalThreshold) {
        anim.update(0.0f, 0.0f, dt);
        return false;
    }

    float step = speed * dt;
    float velX = 0.0f;
    float velY = 0.0f;

    if (fabsf(dx) > arrivalThreshold) {
        float moveX = (dx > 0.0f ? 1.0f : -1.0f) * std::min(step, fabsf(dx));
        float newX  = posX + moveX;
        int tx = static_cast<int>(newX + 8) / TILE_SIZE;
        int ty = static_cast<int>(posY + 8) / TILE_SIZE;
        if (!map.isSolid(tx, ty)) {
            posX = newX;
            velX = (dx > 0.0f ? 1.0f : -1.0f) * speed;
        }
    } else if (fabsf(dy) > arrivalThreshold) {
        float moveY = (dy > 0.0f ? 1.0f : -1.0f) * std::min(step, fabsf(dy));
        float newY  = posY + moveY;
        int tx = static_cast<int>(posX + 8) / TILE_SIZE;
        int ty = static_cast<int>(newY + 8) / TILE_SIZE;
        if (!map.isSolid(tx, ty)) {
            posY = newY;
            velY = (dy > 0.0f ? 1.0f : -1.0f) * speed;
        }
    }

    anim.update(velX, velY, dt);
    return true;
}
