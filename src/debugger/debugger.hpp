#ifndef N64_DEBUGGER_HPP
#define N64_DEBUGGER_HPP
#include <unordered_map>
extern "C" {
#include <util.h>
#include <debugger/debugger.h>
}

struct Breakpoint {
    u64 address;
    bool temporary;
    bool operator<(const Breakpoint& other) const {
        return address < other.address;
    }
};

extern std::unordered_map<u64, Breakpoint> breakpoints;

#endif