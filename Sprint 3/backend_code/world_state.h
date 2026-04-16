#ifndef WORLD_STATE_H
#define WORLD_STATE_H

#include <string>
#include <vector>
#include <map>
#include <utility>

// everything the simulator sends us in one response
struct WorldState {
    int cycle = 0;
    int width = 0;
    int height = 0;
    std::vector<std::vector<char>> grid; // 2d grid of cell characters
    int red_alive = 0;
    int black_alive = 0;
    int red_food = 0;
    int black_food = 0;
};

// maps (row, col) -> how many frames ago a bug was at that position
// used to draw the fading trace lines
using TraceMap = std::map<std::pair<int,int>, int>;

struct TraceState {
    TraceMap red_traces;
    TraceMap black_traces;
    int max_trace_len = 10; // how many past positions to show, user can change with +/-
};

#endif
