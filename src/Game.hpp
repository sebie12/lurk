#pragma once

namespace lurk {

// Owns the window lifecycle and drives the input -> update -> render loop.
// One Game instance == one open window; non-copyable because it holds the
// raylib window resource (RAII: window opens in the ctor, closes in the dtor).
class Game {
public:
    Game(int width, int height, const char* title);
    ~Game();

    Game(const Game&) = delete;
    Game& operator=(const Game&) = delete;

    // Blocks until the window is closed.
    void run();

private:
    void processInput();
    void update(float dt);
    void render() const;
};

} // namespace lurk
