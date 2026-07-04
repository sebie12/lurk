// Round-trip and edge-case tests for the tile/chunk coordinate math.
// Uses an always-on CHECK (not assert, which Release builds compile out via
// NDEBUG) so the test is meaningful regardless of build type.

#include "sim/Coords.hpp"

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
    // floordiv rounds toward negative infinity.
    CHECK(floordiv(0, 16) == 0);
    CHECK(floordiv(15, 16) == 0);
    CHECK(floordiv(16, 16) == 1);
    CHECK(floordiv(-1, 16) == -1);
    CHECK(floordiv(-16, 16) == -1);
    CHECK(floordiv(-17, 16) == -2);

    // floormod is always in [0, b).
    CHECK(floormod(0, 16) == 0);
    CHECK(floormod(-1, 16) == 15);
    CHECK(floormod(-16, 16) == 0);
    CHECK(floormod(17, 16) == 1);

    // The division identity a == floordiv*b + floormod holds across zero.
    for (int a = -100; a <= 100; ++a) {
        CHECK(floordiv(a, CHUNK_TILES) * CHUNK_TILES + floormod(a, CHUNK_TILES) == a);
        CHECK(floormod(a, CHUNK_TILES) >= 0 && floormod(a, CHUNK_TILES) < CHUNK_TILES);
    }

    // tile -> (chunk, local) -> tile round-trips, negatives included.
    for (int ty = -40; ty <= 40; ++ty) {
        for (int tx = -40; tx <= 40; ++tx) {
            const TileCoord t{tx, ty};
            const ChunkCoord c = tileToChunk(t);
            const TileCoord l = tileLocal(t);
            CHECK(l.x >= 0 && l.x < CHUNK_TILES && l.y >= 0 && l.y < CHUNK_TILES);
            CHECK(c.x * CHUNK_TILES + l.x == tx);
            CHECK(c.y * CHUNK_TILES + l.y == ty);
        }
    }

    // World pixels -> tile, covering sub-tile and negative positions.
    CHECK((worldToTile(Vec2{0.0f, 0.0f}) == TileCoord{0, 0}));
    CHECK((worldToTile(Vec2{TILE_SIZE - 0.1f, 0.0f}) == TileCoord{0, 0}));
    CHECK((worldToTile(Vec2{TILE_SIZE, 0.0f}) == TileCoord{1, 0}));
    CHECK((worldToTile(Vec2{-0.1f, -0.1f}) == TileCoord{-1, -1}));
    CHECK((worldToTile(Vec2{-static_cast<float>(TILE_SIZE), 0.0f}) == TileCoord{-1, 0}));

    if (g_failures == 0) {
        std::puts("coords_test: all assertions passed");
        return 0;
    }
    std::printf("coords_test: %d failure(s)\n", g_failures);
    return 1;
}
