# lurk

A 2D survival game in C++ where a **learning hunter** stalks you across an
**infinite, procedurally generated world**. Follow seed-placed clues to the hunter's
lair, infiltrate it, and steal what it guards before it learns your habits, finds
your base, and traps the paths you walk most.

> ⚠️ **Early development.**

## The idea

You survive in an infinite world while a hunter tracks you. Getting caught ends the
run and resets the world with a new seed. Your objective is to follow seed-placed
clues to the hunter's lair, infiltrate it, and steal an object, which ends the game.
The hunter also learns your habits: it can find your base and mine/trap the places
you frequent.

- **Infinite procedural world.** Terrain, biomes, and resources are generated per
  chunk from a single seed. New seed = new world.
- **A hunter that learns.** It starts as an ordinary pathfinding enemy (patrol →
  chase → search), then grows a statistical model of your movement, predicting where
  you'll go, locating your base from where you linger, and planting traps there.
- **A directed goal.** Clue objects across the world point toward the hunter's lair,
  sparse and vague far out, dense and directional as you close in. The endgame is to
  lure the hunter away from home, then rob it.
- **Emergent tension.** Following the clue trail means moving *predictably*, which
  feeds the hunter's model. Vary your routes to survive; beeline and get trapped.

## Tech stack

| Concern | Tool |
|---|---|
| Language | C++20 |
| Build | CMake |
| Dependencies | Conan 2 |
| Rendering / input / audio | [raylib](https://www.raylib.com/) |
| Architecture | [EnTT](https://github.com/skypjack/entt) (ECS) |
| Procedural gen | Chunked, per-seed placeholder terrain now; FastNoiseLite planned |
| Enemy AI | A* pathfinding implemented; FSM (patrol/chase/search) planned, then Markov → Q-learning → deep RL |

## Getting started

### Prerequisites

A C++20 compiler, [CMake](https://cmake.org/) ≥ 3.15, and
[Conan 2](https://conan.io/). raylib and its system dependencies are pulled in by
Conan.

### Build & run

```sh
# 1. Resolve dependencies + generate the CMake toolchain
conan install . --build=missing

# 2. Configure
cmake --preset conan-release

# 3. Compile
cmake --build --preset conan-release

# 4. Run
./build/Release/game
```

On a hybrid-GPU laptop, run on the discrete GPU with `prime-run ./build/Release/game`.

Full details (debug builds, adding dependencies, clean rebuilds, and common
pitfalls) are in [BUILD.md](BUILD.md).

## Project layout

```
src/
  main.cpp          entry point
  core/
    Game.*          Game class: window, camera, and the input→update→render loop
  sim/              engine-agnostic simulation (no raylib types)
    World.*         owns the EnTT registry, spawns player/enemy, drives the tick
    Components.hpp  ECS components (Position, Velocity, Sprite, Collider, Enemy, ...)
    Chunk.hpp        one chunk's tile data
    ChunkManager.*   streams chunks in/out around the player
    WorldGen.*       per-chunk deterministic generation (placeholder terrain for now)
    Pathfinding.hpp  A* over the tile grid
    Movement.hpp     velocity integration + collision resolution
    Collision.hpp    AABB checks against solid tiles
    Coords.hpp       tile/chunk/world coordinate conversions
    Config.hpp       tunable constants (speeds, tile size, spawn radius, ...)
    Vec2.hpp         engine-agnostic 2D vector
CMakeLists.txt    build definition
conanfile.txt     dependency manifest
ROADMAP.md        phase-by-phase development plan
BUILD.md          build & dependency reference
```

## Roadmap

The guiding rule: build a dumb-but-complete game first (Phases 0–6), then layer
intelligence (Phase 7), the endgame (Phase 8), and polish (Phase 9). Each phase gets
running on screen before the next begins. See [ROADMAP.md](ROADMAP.md) for the full
task-by-task plan; the summary below tracks where each phase currently stands.

### Phase 3: Procedural world generation (infinite) — in progress
Infinite world → **chunking is mandatory**.
- [x] Generate the world in chunks (e.g. 16×16 tiles) on demand; unload distant chunks.
  Each chunk's RNG is seeded deterministically via a hash of `(seed, chunkX, chunkY)`
  so revisited chunks regenerate identically.
- [x] Thread a single **seed** through all generation.
- [ ] Add FastNoiseLite; threshold a noise map into terrain tiles
  (water/sand/grass/rock) — terrain is a deterministic placeholder until this lands.
- [ ] Layer multiple octaves (fractal mode) for natural terrain.
- [ ] Map (elevation, moisture) noise pairs to biomes; scatter objects (trees, rocks,
  ore) with noise-weighted probability so they cluster.
- [ ] **Lair + clues:** derive the lair coordinate `L` deterministically from the seed
  (a point on a ring ~2000–5000 tiles from spawn). Because `L` is a pure function of
  the seed, each chunk can compute its direction/distance to `L` locally. Place
  directional clue objects oriented toward `L`, sparse far out and dense near it,
  confined to a widening corridor so the hunt sharpens on approach.

### Phase 4: Survival mechanics
- Add health (and optionally hunger/thirst) to the player.
- Make resource objects harvestable (walk into a tree → gain wood).
- Build a simple inventory and a HUD showing stats (Dear ImGui to start).

### Phase 5: Base building
- Place tiles/structures on the grid, snapped to cells, with placement validation and
  collision.
- Store player modifications as **deltas** separate from the generated world, keyed
  per-chunk and saved/loaded with their chunk.

### Phase 6: Basic enemy + game loop closure — in progress
- [x] Implement **A\*** pathfinding on the tile grid; the hunter entity spawns and
  continuously paths toward the player.
- [ ] Add line-of-sight detection.
- [ ] Build enemy states (FSM or behavior tree): **Patrol → Chase → Search
  last-known-position → give up** (currently the hunter only chases).
- [ ] On catch: game over → regenerate the world with a new seed.
- *Once this phase closes, the game is complete and playable.*

### Phase 7: The adaptive/learning hunter
Done in order; each is a real milestone. The learning system stays decoupled from the
engine: the game logs data and asks "where will the player go?"; the model trains
offline or in a separate process.
- **Level 1, Markov chain:** log cell-to-cell transitions, build a probability map,
  and have the hunter predict and cut off routes instead of just chasing.
- **Level 2, Tabular Q-learning:** reward the hunter for closing distance / catching;
  learn a policy over a Q-table. Genuine RL, no neural net.
- **Level 3, Deep RL:** expose game state over a socket to a Python process, train
  there (PyTorch + Gymnasium), export the model, and load it in C++ via libtorch for
  inference. Stretch goal.
- **Base-finding & trap-laying** (builds on Level 1's frequency data): build a
  visitation heatmap, identify the base as the highest-density cluster, and have the
  hunter periodically travel to hotspots to plant mines/traps, lurking the perimeter
  rather than breaching. Traps get clear tells so they're learnable, not cheap.

### Phase 8: Endgame, infiltrate the lair
- Generate the lair as a distinct, defended structure at coordinate `L`.
- Give the hunter a separate **base-defense** behavior branch (guarding its lair),
  distinct from its roaming/hunting behavior.
- Place the stealable object inside; grabbing it triggers the ending.
- **Bait-and-rob:** the prediction AI makes the hunter chase where it *expects* you to
  be, so the intended play is to lure it far from home, then rush the undefended lair.
  The smarter the enemy, the more exploitable this becomes.

### Phase 9: Polish and ship
- Save/load with nlohmann/json or cereal, storing the seed + per-chunk deltas + player
  state + model data (the infinite world is just the seed; only changes are stored).
- Add sound effects, music, and simple sprite animations.
- Build and package release binaries per platform.

