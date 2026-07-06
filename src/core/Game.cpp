#include "core/Game.hpp"

#include <array>   // std::size on the palette table
#include <cstddef> // std::size_t

#include <entt/entt.hpp>

#include "sim/Components.hpp"
#include "sim/Vec2.hpp"

namespace lurk {

Game::Game(int width, int height, const char* title) {
    InitWindow(width, height, title);
    SetTargetFPS(60);

    // The offset (screen-space center) is refreshed each frame in update() so it
    // tracks resolution changes (e.g. toggling fullscreen); seed it here too.
    camera_.offset = {width / 2.0f, height / 2.0f};
    const Vec2 spawn = world_.playerPosition();
    camera_.target = {spawn.x, spawn.y};
    camera_.rotation = 0.0f;
    camera_.zoom = 1.0f;
}

Game::~Game() {
    CloseWindow();
}

void Game::run() {
    while (!WindowShouldClose()) {
        processInput();
        update(GetFrameTime()); // delta time in seconds; keep updates frame-independent
        render();
    }
}

void Game::processInput() {
/*    
    if (IsKeyPressed(KEY_W)) TraceLog(LOG_INFO, "INPUT: W pressed");
    if (IsKeyPressed(KEY_S)) TraceLog(LOG_INFO, "INPUT: S pressed");
    if (IsKeyPressed(KEY_A)) TraceLog(LOG_INFO, "INPUT: A pressed");
    if (IsKeyPressed(KEY_D)) TraceLog(LOG_INFO, "INPUT: D pressed");
*/
    // Translate raw keys into a movement direction, then hand the sim an
    // engine-agnostic intent. Normalizing keeps diagonals the same speed.
    Vec2 dir{0.0f, 0.0f};
    if (IsKeyDown(KEY_W)) dir.y -= 1.0f;
    if (IsKeyDown(KEY_S)) dir.y += 1.0f;
    if (IsKeyDown(KEY_A)) dir.x -= 1.0f;
    if (IsKeyDown(KEY_D)) dir.x += 1.0f;
    world_.setPlayerMoveDir(normalized(dir));
}

void Game::update(float dt) {
    world_.update(dt);

    // Keep the view centered on the player regardless of the current window /
    // fullscreen resolution: offset is the screen-space point the target maps to.
    camera_.offset = {GetScreenWidth() / 2.0f, GetScreenHeight() / 2.0f};
    const Vec2 p = world_.playerPosition();
    camera_.target = {p.x, p.y};
}

namespace {

// Base terrain colours, indexed by TileId. Lives in core/ because Color is a
// raylib (rendering) type; sim/ only knows the abstract TileId and its gameplay
// props (see kTerrain in Chunk.hpp). Adding a terrain type = one row here in
// enum order; the static_assert flags a missing one.
constexpr Color kPalette[] = {
    /* Water */ Color{ 60, 110, 200, 255},
    /* Sand  */ Color{210, 190, 130, 255},
    /* Grass */ Color{ 70, 150,  80, 255},
    /* Rock  */ Color{120, 120, 130, 255},
};
static_assert(std::size(kPalette) == TILE_TYPE_COUNT,
              "every TileId needs a colour in kPalette");

// One static colour per terrain type for now; per-terrain textures will replace
// this later.
Color tileColor(TileId id) { return kPalette[static_cast<std::size_t>(id)]; }

} // namespace

void Game::render() const {
    BeginDrawing();
    ClearBackground(BLACK);

    BeginMode2D(camera_);

    // Draw only the tiles inside the viewport: convert the screen corners to
    // world space, then to the tile range, and fill that rectangle. Cost scales
    // with the screen, not with how many chunks are resident.
    const Vector2 tl = GetScreenToWorld2D({0.0f, 0.0f}, camera_);
    const Vector2 br = GetScreenToWorld2D(
        {static_cast<float>(GetScreenWidth()), static_cast<float>(GetScreenHeight())}, camera_);
    const TileCoord minTile = worldToTile({tl.x, tl.y});
    const TileCoord maxTile = worldToTile({br.x, br.y});
    for (int ty = minTile.y; ty <= maxTile.y; ++ty) {
        for (int tx = minTile.x; tx <= maxTile.x; ++tx) {
            DrawRectangle(tx * TILE_SIZE, ty * TILE_SIZE, TILE_SIZE, TILE_SIZE,
                          tileColor(world_.tileAt({tx, ty})));
        }
    }

    // Draw every renderable entity (Position + Sprite). The hunter is red so it
    // reads apart from the white player.
    const auto& registry = world_.registry();
    for (const entt::entity e : registry.view<Position, Sprite>()) {
        const Vec2 p = registry.get<Position>(e).value;
        const float s = registry.get<Sprite>(e).size;
        const Color color = registry.all_of<Enemy>(e) ? Color{200, 60, 60, 255} : RAYWHITE;
        DrawRectangle(static_cast<int>(p.x - s / 2.0f),
                      static_cast<int>(p.y - s / 2.0f),
                      static_cast<int>(s),
                      static_cast<int>(s),
                      color);
    }
    EndMode2D();

    DrawText("lurk / WASD to move", 20, 20, 20, GRAY);
    DrawText(TextFormat("chunks loaded: %d", static_cast<int>(world_.loadedChunkCount())),
             20, 44, 20, GRAY);
    EndDrawing();
}

} // namespace lurk
