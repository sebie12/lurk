// ECS movement-system tests: entities integrate velocity and collide, over a
// real entt::registry with a synthetic solid map.

#include "sim/Movement.hpp"

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
    entt::registry reg;
    const entt::entity e = reg.create();
    reg.emplace<Position>(e, Vec2{0.0f, 0.0f});
    reg.emplace<Velocity>(e, Vec2{100.0f, 0.0f}); // 100 px/s
    reg.emplace<Collider>(e, 16.0f);              // 32px box (half = 16)

    // Open terrain: position advances by velocity * dt.
    const auto empty = [](TileCoord) { return false; };
    movementSystem(reg, 0.5f, empty); // 100 * 0.5 = 50 px
    CHECK(reg.get<Position>(e).value.x == 50.0f);
    CHECK(reg.get<Position>(e).value.y == 0.0f);

    // Against a wall (tiles tx >= 5, i.e. world x >= 160): stepping in small
    // increments stops the box flush against it, never inside.
    reg.get<Position>(e).value = Vec2{80.0f, 100.0f};
    reg.get<Velocity>(e).value = Vec2{60.0f, 0.0f};
    const auto wall = [](TileCoord t) { return t.x >= 5; };
    for (int i = 0; i < 100; ++i) movementSystem(reg, 0.05f, wall); // ~3 px steps
    const Vec2 stopped = reg.get<Position>(e).value;
    CHECK(stopped.x + 16.0f <= 5 * TILE_SIZE); // right edge never past x=160
    CHECK(stopped.y == 100.0f);                // free axis undisturbed

    // The system moves every matching entity, not just one.
    const entt::entity e2 = reg.create();
    reg.emplace<Position>(e2, Vec2{-40.0f, -40.0f});
    reg.emplace<Velocity>(e2, Vec2{0.0f, 20.0f});
    reg.emplace<Collider>(e2, 16.0f);
    movementSystem(reg, 1.0f, empty);
    CHECK(reg.get<Position>(e2).value.y == -20.0f);

    if (g_failures == 0) {
        std::puts("movement_test: all assertions passed");
        return 0;
    }
    std::printf("movement_test: %d failure(s)\n", g_failures);
    return 1;
}
