#include "core/Hud.hpp"

#include <algorithm> // std::clamp

#include "sim/Components.hpp"
#include "sim/Config.hpp"

namespace lurk {

namespace {

// HUD colours. These are raylib Color values, so they live here in core/ rather
// than in the engine-agnostic Config.hpp: health is red, stamina green (per the
// UI spec), over a translucent dark backing with a near-black outline.
constexpr Color kHealthFill = Color{200, 60, 60, 255};   // red
constexpr Color kStaminaFill = Color{70, 180, 80, 255};  // green
constexpr Color kBarBacking = Color{30, 30, 30, 200};    // dark, semi-transparent
constexpr Color kBarOutline = Color{15, 15, 15, 255};    // near-black border

} // namespace

void Hud::drawBar(float x, float y, float fraction, Color fill) const {
    fraction = std::clamp(fraction, 0.0f, 1.0f);
    const float w = config::HUD_BAR_WIDTH;
    const float h = config::HUD_BAR_HEIGHT;

    // Backing first (full width), then the fill clipped to `fraction`, then the
    // outline on top so it frames both.
    DrawRectangle(static_cast<int>(x), static_cast<int>(y),
                  static_cast<int>(w), static_cast<int>(h), kBarBacking);
    DrawRectangle(static_cast<int>(x), static_cast<int>(y),
                  static_cast<int>(w * fraction), static_cast<int>(h), fill);
    DrawRectangleLinesEx(Rectangle{x, y, w, h}, config::HUD_BAR_BORDER, kBarOutline);
}

void Hud::draw(const World& world) const {
    const entt::registry& registry = world.registry();
    const entt::entity player = world.player();
    const Health& health = registry.get<Health>(player);
    const Stamina& stamina = registry.get<Stamina>(player);

    // Anchor bottom-left: stamina sits on the bottom row, health stacked above it.
    const float x = config::HUD_BAR_MARGIN;
    const float staminaY =
        static_cast<float>(GetScreenHeight()) - config::HUD_BAR_MARGIN - config::HUD_BAR_HEIGHT;
    const float healthY = staminaY - config::HUD_BAR_SPACING - config::HUD_BAR_HEIGHT;

    drawBar(x, healthY, health.current / health.max, kHealthFill);
    drawBar(x, staminaY, stamina.current / stamina.max, kStaminaFill);
}

} // namespace lurk
