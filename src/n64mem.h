#ifndef N64_N64MEM_H
#define N64_N64MEM_H

#include "n64rom.h"
#include "common/util.h"

typedef struct n64_mem {
    n64_rom_t rom;
} n64_mem_t;

void init_mem(n64_mem_t* mem);

#endif //N64_N64MEM_H
