#include "mips_instructions.h"
#include "r4300i_register_access.h"

#include <mem/n64bus.h>

#define check_signed_overflow_add(op1, op2, res)  (((~((op1) ^ (op2)) & ((op1) ^ (res))) >> ((sizeof(res) * 8) - 1)) & 1)
#define check_signed_overflow_sub(op1, op2, res) (((((op1) ^ (op2)) & ((op1) ^ (res))) >> ((sizeof(res) * 8) - 1)) & 1)
#define check_address_error(mask, virtual) (((!N64CP0.is_64bit_addressing) && (sword)(virtual) != (virtual)) || (((virtual) & (mask)) != 0))

// https://stackoverflow.com/questions/25095741/how-can-i-multiply-64-bit-operands-and-get-128-bit-result-portably/58381061#58381061
/* Prevents a partial vectorization from GCC. */
#if defined(__GNUC__) && !defined(__clang__) && defined(__i386__)
__attribute__((__target__("no-sse")))
#endif
INLINE dword multu_64_to_128(dword lhs, dword rhs, dword *high) {
        /*
         * GCC and Clang usually provide __uint128_t on 64-bit targets,
         * although Clang also defines it on WASM despite having to use
         * builtins for most purposes - including multiplication.
         */
#if defined(__SIZEOF_INT128__) && !defined(__wasm__)
        __uint128_t product = (__uint128_t)lhs * (__uint128_t)rhs;
        *high = (dword)(product >> 64);
        return (dword)(product & 0xFFFFFFFFFFFFFFFF);

        /* Use the _umul128 intrinsic on MSVC x64 to hint for mulq. */
#elif defined(_MSC_VER) && defined(_M_IX64)
        #   pragma intrinsic(_umul128)
    /* This intentionally has the same signature. */
    return _umul128(lhs, rhs, high);

#else
    /*
     * Fast yet simple grade school multiply that avoids
     * 64-bit carries with the properties of multiplying by 11
     * and takes advantage of UMAAL on ARMv6 to only need 4
     * calculations.
     */

    /* First calculate all of the cross products. */
    uint64_t lo_lo = (lhs & 0xFFFFFFFF) * (rhs & 0xFFFFFFFF);
    uint64_t hi_lo = (lhs >> 32)        * (rhs & 0xFFFFFFFF);
    uint64_t lo_hi = (lhs & 0xFFFFFFFF) * (rhs >> 32);
    uint64_t hi_hi = (lhs >> 32)        * (rhs >> 32);

    /* Now add the products together. These will never overflow. */
    uint64_t cross = (lo_lo >> 32) + (hi_lo & 0xFFFFFFFF) + lo_hi;
    uint64_t upper = (hi_lo >> 32) + (cross >> 32)        + hi_hi;

    *high = upper;
    return (cross << 32) | (lo_lo & 0xFFFFFFFF);
#endif /* portable */
}

/* Prevents a partial vectorization from GCC. */
#if defined(__GNUC__) && !defined(__clang__) && defined(__i386__)
__attribute__((__target__("no-sse")))
#endif
INLINE dword mult_64_to_128(sdword lhs, sdword rhs, dword *high) {
    /*
     * GCC and Clang usually provide __uint128_t on 64-bit targets,
     * although Clang also defines it on WASM despite having to use
     * builtins for most purposes - including multiplication.
     */
#if defined(__SIZEOF_INT128__) && !defined(__wasm__)
    __int128_t product = (__int128_t)lhs * (__int128_t)rhs;
    *high = (sdword)(product >> 64);
    return (sdword)(product & 0xFFFFFFFFFFFFFFFF);

    /* Use the _mul128 intrinsic on MSVC x64 to hint for mulq. */
#elif defined(_MSC_VER) && defined(_M_IX64)
    #   pragma intrinsic(_mul128)
    /* This intentionally has the same signature. */
    return _mul128(lhs, rhs, high);

#else
    /*
     * Fast yet simple grade school multiply that avoids
     * 64-bit carries with the properties of multiplying by 11
     * and takes advantage of UMAAL on ARMv6 to only need 4
     * calculations.
     */

    logfatal("This code will be broken for signed multiplies!");

    /* First calculate all of the cross products. */
    uint64_t lo_lo = (lhs & 0xFFFFFFFF) * (rhs & 0xFFFFFFFF);
    uint64_t hi_lo = (lhs >> 32)        * (rhs & 0xFFFFFFFF);
    uint64_t lo_hi = (lhs & 0xFFFFFFFF) * (rhs >> 32);
    uint64_t hi_hi = (lhs >> 32)        * (rhs >> 32);

    /* Now add the products together. These will never overflow. */
    uint64_t cross = (lo_lo >> 32) + (hi_lo & 0xFFFFFFFF) + lo_hi;
    uint64_t upper = (hi_lo >> 32) + (cross >> 32)        + hi_hi;

    *high = upper;
    return (cross << 32) | (lo_lo & 0xFFFFFFFF);
#endif /* portable */
}

MIPS_INSTR(mips_nop) {}

