#pragma once

#include <cmath>

namespace lurk {

// Minimal 2D vector. Lives in sim/ so the simulation stays engine-agnostic
// (no dependency on raylib's Vector2). core/ converts to/from raylib types
// at the rendering boundary.
struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
};

inline Vec2 operator+(Vec2 a, Vec2 b) { return {a.x + b.x, a.y + b.y}; }
inline Vec2 operator*(Vec2 v, float s) { return {v.x * s, v.y * s}; }

// Returns v scaled to unit length, or the zero vector unchanged.
inline Vec2 normalized(Vec2 v) {
    const float len = std::sqrt(v.x * v.x + v.y * v.y);
    return len > 0.0f ? Vec2{v.x / len, v.y / len} : v;
}

} // namespace lurk
