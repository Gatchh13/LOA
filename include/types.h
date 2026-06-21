#pragma once

//-----------------------------------------------------------------------------
// types.h
// Shared primitive types and project-wide constants.
// Include this in every header that needs basic types.
//-----------------------------------------------------------------------------

#include <3ds.h>      // u8, u16, u32, s8, s16, s32, etc. (from libctru)
#include <cstdint>
#include <cmath>

//-----------------------------------------------------------------------------
// Screen dimensions (top screen)
//-----------------------------------------------------------------------------
static constexpr int SCREEN_TOP_W = 400;
static constexpr int SCREEN_TOP_H = 240;
static constexpr int SCREEN_BOT_W = 320;
static constexpr int SCREEN_BOT_H = 240;

//-----------------------------------------------------------------------------
// Tile constants
//-----------------------------------------------------------------------------
static constexpr int TILE_SIZE     = 16;   // pixels per tile
static constexpr int TILEMAP_W     = 25;   // map width  in tiles
static constexpr int TILEMAP_H     = 20;   // map height in tiles

//-----------------------------------------------------------------------------
// Tile IDs used in map data
//-----------------------------------------------------------------------------
static constexpr u8 TILE_GRASS = 0;
static constexpr u8 TILE_WALL  = 1;

//-----------------------------------------------------------------------------
// Simple 2D float vector
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
// Simple integer rectangle (tile coordinates or pixel regions)
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
