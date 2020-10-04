#ifndef N64_RSP_H
#define N64_RSP_H

#include <stdbool.h>
#include <util.h>
#include <log.h>
#include <mem/addresses.h>
#include <system/n64system.h>
#include <rdp/rdp.h>

#include "rsp_types.h"
#include "rsp_interface.h"

#define RSP_CP0_DMA_CACHE        0
#define RSP_CP0_DMA_DRAM         1
#define RSP_CP0_DMA_READ_LENGTH  2
#define RSP_CP0_DMA_WRITE_LENGTH 3
#define RSP_CP0_SP_STATUS        4
#define RSP_CP0_DMA_FULL         5
#define RSP_CP0_DMA_BUSY         6
#define RSP_CP0_DMA_RESERVED     7
#define RSP_CP0_CMD_START        8
#define RSP_CP0_CMD_END          9
#define RSP_CP0_CMD_CURRENT      10
#define RSP_CP0_CMD_STATUS       11
#define RSP_CP0_CMD_CLOCK        12
#define RSP_CP0_CMD_BUSY         13
#define RSP_CP0_CMD_PIPE_BUSY    14
#define RSP_CP0_CMD_TMEM_BUSY    15

#define RSP_OPC_LWC2 0b110010
#define RSP_OPC_SWC2 0b111010

#define LWC2_LBV 0b00000
#define LWC2_LDV 0b00011
#define LWC2_LFV 0b01001
#define LWC2_LHV 0b01000
#define LWC2_LLV 0b00010
#define LWC2_LPV 0b00110
#define LWC2_LQV 0b00100
#define LWC2_LRV 0b00101
#define LWC2_LSV 0b00001
#define LWC2_LTV 0b01011
#define LWC2_LUV 0b00111

#define FUNCT_RSP_VEC_VABS  0b010011
#define FUNCT_RSP_VEC_VADD  0b010000
#define FUNCT_RSP_VEC_VADDC 0b010100
#define FUNCT_RSP_VEC_VAND  0b101000
#define FUNCT_RSP_VEC_VCH   0b100101
#define FUNCT_RSP_VEC_VCL   0b100100
#define FUNCT_RSP_VEC_VCR   0b100110
#define FUNCT_RSP_VEC_VEQ   0b100001
#define FUNCT_RSP_VEC_VGE   0b100011
#define FUNCT_RSP_VEC_VLT   0b100000
#define FUNCT_RSP_VEC_VMACF 0b001000
#define FUNCT_RSP_VEC_VMACQ 0b001011
#define FUNCT_RSP_VEC_VMACU 0b001001
#define FUNCT_RSP_VEC_VMADH 0b001111
#define FUNCT_RSP_VEC_VMADL 0b001100
#define FUNCT_RSP_VEC_VMADM 0b001101
#define FUNCT_RSP_VEC_VMADN 0b001110
#define FUNCT_RSP_VEC_VMOV  0b110011
#define FUNCT_RSP_VEC_VMRG  0b100111
#define FUNCT_RSP_VEC_VMUDH 0b000111
#define FUNCT_RSP_VEC_VMUDL 0b000100
#define FUNCT_RSP_VEC_VMUDM 0b000101
#define FUNCT_RSP_VEC_VMUDN 0b000110
#define FUNCT_RSP_VEC_VMULF 0b000000
#define FUNCT_RSP_VEC_VMULQ 0b000011
#define FUNCT_RSP_VEC_VMULU 0b000001
#define FUNCT_RSP_VEC_VNAND 0b101001
#define FUNCT_RSP_VEC_VNE   0b100010
#define FUNCT_RSP_VEC_VNOP  0b110111
#define FUNCT_RSP_VEC_VNOR  0b101011
#define FUNCT_RSP_VEC_VNXOR 0b101101
#define FUNCT_RSP_VEC_VOR   0b101010
#define FUNCT_RSP_VEC_VRCP  0b110000
#define FUNCT_RSP_VEC_VRCPH 0b110010
#define FUNCT_RSP_VEC_VRCPL 0b110001
#define FUNCT_RSP_VEC_VRNDN 0b001010
#define FUNCT_RSP_VEC_VRNDP 0b000010
#define FUNCT_RSP_VEC_VRSQ  0b110100
#define FUNCT_RSP_VEC_VRSQH 0b110110
#define FUNCT_RSP_VEC_VRSQL 0b110101
#define FUNCT_RSP_VEC_VSAR  0b011101
#define FUNCT_RSP_VEC_VSUB  0b010001
#define FUNCT_RSP_VEC_VSUBC 0b010101
#define FUNCT_RSP_VEC_VXOR  0b101100

