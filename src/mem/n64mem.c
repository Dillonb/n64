#include <stdbool.h>
#include <system/n64system.h>
#include "mem_util.h"
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

void save_rdram_dump(bool bswap) {
    char dump_path[PATH_MAX];
    strcpy(dump_path, n64sys.rom_path);
    strncat(dump_path, ".rdram", PATH_MAX - 1);

    FILE* dump = fopen(dump_path, "wb");

    for (int i = 0; i < N64_RDRAM_SIZE; i += 4) {
        u32 w;
        memcpy(&w, &n64sys.mem.rdram[i], sizeof(u32));
        if (bswap) {
            w = bswap_32(w);
        }
        fwrite(&w, sizeof(u32), 1, dump);
    }

    fclose(dump);

    logalways("Dumped RDRAM to %s", dump_path);
}
