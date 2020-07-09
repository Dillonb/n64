#include "n64_rsp_bus.h"
#include "mem_util.h"
#include "addresses.h"

word n64_rsp_read_word(n64_system_t* system, word address) {
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

void n64_rsp_write_word(n64_system_t* system, word address, word value) {
    address &= 0xFFFFFF;
    switch (address) {
        case RSP_REGION_SP_DMEM:
            word_to_byte_array((byte*) &system->mem.sp_dmem, address - RSP_SREGION_SP_DMEM, value);
            break;
        case RSP_REGION_SP_IMEM:
            word_to_byte_array((byte*) &system->mem.sp_imem, address - RSP_SREGION_SP_IMEM, value);
            break;
        default:
            logfatal("RSP writing word to 0x%08X", address)
    }
}


half n64_rsp_read_half(n64_system_t* system, word address) {
    address &= 0xFFFFFF;
    switch (address) {
        case RSP_REGION_SP_DMEM:
            return half_from_byte_array((byte*) &system->mem.sp_dmem, address - RSP_SREGION_SP_DMEM);
        case RSP_REGION_SP_IMEM:
            return half_from_byte_array((byte*) &system->mem.sp_imem, address - RSP_SREGION_SP_IMEM);
        default:
            logfatal("RSP reading half at 0x%08X", address)
    }
}

void n64_rsp_write_half(n64_system_t* system, word address, half value) {
    address &= 0xFFFFFF;
    switch (address) {
        case RSP_REGION_SP_DMEM:
            half_to_byte_array((byte*) &system->mem.sp_dmem, address - RSP_SREGION_SP_DMEM, value);
            break;
        case RSP_REGION_SP_IMEM:
            half_to_byte_array((byte*) &system->mem.sp_imem, address - RSP_SREGION_SP_IMEM, value);
            break;
        default:
            logfatal("RSP writing half to 0x%08X", address)
    }
}

