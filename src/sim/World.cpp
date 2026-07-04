#include "sim/World.hpp"

#include <algorithm>
#include <cstdlib>

#include "sim/Collision.hpp"
#include "sim/Components.hpp"
#include "sim/Movement.hpp"

namespace lurk {
namespace {
constexpr uint64_t kSeed = 1337; // temporary fixed seed until seeding is wired up
}

World::World() : chunks_(kSeed) {
    const Vec2 spawn = findWalkableSpawn();

    player_ = registry_.create();
    registry_.emplace<Position>(player_, spawn);
    registry_.emplace<Velocity>(player_, Vec2{0.0f, 0.0f});
    registry_.emplace<Sprite>(player_, 32.0f);
    registry_.emplace<Player>(player_);

    chunks_.update(worldToChunk(spawn));
}

void World::update(float dt) {
    // Integrate all mobile entities against terrain: water blocks, land slides.
    movementSystem(registry_, dt,
                   [this](TileCoord t) { return isSolid(chunks_.tileAt(t)); });

    // Re-center chunk streaming on the player each tick.
    chunks_.update(worldToChunk(playerPosition()));
}

void World::setPlayerMoveDir(Vec2 dir) {
    const float speed = registry_.get<Player>(player_).speed;
    registry_.get<Velocity>(player_).value = dir * speed;
}

Vec2 World::playerPosition() const {
    return registry_.get<Position>(player_).value;
}

Vec2 World::findWalkableSpawn() const {
    // Spiral outward in tile rings from the origin; spawn at the first non-solid
    // tile's center so the player never starts inside water.
    for (int r = 0; r < 256; ++r) {
        for (int dy = -r; dy <= r; ++dy) {
            for (int dx = -r; dx <= r; ++dx) {
                if (std::max(std::abs(dx), std::abs(dy)) != r) continue; // ring only
                const TileCoord t{dx, dy};
                if (!isSolid(chunks_.tileAt(t))) {
                    return Vec2{(static_cast<float>(t.x) + 0.5f) * TILE_SIZE,
                                (static_cast<float>(t.y) + 0.5f) * TILE_SIZE};
                }
            }
        }
    }
    return Vec2{0.0f, 0.0f};
}

} // namespace lurk
