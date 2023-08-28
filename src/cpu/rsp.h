#ifndef N64_RSP_H
#define N64_RSP_H

#include <stdbool.h>
#include <util.h>
#include <log.h>
#include <mem/addresses.h>
#include <system/n64system.h>
#include <rdp/rdp.h>
#include <mem/n64bus.h>
#include <cpu/dynarec/dynarec.h>
#include <mem/mem_util.h>

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

// Undocumented
#define SWC2_SWV 0b01010

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

// Undocumented
#define FUNCT_RSP_VEC_VSUT   0b010010
#define FUNCT_RSP_VEC_VADDB  0b010110
#define FUNCT_RSP_VEC_VSUBB  0b010111
#define FUNCT_RSP_VEC_VACCB  0b011000
#define FUNCT_RSP_VEC_VSUCB  0b011001
#define FUNCT_RSP_VEC_VSAD   0b011010
#define FUNCT_RSP_VEC_VSAC   0b011011
#define FUNCT_RSP_VEC_VSUM   0b011100
#define FUNCT_RSP_VEC_0x1E   0b011110
#define FUNCT_RSP_VEC_0x1F   0b011111
#define FUNCT_RSP_VEC_0x2E   0b101110
#define FUNCT_RSP_VEC_0x2F   0b101111
#define FUNCT_RSP_VEC_VEXTT  0b111000
#define FUNCT_RSP_VEC_VEXTQ  0b111001
#define FUNCT_RSP_VEC_VEXTN  0b111010
#define FUNCT_RSP_VEC_0x3B   0b111011
#define FUNCT_RSP_VEC_VINST  0b111100
#define FUNCT_RSP_VEC_VINSQ  0b111101
#define FUNCT_RSP_VEC_VINSN  0b111110
#define FUNCT_RSP_VEC_VNULL  0b111111

#define RSP_DRAM_ADDR_MASK 0xFFFFF8
#define RSP_MEM_ADDR_MASK 0xFF8

#define FLAGREG_BOOL(x) ((x) ? 0xFFFF : 0)

extern rsp_t n64rsp;
#define N64RSP n64rsp
#define N64RSPDYNAREC n64rsp.dynarec

INLINE void quick_invalidate_rsp_icache(u32 address) {
    int index = address / 4;

    N64RSP.icache[index].handler = cache_rsp_instruction;
    N64RSP.icache[index].instruction.raw = word_from_byte_array(N64RSP.sp_imem, address);
#ifdef N64_DYNAREC_V1_ENABLED
    N64RSPDYNAREC->blockcache[index].run = rsp_missing_block_handler;
#endif
}

INLINE void invalidate_rsp_icache(u32 address) {
    quick_invalidate_rsp_icache(address & 0xFFC);
}

