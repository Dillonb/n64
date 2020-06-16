#include "n64bus.h"
#include "../common/log.h"
#include "dma.h"
#include "../cpu/rsp.h"
#include "../vi.h"

#include <endian.h>

INLINE word word_from_byte_array(byte* arr, word index) {
    word* warr = (word*)arr;
    return be32toh(warr[index / sizeof(word)]);
}

INLINE half half_from_byte_array(byte* arr, word index) {
    half* warr = (half*)arr;
    return be16toh(warr[index / sizeof(half)]);
}

INLINE void word_to_byte_array(byte* arr, word index, word value) {
    word* warr = (word*)arr;
    warr[index / sizeof(word)] = htobe32(value);
}

INLINE void half_to_byte_array(byte* arr, word index, half value) {
    half* warr = (half*)arr;
    warr[index / sizeof(half)] = htobe16(value);
}

#define SREGION_RDRAM           0x00000000
#define SREGION_RDRAM_UNUSED    0x00400000
#define SREGION_RDRAM_REGS      0x03F00000
#define SREGION_SP_DMEM         0x04000000
#define SREGION_SP_IMEM         0x04001000
#define SREGION_SP_UNUSED       0x04002000
#define SREGION_SP_REGS         0x04040000
#define SREGION_DP_COMMAND_REGS 0x04100000
#define SREGION_DP_SPAN_REGS    0x04200000
#define SREGION_MI_REGS         0x04300000
#define SREGION_VI_REGS         0x04400000
#define SREGION_AI_REGS         0x04500000
#define SREGION_PI_REGS         0x04600000
#define SREGION_RI_REGS         0x04700000
#define SREGION_SI_REGS         0x04800000
#define SREGION_UNUSED          0x04900000
#define SREGION_CART_2_1        0x05000000
#define SREGION_CART_1_1        0x06000000
#define SREGION_CART_2_2        0x08000000
#define SREGION_CART_1_2        0x10000000
#define SREGION_PIF_BOOT        0x1FC00000
#define SREGION_PIF_RAM         0x1FC007C0
#define SREGION_RESERVED        0x1FC00800
#define SREGION_CART_1_3        0x1FD00000
#define SREGION_SYSAD_DEVICE    0x80000000

#define REGION_RDRAM           SREGION_RDRAM           ... 0x003FFFFF
#define REGION_RDRAM_UNUSED    SREGION_RDRAM_UNUSED    ... 0x03EFFFFF
#define REGION_RDRAM_REGS      SREGION_RDRAM_REGS      ... 0x03FFFFFF
#define REGION_SP_DMEM         SREGION_SP_DMEM         ... 0x04000FFF
#define REGION_SP_IMEM         SREGION_SP_IMEM         ... 0x04001FFF
#define REGION_SP_UNUSED       SREGION_SP_UNUSED       ... 0x0403FFFF
#define REGION_SP_REGS         SREGION_SP_REGS         ... 0x040FFFFF
#define REGION_DP_COMMAND_REGS SREGION_DP_COMMAND_REGS ... 0x041FFFFF
#define REGION_DP_SPAN_REGS    SREGION_DP_SPAN_REGS    ... 0x042FFFFF
#define REGION_MI_REGS         SREGION_MI_REGS         ... 0x043FFFFF
#define REGION_VI_REGS         SREGION_VI_REGS         ... 0x044FFFFF
#define REGION_AI_REGS         SREGION_AI_REGS         ... 0x045FFFFF
#define REGION_PI_REGS         SREGION_PI_REGS         ... 0x046FFFFF
#define REGION_RI_REGS         SREGION_RI_REGS         ... 0x047FFFFF
#define REGION_SI_REGS         SREGION_SI_REGS         ... 0x048FFFFF
#define REGION_UNUSED          SREGION_UNUSED          ... 0x04FFFFFF
#define REGION_CART_2_1        SREGION_CART_2_1        ... 0x05FFFFFF
#define REGION_CART_1_1        SREGION_CART_1_1        ... 0x07FFFFFF
#define REGION_CART_2_2        SREGION_CART_2_2        ... 0x0FFFFFFF
#define REGION_CART_1_2        SREGION_CART_1_2        ... 0x1FBFFFFF
#define REGION_PIF_BOOT        SREGION_PIF_BOOT        ... 0x1FC007BF
#define REGION_PIF_RAM         SREGION_PIF_RAM         ... 0x1FC007FF
#define REGION_RESERVED        SREGION_RESERVED        ... 0x1FCFFFFF
#define REGION_CART_1_3        SREGION_CART_1_3        ... 0x7FFFFFFF
#define REGION_SYSAD_DEVICE    SREGION_SYSAD_DEVICE    ... 0xFFFFFFFF

