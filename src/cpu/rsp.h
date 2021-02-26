#ifndef N64_RSP_H
#define N64_RSP_H

#include <stdbool.h>
#include <util.h>
#include <log.h>
#include <mem/addresses.h>
#include <system/n64system.h>
#include <rdp/rdp.h>
#include <mem/n64bus.h>

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

#define RSP_DRAM_ADDR_MASK 0xFFFFF8
#define RSP_MEM_ADDR_MASK 0xFF8

#define FLAGREG_BOOL(x) ((x) ? 0xFFFF : 0)

extern rsp_t n64rsp;
#define N64RSP n64rsp

INLINE void rsp_dma_read() {
    word length = N64RSP.io.dma.length + 1;

    dram_addr_t dram_addr_reg = N64RSP.io.shadow_dmem_addr;
    mem_addr_t mem_addr_reg = N64RSP.io.shadow_mem_addr;

    length = (length + 0x7) & ~0x7;

    if (mem_addr_reg.address + length > 0x1000) {
        logfatal("RSP DMA READ would read off the end of memory!\n");
    }

    word dram_address = dram_addr_reg.address & RSP_DRAM_ADDR_MASK;
    if (dram_address != dram_addr_reg.address) {
        logwarn("Misaligned DRAM RSP DMA READ! (from 0x%08X, aligned to 0x%08X)", dram_addr_reg.address, dram_address);
    }
    word mem_address = mem_addr_reg.address & RSP_MEM_ADDR_MASK;
    if (mem_address != mem_addr_reg.address) {
        logwarn("Misaligned MEM RSP DMA READ! (from 0x%08X, aligned to 0x%08X)", mem_addr_reg.address, mem_address);
    }

    for (int i = 0; i < N64RSP.io.dma.count + 1; i++) {
        word full_mem_addr = mem_address + (mem_addr_reg.imem ? SREGION_SP_IMEM : SREGION_SP_DMEM);
        for (int j = 0; j < length; j += 4) {
            word val = n64_read_physical_word(dram_address + j);
            n64_write_physical_word(full_mem_addr + j, val);
        }

        int skip = i == N64RSP.io.dma.count ? 0 : N64RSP.io.dma.skip;

        dram_address += (length + skip) & RSP_DRAM_ADDR_MASK;
        mem_address += length;
    }

    // Set registers for reading now that DMA is complete
    N64RSP.io.dram_addr.address = dram_address;
    N64RSP.io.mem_addr.address = mem_address;
    N64RSP.io.mem_addr.imem = mem_addr_reg.imem;

    // Hardware seems to always return this value in the length register
    // No real idea why
    N64RSP.io.dma.raw = 0xFF8 | (N64RSP.io.dma.skip << 20);
}

INLINE void rsp_dma_write() {
    word length = N64RSP.io.dma.length + 1;

    dram_addr_t dram_addr = N64RSP.io.shadow_dmem_addr;
    mem_addr_t mem_addr = N64RSP.io.shadow_mem_addr;

    length = (length + 0x7) & ~0x7;

    if (mem_addr.address + length > 0x1000) {
        logfatal("RSP DMA WRITE would write off the end of memory!\n");
    }

    word dram_address = dram_addr.address & RSP_DRAM_ADDR_MASK;
    if (dram_address != dram_addr.address) {
        logwarn("Misaligned DRAM RSP DMA WRITE! 0x%08X", dram_addr.address);
    }
    word mem_address = mem_addr.address & RSP_MEM_ADDR_MASK;
    if (mem_address != mem_addr.address) {
        logwarn("Misaligned MEM RSP DMA WRITE! 0x%08X", mem_addr.address);
    }

    for (int i = 0; i < N64RSP.io.dma.count + 1; i++) {
        word full_mem_addr = mem_address + (mem_addr.imem ? SREGION_SP_IMEM : SREGION_SP_DMEM);
        for (int j = 0; j < length; j += 4) {
            word val = n64_read_physical_word(full_mem_addr + j);
            n64_write_physical_word(dram_address + j, val);
        }

        int skip = i == N64RSP.io.dma.count ? 0 : N64RSP.io.dma.skip;

        dram_address += (length + skip) & RSP_DRAM_ADDR_MASK;
        mem_address += length;
    }

    N64RSP.io.dram_addr.address = dram_address;
    N64RSP.io.mem_addr.address = mem_address;
    N64RSP.io.mem_addr.imem = mem_addr.imem;

    // Hardware seems to always return this value in the length register
    // No real idea why
    N64RSP.io.dma.raw = 0xFF8 | (N64RSP.io.dma.skip << 20);
}

INLINE void set_rsp_register(byte r, word value) {
    if (r != 0) {
        if (r < 64) {
            N64RSP.gpr[r] = value;
        } else {
            logfatal("Write to invalid RSP register: %d", r);
        }
    }
}

INLINE word get_rsp_register(byte r) {
    if (r < 64) {
        word value = N64RSP.gpr[r];
        return value;
    } else {
        logfatal("Attempted to read invalid RSP register: %d", r);
    }
}

bool rsp_acquire_semaphore();
void rsp_release_semaphore();

