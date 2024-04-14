#include "mips_instructions.h"
#include "r4300i_register_access.h"

#include <cache.h>
#include <mem/n64bus.h>

MIPS_INSTR(mips_nop) {}

MIPS_INSTR(mips_addi) {
    u32 reg_addend = get_register(instruction.i.rs);
    u32 imm_addend = (s32)((s16)instruction.i.immediate);
    u32 result = imm_addend + reg_addend;
    if (check_signed_overflow_add(reg_addend, imm_addend, result)) {
        r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_ARITHMETIC_OVERFLOW, 0);
    } else {
        set_register(instruction.i.rt, (s64)((s32)(result)));
    }
}

MIPS_INSTR(mips_addiu) {
    u32 reg_addend = get_register(instruction.i.rs);
    s16 addend = instruction.i.immediate;
    s32 result = reg_addend + addend;

    set_register(instruction.i.rt, (s64)result);
}

MIPS_INSTR(mips_daddi) {
    u64 addend1 = (s64)((s16)instruction.i.immediate);
    u64 addend2 = get_register(instruction.i.rs);
    u64 result = addend1 + addend2;

    if (check_signed_overflow_add(addend1, addend2, result)) {
        r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_ARITHMETIC_OVERFLOW, 0);
    } else {
        set_register(instruction.i.rt, result);
    }
}


MIPS_INSTR(mips_andi) {
    u64 immediate = instruction.i.immediate;
    u64 result = immediate & get_register(instruction.i.rs);
    set_register(instruction.i.rt, result);
}

MIPS_INSTR(mips_beq) {
    conditional_branch(instruction.i.immediate, get_register(instruction.i.rs) == get_register(instruction.i.rt));
}

MIPS_INSTR(mips_beql) {
    conditional_branch_likely(instruction.i.immediate, get_register(instruction.i.rs) == get_register(instruction.i.rt));
}

MIPS_INSTR(mips_bgtz) {
    s64 reg = get_register(instruction.i.rs);
    conditional_branch(instruction.i.immediate,  reg > 0);
}

MIPS_INSTR(mips_bgtzl) {
    s64 reg = get_register(instruction.i.rs);
    conditional_branch_likely(instruction.i.immediate,  reg > 0);
}

MIPS_INSTR(mips_blez) {
    s64 reg = get_register(instruction.i.rs);
    conditional_branch(instruction.i.immediate, reg <= 0);
}

MIPS_INSTR(mips_blezl) {
    s64 reg = get_register(instruction.i.rs);
    conditional_branch_likely(instruction.i.immediate, reg <= 0);
}

MIPS_INSTR(mips_bne) {
    conditional_branch(instruction.i.immediate, get_register(instruction.i.rs) != get_register(instruction.i.rt));
}

MIPS_INSTR(mips_bnel) {
    u64 rs = get_register(instruction.i.rs);
    u64 rt = get_register(instruction.i.rt);
    logtrace("Branch if: 0x%08" PRIX64 " != 0x%08" PRIX64, rs, rt);
    conditional_branch_likely(instruction.i.immediate, rs != rt);
}

#define ICACHE_OP(op) (((op) << 2) | 0)
#define DCACHE_OP(op) (((op) << 2) | 1)
MIPS_INSTR(mips_cache) {
    // TODO: check cp0 in user or supervisor mode
    s16 offset = instruction.i.immediate;
    u64 virtual_address = get_register(instruction.i.rs) + offset;
    u8 cache_op = instruction.i.rt;

    u32 physical;
    bool cached;
    if (!resolve_virtual_address(virtual_address, BUS_LOAD, &cached, &physical)) {
        on_tlb_exception(virtual_address);
        r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_LOAD), 0);
    } else {
        u32 physical_tag = get_paddr_ptag(physical);
        switch (cache_op) { // page 404
            case ICACHE_OP(0): // Index_Invalidate
                N64CPU.icache[get_icache_line_index(virtual_address)].valid = false;
                break;
            case DCACHE_OP(0): { // Index_Write_Back_Invalidate
                int cache_line = get_dcache_line_index(virtual_address);
                if (N64CPU.dcache[cache_line].valid) {
                    writeback_dcache(virtual_address, physical);
                    N64CPU.dcache[cache_line].valid = false;
                }
                break;
            }
            case ICACHE_OP(1): // Index_Load_Tag
                logfatal("Index_Load_Tag icache unimplemented\n");
                break;
            case DCACHE_OP(1): // Index_Load_Tag
                logfatal("Index_Load_Tag dcache unimplemented\n");
                break;
            case ICACHE_OP(2): // Index_Store_Tag
                N64CPU.icache[get_icache_line_index(virtual_address)].ptag = N64CP0.tag_lo.ptaglo;
                break;
            case DCACHE_OP(2):
                N64CPU.dcache[get_dcache_line_index(virtual_address)].ptag = N64CP0.tag_lo.ptaglo;
                break;
            case DCACHE_OP(3): // Create_Dirty_Exclusive
                logfatal("Create_Dirty_Exclusive unimplemented\n");
                break;
            case ICACHE_OP(4): {
                int cache_line = get_icache_line_index(virtual_address);
                if (N64CPU.icache[cache_line].ptag == physical_tag) {
                    N64CPU.icache[cache_line].valid = false;
                }
                break;
            }
            case DCACHE_OP(4): {
                int cache_line = get_dcache_line_index(virtual_address);
                if (N64CPU.dcache[cache_line].valid && N64CPU.dcache[cache_line].ptag == physical_tag) {
                    N64CPU.dcache[cache_line].valid = false;
                }
                break;
            }
            case ICACHE_OP(5): // Fill
                logfatal("Fill icache unimplemented\n");
                break;
            case DCACHE_OP(5): { // Hit_Write_Back_Invalidate
                int cache_line = get_dcache_line_index(virtual_address);
                if (N64CPU.dcache[cache_line].valid && N64CPU.dcache[cache_line].ptag == physical_tag) {
                    writeback_dcache(virtual_address, physical);
                    N64CPU.dcache[cache_line].valid = false;
                }
                break;
            }
            case ICACHE_OP(6): // Hit_Write_Back icache
                logfatal("Hit_Write_Back icache unimplemented\n");
                break;
            case DCACHE_OP(6): { // Hit_Write_Back dcache
                int cache_line = get_dcache_line_index(virtual_address);
                if (N64CPU.dcache[cache_line].valid && N64CPU.dcache[cache_line].ptag == physical_tag) {
                    writeback_dcache(virtual_address, physical);
                }
                break;
            }
            default:
                logfatal("Invalid cache op\n");
                break;
        }
    }
    return;
}

