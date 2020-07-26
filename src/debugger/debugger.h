#ifndef N64_DEBUGGER_H
#define N64_DEBUGGER_H

#include <gdbstub.h>
#include "../system/n64system.h"

typedef struct n64_system n64_system_t;

void debugger_init(n64_system_t* system);
void debugger_tick(n64_system_t* system);
void debugger_cleanup(n64_system_t* system);

#endif //N64_DEBUGGER_H
