#ifndef N64_DEBUGGER_TYPES
#define N64_DEBUGGER_TYPES

#include <gdbstub.h>

typedef struct n64_debugger_state {
    gdbstub_t* gdb;
    bool broken;
    int steps;
} n64_debugger_state_t;

#endif