MIPS_INSTR(mips_j) {
    u64 target = instruction.j.target;
    target <<= 2;
    target |= ((N64CPU.pc - 4) & 0xFFFFFFFFF0000000); // PC is 4 ahead

    branch_abs(target);
}

MIPS_INSTR(mips_jal) {
    link_r4300i(MIPS_REG_RA);

    u64 target = instruction.j.target;
    target <<= 2;
    target |= ((N64CPU.pc - 4) & 0xFFFFFFFFF0000000); // PC is 4 ahead

    branch_abs(target);
}

MIPS_INSTR(mips_slti) {
    s16 immediate = instruction.i.immediate;
    logtrace("Set if %" PRId64 " < %d", get_register(instruction.i.rs), immediate);
    s64 reg = get_register(instruction.i.rs);
    if (reg < immediate) {
        set_register(instruction.i.rt, 1);
    } else {
        set_register(instruction.i.rt, 0);
    }
}

MIPS_INSTR(mips_sltiu) {
    s16 immediate = instruction.i.immediate;
    logtrace("Set if %" PRId64 " < %d", get_register(instruction.i.rs), immediate);
    if (get_register(instruction.i.rs) < immediate) {
        set_register(instruction.i.rt, 1);
    } else {
        set_register(instruction.i.rt, 0);
    }
}

MIPS_INSTR(mips_mfc0) {
    s32 value = get_cp0_register_word(instruction.r.rd);
    set_register(instruction.r.rt, (s64)value);
}

MIPS_INSTR(mips_mtc0) {
    u32 value = get_register(instruction.r.rt);
    set_cp0_register_word(instruction.r.rd, value);
}

MIPS_INSTR(mips_dmfc0) {
    u64 value = get_cp0_register_dword(instruction.r.rd);
    set_register(instruction.r.rt, value);
}

MIPS_INSTR(mips_dmtc0) {
    u64 value = get_register(instruction.r.rt);
    set_cp0_register_dword(instruction.r.rd, value);
}

MIPS_INSTR(mips_ld) {
    s16 offset = instruction.i.immediate;
    u64 address = get_register(instruction.i.rs) + offset;
    if (check_address_error(0b111, address)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_ADDRESS_ERROR_LOAD, 0);
        return;
    }

    u32 physical;
    bool cached;
    if (!resolve_virtual_address(address, BUS_LOAD, &cached, &physical)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_LOAD), 0);
    } else {
        u64 value = conditional_cache_read_dword(cached, address, physical);
        set_register(instruction.i.rt, value);
    }
}

MIPS_INSTR(mips_lui) {
    // Done this way to avoid the undefined behavior of left shifting a signed integer
    // Should compile to a left shift by 16.
    s64 value = (s16)instruction.i.immediate;
    value *= 65536;

    set_register(instruction.i.rt, value);
}

MIPS_INSTR(mips_lbu) {
    s16 offset = instruction.i.immediate;
    logtrace("LBU offset: %d", offset);
    u64 address = get_register(instruction.i.rs) + offset;
    u32 physical;
    bool cached;
    if (!resolve_virtual_address(address, BUS_LOAD, &cached, &physical)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_LOAD), 0);
    } else {
        u8 value = conditional_cache_read_byte(cached, address, physical);
        set_register(instruction.i.rt, value); // zero extend
    }
}

MIPS_INSTR(mips_lhu) {
    s16 offset = instruction.i.immediate;
    logtrace("LHU offset: %d", offset);
    u64 address = get_register(instruction.i.rs) + offset;
    if ((address & 0b1) > 0) {
        logfatal("TODO: throw an 'address error' exception! Tried to load from unaligned address 0x%016" PRIX64, address);
    }

    u32 physical;
    bool cached;
    if (!resolve_virtual_address(address, BUS_LOAD, &cached, &physical)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_LOAD), 0);
    } else {
        u16 value = conditional_cache_read_half(cached, address, physical);
        set_register(instruction.i.rt, value); // zero extend
    }
}

