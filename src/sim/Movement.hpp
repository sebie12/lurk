#pragma once

#include <entt/entt.hpp>

#include "sim/Collision.hpp"
#include "sim/Components.hpp"

namespace lurk {

// Movement system: advance every mobile, collidable entity by its velocity over
// dt, resolving against solid tiles. Templated on the `solid` predicate so it's
// decoupled from the terrain source (the game passes a chunk query; tests pass a
// synthetic map). Works for any entity with Position + Velocity + Sprite, so the
// player and future enemies share it.
template <class SolidFn>
void movementSystem(entt::registry& registry, float dt, SolidFn solid) {
    auto view = registry.view<Position, const Velocity, const Sprite>();
    for (const entt::entity e : view) {
        auto& pos = view.get<Position>(e);
        const auto& vel = view.get<const Velocity>(e);
        const auto& spr = view.get<const Sprite>(e);
        pos.value = resolveMove(pos.value, spr.size * 0.5f, vel.value * dt, solid);
    }
}

} // namespace lurk
