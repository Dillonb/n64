#ifndef N64_N64SYSTEM_H
#define N64_N64SYSTEM_H
#include <stdbool.h>
#include "n64mem.h"
#include "cpu/r4300i.h"

typedef struct n64_system {
    n64_mem_t mem;
    r4300i_t cpu;
} n64_system_t;

n64_system_t* init_n64system(const char* rom_path, bool enable_frontend);

_Noreturn void n64_system_loop(n64_system_t* system);
void n64_system_cleanup(n64_system_t* system);
#endif //N64_N64SYSTEM_H
