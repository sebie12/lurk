#pragma once

#include <cstdint>

#include "sim/Chunk.hpp"
#include "sim/Coords.hpp"

namespace lurk {

// Per-chunk seed derived from the world seed via a hash mix. NOT seed+cx+cy:
// plain addition is commutative, so (1,0) and (0,1) would collide and generate
// identically. Reserved for chunk-level decisions (object scatter, lair) that
// need their own RNG stream.
uint64_t chunkSeed(uint64_t seed, ChunkCoord c);

// Pure function: the same (seed, coord) always yields the same chunk, so a
// revisited chunk regenerates identically and never needs to be stored.
// Placeholder terrain for now; swapped for FastNoiseLite in a later step.
Chunk generateChunk(uint64_t seed, ChunkCoord c);

} // namespace lurk
