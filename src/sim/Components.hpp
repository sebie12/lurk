#pragma once

#include "sim/Vec2.hpp"

namespace lurk {

// ECS components: plain data, no behavior, no engine types (Vec2 is ours, not
// raylib's) so the simulation stays engine-agnostic. Systems operate over these.

struct Position {
    Vec2 value;
};

struct Velocity {
    Vec2 value; // pixels per second
};

struct Sprite {
    float size; // side length of the square, in pixels
};

// Tag marking the player-controlled entity, carrying its movement speed.
struct Player {
    float speed = 200.0f; // pixels per second
};

} // namespace lurk