#define SVREGION_KSEG0 0x80000000
#define SVREGION_KSEG1 0xA0000000
#define SVREGION_KSSEG 0xC0000000
#define SVREGION_KSEG3 0xE0000000

#define VREGION_KSEG0 SVREGION_KSEG0 ... 0x9FFFFFFF
#define VREGION_KSEG1 SVREGION_KSEG1 ... 0xBFFFFFFF
#define VREGION_KSSEG SVREGION_KSSEG ... 0xDFFFFFFF
#define VREGION_KSEG3 SVREGION_KSEG3 ... 0xFFFFFFFF


#define ADDR_RDRAM_CONFIG_REG       0x03F00000
#define ADDR_RDRAM_DEVICE_ID_REG    0x03F00004
#define ADDR_RDRAM_DELAY_REG        0x03F00008
#define ADDR_RDRAM_MODE_REG         0x03F0000C
#define ADDR_RDRAM_REF_INTERVAL_REG 0x03F00010
#define ADDR_RDRAM_REF_ROW_REG      0x03F00014
#define ADDR_RDRAM_RAS_INTERVAL_REG 0x03F00018
#define ADDR_RDRAM_MIN_INTERVAL_REG 0x03F0001C
#define ADDR_RDRAM_ADDR_SELECT_REG  0x03F00020
#define ADDR_RDRAM_DEVICE_MANUF_REG 0x03F00024

#define ADDR_RDRAM_REG_FIRST ADDR_RDRAM_CONFIG_REG
#define ADDR_RDRAM_REG_LAST  ADDR_RDRAM_DEVICE_MANUF_REG

#define ADDR_PI_DRAM_ADDR_REG 0x04600000

#define ADDR_PI_DRAM_ADDR_REG    0x04600000
#define ADDR_PI_CART_ADDR_REG    0x04600004
#define ADDR_PI_RD_LEN_REG       0x04600008
#define ADDR_PI_WR_LEN_REG       0x0460000C
#define ADDR_PI_STATUS_REG       0x04600010
#define ADDR_PI_DOMAIN1_REG      0x04600014
#define ADDR_PI_BSD_DOM1_PWD_REG 0x04600018
#define ADDR_PI_BSD_DOM1_PGS_REG 0x0460001C
#define ADDR_PI_BSD_DOM1_RLS_REG 0x04600020
#define ADDR_PI_DOMAIN2_REG      0x04600024
#define ADDR_PI_BSD_DOM2_PWD_REG 0x04600028
#define ADDR_PI_BSD_DOM2_PGS_REG 0x0460002C
#define ADDR_PI_BSD_DOM2_RLS_REG 0x04600030

#define ADDR_RI_MODE_REG    0x04700000
#define ADDR_RI_CONFIG_REG  0x04700004
#define ADDR_RI_SELECT_REG  0x0470000C
#define ADDR_RI_REFRESH_REG 0x04700010
#define ADDR_RI_LATENCY_REG 0x04700014
#define ADDR_RI_RERROR_REG  0x04700018
#define ADDR_RI_WERROR_REG  0x0470001C

#define ADDR_RI_FIRST ADDR_RI_MODE_REG
#define ADDR_RI_LAST  ADDR_RI_WERROR_REG

#define ADDR_MI_MODE_REG      0x04300000
#define ADDR_MI_VERSION_REG   0x04300004
#define ADDR_MI_INTR_REG      0x04300008
#define ADDR_MI_INTR_MASK_REG 0x0430000C

#define ADDR_MI_FIRST ADDR_MI_MODE_REG
#define ADDR_MI_LAST  ADDR_MI_INTR_MASK_REG

#define ADDR_SI_DRAM_ADDR_REG      0x04800000
#define ADDR_SI_PIF_ADDR_RD64B_REG 0x04800004
#define ADDR_SI_PIF_ADDR_WR64B_REG 0x04800010
#define ADDR_SI_STATUS_REG         0x04800018

