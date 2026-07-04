#include "Game.hpp"

#include <raylib.h>

namespace lurk {

Game::Game(int width, int height, const char* title) {
    InitWindow(width, height, title);
    SetTargetFPS(60);
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
    // TODO(phase 1): WASD player movement.
}

void Game::update(float /*dt*/) {
    // TODO(phase 1): advance game state using dt, not per-frame constants.
}

void Game::render() const {
    BeginDrawing();
    ClearBackground(BLACK);
    DrawText("lurk", 20, 20, 20, RAYWHITE);
    EndDrawing();
}

} // namespace lurk
