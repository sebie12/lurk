// Streaming (load/unload) and lookup-consistency tests for ChunkManager.

#include "sim/ChunkManager.hpp"
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
    ChunkManager cm(seed);

    // load radius 3 -> a 7x7 block resident around the center.
    cm.update({0, 0});
    CHECK(cm.loadedCount() == 49);

    // A resident tile matches pure generation.
    const TileCoord resident{5, 5}; // in chunk (0,0)
    CHECK(cm.tileAt(resident) == generateChunk(seed, {0, 0}).at(tileLocal(resident)));

    // A far, non-resident tile is still correct (regenerated on the fly).
    const TileCoord far{1000, -1000};
    CHECK(cm.tileAt(far) == generateChunk(seed, tileToChunk(far)).at(tileLocal(far)));

    // Streaming stays bounded: move the center far away and the count holds.
    cm.update({100, 100});
    CHECK(cm.loadedCount() == 49);

    // The old origin chunk is now evicted but remains queryable and identical.
    CHECK(cm.tileAt(resident) == generateChunk(seed, {0, 0}).at(tileLocal(resident)));

    // Small moves don't drop below the loaded block (hysteresis keeps it >= 49).
    cm.update({100, 101});
    CHECK(cm.loadedCount() >= 49);

    if (g_failures == 0) {
        std::puts("chunkmanager_test: all assertions passed");
        return 0;
    }
    std::printf("chunkmanager_test: %d failure(s)\n", g_failures);
    return 1;
}