MIPS_INSTR(mips_addi) {
    word reg_addend = get_register(instruction.i.rs);
    word imm_addend = (sword)((shalf)instruction.i.immediate);
    word result = imm_addend + reg_addend;
    if (check_signed_overflow_add(reg_addend, imm_addend, result)) {
        r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_ARITHMETIC_OVERFLOW, 0);
    } else {
        set_register(instruction.i.rt, (sdword)((sword)(result)));
    }
}

MIPS_INSTR(mips_addiu) {
    word reg_addend = get_register(instruction.i.rs);
    shalf addend = instruction.i.immediate;
    sword result = reg_addend + addend;

    set_register(instruction.i.rt, (sdword)result);
}

MIPS_INSTR(mips_daddi) {
    dword addend1 = (sdword)((shalf)instruction.i.immediate);
    dword addend2 = get_register(instruction.i.rs);
    dword result = addend1 + addend2;

    if (check_signed_overflow_add(addend1, addend2, result)) {
        r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_ARITHMETIC_OVERFLOW, 0);
    } else {
        set_register(instruction.i.rt, result);
    }
}


MIPS_INSTR(mips_andi) {
    dword immediate = instruction.i.immediate;
    dword result = immediate & get_register(instruction.i.rs);
    set_register(instruction.i.rt, result);
}

MIPS_INSTR(mips_beq) {
    conditional_branch(instruction.i.immediate, get_register(instruction.i.rs) == get_register(instruction.i.rt));
}

MIPS_INSTR(mips_beql) {
    conditional_branch_likely(instruction.i.immediate, get_register(instruction.i.rs) == get_register(instruction.i.rt));
}

MIPS_INSTR(mips_bgtz) {
    sdword reg = get_register(instruction.i.rs);
    conditional_branch(instruction.i.immediate,  reg > 0);
}

MIPS_INSTR(mips_bgtzl) {
    sdword reg = get_register(instruction.i.rs);
    conditional_branch_likely(instruction.i.immediate,  reg > 0);
}

MIPS_INSTR(mips_blez) {
    sdword reg = get_register(instruction.i.rs);
    conditional_branch(instruction.i.immediate, reg <= 0);
}

MIPS_INSTR(mips_blezl) {
    sdword reg = get_register(instruction.i.rs);
    conditional_branch_likely(instruction.i.immediate, reg <= 0);
}

MIPS_INSTR(mips_bne) {
    conditional_branch(instruction.i.immediate, get_register(instruction.i.rs) != get_register(instruction.i.rt));
}

MIPS_INSTR(mips_bnel) {
    dword rs = get_register(instruction.i.rs);
    dword rt = get_register(instruction.i.rt);
    logtrace("Branch if: 0x%08lX != 0x%08lX", rs, rt);
    conditional_branch_likely(instruction.i.immediate, rs != rt);
}


MIPS_INSTR(mips_cache) {
    return; // No need to emulate the cache. Might be fun to do someday for accuracy.
}

MIPS_INSTR(mips_j) {
    dword target = instruction.j.target;
    target <<= 2;
    target |= ((N64CPU.pc - 4) & 0xFFFFFFFFF0000000); // PC is 4 ahead

    branch_abs(target);
}

MIPS_INSTR(mips_jal) {
    link(R4300I_REG_LR);

    dword target = instruction.j.target;
    target <<= 2;
    target |= ((N64CPU.pc - 4) & 0xFFFFFFFFF0000000); // PC is 4 ahead

    branch_abs(target);
}

MIPS_INSTR(mips_slti) {
    shalf immediate = instruction.i.immediate;
    logtrace("Set if %ld < %d", get_register(instruction.i.rs), immediate);
    sdword reg = get_register(instruction.i.rs);
    if (reg < immediate) {
        set_register(instruction.i.rt, 1);
    } else {
        set_register(instruction.i.rt, 0);
    }
}

MIPS_INSTR(mips_sltiu) {
    shalf immediate = instruction.i.immediate;
    logtrace("Set if %ld < %d", get_register(instruction.i.rs), immediate);
    if (get_register(instruction.i.rs) < immediate) {
        set_register(instruction.i.rt, 1);
    } else {
        set_register(instruction.i.rt, 0);
    }
}

MIPS_INSTR(mips_mfc0) {
    sword value = get_cp0_register_word(instruction.r.rd);
    set_register(instruction.r.rt, (sdword)value);
}

MIPS_INSTR(mips_mtc0) {
    word value = get_register(instruction.r.rt);
    set_cp0_register_word(instruction.r.rd, value);
}

MIPS_INSTR(mips_dmfc0) {
    dword value = get_cp0_register_dword(instruction.r.rd);
    set_register(instruction.r.rt, value);
}

MIPS_INSTR(mips_dmtc0) {
    dword value = get_register(instruction.r.rt);
    set_cp0_register_dword(instruction.r.rd, value);
}

MIPS_INSTR(mips_ld) {
    shalf offset = instruction.i.immediate;
    dword address = get_register(instruction.i.rs) + offset;
    if (check_address_error(0b111, address)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_ADDRESS_ERROR_LOAD, 0);
        return;
    }

    word physical;
    if (!resolve_virtual_address(address, BUS_LOAD, &physical)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_LOAD), 0);
    } else {
        dword value = n64_read_physical_dword(physical);
        set_register(instruction.i.rt, value);
    }
}

