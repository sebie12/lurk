#pragma once

#include <array>
#include <cstdint>

#include "sim/Coords.hpp"

namespace lurk {

enum class TileId : uint8_t { Water, Sand, Grass, Rock };

// Terrain that blocks movement. Water is impassable; land is walkable for now
// (rock may become solid cliffs once the heightmap drives collision).
inline bool isSolid(TileId id) { return id == TileId::Water; }

// A CHUNK_TILES x CHUNK_TILES block of tiles, stored row-major in one flat
// array (index = y * CHUNK_TILES + x). This is the unit the world generates,
// loads, and unloads as a single piece.
struct Chunk {
    std::array<TileId, CHUNK_TILES * CHUNK_TILES> tiles{};

    // Per-column surface height (the heightmap z, see ROADMAP Phase 3). One
    // scalar per (x,y) column, not a 3D grid; used by 2.5D rendering and later
    // gameplay (slopes, line-of-sight). Zero until noise generation fills it.
    std::array<uint8_t, CHUNK_TILES * CHUNK_TILES> height{};

    // local coordinates in [0, CHUNK_TILES); caller guarantees the bounds
    // (pair with tileLocal() from Coords.hpp).
    TileId& at(TileCoord local) { return tiles[local.y * CHUNK_TILES + local.x]; }
    TileId at(TileCoord local) const { return tiles[local.y * CHUNK_TILES + local.x]; }

    uint8_t& heightAt(TileCoord local) { return height[local.y * CHUNK_TILES + local.x]; }
    uint8_t heightAt(TileCoord local) const { return height[local.y * CHUNK_TILES + local.x]; }
};

} // namespace lurk