MIPS_INSTR(mips_lh) {
    s16 offset = instruction.i.immediate;
    u64 address = get_register(instruction.i.rs) + offset;
    if ((address & 0b1) > 0) {
        logfatal("TODO: throw an 'address error' exception! Tried to load from unaligned address 0x%016" PRIX64, address);
    }

    u32 physical;
    bool cached;
    if (!resolve_virtual_address(address, BUS_LOAD, &cached, &physical)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_LOAD), 0);
    } else {
        s16 value = conditional_cache_read_half(cached, address, physical);
        set_register(instruction.i.rt, (s64)value); // zero extend
    }
}

MIPS_INSTR(mips_lw) {
    s16 offset  = instruction.i.immediate;
    u64 address = get_register(instruction.i.rs) + offset;
    if (check_address_error(0b11, address)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_ADDRESS_ERROR_LOAD, 0);
        return;
    }

    u32 physical;
    bool cached;
    if (!resolve_virtual_address(address, BUS_LOAD, &cached, &physical)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_LOAD), 0);
    } else {
        s32 value = conditional_cache_read_word(cached, address, physical);
        set_register(instruction.i.rt, (s64)value);
    }
}

MIPS_INSTR(mips_lwu) {
    s16 offset  = instruction.i.immediate;
    u64 address = get_register(instruction.i.rs) + offset;
    if ((address & 0b11) > 0) {
        logfatal("TODO: throw an 'address error' exception! Tried to load from unaligned address 0x%016" PRIX64, address);
    }

    u32 physical;
    bool cached;
    if (!resolve_virtual_address(address, BUS_LOAD, &cached, &physical)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_LOAD), 0);
    } else {
        u32 value = conditional_cache_read_word(cached, address, physical);
        set_register(instruction.i.rt, value);
    }
}

MIPS_INSTR(mips_sb) {
    s16 offset  = instruction.i.immediate;
    u64 address = get_register(instruction.i.rs);
    address += offset;
    u32 value = get_register(instruction.i.rt); // A larger value is needed in some cases due to bus weirdness

    u32 physical;
    bool cached;
    if (!resolve_virtual_address(address, BUS_STORE, &cached, &physical)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_STORE), 0);
    } else {
        conditional_cache_write_byte(cached, address, physical, value);
    }
}

MIPS_INSTR(mips_sh) {
    s16 offset  = instruction.i.immediate;
    u64 address = get_register(instruction.i.rs);
    address += offset;
    u32 value = get_register(instruction.i.rt); // A larger value is needed in some cases due to bus weirdness
    u32 physical;
    bool cached;
    if (!resolve_virtual_address(address, BUS_STORE, &cached, &physical)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_STORE), 0);
    } else {
        conditional_cache_write_half(cached, address, physical, value);
    }
}

MIPS_INSTR(mips_sw) {
    s16 offset  = instruction.i.immediate;
    u64 address = get_register(instruction.i.rs);
    address += offset;
    u32 physical;

    if (check_address_error(0b11, address)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_ADDRESS_ERROR_STORE, 0);
        return;
    }

    bool cached;
    if (!resolve_virtual_address(address, BUS_STORE, &cached, &physical)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_STORE), 0);
    } else {
        conditional_cache_write_word(cached, address, physical, get_register(instruction.i.rt));
    }
}

MIPS_INSTR(mips_sd) {
    s16 offset  = instruction.i.immediate;
    u64 address = get_register(instruction.i.rs) + offset;
    u64 value = get_register(instruction.i.rt);

    u32 physical;
    if (check_address_error(0b111, address)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_ADDRESS_ERROR_STORE, 0);
        return;
    }

    bool cached;
    if (!resolve_virtual_address(address, BUS_STORE, &cached, &physical)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_STORE), 0);
    } else {
        conditional_cache_write_dword(cached, address, physical, value);
    }
}

MIPS_INSTR(mips_ori) {
    set_register(instruction.i.rt, instruction.i.immediate | get_register(instruction.i.rs));
}

MIPS_INSTR(mips_xori) {
    set_register(instruction.i.rt, instruction.i.immediate ^ get_register(instruction.i.rs));
}

MIPS_INSTR(mips_daddiu) {
    u64 addend1 = (s64)((s16)instruction.i.immediate);
    u64 addend2 = get_register(instruction.i.rs);
    u64 result = addend1 + addend2;
    set_register(instruction.i.rt, result);
}

MIPS_INSTR(mips_lb) {
    s16 offset  = instruction.i.immediate;
    u64 address = get_register(instruction.i.rs) + offset;

    u32 physical;
    bool cached;
    if (!resolve_virtual_address(address, BUS_LOAD, &cached, &physical)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_LOAD), 0);
    } else {
        s8 value = conditional_cache_read_byte(cached, address, physical);
        set_register(instruction.i.rt, (s64)value);
    }
}

MIPS_INSTR(mips_lwl) {
    s16 offset  = instruction.fi.offset;
    u64 address = get_register(instruction.fi.base) + offset;


    u32 physical;
    bool cached;
    if (!resolve_virtual_address(address, BUS_LOAD, &cached, &physical)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_LOAD), 0);
    } else {
        u32 shift = 8 * ((address ^ 0) & 3);
        u32 mask = 0xFFFFFFFF << shift;
        u32 data = conditional_cache_read_word(cached, address & ~3, physical & ~3);
        s32 result = (get_register(instruction.i.rt) & ~mask) | data << shift;
        set_register(instruction.i.rt, (s64)result);
    }
}

