#ifndef N64_DEBUGGER_TYPES
#define N64_DEBUGGER_TYPES

#include <gdbstub.h>

typedef struct n64_breakpoint n64_breakpoint_t;

typedef struct n64_breakpoint {
    word address;
    n64_breakpoint_t* next;
} n64_breakpoint_t;

typedef struct n64_debugger_state {
    gdbstub_t* gdb;
    bool broken;
    int steps;
    n64_breakpoint_t* breakpoints;
    bool enabled;
} n64_debugger_state_t;

INLINE bool check_breakpoint(n64_debugger_state_t* state, word address) {
    n64_breakpoint_t* cur = state->breakpoints;
    while (cur != NULL) {
        if (cur->address == address) {
            return true;
        }
        cur = cur->next;
    }
    return false;
}
#endif
