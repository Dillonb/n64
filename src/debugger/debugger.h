#ifndef N64_DEBUGGER_H
#define N64_DEBUGGER_H

#ifndef N64_WIN
#include <gdbstub.h>
#include <system/n64system.h>

#define GDB_CPU_PORT 1337

typedef struct n64_system n64_system_t;

void debugger_init();
void debugger_tick();
void debugger_breakpoint_hit();
void debugger_cleanup();
#endif

#endif //N64_DEBUGGER_H