MIPS_INSTR(mips_lwr) {
    s16 offset  = instruction.fi.offset;
    u64 address = get_register(instruction.fi.base) + offset;
    u32 physical;
    bool cached;
    if (!resolve_virtual_address(address, BUS_LOAD, &cached, &physical)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_LOAD), 0);
    } else {
        u32 shift = 8 * ((address ^ 3) & 3);

        u32 mask = 0xFFFFFFFF >> shift;
        u32 data = conditional_cache_read_word(cached, address & ~3, physical & ~3);
        s32 result = (get_register(instruction.i.rt) & ~mask) | data >> shift;
        set_register(instruction.i.rt, (s64)result);
    }
}

MIPS_INSTR(mips_swl) {
    s16 offset  = instruction.fi.offset;
    u64 address = get_register(instruction.fi.base) + offset;

    u32 physical;
    bool cached;
    if (!resolve_virtual_address(address, BUS_STORE, &cached, &physical)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_STORE), 0);
    } else {
        u32 shift = 8 * ((address ^ 0) & 3);
        u32 mask = 0xFFFFFFFF >> shift;
        u32 data = conditional_cache_read_word(cached, address & ~3, physical & ~3);
        u32 oldreg = get_register(instruction.i.rt);
        conditional_cache_write_word(cached, address & ~3, physical & ~3, (data & ~mask) | (oldreg >> shift));
    }
}

MIPS_INSTR(mips_swr) {
    s16 offset  = instruction.fi.offset;
    u64 address = get_register(instruction.fi.base) + offset;
    u32 physical;
    bool cached;
    if (!resolve_virtual_address(address, BUS_STORE, &cached, &physical)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_STORE), 0);
    } else {
        u32 shift = 8 * ((address ^ 3) & 3);
        u32 mask = 0xFFFFFFFF << shift;
        u32 data = conditional_cache_read_word(cached, address & ~3, physical & ~3);
        u32 oldreg = get_register(instruction.i.rt);
        conditional_cache_write_word(cached, address & ~3, physical & ~3, (data & ~mask) | oldreg << shift);
    }
}

MIPS_INSTR(mips_ldl) {
    s16 offset = instruction.fi.offset;
    u64 address = get_register(instruction.fi.base) + offset;
    u32 physical;
    bool cached;
    if (!resolve_virtual_address(address, BUS_LOAD, &cached, &physical)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_LOAD), 0);
    } else {
        int shift = 8 * ((address ^ 0) & 7);
        u64 mask = (u64) 0xFFFFFFFFFFFFFFFF << shift;
        u64 data = conditional_cache_read_dword(cached, address & ~7, physical & ~7);
        u64 oldreg = get_register(instruction.i.rt);

        set_register(instruction.i.rt, (oldreg & ~mask) | (data << shift));
    }
}

MIPS_INSTR(mips_ldr) {
    s16 offset = instruction.fi.offset;
    u64 address = get_register(instruction.fi.base) + offset;
    u32 physical;
    bool cached;
    if (!resolve_virtual_address(address, BUS_LOAD, &cached, &physical)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_LOAD), 0);
    } else {
        int shift = 8 * ((address ^ 7) & 7);
        u64 mask = (u64) 0xFFFFFFFFFFFFFFFF >> shift;
        u64 data = conditional_cache_read_dword(cached, address & ~7, physical & ~7);
        u64 oldreg = get_register(instruction.i.rt);

        set_register(instruction.i.rt, (oldreg & ~mask) | (data >> shift));
    }
}

MIPS_INSTR(mips_sdl) {
    s16 offset = instruction.fi.offset;
    u64 address = get_register(instruction.fi.base) + offset;

    u32 physical;
    bool cached;
    if (!resolve_virtual_address(address, BUS_STORE, &cached, &physical)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_STORE), 0);
    } else {
        int shift = 8 * ((address ^ 0) & 7);
        u64 mask = 0xFFFFFFFFFFFFFFFF;
        mask >>= shift;
        u64 data = conditional_cache_read_dword(cached, address & ~7, physical & ~7);
        u64 oldreg = get_register(instruction.i.rt);
        conditional_cache_write_dword(cached, address & ~7, physical & ~7, (data & ~mask) | (oldreg >> shift));
    }
}

MIPS_INSTR(mips_sdr) {
    s16 offset = instruction.fi.offset;
    u64 address = get_register(instruction.fi.base) + offset;
    u32 physical;
    bool cached;
    if (!resolve_virtual_address(address, BUS_STORE, &cached, &physical)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_STORE), 0);
    } else {
        int shift = 8 * ((address ^ 7) & 7);
        u64 mask = 0xFFFFFFFFFFFFFFFF;
        mask <<= shift;
        u64 data = conditional_cache_read_dword(cached, address & ~7, physical & ~7);
        u64 oldreg = get_register(instruction.i.rt);
        conditional_cache_write_dword(cached, address & ~7, physical & ~7, (data & ~mask) | (oldreg << shift));
    }
}

MIPS_INSTR(mips_ll) {
    // Identical to lw
    s16 offset = instruction.i.immediate;
    u64 address = get_register(instruction.i.rs) + offset;

    u32 physical;
    bool cached;
    if (!resolve_virtual_address(address, BUS_LOAD, &cached, &physical)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_LOAD), 0);
    } else {
        s32 result = conditional_cache_read_word(cached, address, physical);
        if ((address & 0b11) > 0) {
            logfatal("TODO: throw an 'address error' exception! Tried to load from unaligned address 0x%016" PRIX64, address);
        }

        set_register(instruction.i.rt, (s64)result);

        // Unique to ll
        N64CPU.cp0.lladdr = physical >> 4;
        N64CPU.llbit = true;
    }
}