MIPS_INSTR(mips_lui) {
    // Done this way to avoid the undefined behavior of left shifting a signed integer
    // Should compile to a left shift by 16.
    sdword value = (shalf)instruction.i.immediate;
    value *= 65536;

    set_register(instruction.i.rt, value);
}

MIPS_INSTR(mips_lbu) {
    shalf offset = instruction.i.immediate;
    logtrace("LBU offset: %d", offset);
    dword address = get_register(instruction.i.rs) + offset;
    word physical;
    if (!resolve_virtual_address(address, BUS_LOAD, &physical)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_LOAD), 0);
    } else {
        byte value = n64_read_physical_byte(physical);
        set_register(instruction.i.rt, value); // zero extend
    }
}

MIPS_INSTR(mips_lhu) {
    shalf offset = instruction.i.immediate;
    logtrace("LHU offset: %d", offset);
    dword address = get_register(instruction.i.rs) + offset;
    if ((address & 0b1) > 0) {
        logfatal("TODO: throw an 'address error' exception! Tried to load from unaligned address 0x%016lX", address);
    }

    word physical;
    if (!resolve_virtual_address(address, BUS_LOAD, &physical)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_LOAD), 0);
    } else {
        half value = n64_read_physical_half(physical);
        set_register(instruction.i.rt, value); // zero extend
    }
}

MIPS_INSTR(mips_lh) {
    shalf offset = instruction.i.immediate;
    dword address = get_register(instruction.i.rs) + offset;
    if ((address & 0b1) > 0) {
        logfatal("TODO: throw an 'address error' exception! Tried to load from unaligned address 0x%016lX", address);
    }

    word physical;
    if (!resolve_virtual_address(address, BUS_LOAD, &physical)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_LOAD), 0);
    } else {
        shalf value = n64_read_physical_half(physical);
        set_register(instruction.i.rt, (sdword)value); // zero extend
    }
}

MIPS_INSTR(mips_lw) {
    shalf offset  = instruction.i.immediate;
    dword address = get_register(instruction.i.rs) + offset;
    if (check_address_error(0b11, address)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_ADDRESS_ERROR_LOAD, 0);
        return;
    }

    word physical;
    if (!resolve_virtual_address(address, BUS_LOAD, &physical)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_LOAD), 0);
    } else {
        sword value = n64_read_physical_word(physical);
        set_register(instruction.i.rt, (sdword)value);
    }
}

MIPS_INSTR(mips_lwu) {
    shalf offset  = instruction.i.immediate;
    dword address = get_register(instruction.i.rs) + offset;
    if ((address & 0b11) > 0) {
        logfatal("TODO: throw an 'address error' exception! Tried to load from unaligned address 0x%016lX", address);
    }

    word physical;
    if (!resolve_virtual_address(address, BUS_LOAD, &physical)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_LOAD), 0);
    } else {
        word value = n64_read_physical_word(physical);
        set_register(instruction.i.rt, value);
    }
}

MIPS_INSTR(mips_sb) {
    shalf offset  = instruction.i.immediate;
    dword address = get_register(instruction.i.rs);
    address += offset;
    byte value = get_register(instruction.i.rt) & 0xFF;

    word physical;
    if (!resolve_virtual_address(address, BUS_STORE, &physical)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_STORE), 0);
    } else {
        n64_write_physical_byte(physical, value);
    }
}

MIPS_INSTR(mips_sh) {
    shalf offset  = instruction.i.immediate;
    dword address = get_register(instruction.i.rs);
    address += offset;
    half value = get_register(instruction.i.rt);
    word physical;
    if (!resolve_virtual_address(address, BUS_STORE, &physical)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_STORE), 0);
    } else {
        n64_write_physical_half(physical, value);
    }
}

MIPS_INSTR(mips_sw) {
    shalf offset  = instruction.i.immediate;
    dword address = get_register(instruction.i.rs);
    address += offset;
    word physical;

    if (check_address_error(0b11, address)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_ADDRESS_ERROR_STORE, 0);
        return;
    }

    if (!resolve_virtual_address(address, BUS_STORE, &physical)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_STORE), 0);
    } else {
        n64_write_physical_word(physical, get_register(instruction.i.rt));
    }
}

MIPS_INSTR(mips_sd) {
    shalf offset  = instruction.i.immediate;
    dword address = get_register(instruction.i.rs) + offset;
    dword value = get_register(instruction.i.rt);

    word physical;
    if (check_address_error(0b111, address)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_ADDRESS_ERROR_STORE, 0);
        return;
    }

    if (!resolve_virtual_address(address, BUS_STORE, &physical)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_STORE), 0);
    } else {
        n64_write_physical_dword(physical, value);
    }
}

MIPS_INSTR(mips_ori) {
    set_register(instruction.i.rt, instruction.i.immediate | get_register(instruction.i.rs));
}

MIPS_INSTR(mips_xori) {
    set_register(instruction.i.rt, instruction.i.immediate ^ get_register(instruction.i.rs));
}

