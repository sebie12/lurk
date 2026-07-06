#include "sim/World.hpp"

#include <algorithm>
#include <cstdlib>
#include <random>

#include "sim/Collision.hpp"
#include "sim/Components.hpp"
#include "sim/Config.hpp"
#include "sim/Movement.hpp"

namespace lurk {

World::World() : chunks_(config::WORLD_SEED) {
    const Vec2 spawn = findWalkableSpawn({0, 0});

    player_ = registry_.create();
    registry_.emplace<Position>(player_, spawn);
    registry_.emplace<Velocity>(player_, Vec2{0.0f, 0.0f});
    registry_.emplace<Sprite>(player_, 32.0f);
    registry_.emplace<Collider>(player_, config::COLLIDER_HALF);
    registry_.emplace<Player>(player_);
    registry_.emplace<Health>(player_);       // starts full (config::MAX_HEALTH)
    registry_.emplace<Stamina>(player_);      // starts full (config::MAX_STAMINA)
    registry_.emplace<Inventory>(player_);    // empty, capacity config::INVENTORY_CAPACITY

    // Spawn the hunter at a random walkable tile a short distance from the
    // player, so for this first cut it has ground to path across toward them.
    std::mt19937_64 rng(config::WORLD_SEED ^ 0xE1u);
    std::uniform_int_distribution<int> offset(-config::ENEMY_SPAWN_RADIUS,
                                              config::ENEMY_SPAWN_RADIUS);
    const TileCoord playerTile = worldToTile(spawn);
    const TileCoord enemyOrigin{playerTile.x + offset(rng), playerTile.y + offset(rng)};
    const Vec2 enemySpawn = findWalkableSpawn(enemyOrigin);

    enemy_ = registry_.create();
    registry_.emplace<Position>(enemy_, enemySpawn);
    registry_.emplace<Velocity>(enemy_, Vec2{0.0f, 0.0f});
    registry_.emplace<Sprite>(enemy_, 32.0f);
    registry_.emplace<Collider>(enemy_, config::COLLIDER_HALF);
    registry_.emplace<Enemy>(enemy_);
    registry_.emplace<PathFollower>(enemy_);

    chunks_.update(worldToChunk(spawn));
}

void World::update(float dt) {
    const auto solid = [this](TileCoord t) { return isSolid(chunks_.tileAt(t)); };

    // Turn the player's input intent into Velocity + stamina before anything
    // integrates, so movementSystem below moves the player this frame.
    updatePlayer(dt);

    // Hunter chooses/steers its path toward the player (sets Velocity only)...
    pathfindingSystem(registry_, dt, playerPosition(), solid);

    // ...then all mobile entities integrate against terrain: water blocks, land
    // slides. This runs after pathfinding so the hunter's fresh velocity applies.
    movementSystem(registry_, dt, solid);

    // Re-center chunk streaming on the player each tick.
    chunks_.update(worldToChunk(playerPosition()));
}

void World::setPlayerInput(Vec2 dir, bool sprint) {
    playerMoveDir_ = dir;
    playerSprinting_ = sprint;
}

void World::updatePlayer(float dt) {
    auto& stamina = registry_.get<Stamina>(player_);
    const float sprintSpeed = registry_.get<Player>(player_).speed;

    // Sprinting only counts when actually moving and there's stamina left, so a
    // stationary player holding shift neither speeds up nor drains, and running
    // dry silently falls back to walking.
    const bool moving = playerMoveDir_.x != 0.0f || playerMoveDir_.y != 0.0f;
    const bool sprinting = playerSprinting_ && moving && stamina.current > 0.0f;

    if (sprinting) {
        stamina.current = std::max(0.0f, stamina.current - config::STAMINA_DRAIN_RATE * dt);
    } else {
        stamina.current = std::min(stamina.max, stamina.current + config::STAMINA_REGEN_RATE * dt);
    }

    const float factor = sprinting ? 1.0f : config::WALK_SPEED_FACTOR;
    registry_.get<Velocity>(player_).value = playerMoveDir_ * (sprintSpeed * factor);
}

Inventory& World::inventory() {
    return registry_.get<Inventory>(player_);
}

const Inventory& World::inventory() const {
    return registry_.get<Inventory>(player_);
}

Vec2 World::playerPosition() const {
    return registry_.get<Position>(player_).value;
}

Vec2 World::enemyPosition() const {
    return registry_.get<Position>(enemy_).value;
}

Vec2 World::findWalkableSpawn(TileCoord origin) const {
    // Spiral outward in tile rings from `origin`; spawn at the first non-solid
    // tile's center so nothing starts inside water.
    for (int r = 0; r < 256; ++r) {
        for (int dy = -r; dy <= r; ++dy) {
            for (int dx = -r; dx <= r; ++dx) {
                if (std::max(std::abs(dx), std::abs(dy)) != r) continue; // ring only
                const TileCoord t{origin.x + dx, origin.y + dy};
                if (!isSolid(chunks_.tileAt(t))) {
                    return Vec2{(static_cast<float>(t.x) + 0.5f) * TILE_SIZE,
                                (static_cast<float>(t.y) + 0.5f) * TILE_SIZE};
                }
            }
        }
    }
    return Vec2{(static_cast<float>(origin.x) + 0.5f) * TILE_SIZE,
                (static_cast<float>(origin.y) + 0.5f) * TILE_SIZE};
}

} // namespace lurk