MIPS_INSTR(mips_lld) {
    // Instruction is undefined outside of 64 bit mode and 32 bit kernel mode.
    // Throw an exception if we're not in 64 bit mode AND not in kernel mode.
    if (!N64CPU.cp0.is_64bit_addressing && !N64CPU.cp0.kernel_mode) {
        logfatal("LLD is undefined outside of 64 bit mode and 32 bit kernel mode. Throw a reserved instruction exception!");
    }

    // Identical to ld
    s16 offset = instruction.i.immediate;
    u64 address = get_register(instruction.i.rs) + offset;

    u32 physical;
    bool cached;
    if (!resolve_virtual_address(address, BUS_LOAD, &cached, &physical)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_LOAD), 0);
    } else {
        u64 result = conditional_cache_read_dword(cached, address, physical);
        if ((address & 0b111) > 0) {
            logfatal("TODO: throw an 'address error' exception! Tried to load from unaligned address 0x%016" PRIX64, address);
        }
        set_register(instruction.i.rt, result);

        // Unique to lld
        N64CPU.cp0.lladdr = physical >> 4;
        N64CPU.llbit = true;
    }
}

MIPS_INSTR(mips_sc) {
    // Identical to sw
    s16 offset          = instruction.i.immediate;
    u64 address         = get_register(instruction.i.rs) + offset;

    // Exception takes precedence over the instruction failing
    if ((address & 0b11) > 0) {
        logfatal("TODO: throw an 'address error' exception! Tried to store to unaligned address 0x%016" PRIX64, address);
    }

    if (N64CPU.llbit) {
        N64CPU.llbit = false;
        u32 physical_address;
        bool cached;
        if (!resolve_virtual_address(address, BUS_STORE, &cached, &physical_address)) {
            on_tlb_exception(address);
            r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_STORE), 0);
        } else {
            u32 value = get_register(instruction.i.rt);
            conditional_cache_write_word(cached, address, physical_address, value);
            set_register(instruction.i.rt, 1); // Success!
        }
    } else {
        set_register(instruction.i.rt, 0); // Failure.
    }
}

MIPS_INSTR(mips_scd) {
    // Instruction is undefined outside of 64 bit mode and 32 bit kernel mode.
    // Throw an exception if we're not in 64 bit mode AND not in kernel mode.
    if (!N64CPU.cp0.is_64bit_addressing && !N64CPU.cp0.kernel_mode) {
        logfatal("SCD is undefined outside of 64 bit mode and 32 bit kernel mode. Throw a reserved instruction exception!");
    }

    // Identical to sd
    s16 offset          = instruction.i.immediate;
    u64 address         = get_register(instruction.i.rs) + offset;

    // Exception takes precedence over the instruction failing
    if ((address & 0b111) > 0) {
        logfatal("TODO: throw an 'address error' exception! Tried to store to unaligned address 0x%016" PRIX64, address);
    }

    if (N64CPU.llbit) {
        N64CPU.llbit = false;
        bool cached;
        u32 physical_address = resolve_virtual_address_or_die(address, BUS_STORE, &cached);

        u64 value = get_register(instruction.i.rt);
        conditional_cache_write_dword(cached, address, physical_address, value);
        set_register(instruction.i.rt, 1); // Success!

    } else {
        set_register(instruction.i.rt, 0); // Failure.
    }
}

MIPS_INSTR(mips_spc_sll) {
    s32 result = get_register(instruction.r.rt) << instruction.r.sa;
    set_register(instruction.r.rd, (s64)result);
}

MIPS_INSTR(mips_spc_srl) {
    u32 value = get_register(instruction.r.rt);
    s32 result = value >> instruction.r.sa;
    set_register(instruction.r.rd, (s64) result);
}

MIPS_INSTR(mips_spc_sra) {
    s64 value = get_register(instruction.r.rt);
    s32 result = (s64)(value >> (u64)instruction.r.sa);
    set_register(instruction.r.rd, (s64) result);
}

MIPS_INSTR(mips_spc_srav) {
    s64 value = get_register(instruction.r.rt);
    s32 result = (s64)(value >> (get_register(instruction.r.rs) & 0b11111));
    set_register(instruction.r.rd, (s64) result);
}

MIPS_INSTR(mips_spc_sllv) {
    u32 value = get_register(instruction.r.rt);
    s32 result = value << (get_register(instruction.r.rs) & 0b11111);
    set_register(instruction.r.rd, (s64)result);
}

MIPS_INSTR(mips_spc_srlv) {
    u32 value = get_register(instruction.r.rt);
    s32 result = value >> (get_register(instruction.r.rs) & 0b11111);
    set_register(instruction.r.rd, (s64)result);
}

MIPS_INSTR(mips_spc_jr) {
    branch_abs(get_register(instruction.r.rs));
}

MIPS_INSTR(mips_spc_jalr) {
    branch_abs(get_register(instruction.r.rs));
    link_r4300i(instruction.r.rd);
}

MIPS_INSTR(mips_spc_syscall) {
    r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_SYSCALL, 0);
}

MIPS_INSTR(mips_spc_mfhi) {
    set_register(instruction.r.rd, N64CPU.mult_hi);
}

MIPS_INSTR(mips_spc_mthi) {
    N64CPU.mult_hi = get_register(instruction.r.rs);
}

MIPS_INSTR(mips_spc_mflo) {
    set_register(instruction.r.rd, N64CPU.mult_lo);
}

