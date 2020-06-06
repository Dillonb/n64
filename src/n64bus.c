#include "n64bus.h"
#include "common/log.h"

#include <endian.h>

INLINE word word_from_byte_array(byte* arr, int index) {
    word* warr = (word*)arr;
    return be32toh(warr[index / sizeof(word)]);
}

INLINE half half_from_byte_array(byte* arr, int index) {
    half* warr = (half*)arr;
    return be16toh(warr[index / sizeof(half)]);
}

INLINE void word_to_byte_array(byte* arr, int index, word value) {
    word* warr = (word*)arr;
    warr[index / sizeof(word)] = htobe32(value);
}

INLINE void half_to_byte_array(byte* arr, int index, half value) {
    half* warr = (half*)arr;
    warr[index / sizeof(half)] = htobe16(value);
}

#define REGION_RDRAM           0x00000000 ... 0x03EFFFFF
#define REGION_RDRAM_REGS      0x03F00000 ... 0x03FFFFFF
#define REGION_SP_REGS         0x04000000 ... 0x040FFFFF
#define REGION_DP_COMMAND_REGS 0x04100000 ... 0x041FFFFF
#define REGION_DP_SPAN_REGS    0x04200000 ... 0x042FFFFF
#define REGION_MI_REGS         0x04300000 ... 0x043FFFFF
#define REGION_VI_REGS         0x04400000 ... 0x044FFFFF
#define REGION_AI_REGS         0x04500000 ... 0x045FFFFF
#define REGION_PI_REGS         0x04600000 ... 0x046FFFFF
#define REGION_RI_REGS         0x04700000 ... 0x047FFFFF
#define REGION_SI_REGS         0x04800000 ... 0x048FFFFF
#define REGION_UNUSED          0x04900000 ... 0x04FFFFFF
#define REGION_CART_2_1        0x05000000 ... 0x05FFFFFF
#define REGION_CART_1_1        0x06000000 ... 0x07FFFFFF
#define REGION_CART_2_2        0x08000000 ... 0x0FFFFFFF
#define REGION_CART_1_2        0x10000000 ... 0x1FBFFFFF
#define REGION_PIF_BOOT        0x1FC00000 ... 0x1FC007BF
#define REGION_PIF_RAM         0x1FC007C0 ... 0x1FC007FF
#define REGION_RESERVED        0x1FC00800 ... 0x1FCFFFFF
#define REGION_CART_1_3        0x1FD00000 ... 0x7FFFFFFF
#define REGION_SYSAD_DEVICE    0x80000000 ... 0xFFFFFFFF

void n64_write_word(word address, word value) {
    switch (address) {
        case REGION_RDRAM:
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_RDRAM", value, address)
        case REGION_RDRAM_REGS:
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_RDRAM_REGS", value, address)
        case REGION_SP_REGS:
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_SP_REGS", value, address)
        case REGION_DP_COMMAND_REGS:
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_DP_COMMAND_REGS", value, address)
        case REGION_DP_SPAN_REGS:
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_DP_SPAN_REGS", value, address)
        case REGION_MI_REGS:
            logwarn("Ignoring write of word 0x%08X to address 0x%08X in unsupported region: REGION_MI_REGS", value, address)
            break;
        case REGION_VI_REGS:
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_VI_REGS", value, address)
        case REGION_AI_REGS:
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_AI_REGS", value, address)
        case REGION_PI_REGS:
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_PI_REGS", value, address)
        case REGION_RI_REGS:
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_RI_REGS", value, address)
        case REGION_SI_REGS:
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_SI_REGS", value, address)
        case REGION_UNUSED:
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_UNUSED", value, address)
        case REGION_CART_2_1:
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_CART_2_1", value, address)
        case REGION_CART_1_1:
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_CART_1_1", value, address)
        case REGION_CART_2_2:
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_CART_2_2", value, address)
        case REGION_CART_1_2:
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_CART_1_2", value, address)
        case REGION_PIF_BOOT:
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_PIF_BOOT", value, address)
        case REGION_PIF_RAM:
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_PIF_RAM", value, address)
        case REGION_RESERVED:
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_RESERVED", value, address)
        case REGION_CART_1_3:
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_CART_1_3", value, address)
        case REGION_SYSAD_DEVICE:
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_SYSAD_DEVICE", value, address)
        default:
            logfatal("Writing word 0x%08X to unknown address: 0x%08X", value, address)
    }
}

word n64_read_word(word address) {
    switch (address) {
        default:
            logfatal("Reading word from unknown address: 0x%08X", address)
    }
}

