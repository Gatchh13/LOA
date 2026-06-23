//-----------------------------------------------------------------------------
// Player.cpp  (Milestone 1)
//-----------------------------------------------------------------------------

#include "Player.h"
#include "../world/Collision.h"

Player::Player(float startX, float startY)
    : m_pos(startX, startY)
{}

void Player::update(Vec2 axis, float dt, const TileMap& map) {
    Vec2 dir      = axis.normalized();
    Vec2 velocity = dir * (SPEED * dt);
    m_pos         = Collision::resolve(map, m_pos, velocity, PLAYER_W, PLAYER_H);
}

void Player::setPosition(float x, float y) {
    m_pos.x = x;
    m_pos.y = y;
}

