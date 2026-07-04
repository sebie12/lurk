#pragma once

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
template <class SolidFn>
Vec2 resolveMove(Vec2 pos, float half, Vec2 disp, SolidFn solid) {
    Vec2 p = pos;

    Vec2 tryX = p;
    tryX.x += disp.x;
    if (!boxCollides(tryX, half, solid)) p.x = tryX.x;

    Vec2 tryY = p; // note: carries the resolved x
    tryY.y += disp.y;
    if (!boxCollides(tryY, half, solid)) p.y = tryY.y;

    return p;
}

} // namespace lurk