#define ADDR_PIF_RAM_JOYPAD 0x1FC007C4

word vatopa(word address) {
    word physical;
    switch (address) {
        case VREGION_KSEG0:
            // Unmapped translation. Subtract the base address of the space to get the physical address.
            physical = address - SVREGION_KSEG0;
            logtrace("KSEG0: Translated 0x%08X to 0x%08X", address, physical)
            break;
        case VREGION_KSEG1:
            // Unmapped translation. Subtract the base address of the space to get the physical address.
            physical = address - SVREGION_KSEG1;
            logtrace("KSEG1: Translated 0x%08X to 0x%08X", address, physical)
            break;
        case VREGION_KSSEG:
            logfatal("Unimplemented: translating virtual address in VREGION_KSSEG")
        case VREGION_KSEG3:
            logfatal("Unimplemented: translating virtual address in VREGION_KSEG3")
        default:
            logfatal("Address 0x%08X doesn't really look like a virtual address", address)
    }
    return physical;
}

word read_word_rdramreg(n64_system_t* system, word address) {
    if (address % 4 != 0) {
        logfatal("Reading from RDRAM register at non-word-aligned address 0x%08X", address)
    }

    if (address < ADDR_RDRAM_REG_FIRST || address > ADDR_RDRAM_REG_LAST) {
        logfatal("In RDRAM register write handler with out of bounds address 0x%08X", address)
    } else {
        return system->mem.rdram_reg[(address - SREGION_RDRAM_REGS) / 4];
    }
}

void write_word_rdramreg(n64_system_t* system, word address, word value) {
    if (address % 4 != 0) {
        logfatal("Writing to RDRAM register at non-word-aligned address 0x%08X", address)
    }

    if (address < ADDR_RDRAM_REG_FIRST || address > ADDR_RDRAM_REG_LAST) {
        logwarn("In RDRAM register write handler with out of bounds address 0x%08X", address)
    } else {
        system->mem.rdram_reg[(address - SREGION_RDRAM_REGS) / 4] = value;
    }
}

word read_word_pireg(n64_system_t* system, word address) {
    switch (address) {
        case ADDR_PI_DRAM_ADDR_REG:
            return system->mem.pi_reg[PI_DRAM_ADDR_REG];
        case ADDR_PI_CART_ADDR_REG:
            logfatal("Reading word from unsupported PI register: PI_CART_ADDR_REG")
        case ADDR_PI_RD_LEN_REG:
            logfatal("Reading word from unsupported PI register: PI_RD_LEN_REG")
        case ADDR_PI_WR_LEN_REG:
            logfatal("Reading word from unsupported PI register: PI_WR_LEN_REG")
        case ADDR_PI_STATUS_REG:
            return is_dma_active();
        case ADDR_PI_DOMAIN1_REG:
            logfatal("Reading word from unsupported PI register: PI_DOMAIN1_REG")
        case ADDR_PI_BSD_DOM1_PWD_REG:
            logfatal("Reading word from unsupported PI register: PI_BSD_DOM1_PWD_REG")
        case ADDR_PI_BSD_DOM1_PGS_REG:
            logfatal("Reading word from unsupported PI register: PI_BSD_DOM1_PGS_REG")
        case ADDR_PI_BSD_DOM1_RLS_REG:
            logfatal("Reading word from unsupported PI register: PI_BSD_DOM1_RLS_REG")
        case ADDR_PI_DOMAIN2_REG:
            logfatal("Reading word from unsupported PI register: PI_DOMAIN2_REG")
        case ADDR_PI_BSD_DOM2_PWD_REG:
            logfatal("Reading word from unsupported PI register: PI_BSD_DOM2_PWD_REG")
        case ADDR_PI_BSD_DOM2_PGS_REG:
            logfatal("Reading word from unsupported PI register: PI_BSD_DOM2_PGS_REG")
        case ADDR_PI_BSD_DOM2_RLS_REG:
            logfatal("Reading word from unsupported PI register: PI_BSD_DOM2_RLS_REG")
        default:
            logfatal("Reading word from unknown PI register 0x%08X", address)
    }
}

