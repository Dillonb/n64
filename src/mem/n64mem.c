#include <stdbool.h>
#include "n64mem.h"
void init_mem(n64_mem_t* mem) {
    mem->save_data_dirty = false;
    mem->save_data_debounce_counter = -1;
    mem->mempack_data_debounce_counter = -1;

    mem->ri_reg[RI_MODE_REG] = 0xE;
    mem->ri_reg[RI_CONFIG_REG] = 0x40;
    mem->ri_reg[RI_SELECT_REG] = 0x14;
    mem->ri_reg[RI_REFRESH_REG] = 0x63634;
}
