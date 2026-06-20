//-----------------------------------------------------------------------------
// Player.cpp
//-----------------------------------------------------------------------------

#include "Player.h"
#include "../world/Collision.h"

Player::Player(float startX, float startY)
    : m_pos(startX, startY)
{}

void Player::update(Vec2 axis, float dt, const TileMap& map) {
    // Normalize the input axis so diagonal movement isn't faster.
    Vec2 dir = axis.normalized();

    // Scale by speed and delta time to get pixel displacement this frame.
    Vec2 velocity = dir * (SPEED * dt);

    // Resolve collision and update position.
    m_pos = Collision::resolve(map, m_pos, velocity, PLAYER_W, PLAYER_H);
}
