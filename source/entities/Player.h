#pragma once

//-----------------------------------------------------------------------------
// Player.h  (Milestone 6 — adds sprite animation state)
//-----------------------------------------------------------------------------

#include "../../include/types.h"
#include "../world/TileMap.h"
#include "AnimState.h"

class Player {
public:
    Player(float startX, float startY);

    void update(Vec2 axis, float dt, const TileMap& map);

    // Teleport — used by ZoneManager on zone transition.
    void setPosition(float x, float y);

    float getX() const { return m_pos.x; }
    float getY() const { return m_pos.y; }

    float getCenterX() const { return m_pos.x + PLAYER_W * 0.5f; }
    float getCenterY() const { return m_pos.y + PLAYER_H * 0.5f; }

    // Returns the tile coordinate of the player's center.
    int getCenterTileX() const { return static_cast<int>(getCenterX()) / TILE_SIZE; }
    int getCenterTileY() const { return static_cast<int>(getCenterY()) / TILE_SIZE; }

    // Animation state, updated every update() call from movement input.
    // Renderer reads these to pick facing/frame for drawing.
    Facing getFacing() const { return m_anim.facing; }
    u8     getAnimFrame() const { return m_anim.frame; }
    bool   isMoving() const { return m_anim.isMoving; }

    static constexpr int   PLAYER_W = 14;
    static constexpr int   PLAYER_H = 14;
    static constexpr float SPEED    = 80.0f;

private:
    Vec2      m_pos;
    AnimState m_anim;
};

