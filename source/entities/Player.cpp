//-----------------------------------------------------------------------------
// Player.cpp  (Milestone 6 — adds sprite animation state)
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

    // Animate from the INTENDED direction (dir * SPEED), not the actual
    // post-collision displacement — this way the walk cycle keeps playing
    // when the player pushes against a wall, instead of freezing on contact.
    m_anim.update(dir.x * SPEED, dir.y * SPEED, dt);
}

void Player::setPosition(float x, float y) {
    m_pos.x = x;
    m_pos.y = y;
}