MIPS_INSTR(mips_daddiu) {
    dword addend1 = (sdword)((shalf)instruction.i.immediate);
    dword addend2 = get_register(instruction.i.rs);
    dword result = addend1 + addend2;
    set_register(instruction.i.rt, result);
}

MIPS_INSTR(mips_lb) {
    shalf offset  = instruction.i.immediate;
    dword address = get_register(instruction.i.rs) + offset;

    word physical;
    if (!resolve_virtual_address(address, BUS_LOAD, &physical)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_LOAD), 0);
    } else {
        sbyte value = n64_read_physical_byte(physical);
        set_register(instruction.i.rt, (sdword)value);
    }
}

MIPS_INSTR(mips_lwl) {
    shalf offset  = instruction.fi.offset;
    dword address = get_register(instruction.fi.base) + offset;


    word physical;
    if (!resolve_virtual_address(address, BUS_LOAD, &physical)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_LOAD), 0);
    } else {
        word shift = 8 * ((address ^ 0) & 3);
        word mask = 0xFFFFFFFF << shift;
        word data = n64_read_physical_word(physical & ~3);
        sword result = (get_register(instruction.i.rt) & ~mask) | data << shift;
        set_register(instruction.i.rt, (sdword)result);
    }
}

MIPS_INSTR(mips_lwr) {
    shalf offset  = instruction.fi.offset;
    dword address = get_register(instruction.fi.base) + offset;
    word physical;
    if (!resolve_virtual_address(address, BUS_LOAD, &physical)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_LOAD), 0);
    } else {
        word shift = 8 * ((address ^ 3) & 3);

        word mask = 0xFFFFFFFF >> shift;
        word data = n64_read_physical_word(physical & ~3);
        sword result = (get_register(instruction.i.rt) & ~mask) | data >> shift;
        set_register(instruction.i.rt, (sdword)result);
    }
}

MIPS_INSTR(mips_swl) {
    shalf offset  = instruction.fi.offset;
    dword address = get_register(instruction.fi.base) + offset;

    word physical;
    if (!resolve_virtual_address(address, BUS_STORE, &physical)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_STORE), 0);
    } else {
        word shift = 8 * ((address ^ 0) & 3);
        word mask = 0xFFFFFFFF >> shift;
        word data = n64_read_physical_word(physical & ~3);
        word oldreg = get_register(instruction.i.rt);
        n64_write_physical_word(physical & ~3, (data & ~mask) | (oldreg >> shift));
    }
}

MIPS_INSTR(mips_swr) {
    shalf offset  = instruction.fi.offset;
    dword address = get_register(instruction.fi.base) + offset;
    word physical;
    if (!resolve_virtual_address(address, BUS_STORE, &physical)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_STORE), 0);
    } else {
        word shift = 8 * ((address ^ 3) & 3);
        word mask = 0xFFFFFFFF << shift;
        word data = n64_read_physical_word(physical & ~3);
        word oldreg = get_register(instruction.i.rt);
        n64_write_physical_word(physical & ~3, (data & ~mask) | oldreg << shift);
    }
}

MIPS_INSTR(mips_ldl) {
    shalf offset = instruction.fi.offset;
    dword address = get_register(instruction.fi.base) + offset;
    word physical;
    if (!resolve_virtual_address(address, BUS_LOAD, &physical)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_LOAD), 0);
    } else {
        int shift = 8 * ((address ^ 0) & 7);
        dword mask = (dword) 0xFFFFFFFFFFFFFFFF << shift;
        dword data = n64_read_physical_dword(physical & ~7);
        dword oldreg = get_register(instruction.i.rt);

        set_register(instruction.i.rt, (oldreg & ~mask) | (data << shift));
    }
}

MIPS_INSTR(mips_ldr) {
    shalf offset = instruction.fi.offset;
    dword address = get_register(instruction.fi.base) + offset;
    word physical;
    if (!resolve_virtual_address(address, BUS_LOAD, &physical)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_LOAD), 0);
    } else {
        int shift = 8 * ((address ^ 7) & 7);
        dword mask = (dword) 0xFFFFFFFFFFFFFFFF >> shift;
        dword data = n64_read_physical_dword(physical & ~7);
        dword oldreg = get_register(instruction.i.rt);

        set_register(instruction.i.rt, (oldreg & ~mask) | (data >> shift));
    }
}

MIPS_INSTR(mips_sdl) {
    shalf offset = instruction.fi.offset;
    dword address = get_register(instruction.fi.base) + offset;

    word physical;
    if (!resolve_virtual_address(address, BUS_STORE, &physical)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_STORE), 0);
    } else {
        int shift = 8 * ((address ^ 0) & 7);
        dword mask = 0xFFFFFFFFFFFFFFFF;
        mask >>= shift;
        dword data = n64_read_physical_dword(physical & ~7);
        dword oldreg = get_register(instruction.i.rt);
        n64_write_physical_dword(physical & ~7, (data & ~mask) | (oldreg >> shift));
    }
}

