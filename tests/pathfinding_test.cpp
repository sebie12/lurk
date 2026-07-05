// A* pathfinding tests over synthetic solid maps, plus an end-to-end check that
// the enemy pathfinding + movement systems walk a hunter toward a target.

#include "sim/Movement.hpp"
#include "sim/Pathfinding.hpp"

#include <cmath>
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

// Every step in a path is an 8-neighbour of the previous, none is solid, and no
// diagonal step cuts past a solid corner (both shoulder tiles must be open).
template <class SolidFn>
static bool pathIsValid(const std::vector<TileCoord>& path, SolidFn solid) {
    for (std::size_t i = 0; i < path.size(); ++i) {
        if (solid(path[i])) return false;
        if (i > 0) {
            const int sx = path[i].x - path[i - 1].x;
            const int sy = path[i].y - path[i - 1].y;
            const int cheb = std::max(std::abs(sx), std::abs(sy));
            if (cheb != 1) return false; // must be an adjacent tile
            if (sx != 0 && sy != 0) {    // diagonal: no corner cutting
                if (solid({path[i - 1].x + sx, path[i - 1].y}) ||
                    solid({path[i - 1].x, path[i - 1].y + sy})) {
                    return false;
                }
            }
        }
    }
    return true;
}

int main() {
    // 1. Open field: with diagonals the optimal tile count is Chebyshev + 1
    //    (move diagonally until aligned, then straight).
    {
        const auto open = [](TileCoord) { return false; };
        const auto path = findPath({0, 0}, {5, 3}, open);
        CHECK(!path.empty());
        CHECK((path.front() == TileCoord{0, 0}));
        CHECK((path.back() == TileCoord{5, 3}));
        CHECK(path.size() == static_cast<std::size_t>(std::max(5, 3) + 1)); // optimal
        CHECK(pathIsValid(path, open));
    }

    // 2. A wall with a gap forces a detour but stays reachable. Column x==3 is
    //    solid for all y except y==0, so the path must funnel through (3,0).
    {
        const auto wall = [](TileCoord t) { return t.x == 3 && t.y != 0; };
        const auto path = findPath({0, 2}, {6, 2}, wall);
        CHECK(!path.empty());
        CHECK((path.back() == TileCoord{6, 2}));
        CHECK(pathIsValid(path, wall));
        bool throughGap = false;
        for (const TileCoord t : path)
            if (t == TileCoord{3, 0}) throughGap = true;
        CHECK(throughGap);
    }

    // 3. Fully walled-off goal: no path within budget => empty.
    {
        const auto boxed = [](TileCoord t) {
            // Solid ring around the single goal tile (10,10).
            return std::abs(t.x - 10) <= 1 && std::abs(t.y - 10) <= 1 &&
                   !(t.x == 10 && t.y == 10);
        };
        const auto path = findPath({0, 0}, {10, 10}, boxed);
        CHECK(path.empty());
    }

    // 4. Solid goal tile is rejected outright.
    {
        const auto solidGoal = [](TileCoord t) { return t == TileCoord{4, 4}; };
        CHECK(findPath({0, 0}, {4, 4}, solidGoal).empty());
    }

    // 4b. Corner cutting is forbidden: a lone solid at (1,0) blocks the direct
    //     (0,0)->(1,1) diagonal (one shoulder is solid), so the body-safe route
    //     detours through (0,1) -- three tiles, not the two a corner-cut allows.
    {
        const auto corner = [](TileCoord t) { return t == TileCoord{1, 0}; };
        const auto path = findPath({0, 0}, {1, 1}, corner);
        CHECK(!path.empty());
        CHECK((path.back() == TileCoord{1, 1}));
        CHECK(path.size() == 3);
        CHECK(pathIsValid(path, corner));
    }

    // 5. End to end: a hunter with an empty PathFollower closes on a target over
    //    open ground, driven by pathfindingSystem + movementSystem.
    {
        const auto open = [](TileCoord) { return false; };
        const Vec2 target{20.0f * TILE_SIZE, 0.0f * TILE_SIZE};

        entt::registry reg;
        const entt::entity hunter = reg.create();
        reg.emplace<Position>(hunter, Vec2{0.0f, 0.0f});
        reg.emplace<Velocity>(hunter, Vec2{0.0f, 0.0f});
        reg.emplace<Collider>(hunter, 12.0f);
        reg.emplace<Enemy>(hunter, Enemy{160.0f});
        reg.emplace<PathFollower>(hunter);

        const auto dist = [&] {
            const Vec2 p = reg.get<Position>(hunter).value;
            return std::sqrt((p.x - target.x) * (p.x - target.x) +
                             (p.y - target.y) * (p.y - target.y));
        };
        const float start = dist();

        // Simulate a few seconds at 60 Hz.
        for (int i = 0; i < 600; ++i) {
            pathfindingSystem(reg, 1.0f / 60.0f, target, open);
            movementSystem(reg, 1.0f / 60.0f, open);
        }
        CHECK(dist() < start);          // it made progress...
        CHECK(dist() < TILE_SIZE);      // ...and effectively arrived
    }

    if (g_failures == 0) {
        std::puts("pathfinding_test: all assertions passed");
        return 0;
    }
    std::printf("pathfinding_test: %d failure(s)\n", g_failures);
    return 1;
}
