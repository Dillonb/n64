#ifndef N64_DEBUGGER_TYPES
#define N64_DEBUGGER_TYPES

#ifndef N64_WIN
#include <gdbstub.h>
#include <util.h>

typedef struct n64_breakpoint n64_breakpoint_t;

typedef struct n64_breakpoint {
    u32 address;
    n64_breakpoint_t* next;
} n64_breakpoint_t;

typedef struct n64_debugger_state {
    gdbstub_t* gdb;
    bool broken;
    int steps;
    n64_breakpoint_t* breakpoints;
    bool enabled;
} n64_debugger_state_t;

INLINE bool check_breakpoint(n64_debugger_state_t* state, u32 address) {
    n64_breakpoint_t* cur = state->breakpoints;
    while (cur != NULL) {
        if (cur->address == address) {
            logalways("Hit breakpoint at 0x%08X\n", address);
            return true;
        }
        cur = cur->next;
    }
    return false;
}
#endif
#endif