MIPS_INSTR(mips_sdr) {
    shalf offset = instruction.fi.offset;
    dword address = get_register(instruction.fi.base) + offset;
    word physical;
    if (!resolve_virtual_address(address, BUS_STORE, &physical)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_STORE), 0);
    } else {
        int shift = 8 * ((address ^ 7) & 7);
        dword mask = 0xFFFFFFFFFFFFFFFF;
        mask <<= shift;
        dword data = n64_read_physical_dword(physical & ~7);
        dword oldreg = get_register(instruction.i.rt);
        n64_write_physical_dword(physical & ~7, (data & ~mask) | (oldreg << shift));
    }
}

MIPS_INSTR(mips_ll) {
    // Identical to lw
    shalf offset = instruction.i.immediate;
    dword address = get_register(instruction.i.rs) + offset;

    word physical;
    sword result;
    if (!resolve_virtual_address(address, BUS_LOAD, &physical)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_LOAD), 0);
    } else {
        result = n64_read_physical_word(physical);
    }



    if ((address & 0b11) > 0) {
        logfatal("TODO: throw an 'address error' exception! Tried to load from unaligned address 0x%016lX", address);
    }

    set_register(instruction.i.rt, (sdword)result);

    // Unique to ll
    N64CPU.cp0.lladdr = physical;
    N64CPU.llbit = true;
}

MIPS_INSTR(mips_lld) {
    // Instruction is undefined outside of 64 bit mode and 32 bit kernel mode.
    // Throw an exception if we're not in 64 bit mode AND not in kernel mode.
    if (!N64CPU.cp0.is_64bit_addressing && N64CPU.cp0.kernel_mode) {
        logfatal("LLD is undefined outside of 64 bit mode and 32 bit kernel mode. Throw a reserved instruction exception!");
    }

    // Identical to ld
    shalf offset = instruction.i.immediate;
    dword address = get_register(instruction.i.rs) + offset;

    word physical;
    if (!resolve_virtual_address(address, BUS_LOAD, &physical)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_LOAD), 0);
    } else {
        dword result = n64_read_physical_dword(physical);
        if ((address & 0b111) > 0) {
            logfatal("TODO: throw an 'address error' exception! Tried to load from unaligned address 0x%016lX", address);
        }
        set_register(instruction.i.rt, result);

        // Unique to lld
        N64CPU.cp0.lladdr = physical;
        N64CPU.llbit = true;
    }
}

MIPS_INSTR(mips_sc) {
    // Identical to sw
    shalf offset          = instruction.i.immediate;
    dword address         = get_register(instruction.i.rs) + offset;

    // Exception takes precedence over the instruction failing
    if ((address & 0b11) > 0) {
        logfatal("TODO: throw an 'address error' exception! Tried to store to unaligned address 0x%016lX", address);
    }

    if (N64CPU.llbit) {
        word physical_address;
        if (!resolve_virtual_address(address, BUS_STORE, &physical_address)) {
            on_tlb_exception(address);
            r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_STORE), 0);
        } else {
            if (physical_address != N64CPU.cp0.lladdr) {
                logfatal("Undefined: SC physical address is NOT EQUAL to last lladdr!\n");
            }

            word value = get_register(instruction.i.rt);
            n64_write_physical_word(physical_address, value);
            set_register(instruction.i.rt, 1); // Success!
        }
    } else {
        set_register(instruction.i.rt, 0); // Failure.
    }
}

MIPS_INSTR(mips_scd) {
    // Instruction is undefined outside of 64 bit mode and 32 bit kernel mode.
    // Throw an exception if we're not in 64 bit mode AND not in kernel mode.
    if (!N64CPU.cp0.is_64bit_addressing && N64CPU.cp0.kernel_mode) {
        logfatal("SCD is undefined outside of 64 bit mode and 32 bit kernel mode. Throw a reserved instruction exception!");
    }

    // Identical to sd
    shalf offset          = instruction.i.immediate;
    dword address         = get_register(instruction.i.rs) + offset;

    // Exception takes precedence over the instruction failing
    if ((address & 0b111) > 0) {
        logfatal("TODO: throw an 'address error' exception! Tried to store to unaligned address 0x%016lX", address);
    }

    if (N64CPU.llbit) {
        word physical_address = resolve_virtual_address_or_die(address, BUS_STORE);

        if (physical_address != N64CPU.cp0.lladdr) {
            logfatal("Undefined: SCD physical address is NOT EQUAL to last lladdr!\n");
        }

        dword value = get_register(instruction.i.rt);
        n64_write_physical_dword(physical_address, value);
        set_register(instruction.i.rt, 1); // Success!

    } else {
        set_register(instruction.i.rt, 0); // Failure.
    }
}

MIPS_INSTR(mips_spc_sll) {
    sword result = get_register(instruction.r.rt) << instruction.r.sa;
    set_register(instruction.r.rd, (sdword)result);
}

MIPS_INSTR(mips_spc_srl) {
    word value = get_register(instruction.r.rt);
    sword result = value >> instruction.r.sa;
    set_register(instruction.r.rd, (sdword) result);
}

MIPS_INSTR(mips_spc_sra) {
    sdword value = get_register(instruction.r.rt);
    sword result = (sdword)(value >> (dword)instruction.r.sa);
    set_register(instruction.r.rd, (sdword) result);
}