MIPS_INSTR(mips_spc_mtlo) {
    N64CPU.mult_lo = get_register(instruction.r.rs);
}

MIPS_INSTR(mips_spc_dsllv) {
    u64 val = get_register(instruction.r.rt);
    val <<= (get_register(instruction.r.rs) & 0b111111);
    set_register(instruction.r.rd, val);
}

MIPS_INSTR(mips_spc_dsrlv) {
    u64 val = get_register(instruction.r.rt);
    val >>= (get_register(instruction.r.rs) & 0b111111);
    set_register(instruction.r.rd, val);
}

MIPS_INSTR(mips_spc_dsrav) {
    s64 value = get_register(instruction.r.rt);
    s64 result = value >> (get_register(instruction.r.rs) & 0b111111);
    set_register(instruction.r.rd, result);
}

MIPS_INSTR(mips_spc_mult) {
    s32 multiplicand_1 = get_register(instruction.r.rs);
    s32 multiplicand_2 = get_register(instruction.r.rt);

    s64 dmultiplicand_1 = multiplicand_1;
    s64 dmultiplicand_2 = multiplicand_2;

    s64 result = dmultiplicand_1 * dmultiplicand_2;

    s32 result_lower = result & 0xFFFFFFFF;
    s32 result_upper = (result >> 32) & 0xFFFFFFFF;

    N64CPU.mult_lo = (s64)result_lower;
    N64CPU.mult_hi = (s64)result_upper;
}

MIPS_INSTR(mips_spc_multu) {
    u64 multiplicand_1 = get_register(instruction.r.rs) & 0xFFFFFFFF;
    u64 multiplicand_2 = get_register(instruction.r.rt) & 0xFFFFFFFF;

    u64 result = multiplicand_1 * multiplicand_2;

    s32 result_lower = result & 0xFFFFFFFF;
    s32 result_upper = (result >> 32) & 0xFFFFFFFF;

    N64CPU.mult_lo = (s64)result_lower;
    N64CPU.mult_hi = (s64)result_upper;
}

MIPS_INSTR(mips_spc_div) {
    s64 dividend = (s32)get_register(instruction.r.rs);
    s64 divisor  = (s32)get_register(instruction.r.rt);

    if (divisor == 0) {
        logwarn("Divide by zero");
        N64CPU.mult_hi = dividend;
        if (dividend >= 0) {
            N64CPU.mult_lo = (s64)-1;
        } else {
            N64CPU.mult_lo = (s64)1;
        }
    } else {
        s32 quotient  = dividend / divisor;
        s32 remainder = dividend % divisor;

        N64CPU.mult_lo = quotient;
        N64CPU.mult_hi = remainder;
    }

}

MIPS_INSTR(mips_spc_divu) {
    u32 dividend = get_register(instruction.r.rs);
    u32 divisor  = get_register(instruction.r.rt);

    if (divisor == 0) {
        N64CPU.mult_lo = 0xFFFFFFFFFFFFFFFF;
        N64CPU.mult_hi = (s32)dividend;
    } else {
        s32 quotient  = dividend / divisor;
        s32 remainder = dividend % divisor;

        N64CPU.mult_lo = quotient;
        N64CPU.mult_hi = remainder;
    }
}

MIPS_INSTR(mips_spc_dmult) {
    u64 result_upper;
    u64 result_lower = mult_64_to_128(get_register(instruction.r.rs), get_register(instruction.r.rt), &result_upper);

    N64CPU.mult_lo = result_lower;
    N64CPU.mult_hi = result_upper;
}

MIPS_INSTR(mips_spc_dmultu) {
    u64 result_upper;
    u64 result_lower = multu_64_to_128(get_register(instruction.r.rs), get_register(instruction.r.rt), &result_upper);

    N64CPU.mult_lo = result_lower;
    N64CPU.mult_hi = result_upper;
}

MIPS_INSTR(mips_spc_ddiv) {
    s64 dividend = (s64)get_register(instruction.r.rs);
    s64 divisor  = (s64)get_register(instruction.r.rt);

    if (unlikely(divisor == 0)) {
        logwarn("Divide by zero");
        N64CPU.mult_hi = dividend;
        if (dividend >= 0) {
            N64CPU.mult_lo = (s64)-1;
        } else {
            N64CPU.mult_lo = (s64)1;
        }
    } else if (unlikely(divisor == -1 && dividend == INT64_MIN)) {
        N64CPU.mult_lo = dividend;
        N64CPU.mult_hi = 0;
    } else {
        s64 quotient  = (s64)(dividend / divisor);
        s64 remainder = (s64)(dividend % divisor);

        N64CPU.mult_lo = quotient;
        N64CPU.mult_hi = remainder;
    }
}

MIPS_INSTR(mips_spc_ddivu) {
    u64 dividend = get_register(instruction.r.rs);
    u64 divisor  = get_register(instruction.r.rt);

    if (divisor == 0) {
        N64CPU.mult_lo = 0xFFFFFFFFFFFFFFFF;
        N64CPU.mult_hi = dividend;
    } else {
        u64 quotient  = dividend / divisor;
        u64 remainder = dividend % divisor;

        N64CPU.mult_lo = quotient;
        N64CPU.mult_hi = remainder;
    }
}

MIPS_INSTR(mips_spc_add) {
    u32 addend1 = get_register(instruction.r.rs);
    u32 addend2 = get_register(instruction.r.rt);

    u32 result = addend1 + addend2;
    if (check_signed_overflow_add(addend1, addend2, result)) {
        r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_ARITHMETIC_OVERFLOW, 0);
    } else {
        set_register(instruction.r.rd, (s64)((s32)result));
    }
}