void write_word_pireg(n64_system_t* system, word address, word value) {
    switch (address) {
        case ADDR_PI_DRAM_ADDR_REG:
            system->mem.pi_reg[PI_DRAM_ADDR_REG] = value;
            break;
        case ADDR_PI_CART_ADDR_REG:
            system->mem.pi_reg[PI_CART_ADDR_REG] = value;
            break;
            logfatal("Writing word to unsupported PI register: ADDR_PI_CART_ADDR_REG")
        case ADDR_PI_RD_LEN_REG:
            run_dma(system, system->mem.pi_reg[PI_DRAM_ADDR_REG], system->mem.pi_reg[PI_CART_ADDR_REG], value, "DRAM to CART");
            break; // TODO: do we need to persist the `length` value anywhere?
        case ADDR_PI_WR_LEN_REG:
            run_dma(system, system->mem.pi_reg[PI_CART_ADDR_REG], system->mem.pi_reg[PI_DRAM_ADDR_REG], value, "CART to DRAM");
            break; // TODO: do we need to persist the `length` value anywhere?
        case ADDR_PI_STATUS_REG: {
            if (value & 0b01) {
                // Reset controller
                system->mem.pi_reg[PI_DRAM_ADDR_REG] = 0;
                system->mem.pi_reg[PI_CART_ADDR_REG] = 0;
                // TODO: probably other stuff needs to happen here, too.
            }
            if (value & 0b10) {
                logwarn("TODO: Clearing PI intr?")
            }
            break;
        }
        case ADDR_PI_DOMAIN1_REG:
            logfatal("Writing word 0x%08X to unsupported PI register: ADDR_PI_DOMAIN1_REG", value)
        case ADDR_PI_BSD_DOM1_PWD_REG:
            logfatal("Writing word 0x%08X to unsupported PI register: ADDR_PI_BSD_DOM1_PWD_REG", value)
        case ADDR_PI_BSD_DOM1_PGS_REG:
            logfatal("Writing word 0x%08X to unsupported PI register: ADDR_PI_BSD_DOM1_PGS_REG", value)
        case ADDR_PI_BSD_DOM1_RLS_REG:
            logfatal("Writing word 0x%08X to unsupported PI register: ADDR_PI_BSD_DOM1_RLS_REG", value)
        case ADDR_PI_DOMAIN2_REG:
            logfatal("Writing word 0x%08X to unsupported PI register: ADDR_PI_DOMAIN2_REG", value)
        case ADDR_PI_BSD_DOM2_PWD_REG:
            logfatal("Writing word 0x%08X to unsupported PI register: ADDR_PI_BSD_DOM2_PWD_REG", value)
        case ADDR_PI_BSD_DOM2_PGS_REG:
            logfatal("Writing word 0x%08X to unsupported PI register: ADDR_PI_BSD_DOM2_PGS_REG", value)
        case ADDR_PI_BSD_DOM2_RLS_REG:
            logfatal("Writing word 0x%08X to unsupported PI register: ADDR_PI_BSD_DOM2_RLS_REG", value)
        default:
            logfatal("Writing word 0x%08X to unknown PI register 0x%08X", value, address)
    }
}

word read_word_rireg(n64_system_t* system, word address) {
    if (address % 4 != 0) {
        logfatal("Reading from RI register at non-word-aligned address 0x%08X", address)
    }

    if (address < ADDR_RI_MODE_REG || address > ADDR_RI_WERROR_REG) {
        logfatal("In RI write handler with out of bounds address 0x%08X", address)
    }

    return system->mem.ri_reg[(address - SREGION_RI_REGS) / 4];
}

void write_word_rireg(n64_system_t* system, word address, word value) {
    if (address % 4 != 0) {
        logfatal("Writing to RI register at non-word-aligned address 0x%08X", address)
    }

    if (address < ADDR_RI_FIRST || address > ADDR_RI_LAST) {
        logfatal("In RI write handler with out of bounds address 0x%08X", address)
    }

    system->mem.ri_reg[(address - SREGION_RI_REGS) / 4] = value;
}

void write_word_mireg(n64_system_t* system, word address, word value) {
    if (address % 4 != 0) {
        logfatal("Writing to MI register at non-word-aligned address 0x%08X", address)
    }

    if (address < ADDR_MI_FIRST || address > ADDR_MI_LAST) {
        logfatal("In MI write handler with out of bounds address 0x%08X", address)
    }

    system->mem.mi_reg[(address - SREGION_MI_REGS) / 4] = value;
}