MIPS_INSTR(mips_spc_srav) {
    sdword value = get_register(instruction.r.rt);
    sword result = (sdword)(value >> (get_register(instruction.r.rs) & 0b11111));
    set_register(instruction.r.rd, (sdword) result);
}

MIPS_INSTR(mips_spc_sllv) {
    word value = get_register(instruction.r.rt);
    sword result = value << (get_register(instruction.r.rs) & 0b11111);
    set_register(instruction.r.rd, (sdword)result);
}

MIPS_INSTR(mips_spc_srlv) {
    word value = get_register(instruction.r.rt);
    sword result = value >> (get_register(instruction.r.rs) & 0b11111);
    set_register(instruction.r.rd, (sdword)result);
}

MIPS_INSTR(mips_spc_jr) {
    branch_abs(get_register(instruction.r.rs));
}

MIPS_INSTR(mips_spc_jalr) {
    branch_abs(get_register(instruction.r.rs));
    link(instruction.r.rd);
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
    dword val = get_register(instruction.r.rt);
    val <<= (get_register(instruction.r.rs) & 0b111111);
    set_register(instruction.r.rd, val);
}

MIPS_INSTR(mips_spc_dsrlv) {
    dword val = get_register(instruction.r.rt);
    val >>= (get_register(instruction.r.rs) & 0b111111);
    set_register(instruction.r.rd, val);
}

MIPS_INSTR(mips_spc_dsrav) {
    sdword value = get_register(instruction.r.rt);
    sdword result = value >> (get_register(instruction.r.rs) & 0b111111);
    set_register(instruction.r.rd, result);
}

MIPS_INSTR(mips_spc_mult) {
    sword multiplicand_1 = get_register(instruction.r.rs);
    sword multiplicand_2 = get_register(instruction.r.rt);

    sdword dmultiplicand_1 = multiplicand_1;
    sdword dmultiplicand_2 = multiplicand_2;

    sdword result = dmultiplicand_1 * dmultiplicand_2;

    sword result_lower = result         & 0xFFFFFFFF;
    sword result_upper = (result >> 32) & 0xFFFFFFFF;

    N64CPU.mult_lo = (sdword)result_lower;
    N64CPU.mult_hi = (sdword)result_upper;
}

MIPS_INSTR(mips_spc_multu) {
    dword multiplicand_1 = get_register(instruction.r.rs) & 0xFFFFFFFF;
    dword multiplicand_2 = get_register(instruction.r.rt) & 0xFFFFFFFF;

    dword result = multiplicand_1 * multiplicand_2;

    sword result_lower = result         & 0xFFFFFFFF;
    sword result_upper = (result >> 32) & 0xFFFFFFFF;

    N64CPU.mult_lo = (sdword)result_lower;
    N64CPU.mult_hi = (sdword)result_upper;
}

MIPS_INSTR(mips_spc_div) {
    sdword dividend = (sword)get_register(instruction.r.rs);
    sdword divisor  = (sword)get_register(instruction.r.rt);

    if (divisor == 0) {
        logwarn("Divide by zero");
        N64CPU.mult_hi = dividend;
        if (dividend >= 0) {
            N64CPU.mult_lo = (sdword)-1;
        } else {
            N64CPU.mult_lo = (sdword)1;
        }
    } else {
        sdword quotient  = dividend / divisor;
        sdword remainder = dividend % divisor;

        N64CPU.mult_lo = quotient;
        N64CPU.mult_hi = remainder;
    }

}

MIPS_INSTR(mips_spc_divu) {
    word dividend = get_register(instruction.r.rs);
    word divisor  = get_register(instruction.r.rt);

    if (divisor == 0) {
        N64CPU.mult_lo = 0xFFFFFFFFFFFFFFFF;
        N64CPU.mult_hi = (sword)dividend;
    } else {
        sword quotient  = dividend / divisor;
        sword remainder = dividend % divisor;

        N64CPU.mult_lo = quotient;
        N64CPU.mult_hi = remainder;
    }
}

MIPS_INSTR(mips_spc_dmult) {
    dword result_upper;
    dword result_lower = mult_64_to_128(get_register(instruction.r.rs), get_register(instruction.r.rt), &result_upper);

    N64CPU.mult_lo = result_lower;
    N64CPU.mult_hi = result_upper;
}

MIPS_INSTR(mips_spc_dmultu) {
    dword result_upper;
    dword result_lower = multu_64_to_128(get_register(instruction.r.rs), get_register(instruction.r.rt), &result_upper);

    N64CPU.mult_lo = result_lower;
    N64CPU.mult_hi = result_upper;
}

MIPS_INSTR(mips_spc_ddiv) {
    sdword dividend = (sdword)get_register(instruction.r.rs);
    sdword divisor  = (sdword)get_register(instruction.r.rt);

    if (divisor == 0) {
        logwarn("Divide by zero");
        N64CPU.mult_hi = dividend;
        if (dividend >= 0) {
            N64CPU.mult_lo = (sdword)-1;
        } else {
            N64CPU.mult_lo = (sdword)1;
        }
    } else {
        sdword quotient  = (sdword)(dividend / divisor);
        sdword remainder = (sdword)(dividend % divisor);

        N64CPU.mult_lo = quotient;
        N64CPU.mult_hi = remainder;
    }
}

