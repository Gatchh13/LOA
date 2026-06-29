#pragma once

//-----------------------------------------------------------------------------
// Movement.h  (Milestone 12 — Engine Cleanup & Architecture Consolidation)
//
// Shared "seek toward a target point" stepper, extracted from
// NPCManager::moveNPC() and EnemyManager::moveToward(), which were
// byte-for-byte identical except for which struct's pos_x/pos_y/anim
// fields they touched and two named constants (speed, arrival
// threshold) that already had the same numeric value in one case
// (ARRIVAL_THRESHOLD == ENEMY_RETURN_THRESHOLD == 2.0f) and only
// differed in the other (NPC_SPEED=50 vs ENEMY_SPEED=40 — genuine
// per-system tuning, preserved exactly via the speed parameter below).
//
// WHAT THIS IS:
//   A single-axis-at-a-time "walk toward (targetX, targetY)" step,
//   horizontal first then vertical (matching the existing convention
//   both callers already used), with a tile-solid collision check per
//   axis and an AnimState update from the resulting velocity. This is
//   the algorithm NPCs and enemies both already used for "walk toward
//   my current destination" — moved here unchanged, not redesigned.
//
// WHAT THIS IS NOT:
//   Player's movement (Collision::resolve, in source/world/Collision.h)
//   is a DIFFERENT algorithm — continuous two-axis simultaneous
//   resolution driven by a normalized analog/digital input vector, used
//   for direct player control rather than seeking a fixed destination
//   point. Player.cpp's own comment is explicit that the continuous
//   resolver is what keeps the walk animation playing while pushing
//   against a wall — a property of THAT algorithm, not this one.
//   Unifying all three into one function was considered and rejected:
//   it would require either (a) moving Player onto a seek-based
//   stepper, changing movement feel (explicitly forbidden by this
//   milestone's brief), or (b) moving NPC/Enemy onto the continuous
//   resolver, a materially bigger and riskier change to two systems
//   whose movement has been tuned and tested since Milestone 2/8 with
//   no reported issues. "Only consolidate duplicated code" — the
//   duplication that actually existed was NPC vs Enemy, not all three.
//
// FUTURE COMPATIBILITY (per this milestone's brief — knockback, dash,
// 8-directional movement):
//   - Knockback: a future caller can apply an external displacement to
//     pos_x/pos_y directly before calling seekTowardTarget() (or after,
//     overriding the result) without this function needing to know
//     knockback exists — it only ever computes "the step toward the
//     target," it never owns the position outright. See Enemy.h's new
//     knockback fields (Milestone 12, Task 7) for how this plays out
//     once a real knockback caller exists.
//   - Dash: a higher speed value passed into this same function would
//     produce a dash-like step with zero changes here — speed is
//     already a parameter, not a baked-in constant.
//   - 8-directional movement: this function already moves on two
//     independent axes (horizontal-then-vertical, not "one of 4 fixed
//     directions") — it has no enum of allowed directions to begin
//     with, so there is nothing here that assumes 4-directional input.
//     The actual 4-direction assumption in this codebase lives in
//     AnimState::Facing (sprite-facing, a rendering concern), which
//     this milestone's brief explicitly says NOT to change.
//
// MEMORY: stateless free functions — 0 bytes RAM, no struct, no class.
//-----------------------------------------------------------------------------

#include "../../include/types.h"
#include "../world/TileMap.h"
#include "../entities/AnimState.h"

// Seek one entity's position toward (targetX, targetY), one axis at a
// time (horizontal first, then vertical), respecting tile collision,
// and update its AnimState from the resulting velocity.
//
// posX/posY:  the entity's current position (top-left of its sprite
//             rect, same convention as NPC::pos_x/Enemy::pos_x) — IN/OUT.
// targetX/Y:  where the entity is trying to reach.
// speed:      pixels per second (NPC_SPEED=50, ENEMY_SPEED=40 today —
//             pass the caller's own tuned constant, unchanged).
// arrivalThreshold: "close enough" distance in pixels (2.0f for both
//             existing callers today).
// dt:         real frame delta time.
// map:        for the per-axis tile-solid check.
// anim:       updated from the resulting per-axis velocity, exactly as
//             both original implementations already did.
//
// Returns true if the entity is still moving (hasn't arrived yet) —
// same return-value contract as the two original functions.
bool seekTowardTarget(float& posX, float& posY,
                      float targetX, float targetY,
                      float speed, float arrivalThreshold,
                      float dt, const TileMap& map,
                      AnimState& anim);