word read_word_mireg(n64_system_t* system, word address) {
    if (address % 4 != 0) {
        logfatal("Writing to MI register at non-word-aligned address 0x%08X", address)
    }

    if (address < ADDR_MI_FIRST || address > ADDR_MI_LAST) {
        logfatal("In MI write handler with out of bounds address 0x%08X", address)
    }

    return system->mem.mi_reg[(address - SREGION_MI_REGS) / 4];
}

void write_word_sireg(n64_system_t* system, word address, word value) {
    switch (address) {
        case ADDR_SI_STATUS_REG:
            logwarn("TODO: any write to SI status register clears interrupt")
            break;
        default:
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_SI_REGS", value, address)
    }
}

void n64_write_word(n64_system_t* system, word address, word value) {
    switch (address) {
        case REGION_RDRAM:
            word_to_byte_array((byte*) &system->mem.rdram, address - SREGION_RDRAM, value);
            break;
        case REGION_RDRAM_REGS:
            write_word_rdramreg(system, address, value);
            break;
        case REGION_RDRAM_UNUSED:
            return;
        case REGION_SP_DMEM:
            word_to_byte_array((byte*) &system->mem.sp_dmem, address - SREGION_SP_DMEM, value);
            break;
        case REGION_SP_IMEM:
            word_to_byte_array((byte*) &system->mem.sp_imem, address - SREGION_SP_IMEM, value);
            break;
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_SP_IMEM", value, address)
        case REGION_SP_UNUSED:
            return;
        case REGION_SP_REGS:
            write_word_spreg(system, address, value);
            break;
        case REGION_DP_COMMAND_REGS:
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_DP_COMMAND_REGS", value, address)
        case REGION_DP_SPAN_REGS:
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_DP_SPAN_REGS", value, address)
        case REGION_MI_REGS:
            write_word_mireg(system, address, value);
            break;
        case REGION_VI_REGS:
            write_word_vireg(system, address, value);
            break;
        case REGION_AI_REGS:
            logwarn("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_AI_REGS", value, address)
            break;
        case REGION_PI_REGS:
            write_word_pireg(system, address, value);
            break;
        case REGION_RI_REGS:
            write_word_rireg(system, address, value);
            break;
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_RI_REGS", value, address)
        case REGION_SI_REGS:
            write_word_sireg(system, address, value);
            break;
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
            if (address == ADDR_PIF_RAM_JOYPAD) {
                logwarn("Ignoring write to JOYPAD in REGION_PIF_RAM")
                break;
            } else {
                word_to_byte_array(system->mem.pif_ram, address - SREGION_PIF_RAM, value);
                logwarn("Writing word 0x%08X to address 0x%08X in region: REGION_PIF_RAM", value, address)
            }
            break;
        case REGION_RESERVED:
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_RESERVED", value, address)
        case REGION_CART_1_3:
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_CART_1_3", value, address)
        case REGION_SYSAD_DEVICE:
            n64_write_word(system, vatopa(address), value);
            break;
        default:
            logfatal("Writing word 0x%08X to unknown address: 0x%08X", value, address)
    }
}

word read_unused(word address) {
    logwarn("Reading unused value at 0x%08X!", address)
    return 0;
}

