// Determinism and seed-mixing tests for chunk generation.

#include "sim/WorldGen.hpp"

#include <cstdio>

using namespace lurk;

static int g_failures = 0;

#define CHECK(cond)                                                       \
    do {                                                                  \
        if (!(cond)) {                                                    \
            std::printf("FAIL (line %d): %s\n", __LINE__, #cond);         \
            ++g_failures;                                                 \
        }                                                                 \
    } while (0)

int main() {
    const uint64_t seed = 1337;

    // Determinism: identical inputs -> identical chunk (the whole point; a
    // revisited chunk must regenerate the same, so it need not be stored).
    const Chunk a = generateChunk(seed, {3, -7});
    const Chunk b = generateChunk(seed, {3, -7});
    CHECK(a.tiles == b.tiles);

    // A different seed almost certainly yields a different chunk.
    const Chunk c = generateChunk(seed + 1, {3, -7});
    CHECK(!(a.tiles == c.tiles));

    // Generation is not a constant field: scanning a wide region turns up more
    // than one biome. (Robust to smooth noise, where two adjacent chunks can
    // legitimately be uniform, e.g. both open water.)
    bool seen[4] = {false, false, false, false};
    int distinct = 0;
    for (int cy = -4; cy <= 4; ++cy) {
        for (int cx = -4; cx <= 4; ++cx) {
            for (const TileId t : generateChunk(seed, {cx, cy}).tiles) {
                const int v = static_cast<int>(t);
                if (!seen[v]) {
                    seen[v] = true;
                    ++distinct;
                }
            }
        }
    }
    CHECK(distinct >= 2);

    // chunkSeed must NOT collide on transposed coordinates (the additive-seed
    // bug: seed+cx+cy gives the same value for (1,0) and (0,1)).
    CHECK(chunkSeed(seed, {1, 0}) != chunkSeed(seed, {0, 1}));
    CHECK(chunkSeed(seed, {2, 3}) != chunkSeed(seed, {3, 2}));
    CHECK(chunkSeed(seed, {-1, 0}) != chunkSeed(seed, {0, -1}));

    // Every generated tile is a valid TileId (0..3).
    for (const TileId t : a.tiles) {
        const int v = static_cast<int>(t);
        CHECK(v >= 0 && v <= 3);
    }

    if (g_failures == 0) {
        std::puts("worldgen_test: all assertions passed");
        return 0;
    }
    std::printf("worldgen_test: %d failure(s)\n", g_failures);
    return 1;
}
