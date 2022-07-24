#ifndef N64_N64MEM_H
#define N64_N64MEM_H

#include "n64rom.h"
#include "addresses.h"
#include <log.h>
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
    u32 dram_address;
    u32 pif_address;
} si_reg_t;

typedef enum n64_save_type {
    SAVE_NONE,
    SAVE_EEPROM_4k,
    SAVE_EEPROM_16k,
    SAVE_FLASH_1m,
    SAVE_SRAM_256k
} n64_save_type_t;

typedef enum flash_state {
    FLASH_STATE_IDLE,
    FLASH_STATE_ERASE,
    FLASH_STATE_WRITE,
    FLASH_STATE_READ,
    FLASH_STATE_STATUS
} flash_state_t;

INLINE bool is_eeprom(n64_save_type_t type) {
    return type == SAVE_EEPROM_4k || type == SAVE_EEPROM_16k;
}

INLINE void assert_is_eeprom(n64_save_type_t save_type) {
    if (!is_eeprom(save_type)) {
        logfatal("Expected save type to be EEPROM, but was not! Is this game in the game DB?");
    }
}

INLINE bool is_sram(n64_save_type_t type) {
    return type == SAVE_SRAM_256k;
}

INLINE void assert_is_sram(n64_save_type_t save_type) {
    if (!is_sram(save_type)) {
        logfatal("Expected save type to be SRAM, but was not! Is this game in the game DB?");
    }
}

INLINE bool is_flash(n64_save_type_t type) {
    return type == SAVE_FLASH_1m;
}

INLINE void assert_is_flash(n64_save_type_t save_type) {
    if (!is_flash(save_type)) {
        logfatal("Expected save type to be FLASH, but was not! Is this game in the game DB?");
    }
}

typedef struct n64_mem {
    u8 rdram[N64_RDRAM_SIZE];
    n64_rom_t rom;
    u32 rdram_reg[10];
    u32 pi_reg[13];
    u32 ri_reg[8];
    si_reg_t si_reg;
    u8 pif_ram[PIF_RAM_SIZE];
    char save_file_path[PATH_MAX];
    char mempack_file_path[PATH_MAX];
    n64_save_type_t save_type;
    u8 isviewer_buffer[CART_ISVIEWER_SIZE];

    u8* save_data;
    bool save_data_dirty;
    int save_data_debounce_counter;
    size_t save_size;

    struct {
        flash_state_t state;
        u64 status_reg;

        // TODO: do these share the same reg?
        size_t erase_offset;
        size_t write_offset;

        u8 write_buffer[128];
    } flash;

    u8* mempack_data;
    bool mempack_data_dirty;
    int mempack_data_debounce_counter;

} n64_mem_t;


void save_rdram_dump(bool bswap);
void init_mem(n64_mem_t* mem);

#endif //N64_N64MEM_H
