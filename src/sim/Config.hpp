#pragma once

#include <cstdint>

namespace lurk {

// One home for the game's tunable constants, so values like the chunk size stop
// being magic numbers sprinkled across the code.
//
// They fall into two groups, and the split matters for making these settings
// file-driven (e.g. JSON) later:
//
//   * STRUCTURAL constants are compile-time by nature. Fixed-size arrays are
//     dimensioned from them -- Chunk stores its tiles in std::array<..,
//     CHUNK_TILES * CHUNK_TILES> -- so they must stay `constexpr` and CANNOT be
//     read from a config file without first switching those containers to
//     dynamic storage (std::vector) and paying the runtime cost.
//
//   * TUNING constants are plain default values with no such constraint. A
//     future GameConfig struct loaded from JSON at startup could override any of
//     these; today they are the defaults baked in.
namespace config {

// --- World grid (STRUCTURAL: compile-time only) ---
inline constexpr int TILE_SIZE = 32;    // pixels per tile edge
inline constexpr int CHUNK_TILES = 16;  // tiles per chunk edge (Chunk arrays size off this)

// --- Chunk streaming (TUNING) ---
// Unload radius exceeds load radius on purpose: the gap is hysteresis, so pacing
// back and forth across a chunk border doesn't thrash chunks.
inline constexpr int LOAD_RADIUS = 3;   // chunks kept around the center (7x7 = 49)
inline constexpr int UNLOAD_RADIUS = 5; // evict beyond this Chebyshev distance

// --- World generation (TUNING) ---
inline constexpr std::uint64_t WORLD_SEED = 1337; // new seed => new world

// --- Entities (TUNING) ---
inline constexpr float PLAYER_SPEED = 200.0f;   // px/s
inline constexpr float ENEMY_SPEED = 160.0f;    // px/s
inline constexpr float COLLIDER_HALF = 12.0f;   // hitbox half-extent (24px box in 32px tiles)
inline constexpr int ENEMY_SPAWN_RADIUS = 18;   // tiles from the player for the hunter's spawn

// --- Enemy pathfinding (TUNING) ---
inline constexpr float REPATH_INTERVAL = 0.5f;         // seconds between A* recomputes
inline constexpr float WAYPOINT_ARRIVE_DIST = 2.0f;    // px within a waypoint counts as reached
inline constexpr int PATHFIND_MAX_EXPANSIONS = 4000;   // A* node-pop budget (world is infinite)

} // namespace config

// The grid constants are referenced pervasively (Coords, Chunk, rendering), so
// expose them unqualified in `lurk`: existing `TILE_SIZE` / `CHUNK_TILES` call
// sites keep working and stay terse. Tuning values are read as `config::NAME`,
// which flags them at the call site as knobs rather than fixed structure.
using config::CHUNK_TILES;
using config::TILE_SIZE;

} // namespace lurk
