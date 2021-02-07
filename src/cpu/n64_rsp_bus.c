#include "n64_rsp_bus.h"
#include <mem/mem_util.h>

#define RSP_BYTE(addr) (rsp->sp_dmem[BYTE_ADDRESS(addr)])

#define GET_RSP_HALF(addr) ((RSP_BYTE(addr) << 8) | RSP_BYTE((addr) + 1))
#define SET_RSP_HALF(addr, value) do { RSP_BYTE(addr) = ((value) >> 8) & 0xFF; RSP_BYTE((addr) + 1) = (value) & 0xFF;} while(0)

#define GET_RSP_WORD(addr) ((GET_RSP_HALF(addr) << 16) | GET_RSP_HALF((addr) + 2))
#define SET_RSP_WORD(addr, value) do { SET_RSP_HALF(addr, ((value) >> 16) & 0xFFFF); SET_RSP_HALF((addr) + 2, (value) & 0xFFFF);} while(0)

word n64_rsp_read_word(rsp_t* rsp, word address) {
    address &= 0xFFF;
    return GET_RSP_WORD(address);
}

void n64_rsp_write_word(rsp_t* rsp, word address, word value) {
    address &= 0xFFF;
    SET_RSP_WORD(address, value);
}

half n64_rsp_read_half(rsp_t* rsp, word address) {
    address &= 0xFFF;
    return GET_RSP_HALF(address);
}

void n64_rsp_write_half(rsp_t* rsp, word address, half value) {
    address &= 0xFFF;
    SET_RSP_HALF(address, value);
}

byte n64_rsp_read_byte(rsp_t* rsp, word address) {
    address &= 0xFFF;
    return RSP_BYTE(address);
}

void n64_rsp_write_byte(rsp_t* rsp, word address, byte value) {
    address &= 0xFFF;
    RSP_BYTE(address) = value;
}
