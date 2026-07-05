#pragma once

#include <algorithm>
#include <cstdlib>
#include <queue>
#include <unordered_map>
#include <vector>

#include "sim/Config.hpp"
#include "sim/Coords.hpp"

namespace lurk {

// A* shortest path over the tile grid, from `start` to `goal` inclusive.
//
// Templated on the `solid` predicate so it's decoupled from the terrain source,
// exactly like Collision/Movement: the game passes a chunk query, tests pass a
// synthetic map.
//
// Movement is 8-directional so the body can travel diagonally and slip around the
// concave corners of water/rock instead of wedging against them. Straight steps
// cost 10 and diagonal steps 14 (~10*sqrt(2)), with the octile distance as an
// admissible heuristic. A diagonal step is only allowed when both orthogonal
// "shoulder" tiles are open: the body is a full tile wide, so cutting a corner
// past a solid tile would clip it -- forbidding that keeps every emitted path
// physically followable by the collision resolver.
//
// The world is infinite, so the search is bounded by `maxExpansions`: if the goal
// can't be reached within that many node pops the path comes back empty and the
// caller can idle or fall back. Returns empty, too, when the goal tile is solid.
template <class SolidFn>
std::vector<TileCoord> findPath(TileCoord start, TileCoord goal, SolidFn solid,
                                int maxExpansions = config::PATHFIND_MAX_EXPANSIONS) {
    if (solid(goal)) return {};
    if (start == goal) return {start};

    // Open-set entry; ordered by f = g + h (smallest first) via a min-heap.
    struct Node {
        int f;
        int g;
        TileCoord t;
    };
    struct Cmp {
        bool operator()(const Node& a, const Node& b) const { return a.f > b.f; }
    };

    // Octile distance, scaled to the 10 (straight) / 14 (diagonal) step costs.
    const auto heuristic = [](TileCoord a, TileCoord b) {
        const int ax = std::abs(a.x - b.x);
        const int ay = std::abs(a.y - b.y);
        return 10 * (ax + ay) - 6 * std::min(ax, ay);
    };

    std::priority_queue<Node, std::vector<Node>, Cmp> open;
    std::unordered_map<TileCoord, int, TileCoordHash> gScore;
    std::unordered_map<TileCoord, TileCoord, TileCoordHash> cameFrom;

    open.push({heuristic(start, goal), 0, start});
    gScore[start] = 0;

    // 4 straight neighbours first, then 4 diagonals; costs parallel the offsets.
    static constexpr int dx[8] = {1, -1, 0, 0, 1, 1, -1, -1};
    static constexpr int dy[8] = {0, 0, 1, -1, 1, -1, 1, -1};
    static constexpr int cost[8] = {10, 10, 10, 10, 14, 14, 14, 14};

    int expansions = 0;
    while (!open.empty() && expansions < maxExpansions) {
        const Node cur = open.top();
        open.pop();

        // A stale duplicate: we already reached this tile more cheaply. Skip it
        // (lazy deletion — cheaper than decrease-key on a binary heap).
        const auto it = gScore.find(cur.t);
        if (it != gScore.end() && cur.g > it->second) continue;

        if (cur.t == goal) {
            std::vector<TileCoord> path;
            TileCoord t = goal;
            path.push_back(t);
            while (!(t == start)) {
                t = cameFrom[t];
                path.push_back(t);
            }
            std::reverse(path.begin(), path.end());
            return path;
        }

        ++expansions;
        for (int i = 0; i < 8; ++i) {
            const TileCoord nb{cur.t.x + dx[i], cur.t.y + dy[i]};
            if (solid(nb)) continue;
            // Diagonal: refuse to cut past a solid corner so the full-tile body
            // keeps clearance on both shoulders it sweeps through.
            if (dx[i] != 0 && dy[i] != 0 &&
                (solid({cur.t.x + dx[i], cur.t.y}) || solid({cur.t.x, cur.t.y + dy[i]}))) {
                continue;
            }
            const int ng = cur.g + cost[i];
            const auto git = gScore.find(nb);
            if (git == gScore.end() || ng < git->second) {
                gScore[nb] = ng;
                cameFrom[nb] = cur.t;
                open.push({ng + heuristic(nb, goal), ng, nb});
            }
        }
    }

    return {}; // unreachable within the search budget
}

} // namespace lurk
