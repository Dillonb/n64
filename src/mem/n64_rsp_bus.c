#include "n64_rsp_bus.h"
#include "mem_util.h"
#include "addresses.h"

word n64_rsp_read_word(n64_system_t* system, word address) {
    return word_from_byte_array_unaligned((byte*) &system->mem.sp_dmem, (address & 0xFFF));
}

void n64_rsp_write_word(n64_system_t* system, word address, word value) {
    word_to_byte_array_unaligned((byte*) &system->mem.sp_dmem, address & 0xFFF, value);
}

half n64_rsp_read_half(n64_system_t* system, word address) {
    return half_from_byte_array_unaligned((byte*) &system->mem.sp_dmem, address & 0xFFF);
}

void n64_rsp_write_half(n64_system_t* system, word address, half value) {
    half_to_byte_array_unaligned((byte*) &system->mem.sp_dmem, address & 0xFFF, value);
}

byte n64_rsp_read_byte(n64_system_t* system, word address) {
    return system->mem.sp_dmem[address & 0xFFF];
}

void n64_rsp_write_byte(n64_system_t* system, word address, byte value) {
    address &= 0xFFF;
    system->mem.sp_dmem[address & 0xFFF] = value;
}
