#include "debugger.hpp"

extern "C" {
#include <mem/n64bus.h>
}

std::unordered_map<u64, Breakpoint> breakpoints;

bool check_breakpoint(u64 address) {
    bool hit = breakpoints.find(address) != breakpoints.end();
    if (hit) {
        n64sys.debugger_state.broken = true;
        if (breakpoints[address].temporary) {
            breakpoints.erase(address);
        }
    }
    return hit;
}

void debugger_step() {
    // To step once, set a temporary breakpoint at the next PC and unpause
    breakpoints[N64CPU.next_pc] = {N64CPU.next_pc, true};
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
