#ifndef N64_N64MEM_H
#define N64_N64MEM_H

#include "n64rom.h"
#include "common/util.h"

#define SP_DMEM_SIZE 0x1000
#define SP_IMEM_SIZE 0x1000

typedef enum ri_reg {
    RI_MODE_REG,
    RI_CONFIG_REG,
    RI_CURRENT_LOAD_REG,
    RI_SELECT_REG,
    RI_REFRESH_REG,
    RI_LATENCY_REG,
    RI_RERROR_REG,
    RI_WERROR_REG
} ri_reg_t;

typedef struct n64_mem {
    n64_rom_t rom;
    byte sp_dmem[SP_DMEM_SIZE];
    byte sp_imem[SP_IMEM_SIZE];
    word ri_reg[8];
} n64_mem_t;

void init_mem(n64_mem_t* mem);

#endif //N64_N64MEM_H
