#pragma once

#include <cmath>

#include "sim/Coords.hpp"

namespace lurk {

// AABB-vs-tilegrid collision. Templated on a `solid` predicate (TileCoord ->
// bool) so it's decoupled from where tiles come from: the game passes a terrain
// query, tests pass a synthetic map.

// True if the axis-aligned box centered at `c` with half-extent `half` overlaps
// any solid tile. The tiny epsilon on the max edges avoids reporting a touch
// when the box sits flush against the next tile.
template <class SolidFn>
bool boxCollides(Vec2 c, float half, SolidFn solid) {
    constexpr float eps = 0.0001f;
    const TileCoord lo = worldToTile({c.x - half, c.y - half});
    const TileCoord hi = worldToTile({c.x + half - eps, c.y + half - eps});
    for (int ty = lo.y; ty <= hi.y; ++ty) {
        for (int tx = lo.x; tx <= hi.x; ++tx) {
            if (solid({tx, ty})) return true;
        }
    }
    return false;
}

// Move the box at `pos` by `disp`, resolving against solid tiles one axis at a
// time. Blocking each axis independently lets the box slide along a wall it
// hits diagonally. Per-frame displacement is small, so blocking (rather than
// snapping) still stops the box within a step of the wall.
//
// Speed-preserving slide: when a *diagonal* move is blocked on one axis, the
// speed that axis would have spent is redirected into the free axis. Without
// this, a body steering mostly sideways into a wall would only advance by its
// tiny perpendicular component -- crawling along the wall at a fraction of its
// speed and appearing stuck at convex corners. Redirecting lets it slide at full
// speed and round the corner in a step or two. Pure axis-aligned moves into a
// wall are left to stop (no phantom sideways drift).
template <class SolidFn>
Vec2 resolveMove(Vec2 pos, float half, Vec2 disp, SolidFn solid) {
    Vec2 p = pos;

    Vec2 tryX = p;
    tryX.x += disp.x;
    const bool freeX = !boxCollides(tryX, half, solid);
    if (freeX) p.x = tryX.x;

    Vec2 tryY = p; // note: carries the resolved x
    tryY.y += disp.y;
    const bool freeY = !boxCollides(tryY, half, solid);
    if (freeY) p.y = tryY.y;

    // Exactly one axis blocked on a diagonal move: pour the blocked axis's speed
    // into the one that's free so the slide keeps full pace and clears corners.
    if (freeX != freeY && disp.x != 0.0f && disp.y != 0.0f) {
        if (!freeX) { // x blocked -> add its magnitude to the y slide
            Vec2 t = p;
            t.y += (disp.y >= 0.0f ? 1.0f : -1.0f) * std::abs(disp.x);
            if (!boxCollides(t, half, solid)) p.y = t.y;
        } else {      // y blocked -> add its magnitude to the x slide
            Vec2 t = p;
            t.x += (disp.x >= 0.0f ? 1.0f : -1.0f) * std::abs(disp.y);
            if (!boxCollides(t, half, solid)) p.x = t.x;
        }
    }

    return p;
}

} // namespace lurk
