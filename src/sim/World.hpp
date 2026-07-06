#pragma once

#include <cstddef>

#include <entt/entt.hpp>

#include "sim/Chunk.hpp"
#include "sim/ChunkManager.hpp"
#include "sim/Coords.hpp"

namespace lurk {

struct Inventory; // defined in Components.hpp; only the accessors need the type

// The simulated world: owns the ECS registry (all entities) and the chunked
// terrain, and advances them each tick. Rendering lives in core/ and reads this
// state via registry() and tileAt().
class World {
public:
    World();

    void update(float dt);

    // Set the player's movement intent from input: `dir` need not be unit length,
    // `sprint` is whether the sprint key is held. Both are applied in update(),
    // where speed and stamina drain/regen are resolved against dt.
    void setPlayerInput(Vec2 dir, bool sprint);

    entt::registry& registry() { return registry_; }
    const entt::registry& registry() const { return registry_; }
    entt::entity player() const { return player_; }
    entt::entity enemy() const { return enemy_; }
    Vec2 playerPosition() const;
    Vec2 enemyPosition() const;

    // Convenience access to the player's Inventory component (see Inventory in
    // Components.hpp for why it lives on the entity rather than standalone).
    Inventory& inventory();
    const Inventory& inventory() const;

    // Terrain query used by rendering and collision. Reads the resident chunk,
    // regenerating on the fly for the rare off-radius tile.
    TileId tileAt(TileCoord t) const { return chunks_.tileAt(t); }
    std::size_t loadedChunkCount() const { return chunks_.loadedCount(); }

private:
    // First walkable tile found spiralling out from `origin`, in world pixels.
    Vec2 findWalkableSpawn(TileCoord origin) const;

    // Apply the stored input intent to the player: resolve sprint vs. walk speed
    // into Velocity and drain/regenerate Stamina over dt.
    void updatePlayer(float dt);

    entt::registry registry_;
    entt::entity player_{entt::null};
    entt::entity enemy_{entt::null};
    ChunkManager chunks_;

    // Player input intent, captured by setPlayerInput() and consumed in update().
    Vec2 playerMoveDir_{0.0f, 0.0f};
    bool playerSprinting_{false};
};

} // namespace lurk
