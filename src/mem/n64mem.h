#ifndef N64_N64MEM_H
#define N64_N64MEM_H

#include "n64rom.h"
#include <util.h>
#include <limits.h>

#define N64_RDRAM_SIZE   0x800000
#define PIF_RAM_SIZE 64

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

typedef enum n64_save_type {
    SAVE_NONE,
    SAVE_EEPROM_4k,
    SAVE_EEPROM_16k,
    SAVE_EEPROM_256k,
    SAVE_EEPROM_1Mb,
    SAVE_SRAM_768k
} n64_save_type_t;

typedef struct n64_mem {
    byte rdram[N64_RDRAM_SIZE];
    n64_rom_t rom;
    word rdram_reg[10];
    word pi_reg[13];
    word ri_reg[8];
    si_reg_t si_reg;
    byte pif_ram[PIF_RAM_SIZE];
    char save_file_path[PATH_MAX];
    char mempack_file_path[PATH_MAX];
    n64_save_type_t save_type;

    byte* save_data;
    bool save_data_dirty;
    int save_data_debounce_counter;
    size_t save_size;

    byte* mempack_data;
    bool mempack_data_dirty;
    int mempack_data_debounce_counter;

} n64_mem_t;

void init_mem(n64_mem_t* mem);

#endif //N64_N64MEM_H
