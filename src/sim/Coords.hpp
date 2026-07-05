#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>

#include "sim/Config.hpp" // TILE_SIZE, CHUNK_TILES
#include "sim/Vec2.hpp"

namespace lurk {

// Integer coordinate on the tile grid (one unit == one tile).
struct TileCoord {
    int x = 0;
    int y = 0;
};

// Integer coordinate on the chunk grid (one unit == CHUNK_TILES tiles).
struct ChunkCoord {
    int x = 0;
    int y = 0;
};

inline constexpr bool operator==(TileCoord a, TileCoord b) { return a.x == b.x && a.y == b.y; }
inline constexpr bool operator==(ChunkCoord a, ChunkCoord b) { return a.x == b.x && a.y == b.y; }

// Floor division: rounds toward negative infinity, unlike C++ '/', which
// truncates toward zero. Required because the world extends into negative
// coordinates: floordiv(-1, 16) == -1, whereas -1 / 16 == 0.
inline constexpr int floordiv(int a, int b) {
    const int q = a / b;
    const int r = a % b;
    return (r != 0 && ((r < 0) != (b < 0))) ? q - 1 : q;
}

// Positive modulo: result is always in [0, b) for b > 0. Companion to floordiv,
// giving a tile's index within its chunk. floormod(-1, 16) == 15.
inline constexpr int floormod(int a, int b) {
    const int r = a % b;
    return (r != 0 && ((r < 0) != (b < 0))) ? r + b : r;
}

// World pixels -> tile grid.
inline TileCoord worldToTile(Vec2 p) {
    return {floordiv(static_cast<int>(std::floor(p.x)), TILE_SIZE),
            floordiv(static_cast<int>(std::floor(p.y)), TILE_SIZE)};
}

// Tile grid -> chunk grid.
inline constexpr ChunkCoord tileToChunk(TileCoord t) {
    return {floordiv(t.x, CHUNK_TILES), floordiv(t.y, CHUNK_TILES)};
}

// A tile's local index within its own chunk; both components in [0, CHUNK_TILES).
inline constexpr TileCoord tileLocal(TileCoord t) {
    return {floormod(t.x, CHUNK_TILES), floormod(t.y, CHUNK_TILES)};
}

// World pixels -> chunk grid (convenience).
inline ChunkCoord worldToChunk(Vec2 p) { return tileToChunk(worldToTile(p)); }

// Hash for using ChunkCoord as an unordered_map key: pack the two 32-bit
// components into 64 bits, then avalanche so nearby chunks don't cluster.
struct ChunkCoordHash {
    std::size_t operator()(ChunkCoord c) const noexcept {
        std::uint64_t h = (static_cast<std::uint64_t>(static_cast<std::uint32_t>(c.x)) << 32) |
                          static_cast<std::uint32_t>(c.y);
        h *= 0x9E3779B97F4A7C15ull;
        return static_cast<std::size_t>(h ^ (h >> 32));
    }
};

// Same packing for TileCoord: A* keeps its open/closed sets keyed by tile.
struct TileCoordHash {
    std::size_t operator()(TileCoord t) const noexcept {
        std::uint64_t h = (static_cast<std::uint64_t>(static_cast<std::uint32_t>(t.x)) << 32) |
                          static_cast<std::uint32_t>(t.y);
        h *= 0x9E3779B97F4A7C15ull;
        return static_cast<std::size_t>(h ^ (h >> 32));
    }
};

} // namespace lurk
