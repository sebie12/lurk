#include "sim/WorldGen.hpp"

#include "FastNoiseLite.h"

namespace lurk {
namespace {

// splitmix64 finalizer: strong avalanche, so nearby integer inputs produce
// uncorrelated outputs. Used to hash seeds and coordinates.
uint64_t mix(uint64_t x) {
    x += 0x9E3779B97F4A7C15ull;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ull;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBull;
    return x ^ (x >> 31);
}

// Hash of (seed, x, y) into a well-distributed 64-bit value. Folding the
// coordinates one at a time (rather than adding them) keeps (x,y) and (y,x)
// distinct.
uint64_t hashCoords(uint64_t seed, int x, int y) {
    uint64_t h = mix(seed);
    h = mix(h ^ static_cast<uint32_t>(x));
    h = mix(h ^ static_cast<uint32_t>(y));
    return h;
}

// A fractal OpenSimplex2 field. Low frequency == large features (continents);
// more octaves == more fine detail layered on top. The FBm shape (octave
// count, lacunarity, gain) is tunable from Config.hpp.
FastNoiseLite makeNoise(int seed, float frequency) {
    FastNoiseLite n;
    n.SetSeed(seed);
    n.SetNoiseType(FastNoiseLite::NoiseType_OpenSimplex2);
    n.SetFractalType(FastNoiseLite::FractalType_FBm);
    n.SetFractalOctaves(config::FRACTAL_OCTAVES);
    n.SetFractalLacunarity(config::FRACTAL_LACUNARITY);
    n.SetFractalGain(config::FRACTAL_GAIN);
    n.SetFrequency(frequency);
    return n;
}

// Map an (elevation, moisture) pair, each in [0,1], to a terrain tile. The
// cut-off levels are tunable in Config.hpp, so how often each terrain appears
// can be adjusted without touching this logic.
TileId biome(float elevation, float moisture) {
    if (elevation < config::WATER_LEVEL) return TileId::Water; // seas and lakes
    if (elevation < config::BEACH_LEVEL) return TileId::Sand;  // beaches at the shoreline
    if (elevation > config::ROCK_LEVEL) return TileId::Rock;   // mountains
    // dry flats vs grassland
    return moisture < config::DRY_MOISTURE ? TileId::Sand : TileId::Grass;
}

} // namespace

uint64_t chunkSeed(uint64_t seed, ChunkCoord c) {
    return hashCoords(seed, c.x, c.y);
}

Chunk generateChunk(uint64_t seed, ChunkCoord c) {
    // Two decorrelated noise fields derived from the one world seed. Sampling at
    // GLOBAL tile coordinates makes the terrain one continuous function over the
    // whole world, so chunks are seamless at their borders.
    FastNoiseLite elevation = makeNoise(static_cast<int>(seed), config::ELEVATION_FREQUENCY);
    FastNoiseLite moisture =
        makeNoise(static_cast<int>(seed ^ 0x9E3779B97F4A7C15ull), config::MOISTURE_FREQUENCY);

    Chunk chunk;
    for (int ly = 0; ly < CHUNK_TILES; ++ly) {
        for (int lx = 0; lx < CHUNK_TILES; ++lx) {
            const float gx = static_cast<float>(c.x * CHUNK_TILES + lx);
            const float gy = static_cast<float>(c.y * CHUNK_TILES + ly);

            // GetNoise returns ~[-1,1]; remap to [0,1].
            const float e = elevation.GetNoise(gx, gy) * 0.5f + 0.5f;
            const float m = moisture.GetNoise(gx, gy) * 0.5f + 0.5f;

            const TileCoord local{lx, ly};
            chunk.at(local) = biome(e, m);
            // Elevation doubles as the column's surface height (heightmap z).
            chunk.heightAt(local) = static_cast<uint8_t>(e * 255.0f);
        }
    }
    return chunk;
}

} // namespace lurk
