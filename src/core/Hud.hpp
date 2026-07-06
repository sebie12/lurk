#pragma once

#include <raylib.h> // Color, used in the drawBar signature

#include "sim/World.hpp"

namespace lurk {

// Draws the on-screen HUD (the overlay layer, in screen space -- not inside the
// world camera). Owns no state of its own: it reads the player's vitals from the
// World each frame and renders them. Lives in core/ because it uses raylib
// drawing calls; sim/ stays engine-agnostic.
//
// This is the single place responsible for building the HUD's UI elements, so
// adding a widget (e.g. an inventory hotbar) is a new private draw* helper here
// rather than more code scattered through the render loop.
class Hud {
public:
    // Render the HUD for the current frame. Call between EndMode2D() and
    // EndDrawing() so it sits on top of the world, unaffected by the camera.
    void draw(const World& world) const;

private:
    // Draw one labelled stat bar: a dark backing, a `fraction` (0..1) fill in
    // `fill`, and an outline, with its top-left at (x, y). Size/border come from
    // config so all bars stay consistent.
    void drawBar(float x, float y, float fraction, Color fill) const;
};

} // namespace lurk
