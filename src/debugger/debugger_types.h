#ifndef N64_DEBUGGER_TYPES
#define N64_DEBUGGER_TYPES

#include <util.h>

typedef struct n64_breakpoint n64_breakpoint_t;

typedef struct n64_breakpoint {
    u32 address;
    n64_breakpoint_t* next;
} n64_breakpoint_t;

typedef struct n64_debugger_state {
    bool broken;
    bool enabled;
} n64_debugger_state_t;


#endif
