#ifndef N64_N64MEM_H
#define N64_N64MEM_H

#include "n64rom.h"
#include "../common/util.h"

#define RDRAM_SIZE   0x800000
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

typedef enum pi_reg {
    PI_DRAM_ADDR_REG,
    PI_CART_ADDR_REG,
    PI_RD_LEN_REG,
    PI_WR_LEN_REG,
    PI_DOMAIN1_REG,
    PI_BSD_DOM1_PWD_REG,
    PI_BSD_DOM1_PGS_REG,
    PI_BSD_DOM1_RLS_REG,
    PI_DOMAIN2_REG,
    PI_BSD_DOM2_PWD_REG,
    PI_BSD_DOM2_PGS_REG,
    PI_BSD_DOM2_RLS_REG,
} pi_reg_t;

typedef struct si_reg {
    word dram_address;
} si_reg_t;

typedef struct n64_mem {
    n64_rom_t rom;
    byte rdram[RDRAM_SIZE];
    byte sp_dmem[SP_DMEM_SIZE];
    byte sp_imem[SP_IMEM_SIZE];
    word rdram_reg[10];
    word pi_reg[13];
    word ri_reg[8];
    si_reg_t si_reg;
    byte pif_ram[64];
} n64_mem_t;

void init_mem(n64_mem_t* mem);

#endif //N64_N64MEM_H
