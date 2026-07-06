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

// --- Terrain resolution (one knob for how finely the world is tiled) ---
// Every value that depends on tile granularity derives from this: the tile size,
// the noise feature scale (ELEVATION/MOISTURE_FREQUENCY), the enemy spawn radius
// (measured in tiles), and the pathfinding budget. So retuning terrain detail is
// a single edit here rather than a hand-balanced sweep across the file. Each
// derived value pairs a BASE_* reference (its value at resolution 1) with the
// scaling rule below it.
//
// Expressed relative to a 32px baseline tile: 1 = coarse (32px tiles), 2 = 16px,
// 4 = 8px. Higher => finer terrain, smoother biome edges, more tiles on screen.
// PLAYER_SPEED, ENEMY_SPEED and COLLIDER_HALF stay in pixels and are deliberately
// NOT scaled, so the game's physical scale and feel are unchanged when you retune.
inline constexpr int TERRAIN_RESOLUTION = 2;

// --- World grid (STRUCTURAL: compile-time only) ---
inline constexpr int BASE_TILE_SIZE = 32; // tile edge in px at resolution 1
static_assert(BASE_TILE_SIZE % TERRAIN_RESOLUTION == 0,
              "TERRAIN_RESOLUTION must divide BASE_TILE_SIZE evenly (use 1, 2, 4, 8, 16 or 32)");
inline constexpr int TILE_SIZE = BASE_TILE_SIZE / TERRAIN_RESOLUTION; // pixels per tile edge
inline constexpr int CHUNK_TILES = 32;  // tiles per chunk edge (Chunk arrays size off this)

// --- Chunk streaming (TUNING) ---
// Expressed as a view distance in TILES so the loaded world stays the same size
// regardless of CHUNK_TILES: the per-chunk radii are derived below, so changing
// the chunk size auto-adjusts how many chunks are kept resident.
inline constexpr int LOAD_DISTANCE_TILES = 48;  // keep terrain resident at least this far out
inline constexpr int UNLOAD_MARGIN_TILES = 32;  // extra slack before eviction (hysteresis gap)

// Ceil-divide into chunks so the resident region always covers the requested
// distance. Unload radius exceeds load radius on purpose: the gap is hysteresis,
// so pacing back and forth across a chunk border doesn't thrash chunks.
inline constexpr int LOAD_RADIUS = (LOAD_DISTANCE_TILES + CHUNK_TILES - 1) / CHUNK_TILES;
inline constexpr int UNLOAD_RADIUS =
    (LOAD_DISTANCE_TILES + UNLOAD_MARGIN_TILES + CHUNK_TILES - 1) / CHUNK_TILES;
static_assert(UNLOAD_RADIUS > LOAD_RADIUS, "unload radius must exceed load radius for hysteresis");

// --- World generation (TUNING) ---
inline constexpr std::uint64_t WORLD_SEED = 1337; // new seed => new world

// Terrain-type frequency knobs. Both noise fields (elevation, moisture) are
// remapped to [0,1] and are roughly uniform, so each threshold below is
// approximately the FRACTION of the world that falls on its side of the cut --
// a handy mental model for tuning how often a terrain appears.
//
// Elevation bands, from low ground up (must stay ordered
// WATER_LEVEL <= BEACH_LEVEL <= ROCK_LEVEL):
//   elevation < WATER_LEVEL            -> Water   (raise => more sea/lakes)
//   WATER_LEVEL..BEACH_LEVEL           -> Sand    (shoreline beaches)
//   elevation > ROCK_LEVEL             -> Rock    (lower => more mountains)
// The middle band splits on moisture:
//   moisture < DRY_MOISTURE            -> Sand    (dry flats; raise => more desert)
//   otherwise                          -> Grass
inline constexpr float WATER_LEVEL = 0.38f;   // ~38% of the world is water
inline constexpr float BEACH_LEVEL = 0.44f;   // sand up to here, then inland terrain
inline constexpr float ROCK_LEVEL = 0.72f;    // top ~28% of elevation is rocky
inline constexpr float DRY_MOISTURE = 0.35f;  // ~35% of mid-elevation land is dry sand

