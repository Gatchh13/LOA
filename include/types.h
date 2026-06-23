#pragma once

//-----------------------------------------------------------------------------
// types.h
// Shared primitive types and project-wide constants.
// Include this in every header that needs basic types.
//
// Milestone 1 additions:
//   - ZoneID enum
//   - Extended tile IDs (forest, dungeon tiles)
//   - MAX_ZONE_W / MAX_ZONE_H for variable-size maps
//-----------------------------------------------------------------------------

#include <3ds.h>
#include <cstdint>
#include <cmath>

//-----------------------------------------------------------------------------
// Screen dimensions
//-----------------------------------------------------------------------------
static constexpr int SCREEN_TOP_W = 400;
static constexpr int SCREEN_TOP_H = 240;
static constexpr int SCREEN_BOT_W = 320;
static constexpr int SCREEN_BOT_H = 240;

//-----------------------------------------------------------------------------
// Tile constants
//-----------------------------------------------------------------------------
static constexpr int TILE_SIZE  = 16;
static constexpr int MAX_ZONE_W = 32;   // maximum map width  in tiles (any zone)
static constexpr int MAX_ZONE_H = 24;   // maximum map height in tiles (any zone)

//-----------------------------------------------------------------------------
// Tile IDs
// 0–15 are passable ground tiles.
// 16+ are solid wall/obstacle tiles.
// The renderer uses these IDs to pick a fallback color or sprite index.
//-----------------------------------------------------------------------------
static constexpr u8 TILE_GRASS         = 0;   // town / open ground
static constexpr u8 TILE_DIRT          = 1;   // path / worn ground
static constexpr u8 TILE_FOREST_FLOOR  = 2;   // forest undergrowth
static constexpr u8 TILE_STONE_FLOOR   = 3;   // dungeon floor
static constexpr u8 TILE_WATER         = 4;   // impassable water (no wall sprite)

static constexpr u8 TILE_WALL          = 16;  // generic solid wall
static constexpr u8 TILE_TREE          = 17;  // forest tree (solid)
static constexpr u8 TILE_STONE_WALL    = 18;  // dungeon stone wall
static constexpr u8 TILE_FENCE         = 19;  // town fence (solid)

// Solid test: any tile ID >= TILE_FIRST_SOLID is impassable.
static constexpr u8 TILE_FIRST_SOLID   = 16;

//-----------------------------------------------------------------------------
// Zone identifiers
//-----------------------------------------------------------------------------
enum class ZoneID : u8 {
    TOWN            = 0,
    FOREST          = 1,
    DUNGEON_ENTRANCE= 2,
    COUNT           = 3
};

static constexpr u8 ZONE_COUNT = static_cast<u8>(ZoneID::COUNT);

//-----------------------------------------------------------------------------
// Vec2 — 2D float vector
//-----------------------------------------------------------------------------
struct Vec2 {
    float x;
    float y;

    Vec2() : x(0.0f), y(0.0f) {}
    Vec2(float x_, float y_) : x(x_), y(y_) {}

    Vec2 operator+(const Vec2& o) const { return Vec2(x + o.x, y + o.y); }
    Vec2 operator-(const Vec2& o) const { return Vec2(x - o.x, y - o.y); }
    Vec2 operator*(float s)       const { return Vec2(x * s,   y * s);   }

    Vec2& operator+=(const Vec2& o) { x += o.x; y += o.y; return *this; }
    Vec2& operator-=(const Vec2& o) { x -= o.x; y -= o.y; return *this; }

    float length() const { return sqrtf(x * x + y * y); }

    Vec2 normalized() const {
        float len = length();
        if (len < 0.0001f) return Vec2(0.0f, 0.0f);
        return Vec2(x / len, y / len);
    }
};

//-----------------------------------------------------------------------------
// Rect — integer axis-aligned rectangle
//-----------------------------------------------------------------------------
struct Rect {
    int x, y, w, h;
    Rect() : x(0), y(0), w(0), h(0) {}
    Rect(int x_, int y_, int w_, int h_) : x(x_), y(y_), w(w_), h(h_) {}

    bool overlaps(const Rect& o) const {
        return x < o.x + o.w &&
               x + w > o.x   &&
               y < o.y + o.h &&
               y + h > o.y;
    }
};