INLINE void rsp_dma_read(rsp_t* rsp) {
    word length = rsp->io.dma_read.length + 1;

    length = (length + 0x7) & ~0x7;

    word dram_address = rsp->io.dram_addr.address & 0xFFFFF8;
    if (dram_address != rsp->io.dram_addr.address) {
        logfatal("Misaligned DRAM RSP DMA READ!");
    }
    word mem_address = rsp->io.mem_addr.address & 0xFFC;
    if (mem_address != rsp->io.mem_addr.address) {
        logfatal("Misaligned MEM RSP DMA READ!");
    }

    for (int i = 0; i < rsp->io.dma_read.count + 1; i++) {
        word mem_addr = mem_address + (rsp->io.mem_addr.imem ? SREGION_SP_IMEM : SREGION_SP_DMEM);
        for (int j = 0; j < length; j += 4) {
            word val = rsp->read_physical_word(dram_address + j);
            rsp->write_physical_word(mem_addr + j, val);
        }

        dram_address += length + rsp->io.dma_read.skip;
        mem_address += length;
    }
}

INLINE void rsp_dma_write(rsp_t* rsp) {
    word length = rsp->io.dma_write.length + 1;

    length = (length + 0x7) & ~0x7;

    word dram_address = rsp->io.dram_addr.address & 0xFFFFF8;
    if (dram_address != rsp->io.dram_addr.address) {
        logfatal("Misaligned DRAM RSP DMA WRITE!");
    }
    word mem_address = rsp->io.mem_addr.address & 0xFFC;
    if (mem_address != rsp->io.mem_addr.address) {
        logfatal("Misaligned MEM RSP DMA WRITE!");
    }

    for (int i = 0; i < rsp->io.dma_write.count + 1; i++) {
        word mem_addr = mem_address + (rsp->io.mem_addr.imem ? SREGION_SP_IMEM : SREGION_SP_DMEM);
        for (int j = 0; j < length; j += 4) {
            word val = rsp->read_physical_word(mem_addr + j);
            rsp->write_physical_word(dram_address + j, val);
        }

        dram_address += length + rsp->io.dma_write.skip;
        mem_address += length;
    }
}

INLINE void set_rsp_register(rsp_t* rsp, byte r, word value) {
    if (r != 0) {
        if (r < 64) {
            rsp->gpr[r] = value;
        } else {
            logfatal("Write to invalid RSP register: %d", r);
        }
    }
}

INLINE word get_rsp_register(rsp_t* rsp, byte r) {
    if (r < 64) {
        word value = rsp->gpr[r];
        return value;
    } else {
        logfatal("Attempted to read invalid RSP register: %d", r);
    }
}

bool rsp_acquire_semaphore(n64_system_t* system);

INLINE word get_rsp_cp0_register(n64_system_t* system, byte r) {
    switch (r) {
        case RSP_CP0_DMA_CACHE:
            logfatal("Read from unknown RSP CP0 register $c%d: RSP_CP0_DMA_CACHE", r);
        case RSP_CP0_DMA_DRAM:
            logfatal("Read from unknown RSP CP0 register $c%d: RSP_CP0_DMA_DRAM", r);
        case RSP_CP0_DMA_READ_LENGTH:
            logfatal("Read from unknown RSP CP0 register $c%d: RSP_CP0_DMA_READ_LENGTH", r);
        case RSP_CP0_DMA_WRITE_LENGTH:
            logfatal("Read from unknown RSP CP0 register $c%d: RSP_CP0_DMA_WRITE_LENGTH", r);
        case RSP_CP0_SP_STATUS: return system->rsp.status.raw;
        case RSP_CP0_DMA_FULL:  return system->rsp.status.dma_full;
        case RSP_CP0_DMA_BUSY:  return system->rsp.status.dma_busy;
        case RSP_CP0_DMA_RESERVED: return rsp_acquire_semaphore(system);
        case RSP_CP0_CMD_START:
            logfatal("Read from unknown RSP CP0 register $c%d: RSP_CP0_CMD_START", r);
        case RSP_CP0_CMD_END:     return system->dpc.end;
        case RSP_CP0_CMD_CURRENT: return system->dpc.current;
        case RSP_CP0_CMD_STATUS:  return system->dpc.status.raw;
        case RSP_CP0_CMD_CLOCK:
            logfatal("Read from unknown RSP CP0 register $c%d: RSP_CP0_CMD_CLOCK", r);
        case RSP_CP0_CMD_BUSY:
            logfatal("Read from unknown RSP CP0 register $c%d: RSP_CP0_CMD_BUSY", r);
        case RSP_CP0_CMD_PIPE_BUSY:
            logfatal("Read from unknown RSP CP0 register $c%d: RSP_CP0_CMD_PIPE_BUSY", r);
        case RSP_CP0_CMD_TMEM_BUSY:
            logfatal("Read from unknown RSP CP0 register $c%d: RSP_CP0_CMD_TMEM_BUSY", r);
        default:
            logfatal("Unsupported RSP CP0 $c%d read", r);
    }
}