MIPS_INSTR(mips_spc_addu) {
    u32 rs = get_register(instruction.r.rs);
    u32 rt = get_register(instruction.r.rt);
    s32 result = rs + rt;
    set_register(instruction.r.rd, (s64)result);
}

MIPS_INSTR(mips_spc_and) {
    u64 result = get_register(instruction.r.rs) & get_register(instruction.r.rt);
    set_register(instruction.r.rd, result);
}

MIPS_INSTR(mips_spc_nor) {
    u64 result = ~(get_register(instruction.r.rs) | get_register(instruction.r.rt));
    set_register(instruction.r.rd, result);
}

MIPS_INSTR(mips_spc_sub) {
    s32 operand1 = get_register(instruction.r.rs);
    s32 operand2 = get_register(instruction.r.rt);

    s32 result = operand1 - operand2;

    if (check_signed_overflow_sub(operand1, operand2, result)) {
        r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_ARITHMETIC_OVERFLOW, 0);
    } else {
        set_register(instruction.r.rd, (s64)result);
    }

}

MIPS_INSTR(mips_spc_subu) {
    u32 operand1 = get_register(instruction.r.rs);
    u32 operand2 = get_register(instruction.r.rt);

    s32 result = operand1 - operand2;
    set_register(instruction.r.rd, (s64)result);
}

MIPS_INSTR(mips_spc_or) {
    set_register(instruction.r.rd, get_register(instruction.r.rs) | get_register(instruction.r.rt));
}

MIPS_INSTR(mips_spc_xor) {
    set_register(instruction.r.rd, get_register(instruction.r.rs) ^ get_register(instruction.r.rt));
}

MIPS_INSTR(mips_spc_slt) {
    s64 op1 = get_register(instruction.r.rs);
    s64 op2 = get_register(instruction.r.rt);
    set_register(instruction.r.rd, op1 < op2 ? 1 : 0);
}

MIPS_INSTR(mips_spc_sltu) {
    u64 op1 = get_register(instruction.r.rs);
    u64 op2 = get_register(instruction.r.rt);

    logtrace("Set if %" PRIu64 " < %" PRIu64, op1, op2);
    if (op1 < op2) {
        set_register(instruction.r.rd, 1);
    } else {
        set_register(instruction.r.rd, 0);
    }
}

MIPS_INSTR(mips_spc_dadd) {
    u64 addend1 = get_register(instruction.r.rs);
    u64 addend2 = get_register(instruction.r.rt);
    u64 result = addend1 + addend2;

    if (check_signed_overflow_add(addend1, addend2, result)) {
        r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_ARITHMETIC_OVERFLOW, 0);
    } else {
        set_register(instruction.r.rd, result);
    }
}

MIPS_INSTR(mips_spc_daddu) {
    u64 addend1 = get_register(instruction.r.rs);
    u64 addend2 = get_register(instruction.r.rt);
    u64 result = addend1 + addend2;
    set_register(instruction.r.rd, result);
}

MIPS_INSTR(mips_spc_dsub) {
    s64 minuend = get_register(instruction.r.rs);
    s64 subtrahend = get_register(instruction.r.rt);
    s64 difference = minuend - subtrahend;

    if (check_signed_overflow_sub(minuend, subtrahend, difference)) {
        r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_ARITHMETIC_OVERFLOW, 0);
    } else {
        set_register(instruction.r.rd, difference);
    }
}

MIPS_INSTR(mips_spc_dsubu) {
    u64 minuend = get_register(instruction.r.rs);
    u64 subtrahend = get_register(instruction.r.rt);
    u64 difference = minuend - subtrahend;
    set_register(instruction.r.rd, difference);
}

MIPS_INSTR(mips_spc_teq) {
    u64 rs = get_register(instruction.r.rs);
    u64 rt = get_register(instruction.r.rt);

    if (rs == rt) {
        r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_TRAP, 0);
    }
}

MIPS_INSTR(mips_spc_break) {
    r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_BREAKPOINT, 0);
}

MIPS_INSTR(mips_spc_tne) {
    u64 rs = get_register(instruction.r.rs);
    u64 rt = get_register(instruction.r.rt);

    if (rs != rt) {
        r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_TRAP, 0);
    }
}

MIPS_INSTR(mips_spc_tge) {
    s64 rs = get_register(instruction.r.rs);
    s64 rt = get_register(instruction.r.rt);

    if (rs >= rt) {
        r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_TRAP, 0);
    }
}

MIPS_INSTR(mips_spc_tgeu) {
    u64 rs = get_register(instruction.r.rs);
    u64 rt = get_register(instruction.r.rt);

    if (rs >= rt) {
        r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_TRAP, 0);
    }
}

MIPS_INSTR(mips_spc_tlt) {
    s64 rs = get_register(instruction.r.rs);
    s64 rt = get_register(instruction.r.rt);

    if (rs < rt) {
        r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_TRAP, 0);
    }
}

MIPS_INSTR(mips_spc_tltu) {
    u64 rs = get_register(instruction.r.rs);
    u64 rt = get_register(instruction.r.rt);

    if (rs < rt) {
        r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_TRAP, 0);
    }
}


MIPS_INSTR(mips_spc_dsll) {
    u64 value = get_register(instruction.r.rt);
    value <<= instruction.r.sa;
    set_register(instruction.r.rd, value);
}

