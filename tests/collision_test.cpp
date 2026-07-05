// AABB tile-collision tests against a synthetic map (no world/noise needed).

#include "sim/Collision.hpp"

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
    // A vertical wall: every tile with tx >= 5 is solid (world x >= 160).
    const auto solid = [](TileCoord t) { return t.x >= 5; };
    const float half = 16.0f;

    // Detection: clear space is free; a box inside the wall region collides.
    CHECK(!boxCollides(Vec2{50.0f, 50.0f}, half, solid));
    CHECK(boxCollides(Vec2{200.0f, 50.0f}, half, solid));

    // Walking right in small steps stops flush against the wall, never inside.
    Vec2 p{80.0f, 100.0f};
    for (int i = 0; i < 300; ++i) p = resolveMove(p, half, Vec2{3.0f, 0.0f}, solid);
    CHECK(!boxCollides(p, half, solid));
    CHECK(p.x + half <= 5 * TILE_SIZE);       // never crosses into the wall
    CHECK(p.x + half >= 5 * TILE_SIZE - 3);   // stops within one step of it
    CHECK(p.y == 100.0f);                     // no drift on the free axis

    // Diagonal into the wall: X is blocked but Y slides freely along it.
    Vec2 q{80.0f, 100.0f};
    for (int i = 0; i < 300; ++i) q = resolveMove(q, half, Vec2{3.0f, 3.0f}, solid);
    CHECK(q.x + half <= 5 * TILE_SIZE);       // blocked on X
    CHECK(q.y >= 100.0f + 3.0f * 250.0f);     // Y kept advancing (slid)

    // Speed-preserving slide: a body flush against the wall pushed mostly
    // sideways (big X, small Y) has its blocked X speed redirected into Y, so it
    // advances by |dx|+|dy|, not just the tiny |dy| it would crawl otherwise.
    {
        Vec2 r{144.0f, 100.0f}; // right edge flush at world x=160 (wall boundary)
        r = resolveMove(r, half, Vec2{20.0f, 4.0f}, solid);
        CHECK(r.x == 144.0f);   // X stayed blocked
        CHECK(r.y == 124.0f);   // Y advanced by 4 + 20 (full speed preserved)
    }

    // Convex-corner rounding: a one-tile-wide bar (column 5, rows <= 5). A body
    // greedily steering toward a goal past the bar's bottom slides down at full
    // speed and rounds the corner, reaching the far side -- it does not crawl to
    // a halt against the corner.
    {
        const auto bar = [](TileCoord t) { return t.x == 5 && t.y <= 5; };
        const Vec2 goal{8.5f * TILE_SIZE, 8.5f * TILE_SIZE};
        Vec2 c{4.5f * TILE_SIZE, 3.5f * TILE_SIZE};
        for (int i = 0; i < 600; ++i) {
            const Vec2 to{goal.x - c.x, goal.y - c.y};
            c = resolveMove(c, half, normalized(to) * (160.0f / 60.0f), bar);
        }
        const float dx = goal.x - c.x, dy = goal.y - c.y;
        CHECK(!boxCollides(c, half, bar));
        CHECK(std::sqrt(dx * dx + dy * dy) < TILE_SIZE); // reached the far side
    }

    // Open terrain: movement is unobstructed.
    const auto empty = [](TileCoord) { return false; };
    Vec2 f{0.0f, 0.0f};
    f = resolveMove(f, half, Vec2{10.0f, -7.0f}, empty);
    CHECK(f.x == 10.0f && f.y == -7.0f);

    if (g_failures == 0) {
        std::puts("collision_test: all assertions passed");
        return 0;
    }
    std::printf("collision_test: %d failure(s)\n", g_failures);
    return 1;
}
