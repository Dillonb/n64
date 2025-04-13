#include "debugger.hpp"

extern "C" {
#include <mem/n64bus.h>
}

std::unordered_map<u64, Breakpoint> breakpoints;
// The debugger was just stepped - we should break on the next instruction
bool break_for_step = false;

bool check_breakpoint(u64 address) {
    bool hit = breakpoints.find(address) != breakpoints.end() || break_for_step;
    if (hit) {
        n64sys.debugger_state.broken = true;
        if (breakpoints.find(address) != breakpoints.end() && breakpoints[address].temporary) {
            breakpoints.erase(address);
        }
        break_for_step = false;
    }
    return hit;
}

void debugger_step() {
    break_for_step = true;
    n64sys.debugger_state.broken = false;
}

void n64_debug_set_breakpoint(u64 address) {
    breakpoints[address] = {address, false};
}

void n64_debug_clear_breakpoint(u64 address) {
    breakpoints.erase(address);
}

void debugger_init() {
    breakpoints.clear();
}

void debugger_tick() {
}

void debugger_breakpoint_hit() {
    n64sys.debugger_state.broken = true;
}

void debugger_cleanup() {
}