INLINE void set_rsp_cp0_register(n64_system_t* system, byte r, word value) {
    switch (r) {
        case RSP_CP0_DMA_CACHE: system->rsp.io.mem_addr.raw = value; break;
        case RSP_CP0_DMA_DRAM:  system->rsp.io.dram_addr.raw = value; break;
        case RSP_CP0_DMA_READ_LENGTH:
            system->rsp.io.dma_read.raw = value;
            rsp_dma_read(&system->rsp);
            break;
        case RSP_CP0_DMA_WRITE_LENGTH:
            system->rsp.io.dma_write.raw = value;
            rsp_dma_write(&system->rsp);
            break;
        case RSP_CP0_SP_STATUS:
            rsp_status_reg_write(system, value);
            break;
        case RSP_CP0_DMA_FULL:
            logfatal("Write to unknown RSP CP0 register $c%d: RSP_CP0_DMA_FULL", r);
        case RSP_CP0_DMA_BUSY:
            logfatal("Write to unknown RSP CP0 register $c%d: RSP_CP0_DMA_BUSY", r);
        case RSP_CP0_DMA_RESERVED: {
            if (value == 0) {
                system->rsp.semaphore_held = false;
            } else {
                logfatal("Wrote non-zero value 0x%08X to $c7 RSP_CP0_DMA_RESERVED", value);
            }
            break;
        }
        case RSP_CP0_CMD_START:
            system->dpc.start = value & 0xFFFFFF;
            system->dpc.current = system->dpc.start;
            break;
        case RSP_CP0_CMD_END:
            system->dpc.end = value & 0xFFFFFF;
            rdp_run_command(system);
            break;
        case RSP_CP0_CMD_CURRENT:
            logfatal("Write to unknown RSP CP0 register $c%d: RSP_CP0_CMD_CURRENT", r);
        case RSP_CP0_CMD_STATUS:
            rdp_status_reg_write(system, value);
            break;
        case RSP_CP0_CMD_CLOCK:
            logfatal("Write to unknown RSP CP0 register $c%d: RSP_CP0_CMD_CLOCK", r);
        case RSP_CP0_CMD_BUSY:
            logfatal("Write to unknown RSP CP0 register $c%d: RSP_CP0_CMD_BUSY", r);
        case RSP_CP0_CMD_PIPE_BUSY:
            logfatal("Write to unknown RSP CP0 register $c%d: RSP_CP0_CMD_PIPE_BUSY", r);
        case RSP_CP0_CMD_TMEM_BUSY:
            logfatal("Write to unknown RSP CP0 register $c%d: RSP_CP0_CMD_TMEM_BUSY", r);
        default:
            logfatal("Unsupported RSP CP0 $c%d written to", r);
    }
}

INLINE sdword get_rsp_accumulator(rsp_t* rsp, int e) {
    sdword val = (sdword)rsp->acc.h.elements[e] << 48;
    val       |= (sdword)rsp->acc.m.elements[e] << 32;
    val       |= (sdword)rsp->acc.l.elements[e] << 16;
    val >>= 16;
    return val;
}

INLINE void set_rsp_accumulator(rsp_t* rsp, int e, dword val) {
    rsp->acc.h.elements[e] = (val >> 32) & 0xFFFF;
    rsp->acc.m.elements[e] = (val >> 16) & 0xFFFF;
    rsp->acc.l.elements[e] = val & 0xFFFF;
}

INLINE half rsp_get_vco(rsp_t* rsp) {
    half value = 0;
    for (int i = 0; i < 8; i++) {
        bool h = rsp->vco.h.elements[7 - i] != 0;
        bool l = rsp->vco.l.elements[7 - i] != 0;
        word mask = (l << i) | (h << (i + 8));
        value |= mask;
    }
    return value;
}

INLINE half rsp_get_vcc(rsp_t* rsp) {
    half value = 0;
    for (int i = 0; i < 8; i++) {
        bool h = rsp->vcc.h.elements[7 - i] != 0;
        bool l = rsp->vcc.l.elements[7 - i] != 0;
        word mask = (l << i) | (h << (i + 8));
        value |= mask;
    }
    return value;
}

INLINE byte rsp_get_vce(rsp_t* rsp) {
    byte value = 0;
    for (int i = 0; i < 8; i++) {
        bool l = rsp->vce.elements[7 - i] != 0;
        value |= (l << i);
    }
    return value;
}

void rsp_step(n64_system_t* system);
int rsp_run(n64_system_t* system, int steps);

#endif //N64_RSP_H
