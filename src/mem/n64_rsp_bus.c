#include "n64_rsp_bus.h"
#include "mem_util.h"
#include "addresses.h"

word n64_rsp_read_word(n64_system_t* system, word address) {
    printf("RSP reading word from 0x%08X - which is actually 0x%08X\n", address, address & 0xFFFFFF);
    address &= 0xFFFFFF;
    switch (address) {
        case RSP_REGION_SP_DMEM:
            return word_from_byte_array((byte*) &system->mem.sp_dmem, address - RSP_SREGION_SP_DMEM);
        case RSP_REGION_SP_IMEM:
            return word_from_byte_array((byte*) &system->mem.sp_imem, address - RSP_SREGION_SP_IMEM);
        default:
            logfatal("RSP reading word at 0x%08X", address)
    }
}

