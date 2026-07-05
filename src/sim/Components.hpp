#pragma once

#include <cstddef>
#include <vector>

#include "sim/Config.hpp"
#include "sim/Coords.hpp"
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

// Rendering size only. Kept separate from the collision box (Collider) so art
// and physics can differ -- a sprite can be larger than what it bumps into.
struct Sprite {
    float size; // side length of the drawn square, in pixels
};

// Collision half-extent: a square hitbox, centered on Position, `half` pixels to
// each side. Decoupled from both Sprite and TILE_SIZE on purpose -- an entity's
// hitbox is chosen for how it should move (e.g. a hair smaller than a tile for
// corner/corridor clearance), independent of the art or how fine the tiles are.
struct Collider {
    float half; // half the side length of the collision box, in pixels
};

// Tag marking the player-controlled entity, carrying its movement speed.
struct Player {
    float speed = config::PLAYER_SPEED; // pixels per second
};

// Tag marking the hunter/villain, carrying its movement speed. Slower than the
// player for now so a fleeing player can stay ahead.
struct Enemy {
    float speed = config::ENEMY_SPEED; // pixels per second
};

// A tile-by-tile path an entity is walking, plus a bookkeeping cursor. The
// pathfinding system fills `path` (from A*) and advances `next` as waypoints are
// reached; the movement system does the actual integration. Empty path => idle.
struct PathFollower {
    std::vector<TileCoord> path;   // start .. goal, inclusive
    std::size_t next = 0;          // index of the waypoint currently steered toward
    float repathTimer = 0.0f;      // seconds until the path is recomputed
};

} // namespace lurk