INLINE void rsp_dma_read() {
    u32 length = N64RSP.io.dma.length + 1;

    dram_addr_t dram_addr_reg = N64RSP.io.shadow_dram_addr;
    mem_addr_t mem_addr_reg = N64RSP.io.shadow_mem_addr;

    length = (length + 0x7) & ~0x7;

    u32 dram_address = dram_addr_reg.address & RSP_DRAM_ADDR_MASK;
    if (dram_address != dram_addr_reg.address) {
        logwarn("Misaligned DRAM RSP DMA READ! (from 0x%08X, aligned to 0x%08X)", dram_addr_reg.address, dram_address);
    }
    u32 mem_address = mem_addr_reg.address & RSP_MEM_ADDR_MASK;
    if (mem_address != mem_addr_reg.address) {
        logwarn("Misaligned MEM RSP DMA READ! (from 0x%08X, aligned to 0x%08X)", mem_addr_reg.address, mem_address);
    }

    for (int i = 0; i < N64RSP.io.dma.count + 1; i++) {
        u8* mem = (mem_addr_reg.imem ? N64RSP.sp_imem : N64RSP.sp_dmem);
        u8* rdram = n64sys.mem.rdram + dram_address;
        for (int j = 0; j < length; j++) {
            u16 addr = (mem_address + j) & 0xFFF;
            mem[addr] = rdram[j];
        }

        if (mem_addr_reg.imem) {
            for (int j = 0; j < length; j += 4) {
                quick_invalidate_rsp_icache((mem_address + j) & 0xFFF);
            }
        }

        int skip = i == N64RSP.io.dma.count ? 0 : N64RSP.io.dma.skip;

        dram_address += (length + skip);
        dram_address &= RSP_DRAM_ADDR_MASK;
        mem_address += length;
        mem_address &= RSP_MEM_ADDR_MASK;
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
    u32 length = N64RSP.io.dma.length + 1;

    dram_addr_t dram_addr = N64RSP.io.shadow_dram_addr;
    mem_addr_t mem_addr = N64RSP.io.shadow_mem_addr;

    length = (length + 0x7) & ~0x7;

    u32 dram_address = dram_addr.address & RSP_DRAM_ADDR_MASK;
    if (dram_address != dram_addr.address) {
        logwarn("Misaligned DRAM RSP DMA WRITE! 0x%08X", dram_addr.address);
    }
    u32 mem_address = mem_addr.address & RSP_MEM_ADDR_MASK;
    if (mem_address != mem_addr.address) {
        logwarn("Misaligned MEM RSP DMA WRITE! 0x%08X", mem_addr.address);
    }

    for (int i = 0; i < N64RSP.io.dma.count + 1; i++) {
        u8* mem = (mem_addr.imem ? N64RSP.sp_imem : N64RSP.sp_dmem);
        u8* rdram = n64sys.mem.rdram + dram_address;
        for (int j = 0; j < length; j++) {
            u16 addr = (mem_address + j) & 0xFFF;
            rdram[j] = mem[addr];
        }


        // Invalidate all pages touched by the DMA
        // This is probably unnecessary, since why would someone be copying code from the RSP to the CPU and then executing it?
        for (int j = 0; j < length; j += BLOCKCACHE_PAGE_SIZE) {
            invalidate_dynarec_page(dram_address + j);
        }

        int skip = i == N64RSP.io.dma.count ? 0 : N64RSP.io.dma.skip;

        dram_address += (length + skip);
        dram_address &= RSP_DRAM_ADDR_MASK;
        mem_address += length;
        mem_address &= RSP_MEM_ADDR_MASK;
    }

    N64RSP.io.dram_addr.address = dram_address;
    N64RSP.io.mem_addr.address = mem_address;
    N64RSP.io.mem_addr.imem = mem_addr.imem;

    // Hardware seems to always return this value in the length register
    // No real idea why
    N64RSP.io.dma.raw = 0xFF8 | (N64RSP.io.dma.skip << 20);
}

INLINE void set_rsp_register(u8 r, u32 value) {
    N64RSP.gpr[r] = value;
    N64RSP.gpr[0] = 0;
}

INLINE u32 get_rsp_register(u8 r) {
    return N64RSP.gpr[r];
}

bool rsp_acquire_semaphore();
void rsp_release_semaphore();

INLINE u32 get_rsp_cp0_register(u8 r) {
    switch (r) {
        case RSP_CP0_DMA_CACHE:
            return N64RSP.io.mem_addr.raw;
        case RSP_CP0_DMA_DRAM:
            return N64RSP.io.dram_addr.raw;
        case RSP_CP0_DMA_READ_LENGTH:
        case RSP_CP0_DMA_WRITE_LENGTH:
            return N64RSP.io.dma.raw;
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

INLINE void set_rsp_cp0_register(u8 r, u32 value) {
    switch (r) {
        case RSP_CP0_DMA_CACHE: N64RSP.io.shadow_mem_addr.raw = value; break;
        case RSP_CP0_DMA_DRAM:  N64RSP.io.shadow_dram_addr.raw = value; break;
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
            rdp_start_reg_write(value);
            break;
        case RSP_CP0_CMD_END:
            rdp_end_reg_write(value);
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

INLINE s64 get_rsp_accumulator(int e) {
    s64 val = (s64)N64RSP.acc.h.elements[e] << 32;
    val       |= (s64)N64RSP.acc.m.elements[e] << 16;
    val       |= (s64)N64RSP.acc.l.elements[e] << 0;
    if ((val & 0x0000800000000000) != 0) {
        val |= 0xFFFF000000000000;
    }
    return val;
}

INLINE void set_rsp_accumulator(int e, u64 val) {
    N64RSP.acc.h.elements[e] = (val >> 32) & 0xFFFF;
    N64RSP.acc.m.elements[e] = (val >> 16) & 0xFFFF;
    N64RSP.acc.l.elements[e] = val & 0xFFFF;
}

INLINE u16 rsp_get_vco() {
    u16 value = 0;
    for (int i = 0; i < 8; i++) {
        bool h = N64RSP.vco.h.elements[7 - i] != 0;
        bool l = N64RSP.vco.l.elements[7 - i] != 0;
        u32 mask = (l << i) | (h << (i + 8));
        value |= mask;
    }
    return value;
}

INLINE u16 rsp_get_vcc() {
    u16 value = 0;
    for (int i = 0; i < 8; i++) {
        bool h = N64RSP.vcc.h.elements[7 - i] != 0;
        bool l = N64RSP.vcc.l.elements[7 - i] != 0;
        u32 mask = (l << i) | (h << (i + 8));
        value |= mask;
    }
    return value;
}

INLINE u8 rsp_get_vce() {
    u8 value = 0;
    for (int i = 0; i < 8; i++) {
        bool l = N64RSP.vce.elements[7 - i] != 0;
        value |= (l << i);
    }
    return value;
}

INLINE void rsp_set_vcc(u16 vcc) {
    for (int i = 0; i < 8; i++) {
        N64RSP.vcc.l.elements[7 - i] = FLAGREG_BOOL(vcc & 1);
        vcc >>= 1;
    }

    for (int i = 0; i < 8; i++) {
        N64RSP.vcc.h.elements[7 - i] = FLAGREG_BOOL(vcc & 1);
        vcc >>= 1;
    }
}

INLINE void rsp_set_vco(u16 vco) {
    for (int i = 0; i < 8; i++) {
        N64RSP.vco.l.elements[7 - i] = FLAGREG_BOOL(vco & 1);
        vco >>= 1;
    }

    for (int i = 0; i < 8; i++) {
        N64RSP.vco.h.elements[7 - i] = FLAGREG_BOOL(vco & 1);
        vco >>= 1;
    }
}

INLINE void rsp_set_vce(u16 vce) {
    for (int i = 0; i < 8; i++) {
        N64RSP.vce.elements[7 - i] = FLAGREG_BOOL(vce & 1);
        vce >>= 1;
    }
}

void rsp_step();
void rsp_run();
#ifdef N64_DYNAREC_V1_ENABLED
void rsp_dynarec_run();
#endif
vu_reg_t ext_get_vte(vu_reg_t* vt, u8 e);

#endif //N64_RSP_H
