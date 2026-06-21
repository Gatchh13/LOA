#pragma once

//-----------------------------------------------------------------------------
// Player.h
// Player position, speed, and movement logic.
// Does not own a sprite — the Renderer draws the player given its position.
//-----------------------------------------------------------------------------

#include "../../include/types.h"
#include "../world/TileMap.h"

class Player {
public:
    // Spawn position in world pixels.
    Player(float startX, float startY);

    // Move the player based on input axis and delta time.
    // Collision against the tile map is resolved here.
    void update(Vec2 axis, float dt, const TileMap& map);

    // World-pixel position (top-left of the player rect).
    float getX() const { return m_pos.x; }
    float getY() const { return m_pos.y; }

    // Center of the player in world pixels (useful for camera targeting).
    float getCenterX() const { return m_pos.x + PLAYER_W * 0.5f; }
    float getCenterY() const { return m_pos.y + PLAYER_H * 0.5f; }

    // Player hitbox size in pixels (matches sprite size for bootstrap).
    static constexpr int PLAYER_W = 14;  // slightly narrower than tile for comfort
    static constexpr int PLAYER_H = 14;

    // Movement speed in pixels per second.
    static constexpr float SPEED = 80.0f;

private:
    Vec2 m_pos;
};
