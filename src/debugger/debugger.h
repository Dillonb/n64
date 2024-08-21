#ifndef N64_DEBUGGER_H
#define N64_DEBUGGER_H

#include <util.h>
#include <stdbool.h>

void debugger_init();
void debugger_tick();
void debugger_step();
bool check_breakpoint(u64 address);
void debugger_cleanup();
void n64_debug_set_breakpoint(u64 address);
void n64_debug_clear_breakpoint(u64 address);

#endif //N64_DEBUGGER_H
