#ifndef N64_N64_RSP_BUS_H
#define N64_N64_RSP_BUS_H

#include <util.h>
#include <mem/mem_util.h>
#include <cpu/rsp_types.h>

#include "rsp.h"

#define RSP_BYTE(addr) (N64RSP.sp_dmem[BYTE_ADDRESS(addr) & 0xFFF])

#define GET_RSP_HALF(addr) ((RSP_BYTE(addr) << 8) | RSP_BYTE((addr) + 1))
#define SET_RSP_HALF(addr, value) do { RSP_BYTE(addr) = ((value) >> 8) & 0xFF; RSP_BYTE((addr) + 1) = (value) & 0xFF;} while(0)

#define GET_RSP_WORD(addr) ((GET_RSP_HALF(addr) << 16) | GET_RSP_HALF((addr) + 2))
#define SET_RSP_WORD(addr, value) do { SET_RSP_HALF(addr, ((value) >> 16) & 0xFFFF); SET_RSP_HALF((addr) + 2, (value) & 0xFFFF);} while(0)

INLINE u32 n64_rsp_read_word(u32 address) {
    address &= 0xFFF;
    return GET_RSP_WORD(address);
}

INLINE void n64_rsp_write_word(u32 address, u32 value) {
    address &= 0xFFF;
    SET_RSP_WORD(address, value);
}

INLINE u16 n64_rsp_read_half(u32 address) {
    address &= 0xFFF;
    return GET_RSP_HALF(address);
}

INLINE void n64_rsp_write_half(u32 address, u16 value) {
    address &= 0xFFF;
    SET_RSP_HALF(address, value);
}

INLINE u8 n64_rsp_read_byte(u32 address) {
    address &= 0xFFF;
    return RSP_BYTE(address);
}

INLINE void n64_rsp_write_byte(u32 address, u8 value) {
    address &= 0xFFF;
    RSP_BYTE(address) = value;
}

#endif //N64_N64_RSP_BUS_H