MIPS_INSTR(mips_spc_dsrl) {
    u64 value = get_register(instruction.r.rt);
    value >>= instruction.r.sa;
    set_register(instruction.r.rd, value);
}

MIPS_INSTR(mips_spc_dsra) {
    s64 value = get_register(instruction.r.rt);
    value >>= instruction.r.sa;
    set_register(instruction.r.rd, value);
}

MIPS_INSTR(mips_spc_dsll32) {
    u64 value = get_register(instruction.r.rt);
    value <<= (instruction.r.sa + 32);
    set_register(instruction.r.rd, value);
}

MIPS_INSTR(mips_spc_dsrl32) {
    u64 value = get_register(instruction.r.rt);
    value >>= (instruction.r.sa + 32);
    set_register(instruction.r.rd, value);
}

MIPS_INSTR(mips_spc_dsra32) {
    s64 value = get_register(instruction.r.rt);
    value >>= (instruction.r.sa + 32);
    set_register(instruction.r.rd, value);
}

MIPS_INSTR(mips_ri_bltz) {
    s64 reg = get_register(instruction.i.rs);
    conditional_branch(instruction.i.immediate, reg < 0);
}

MIPS_INSTR(mips_ri_bltzl) {
    s64 reg = get_register(instruction.i.rs);
    conditional_branch_likely(instruction.i.immediate, reg < 0);
}

MIPS_INSTR(mips_ri_bgez) {
    s64 reg = get_register(instruction.i.rs);
    conditional_branch(instruction.i.immediate, reg >= 0);
}

MIPS_INSTR(mips_ri_bgezl) {
    s64 reg = get_register(instruction.i.rs);
    conditional_branch_likely(instruction.i.immediate, reg >= 0);
}

MIPS_INSTR(mips_ri_bltzal) {
    s64 reg = get_register(instruction.i.rs);
    conditional_branch(instruction.i.immediate, reg < 0);
    link_r4300i(MIPS_REG_RA);
}

MIPS_INSTR(mips_ri_bgezal) {
    s64 reg = get_register(instruction.i.rs);
    conditional_branch(instruction.i.immediate, reg >= 0);
    link_r4300i(MIPS_REG_RA);
}

MIPS_INSTR(mips_ri_bgezall) {
    s64 reg = get_register(instruction.i.rs);
    link_r4300i(MIPS_REG_RA);
    conditional_branch_likely(instruction.i.immediate, reg >= 0);
}

MIPS_INSTR(mips_eret) {
    if (N64CPU.cp0.status.erl) {
        set_pc_dword_r4300i(N64CPU.cp0.error_epc);
        N64CPU.cp0.status.erl = false;
    } else {
        set_pc_dword_r4300i(N64CPU.cp0.EPC);
        N64CPU.cp0.status.exl = false;
    }
    cp0_status_updated();
    N64CPU.llbit = false;
}

MIPS_INSTR(mips_ri_tgei) {
    s64 rs = get_register(instruction.i.rs);
    s16 imm = instruction.i.immediate;

    if (rs >= imm) {
        r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_TRAP, 0);
    }
}

MIPS_INSTR(mips_ri_tgeiu) {
    u64 rs = get_register(instruction.i.rs);
    u64 imm = (s64)(((s16)instruction.i.immediate));

    if (rs >= imm) {
        r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_TRAP, 0);
    }
}

MIPS_INSTR(mips_ri_tlti) {
    s64 rs = get_register(instruction.i.rs);
    s16 imm = instruction.i.immediate;

    if (rs < imm) {
        r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_TRAP, 0);
    }
}

MIPS_INSTR(mips_ri_tltiu) {
    u64 rs = get_register(instruction.i.rs);
    u64 imm = (s64)(((s16)instruction.i.immediate));

    if (rs < imm) {
        r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_TRAP, 0);
    }
}

MIPS_INSTR(mips_ri_teqi) {
    s64 rs = get_register(instruction.i.rs);
    s16 imm = instruction.i.immediate;

    if (rs == imm) {
        r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_TRAP, 0);
    }
}

MIPS_INSTR(mips_ri_tnei) {
    s64 rs = get_register(instruction.i.rs);
    s16 imm = instruction.i.immediate;

    if (rs != imm) {
        r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_TRAP, 0);
    }
}

MIPS_INSTR(mips_mfc2) {
    checkcp2;
    s32 value = N64CPU.cp2_latch;
    set_register(instruction.r.rt, (s64)value);
}

MIPS_INSTR(mips_mtc2) {
    checkcp2;
    // Identical to DMTC2
    N64CPU.cp2_latch = get_register(instruction.r.rt);;
}

MIPS_INSTR(mips_dmfc2) {
    checkcp2;
    set_register(instruction.r.rt, N64CPU.cp2_latch);
}

MIPS_INSTR(mips_dmtc2) {
    checkcp2;
    N64CPU.cp2_latch = get_register(instruction.r.rt);
}

MIPS_INSTR(mips_cfc2) {
    checkcp2;
    logwarn("Main CPU CFC2 unimplemented! Doing nothing.");
}

MIPS_INSTR(mips_ctc2) {
    checkcp2;
    logwarn("Main CPU CTC2 unimplemented! Doing nothing.");
}

MIPS_INSTR(mips_cp2_invalid) {
    checkcp2;
    r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_RESERVED_INSTR, 2);
}

MIPS_INSTR(mips_invalid) {
    r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_RESERVED_INSTR, 0);
}