void n64_write_byte(word address, byte value) {
    switch (address) {
        case REGION_RDRAM:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_RDRAM", value, address)
        case REGION_RDRAM_REGS:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_RDRAM_REGS", value, address)
        case REGION_SP_REGS:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_SP_REGS", value, address)
        case REGION_DP_COMMAND_REGS:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_DP_COMMAND_REGS", value, address)
        case REGION_DP_SPAN_REGS:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_DP_SPAN_REGS", value, address)
        case REGION_MI_REGS:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_MI_REGS", value, address)
        case REGION_VI_REGS:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_VI_REGS", value, address)
        case REGION_AI_REGS:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_AI_REGS", value, address)
        case REGION_PI_REGS:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_PI_REGS", value, address)
        case REGION_RI_REGS:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_RI_REGS", value, address)
        case REGION_SI_REGS:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_SI_REGS", value, address)
        case REGION_UNUSED:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_UNUSED", value, address)
        case REGION_CART_2_1:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_CART_2_1", value, address)
        case REGION_CART_1_1:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_CART_1_1", value, address)
        case REGION_CART_2_2:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_CART_2_2", value, address)
        case REGION_CART_1_2:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_CART_1_2", value, address)
        case REGION_PIF_BOOT:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_PIF_BOOT", value, address)
        case REGION_PIF_RAM:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_PIF_RAM", value, address)
        case REGION_RESERVED:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_RESERVED", value, address)
        case REGION_CART_1_3:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_CART_1_3", value, address)
        case REGION_SYSAD_DEVICE:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_SYSAD_DEVICE", value, address)
        default:
            logfatal("Writing byte 0x%02X to unknown address: 0x%08X", value, address)
    }
}

byte n64_read_byte(word address) {
    switch (address) {
        case REGION_RDRAM:
            logfatal("Reading byte from address 0x%08X in unsupported region: REGION_RDRAM", address)
        case REGION_RDRAM_REGS:
            logfatal("Reading byte from address 0x%08X in unsupported region: REGION_RDRAM_REGS", address)
        case REGION_SP_REGS:
            logfatal("Reading byte from address 0x%08X in unsupported region: REGION_SP_REGS", address)
        case REGION_DP_COMMAND_REGS:
            logfatal("Reading byte from address 0x%08X in unsupported region: REGION_DP_COMMAND_REGS", address)
        case REGION_DP_SPAN_REGS:
            logfatal("Reading byte from address 0x%08X in unsupported region: REGION_DP_SPAN_REGS", address)
        case REGION_MI_REGS:
            logfatal("Reading byte from address 0x%08X in unsupported region: REGION_MI_REGS", address)
        case REGION_VI_REGS:
            logfatal("Reading byte from address 0x%08X in unsupported region: REGION_VI_REGS", address)
        case REGION_AI_REGS:
            logfatal("Reading byte from address 0x%08X in unsupported region: REGION_AI_REGS", address)
        case REGION_PI_REGS:
            logfatal("Reading byte from address 0x%08X in unsupported region: REGION_PI_REGS", address)
        case REGION_RI_REGS:
            logfatal("Reading byte from address 0x%08X in unsupported region: REGION_RI_REGS", address)
        case REGION_SI_REGS:
            logfatal("Reading byte from address 0x%08X in unsupported region: REGION_SI_REGS", address)
        case REGION_UNUSED:
            logfatal("Reading byte from address 0x%08X in unsupported region: REGION_UNUSED", address)
        case REGION_CART_2_1:
            logfatal("Reading byte from address 0x%08X in unsupported region: REGION_CART_2_1", address)
        case REGION_CART_1_1:
            logfatal("Reading byte from address 0x%08X in unsupported region: REGION_CART_1_1", address)
        case REGION_CART_2_2:
            logfatal("Reading byte from address 0x%08X in unsupported region: REGION_CART_2_2", address)
        case REGION_CART_1_2:
            logfatal("Reading byte from address 0x%08X in unsupported region: REGION_CART_1_2", address)
        case REGION_PIF_BOOT:
            logfatal("Reading byte from address 0x%08X in unsupported region: REGION_PIF_BOOT", address)
        case REGION_PIF_RAM:
            logfatal("Reading byte from address 0x%08X in unsupported region: REGION_PIF_RAM", address)
        case REGION_RESERVED:
            logfatal("Reading byte from address 0x%08X in unsupported region: REGION_RESERVED", address)
        case REGION_CART_1_3:
            logfatal("Reading byte from address 0x%08X in unsupported region: REGION_CART_1_3", address)
        case REGION_SYSAD_DEVICE:
            logfatal("Reading byte from address 0x%08X in unsupported region: REGION_SYSAD_DEVICE", address)
        default:
            logfatal("Reading byte from unknown address: 0x%08X", address)
    }
}
