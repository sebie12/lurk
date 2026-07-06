#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "sim/Coords.hpp"

namespace lurk {

// Terrain types. Keep `Count` last: it is a sentinel that sizes the tables
// below and lets loops cover every type without hardcoding a number. To add a
// terrain (e.g. DeepWater, ShallowWater, DeepGrass), insert it before `Count`
// and give it a row in `kTerrain` here and in `kPalette` (core/Game.cpp).
enum class TileId : uint8_t { Water, Sand, Grass, Rock, Count };

// Number of concrete terrain types (Count itself is not one).
inline constexpr std::size_t TILE_TYPE_COUNT = static_cast<std::size_t>(TileId::Count);

// Gameplay attributes of a terrain type, kept separate from its on-screen
// colour (a raylib type, so it lives in core/). This is the sim's single source
// of truth for how terrain behaves; adding a flag here (e.g. `slows`) extends
// every terrain at once.
struct TerrainProps {
    bool solid = false;     // blocks movement: collision and pathfinding treat it as a wall
    bool conceals = false;  // hides its occupant from line-of-sight (e.g. future deep grass)
};

// One row per TileId, in enum order. Defaults are "open ground", so a row only
// spells out what differs. The static_assert makes a forgotten row a compile
// error rather than a tile that silently behaves like open ground.
inline constexpr TerrainProps kTerrain[] = {
    /* Water */ {.solid = true},
    /* Sand  */ {},
    /* Grass */ {},
    /* Rock  */ {},
};
static_assert(std::size(kTerrain) == TILE_TYPE_COUNT,
              "every TileId before Count needs a row in kTerrain");

inline constexpr const TerrainProps& terrain(TileId id) {
    return kTerrain[static_cast<std::size_t>(id)];
}

// Terrain that blocks movement. Reads the table, so making a new terrain solid
// (e.g. deep water) is a one-line change to its row, not an edit here.
inline constexpr bool isSolid(TileId id) { return terrain(id).solid; }

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