// Fractal (FBm) noise shape. The elevation/moisture fields are fractal Brownian
// motion: several octaves of OpenSimplex2 summed, each octave higher frequency
// (scaled by LACUNARITY) and lower amplitude (scaled by GAIN) than the last.
// More octaves + higher gain => rougher, more detailed terrain; fewer octaves
// => smoother, blobbier continents. Frequency sets the base feature scale:
// smaller value => larger landmasses.
inline constexpr int FRACTAL_OCTAVES = 4;         // detail layers summed per sample
inline constexpr float FRACTAL_LACUNARITY = 2.0f; // per-octave frequency multiplier
inline constexpr float FRACTAL_GAIN = 0.5f;       // per-octave amplitude multiplier
// Frequencies are per-tile, so a finer grid (more tiles per feature) needs a
// proportionally lower frequency to keep features the same on-screen size. Hence
// divide the resolution-1 base by TERRAIN_RESOLUTION: the terrain is unchanged in
// scale, just drawn at finer resolution.
inline constexpr float BASE_ELEVATION_FREQUENCY = 0.008f; // elevation base scale at resolution 1
inline constexpr float BASE_MOISTURE_FREQUENCY = 0.012f;  // moisture base scale at resolution 1
inline constexpr float ELEVATION_FREQUENCY = BASE_ELEVATION_FREQUENCY / TERRAIN_RESOLUTION;
inline constexpr float MOISTURE_FREQUENCY = BASE_MOISTURE_FREQUENCY / TERRAIN_RESOLUTION;

// --- Entities (TUNING) ---
// PLAYER_SPEED is the player's SPRINT (full) speed; walking is WALK_SPEED_FACTOR
// of it (see the vitals block below).
inline constexpr float PLAYER_SPEED = 200.0f;   // px/s (sprint)
inline constexpr float ENEMY_SPEED = 160.0f;    // px/s
inline constexpr float COLLIDER_HALF = 12.0f;   // hitbox half-extent in px (unaffected by resolution)
// In tiles, so it scales with resolution to keep the spawn a constant pixel
// distance from the player as tiles get finer.
inline constexpr int BASE_ENEMY_SPAWN_RADIUS = 18; // tiles at resolution 1
inline constexpr int ENEMY_SPAWN_RADIUS = BASE_ENEMY_SPAWN_RADIUS * TERRAIN_RESOLUTION;

// --- Player vitals (TUNING) ---
// Health and stamina both start full at MAX_* and are clamped to [0, max]. The
// HUD draws current/max as a filled bar.
inline constexpr float MAX_HEALTH = 100.0f;   // starting/maximum health
inline constexpr float MAX_STAMINA = 100.0f;  // starting/maximum stamina

// Sprinting (holding shift while moving) runs at PLAYER_SPEED and drains stamina;
// releasing shift (or running dry) drops to WALK_SPEED_FACTOR of that speed and
// stamina regenerates. Rates are per second so the drain/regen stay
// frame-independent.
inline constexpr float WALK_SPEED_FACTOR = 0.5f;   // walk = half of sprint speed
inline constexpr float STAMINA_DRAIN_RATE = 25.0f; // stamina/s spent while sprinting
inline constexpr float STAMINA_REGEN_RATE = 15.0f; // stamina/s recovered otherwise

// --- Inventory (TUNING) ---
inline constexpr int INVENTORY_CAPACITY = 20; // max distinct item stacks carried

// --- HUD (TUNING) ---
// Health/stamina bars, anchored bottom-left with health stacked above stamina.
// Pure pixel values (no raylib types) so they belong here; the bar COLOURS are
// raylib Colors and live in core/Hud.cpp, which is not engine-agnostic.
inline constexpr float HUD_BAR_WIDTH = 220.0f;   // bar length in px
inline constexpr float HUD_BAR_HEIGHT = 18.0f;   // bar thickness in px
inline constexpr float HUD_BAR_MARGIN = 20.0f;   // gap from the screen edges in px
inline constexpr float HUD_BAR_SPACING = 6.0f;   // vertical gap between the two bars in px
inline constexpr float HUD_BAR_BORDER = 2.0f;    // outline thickness in px

// --- Enemy pathfinding (TUNING) ---
inline constexpr float REPATH_INTERVAL = 0.5f;         // seconds between A* recomputes
inline constexpr float WAYPOINT_ARRIVE_DIST = 2.0f;    // px within a waypoint counts as reached
// A* node-pop budget (world is infinite). A path covering the same pixel span
// crosses ~RESOLUTION x more tiles per axis, and the explored area grows with the
// square of that, so scale the base budget by RESOLUTION^2.
inline constexpr int BASE_PATHFIND_MAX_EXPANSIONS = 4000; // at resolution 1
inline constexpr int PATHFIND_MAX_EXPANSIONS =
    BASE_PATHFIND_MAX_EXPANSIONS * TERRAIN_RESOLUTION * TERRAIN_RESOLUTION;

} // namespace config

// The grid constants are referenced pervasively (Coords, Chunk, rendering), so
// expose them unqualified in `lurk`: existing `TILE_SIZE` / `CHUNK_TILES` call
// sites keep working and stay terse. Tuning values are read as `config::NAME`,
// which flags them at the call site as knobs rather than fixed structure.
using config::CHUNK_TILES;
using config::TILE_SIZE;

} // namespace lurk
