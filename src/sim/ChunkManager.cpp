#include "sim/ChunkManager.hpp"

#include <algorithm>
#include <cstdlib>

#include "sim/WorldGen.hpp"

namespace lurk {

void ChunkManager::update(ChunkCoord center) {
    // Load: ensure the (2*kLoadRadius+1)^2 block around the center is resident.
    for (int dy = -kLoadRadius; dy <= kLoadRadius; ++dy) {
        for (int dx = -kLoadRadius; dx <= kLoadRadius; ++dx) {
            residentChunk({center.x + dx, center.y + dy});
        }
    }

    // Unload: evict anything beyond the unload radius (Chebyshev distance).
    for (auto it = chunks_.begin(); it != chunks_.end();) {
        const int dist = std::max(std::abs(it->first.x - center.x),
                                  std::abs(it->first.y - center.y));
        if (dist > kUnloadRadius) {
            it = chunks_.erase(it);
        } else {
            ++it;
        }
    }
}

TileId ChunkManager::tileAt(TileCoord t) const {
    const ChunkCoord c = tileToChunk(t);
    const auto it = chunks_.find(c);
    if (it != chunks_.end()) {
        return it->second.at(tileLocal(t));
    }
    return generateChunk(seed_, c).at(tileLocal(t)); // non-resident fallback
}

Chunk& ChunkManager::residentChunk(ChunkCoord c) {
    auto it = chunks_.find(c);
    if (it == chunks_.end()) {
        it = chunks_.emplace(c, generateChunk(seed_, c)).first;
    }
    return it->second;
}

} // namespace lurk
