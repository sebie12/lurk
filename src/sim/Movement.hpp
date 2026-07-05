#pragma once

#include <cmath>
#include <cstdio>

#include <entt/entt.hpp>

#include "sim/Collision.hpp"
#include "sim/Components.hpp"
#include "sim/Pathfinding.hpp"

namespace lurk {

// Movement system: advance every mobile, collidable entity by its velocity over
// dt, resolving against solid tiles. Templated on the `solid` predicate so it's
// decoupled from the terrain source (the game passes a chunk query; tests pass a
// synthetic map). Works for any entity with Position + Velocity + Sprite, so the
// player and future enemies share it.
template <class SolidFn>
void movementSystem(entt::registry& registry, float dt, SolidFn solid) {
    auto view = registry.view<Position, const Velocity, const Collider>();
    for (const entt::entity e : view) {
        auto& pos = view.get<Position>(e);
        const auto& vel = view.get<const Velocity>(e);
        const auto& col = view.get<const Collider>(e);
        pos.value = resolveMove(pos.value, col.half, vel.value * dt, solid);
    }
}

// Pathfinding / steering system for hunters: each Enemy with a PathFollower walks
// a tile path toward `target` (the player). It only sets each enemy's Velocity;
// movementSystem still performs the integration and collision, so hunters obey
// the same terrain rules as the player.
//
// The path is recomputed on a timer (the target moves) or when the current one is
// exhausted, via A* over the same `solid` predicate the movement uses. Steering
// aims at the center of the next tile and pops waypoints as they're reached.
template <class SolidFn>
void pathfindingSystem(entt::registry& registry, float dt, Vec2 target, SolidFn solid) {
    const TileCoord goalTile = worldToTile(target);

    auto view = registry.view<Position, Velocity, Enemy, PathFollower>();
    for (const entt::entity e : view) {
        auto& pos = view.get<Position>(e);
        auto& vel = view.get<Velocity>(e);
        const auto& enemy = view.get<Enemy>(e);
        auto& follower = view.get<PathFollower>(e);

        follower.repathTimer -= dt;
        const bool exhausted = follower.next >= follower.path.size();
        if (follower.repathTimer <= 0.0f || exhausted) {
            const TileCoord startTile = worldToTile(pos.value);
            follower.path = findPath(startTile, goalTile, solid);
            // Index 0 is the tile we're standing on; steer toward index 1 onward.
            follower.next = follower.path.size() > 1 ? 1 : follower.path.size();
            follower.repathTimer = config::REPATH_INTERVAL;

            // Log every repath so pathing behaviour is visible. An empty path
            // means A* found no route to the goal within its expansion budget
            // (e.g. a large water body to detour, or a fully enclosed target) --
            // that's the case where the hunter appears to "give up".
            if (follower.path.empty()) {
                std::printf("[pathfinding] enemy=%u  (%d,%d) -> (%d,%d)  NO PATH "
                            "(unreachable within budget)\n",
                            static_cast<unsigned>(entt::to_integral(e)),
                            startTile.x, startTile.y, goalTile.x, goalTile.y);
            } else {
                std::printf("[pathfinding] enemy=%u  (%d,%d) -> (%d,%d)  path=%zu tiles\n",
                            static_cast<unsigned>(entt::to_integral(e)),
                            startTile.x, startTile.y, goalTile.x, goalTile.y,
                            follower.path.size());
            }
            std::fflush(stdout);
        }

        // Skip any waypoints already reached this frame, then steer at the next.
        vel.value = Vec2{0.0f, 0.0f};
        while (follower.next < follower.path.size()) {
            const TileCoord wp = follower.path[follower.next];
            const Vec2 center{(static_cast<float>(wp.x) + 0.5f) * TILE_SIZE,
                              (static_cast<float>(wp.y) + 0.5f) * TILE_SIZE};
            const Vec2 delta{center.x - pos.value.x, center.y - pos.value.y};
            const float dist = std::sqrt(delta.x * delta.x + delta.y * delta.y);
            if (dist < config::WAYPOINT_ARRIVE_DIST) {
                ++follower.next; // waypoint reached; advance to the next
                continue;
            }
            vel.value = normalized(delta) * enemy.speed;
            break;
        }
    }
}

} // namespace lurk
