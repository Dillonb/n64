#include "n64_rsp_bus.h"
#include <mem/mem_util.h>

word n64_rsp_read_word(rsp_t* rsp, word address) {
    return be32toh(word_from_byte_array_unaligned((byte*) &rsp->sp_dmem, (address & 0xFFF)));
}

void n64_rsp_write_word(rsp_t* rsp, word address, word value) {
    word_to_byte_array_unaligned((byte*) &rsp->sp_dmem, address & 0xFFF, htobe32(value));
}

half n64_rsp_read_half(rsp_t* rsp, word address) {
    return be16toh(half_from_byte_array_unaligned((byte*) &rsp->sp_dmem, address & 0xFFF));
}

void n64_rsp_write_half(rsp_t* rsp, word address, half value) {
    half_to_byte_array_unaligned((byte*) &rsp->sp_dmem, address & 0xFFF, htobe16(value));
}

byte n64_rsp_read_byte(rsp_t* rsp, word address) {
    return rsp->sp_dmem[address & 0xFFF];
}

void n64_rsp_write_byte(rsp_t* rsp, word address, byte value) {
    address &= 0xFFF;
    rsp->sp_dmem[address & 0xFFF] = value;
}
