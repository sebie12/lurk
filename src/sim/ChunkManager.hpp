#pragma once

#include <cstddef>
#include <cstdint>
#include <unordered_map>

#include "sim/Chunk.hpp"
#include "sim/Coords.hpp"

namespace lurk {

// Owns the resident chunks and streams them in/out around a moving center.
// Because generation is deterministic and pure, an unloaded chunk regenerates
// identically on return and never needs to be persisted (only player deltas
// will, later).
class ChunkManager {
public:
    explicit ChunkManager(uint64_t seed) : seed_(seed) {}

    // Ensure every chunk within the load radius of `center` is resident, then
    // evict those beyond the larger unload radius. Call as the player moves.
    void update(ChunkCoord center);

    // Terrain lookup. A resident chunk is read directly; a non-resident tile is
    // regenerated on the fly (cheap and deterministic) so callers never see a
    // hole. const: never changes residency.
    TileId tileAt(TileCoord t) const;

    std::size_t loadedCount() const { return chunks_.size(); }

private:
    Chunk& residentChunk(ChunkCoord c); // get-or-generate (mutating)

    uint64_t seed_;
    std::unordered_map<ChunkCoord, Chunk, ChunkCoordHash> chunks_;

    static constexpr int kLoadRadius = config::LOAD_RADIUS;
    static constexpr int kUnloadRadius = config::UNLOAD_RADIUS;
};

} // namespace lurk