INLINE word get_rsp_cp0_register(byte r) {
    switch (r) {
        case RSP_CP0_DMA_CACHE: return N64RSP.io.shadow_mem_addr.raw;
            logfatal("Read from unknown RSP CP0 register $c%d: RSP_CP0_DMA_CACHE", r);
        case RSP_CP0_DMA_DRAM:
            return N64RSP.io.shadow_dmem_addr.raw;
        case RSP_CP0_DMA_READ_LENGTH:
            logfatal("Read from unknown RSP CP0 register $c%d: RSP_CP0_DMA_READ_LENGTH", r);
        case RSP_CP0_DMA_WRITE_LENGTH:
            logfatal("Read from unknown RSP CP0 register $c%d: RSP_CP0_DMA_WRITE_LENGTH", r);
        case RSP_CP0_SP_STATUS: return N64RSP.status.raw;
        case RSP_CP0_DMA_FULL:  return N64RSP.status.dma_full;
        case RSP_CP0_DMA_BUSY:  return N64RSP.status.dma_busy;
        case RSP_CP0_DMA_RESERVED: return rsp_acquire_semaphore();
        case RSP_CP0_CMD_START:
            logfatal("Read from unknown RSP CP0 register $c%d: RSP_CP0_CMD_START", r);
        case RSP_CP0_CMD_END:     return n64sys.dpc.end;
        case RSP_CP0_CMD_CURRENT: return n64sys.dpc.current;
        case RSP_CP0_CMD_STATUS:  return n64sys.dpc.status.raw;
        case RSP_CP0_CMD_CLOCK:
            logwarn("Read from RDP clock: returning 0.");
            return 0;
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

INLINE void set_rsp_cp0_register(byte r, word value) {
    switch (r) {
        case RSP_CP0_DMA_CACHE: N64RSP.io.shadow_mem_addr.raw = value; break;
        case RSP_CP0_DMA_DRAM:  N64RSP.io.shadow_dmem_addr.raw = value; break;
        case RSP_CP0_DMA_READ_LENGTH:
            N64RSP.io.dma.raw = value;
            rsp_dma_read();
            break;
        case RSP_CP0_DMA_WRITE_LENGTH:
            N64RSP.io.dma.raw = value;
            rsp_dma_write();
            break;
        case RSP_CP0_SP_STATUS:
            rsp_status_reg_write(value);
            break;
        case RSP_CP0_DMA_FULL:
            logfatal("Write to unknown RSP CP0 register $c%d: RSP_CP0_DMA_FULL", r);
        case RSP_CP0_DMA_BUSY:
            logfatal("Write to unknown RSP CP0 register $c%d: RSP_CP0_DMA_BUSY", r);
        case RSP_CP0_DMA_RESERVED: {
            if (value == 0) {
                rsp_release_semaphore();
            } else {
                logfatal("Wrote non-zero value 0x%08X to $c7 RSP_CP0_DMA_RESERVED", value);
            }
            break;
        }
        case RSP_CP0_CMD_START:
            n64sys.dpc.start = value & 0xFFFFFF;
            n64sys.dpc.current = n64sys.dpc.start;
            break;
        case RSP_CP0_CMD_END:
            n64sys.dpc.end = value & 0xFFFFFF;
            rdp_run_command();
            break;
        case RSP_CP0_CMD_CURRENT:
            logfatal("Write to unknown RSP CP0 register $c%d: RSP_CP0_CMD_CURRENT", r);
        case RSP_CP0_CMD_STATUS:
            rdp_status_reg_write(value);
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

INLINE sdword get_rsp_accumulator(int e) {
    sdword val = (sdword)N64RSP.acc.h.elements[e] << 48;
    val       |= (sdword)N64RSP.acc.m.elements[e] << 32;
    val       |= (sdword)N64RSP.acc.l.elements[e] << 16;
    val >>= 16;
    return val;
}

INLINE void set_rsp_accumulator(int e, dword val) {
    N64RSP.acc.h.elements[e] = (val >> 32) & 0xFFFF;
    N64RSP.acc.m.elements[e] = (val >> 16) & 0xFFFF;
    N64RSP.acc.l.elements[e] = val & 0xFFFF;
}

INLINE half rsp_get_vco() {
    half value = 0;
    for (int i = 0; i < 8; i++) {
        bool h = N64RSP.vco.h.elements[7 - i] != 0;
        bool l = N64RSP.vco.l.elements[7 - i] != 0;
        word mask = (l << i) | (h << (i + 8));
        value |= mask;
    }
    return value;
}

INLINE half rsp_get_vcc() {
    half value = 0;
    for (int i = 0; i < 8; i++) {
        bool h = N64RSP.vcc.h.elements[7 - i] != 0;
        bool l = N64RSP.vcc.l.elements[7 - i] != 0;
        word mask = (l << i) | (h << (i + 8));
        value |= mask;
    }
    return value;
}

INLINE byte rsp_get_vce() {
    byte value = 0;
    for (int i = 0; i < 8; i++) {
        bool l = N64RSP.vce.elements[7 - i] != 0;
        value |= (l << i);
    }
    return value;
}

INLINE void rsp_set_vcc(half vcc) {
    for (int i = 0; i < 8; i++) {
        N64RSP.vcc.l.elements[7 - i] = FLAGREG_BOOL(vcc & 1);
        vcc >>= 1;
    }

    for (int i = 0; i < 8; i++) {
        N64RSP.vcc.h.elements[7 - i] = FLAGREG_BOOL(vcc & 1);
        vcc >>= 1;
    }
}

INLINE void rsp_set_vco(half vco) {
    for (int i = 0; i < 8; i++) {
        N64RSP.vco.l.elements[7 - i] = FLAGREG_BOOL(vco & 1);
        vco >>= 1;
    }

    for (int i = 0; i < 8; i++) {
        N64RSP.vco.h.elements[7 - i] = FLAGREG_BOOL(vco & 1);
        vco >>= 1;
    }
}

INLINE void rsp_set_vce(half vce) {
    for (int i = 0; i < 8; i++) {
        N64RSP.vce.elements[7 - i] = FLAGREG_BOOL(vce & 1);
        vce >>= 1;
    }
}

void rsp_step();
void rsp_run();
vu_reg_t ext_get_vte(vu_reg_t* vt, byte e);

#endif //N64_RSP_H