MIPS_INSTR(mips_spc_ddivu) {
    dword dividend = get_register(instruction.r.rs);
    dword divisor  = get_register(instruction.r.rt);

    if (divisor == 0) {
        N64CPU.mult_lo = 0xFFFFFFFFFFFFFFFF;
        N64CPU.mult_hi = dividend;
    } else {
        dword quotient  = dividend / divisor;
        dword remainder = dividend % divisor;

        N64CPU.mult_lo = quotient;
        N64CPU.mult_hi = remainder;
    }
}

MIPS_INSTR(mips_spc_add) {
    word addend1 = get_register(instruction.r.rs);
    word addend2 = get_register(instruction.r.rt);

    word result = addend1 + addend2;
    if (check_signed_overflow_add(addend1, addend2, result)) {
        r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_ARITHMETIC_OVERFLOW, 0);
    } else {
        set_register(instruction.r.rd, (sdword)((sword)result));
    }
}

MIPS_INSTR(mips_spc_addu) {
    word rs = get_register(instruction.r.rs);
    word rt = get_register(instruction.r.rt);
    sword result = rs + rt;
    set_register(instruction.r.rd, (sdword)result);
}

MIPS_INSTR(mips_spc_and) {
    dword result = get_register(instruction.r.rs) & get_register(instruction.r.rt);
    set_register(instruction.r.rd, result);
}

MIPS_INSTR(mips_spc_nor) {
    dword result = ~(get_register(instruction.r.rs) | get_register(instruction.r.rt));
    set_register(instruction.r.rd, result);
}

MIPS_INSTR(mips_spc_sub) {
    sword operand1 = get_register(instruction.r.rs);
    sword operand2 = get_register(instruction.r.rt);

    sword result = operand1 - operand2;

    if (check_signed_overflow_sub(operand1, operand2, result)) {
        r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_ARITHMETIC_OVERFLOW, 0);
    } else {
        set_register(instruction.r.rd, (sdword)result);
    }

}

MIPS_INSTR(mips_spc_subu) {
    word operand1 = get_register(instruction.r.rs);
    word operand2 = get_register(instruction.r.rt);

    sword result = operand1 - operand2;
    set_register(instruction.r.rd, (sdword)result);
}

MIPS_INSTR(mips_spc_or) {
    set_register(instruction.r.rd, get_register(instruction.r.rs) | get_register(instruction.r.rt));
}

MIPS_INSTR(mips_spc_xor) {
    set_register(instruction.r.rd, get_register(instruction.r.rs) ^ get_register(instruction.r.rt));
}

MIPS_INSTR(mips_spc_slt) {
    sdword op1 = get_register(instruction.r.rs);
    sdword op2 = get_register(instruction.r.rt);

    // RS - RT
    sdword result = op1 - op2;
    // if RS is LESS than RT
    // aka, if result is negative

    logtrace("Set if %ld < %ld", op1, op2);
    if (result < 0) {
        set_register(instruction.r.rd, 1);
    } else {
        set_register(instruction.r.rd, 0);
    }
}

MIPS_INSTR(mips_spc_sltu) {
    dword op1 = get_register(instruction.r.rs);
    dword op2 = get_register(instruction.r.rt);

    logtrace("Set if %lu < %lu", op1, op2);
    if (op1 < op2) {
        set_register(instruction.r.rd, 1);
    } else {
        set_register(instruction.r.rd, 0);
    }
}

MIPS_INSTR(mips_spc_dadd) {
    dword addend1 = get_register(instruction.r.rs);
    dword addend2 = get_register(instruction.r.rt);
    dword result = addend1 + addend2;

    if (check_signed_overflow_add(addend1, addend2, result)) {
        r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_ARITHMETIC_OVERFLOW, 0);
    } else {
        set_register(instruction.r.rd, result);
    }
}

MIPS_INSTR(mips_spc_daddu) {
    dword addend1 = get_register(instruction.r.rs);
    dword addend2 = get_register(instruction.r.rt);
    dword result = addend1 + addend2;
    set_register(instruction.r.rd, result);
}

MIPS_INSTR(mips_spc_dsub) {
    sdword minuend = get_register(instruction.r.rs);
    sdword subtrahend = get_register(instruction.r.rt);
    sdword difference = minuend - subtrahend;

    if (check_signed_overflow_sub(minuend, subtrahend, difference)) {
        r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_ARITHMETIC_OVERFLOW, 0);
    } else {
        set_register(instruction.r.rd, difference);
    }
}

MIPS_INSTR(mips_spc_dsubu) {
    sdword minuend = get_register(instruction.r.rs);
    sdword subtrahend = get_register(instruction.r.rt);
    sdword difference = minuend - subtrahend;
    set_register(instruction.r.rd, difference);
}

MIPS_INSTR(mips_spc_teq) {
    dword rs = get_register(instruction.r.rs);
    dword rt = get_register(instruction.r.rt);

    if (rs == rt) {
        r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_TRAP, 0);
    }
}

MIPS_INSTR(mips_spc_break) {
    r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_BREAKPOINT, 0);
}