word n64_read_word(n64_system_t* system, word address) {
    switch (address) {
        case REGION_RDRAM:
            return word_from_byte_array((byte*) &system->mem.rdram, address - SREGION_RDRAM);
        case REGION_RDRAM_UNUSED:
            return read_unused(address);
        case REGION_RDRAM_REGS:
            return read_word_rdramreg(system, address);
        case REGION_SP_DMEM:
            return word_from_byte_array((byte*) &system->mem.sp_dmem, address - SREGION_SP_DMEM);
        case REGION_SP_IMEM:
            return word_from_byte_array((byte*) &system->mem.sp_imem, address - SREGION_SP_IMEM);
        case REGION_SP_UNUSED:
            return read_unused(address);
        case REGION_SP_REGS:
            return read_word_spreg(system, address);
        case REGION_DP_COMMAND_REGS:
            logfatal("Reading word from address 0x%08X in unsupported region: REGION_DP_COMMAND_REGS", address)
        case REGION_DP_SPAN_REGS:
            logfatal("Reading word from address 0x%08X in unsupported region: REGION_DP_SPAN_REGS", address)
        case REGION_MI_REGS:
            return read_word_mireg(system, address);
        case REGION_VI_REGS:
            logfatal("Reading word from address 0x%08X in unsupported region: REGION_VI_REGS", address)
        case REGION_AI_REGS:
            logfatal("Reading word from address 0x%08X in unsupported region: REGION_AI_REGS", address)
        case REGION_PI_REGS:
            return read_word_pireg(system, address);
        case REGION_RI_REGS:
            return read_word_rireg(system, address);
            logfatal("Reading word from address 0x%08X in unsupported region: REGION_RI_REGS", address)
        case REGION_SI_REGS:
            logfatal("Reading word from address 0x%08X in unsupported region: REGION_SI_REGS", address)
        case REGION_UNUSED:
            logfatal("Reading word from address 0x%08X in unsupported region: REGION_UNUSED", address)
        case REGION_CART_2_1:
            logfatal("Reading word from address 0x%08X in unsupported region: REGION_CART_2_1", address)
        case REGION_CART_1_1:
            logfatal("Reading word from address 0x%08X in unsupported region: REGION_CART_1_1", address)
        case REGION_CART_2_2:
            logfatal("Reading word from address 0x%08X in unsupported region: REGION_CART_2_2", address)
        case REGION_CART_1_2: {
            word index = address - SREGION_CART_1_2;
            if (index > system->mem.rom.size - 3) { // -3 because we're reading an entire word
                logfatal("Address 0x%08X accessed an index %d/0x%X outside the bounds of the ROM!", address, index, index)
            }
            return word_from_byte_array(system->mem.rom.rom, index);
        }
        case REGION_PIF_BOOT:
            logfatal("Reading word from address 0x%08X in unsupported region: REGION_PIF_BOOT", address)
        case REGION_PIF_RAM:
            logfatal("Reading word from address 0x%08X in unsupported region: REGION_PIF_RAM", address)
        case REGION_RESERVED:
            logfatal("Reading word from address 0x%08X in unsupported region: REGION_RESERVED", address)
        case REGION_CART_1_3:
            logfatal("Reading word from address 0x%08X in unsupported region: REGION_CART_1_3", address)
        case REGION_SYSAD_DEVICE:
            return n64_read_word(system, vatopa(address));
        default:
            logfatal("Reading word from unknown address: 0x%08X", address)
    }
}

void n64_write_byte(n64_system_t* system, word address, byte value) {
    switch (address) {
        case REGION_RDRAM:
            system->mem.rdram[address] = value;
            break;
        case REGION_RDRAM_REGS:
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_RDRAM_REGS", value, address)
        case REGION_SP_DMEM: {
            system->mem.sp_dmem[address - SREGION_SP_DMEM] = value;
            break;
        }
        case REGION_SP_IMEM: {
            system->mem.sp_imem[address - SREGION_SP_IMEM] = value;
            break;
        }
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
            return n64_write_byte(system, vatopa(address), value);
            logfatal("Writing byte 0x%02X to address 0x%08X in unsupported region: REGION_SYSAD_DEVICE", value, address)
        default:
            logfatal("Writing byte 0x%02X to unknown address: 0x%08X", value, address)
    }
}

byte n64_read_byte(n64_system_t* system, word address) {
    switch (address) {
        case REGION_RDRAM:
            return system->mem.rdram[address];
        case REGION_RDRAM_REGS:
            logfatal("Reading byte from address 0x%08X in unsupported region: REGION_RDRAM_REGS", address)
        case REGION_SP_DMEM:
            return system->mem.sp_dmem[address - SREGION_SP_DMEM];
        case REGION_SP_IMEM:
            return system->mem.sp_imem[address - SREGION_SP_IMEM];
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
        case REGION_CART_1_2: {
            word index = address - SREGION_CART_1_2;
            if (index > system->mem.rom.size) {
                logfatal("Address 0x%08X accessed an index %d/0x%X outside the bounds of the ROM!", address, index, index)
            }
            return system->mem.rom.rom[index];
        }
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
            return n64_read_byte(system, vatopa(address));
            logfatal("Reading byte from address 0x%08X in unsupported region: REGION_SYSAD_DEVICE", address)
        default:
            logfatal("Reading byte from unknown address: 0x%08X", address)
    }
}