MIPS_INSTR(mips_spc_tne) {
    dword rs = get_register(instruction.r.rs);
    dword rt = get_register(instruction.r.rt);

    if (rs != rt) {
        r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_TRAP, 0);
    }
}

MIPS_INSTR(mips_spc_tge) {
    sdword rs = get_register(instruction.r.rs);
    sdword rt = get_register(instruction.r.rt);

    if (rs >= rt) {
        r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_TRAP, 0);
    }
}

MIPS_INSTR(mips_spc_tgeu) {
    dword rs = get_register(instruction.r.rs);
    dword rt = get_register(instruction.r.rt);

    if (rs >= rt) {
        r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_TRAP, 0);
    }
}

MIPS_INSTR(mips_spc_tlt) {
    sdword rs = get_register(instruction.r.rs);
    sdword rt = get_register(instruction.r.rt);

    if (rs < rt) {
        r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_TRAP, 0);
    }
}

MIPS_INSTR(mips_spc_tltu) {
    dword rs = get_register(instruction.r.rs);
    dword rt = get_register(instruction.r.rt);

    if (rs < rt) {
        r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_TRAP, 0);
    }
}


MIPS_INSTR(mips_spc_dsll) {
    dword value = get_register(instruction.r.rt);
    value <<= instruction.r.sa;
    set_register(instruction.r.rd, value);
}

MIPS_INSTR(mips_spc_dsrl) {
    dword value = get_register(instruction.r.rt);
    value >>= instruction.r.sa;
    set_register(instruction.r.rd, value);
}

MIPS_INSTR(mips_spc_dsra) {
    sdword value = get_register(instruction.r.rt);
    value >>= instruction.r.sa;
    set_register(instruction.r.rd, value);
}

MIPS_INSTR(mips_spc_dsll32) {
    dword value = get_register(instruction.r.rt);
    value <<= (instruction.r.sa + 32);
    set_register(instruction.r.rd, value);
}

MIPS_INSTR(mips_spc_dsrl32) {
    dword value = get_register(instruction.r.rt);
    value >>= (instruction.r.sa + 32);
    set_register(instruction.r.rd, value);
}

MIPS_INSTR(mips_spc_dsra32) {
    sdword value = get_register(instruction.r.rt);
    value >>= (instruction.r.sa + 32);
    set_register(instruction.r.rd, value);
}

MIPS_INSTR(mips_ri_bltz) {
    sdword reg = get_register(instruction.i.rs);
    conditional_branch(instruction.i.immediate, reg < 0);
}

MIPS_INSTR(mips_ri_bltzl) {
    sdword reg = get_register(instruction.i.rs);
    conditional_branch_likely(instruction.i.immediate, reg < 0);
}

MIPS_INSTR(mips_ri_bgez) {
    sdword reg = get_register(instruction.i.rs);
    conditional_branch(instruction.i.immediate, reg >= 0);
}

MIPS_INSTR(mips_ri_bgezl) {
    sdword reg = get_register(instruction.i.rs);
    conditional_branch_likely(instruction.i.immediate, reg >= 0);
}

MIPS_INSTR(mips_ri_bltzal) {
    sdword reg = get_register(instruction.i.rs);
    conditional_branch(instruction.i.immediate, reg < 0);
    link(R4300I_REG_LR);
}

MIPS_INSTR(mips_ri_bgezal) {
    sdword reg = get_register(instruction.i.rs);
    conditional_branch(instruction.i.immediate, reg >= 0);
    link(R4300I_REG_LR);
}

MIPS_INSTR(mips_ri_bgezall) {
    sdword reg = get_register(instruction.i.rs);
    link(R4300I_REG_LR);
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
    sdword rs = get_register(instruction.i.rs);
    shalf imm = instruction.i.immediate;

    if (rs >= imm) {
        r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_TRAP, 0);
    }
}

MIPS_INSTR(mips_ri_tgeiu) {
    dword rs = get_register(instruction.i.rs);
    dword imm = (sdword)(((shalf)instruction.i.immediate));

    if (rs >= imm) {
        r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_TRAP, 0);
    }
}

MIPS_INSTR(mips_ri_tlti) {
    sdword rs = get_register(instruction.i.rs);
    shalf imm = instruction.i.immediate;

    if (rs < imm) {
        r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_TRAP, 0);
    }
}

MIPS_INSTR(mips_ri_tltiu) {
    dword rs = get_register(instruction.i.rs);
    dword imm = (sdword)(((shalf)instruction.i.immediate));

    if (rs < imm) {
        r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_TRAP, 0);
    }
}

MIPS_INSTR(mips_ri_teqi) {
    sdword rs = get_register(instruction.i.rs);
    shalf imm = instruction.i.immediate;

    if (rs == imm) {
        r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_TRAP, 0);
    }
}

MIPS_INSTR(mips_ri_tnei) {
    sdword rs = get_register(instruction.i.rs);
    shalf imm = instruction.i.immediate;

    if (rs != imm) {
        r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_TRAP, 0);
    }
}

MIPS_INSTR(mips_invalid) {
    r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_RESERVED_INSTR, 0);
}
