#ifndef N64_MIPS_INSTRUCTIONS_H
#define N64_MIPS_INSTRUCTIONS_H
#include "r4300i.h"
#include "mips_instruction_decode.h"

#define check_signed_overflow_add(op1, op2, res)  (((~((op1) ^ (op2)) & ((op1) ^ (res))) >> ((sizeof(res) * 8) - 1)) & 1)
#define check_signed_overflow_sub(op1, op2, res) (((((op1) ^ (op2)) & ((op1) ^ (res))) >> ((sizeof(res) * 8) - 1)) & 1)
#define check_address_error(mask, virtual) (((!N64CP0.is_64bit_addressing) && (s32)(virtual) != (virtual)) || (((virtual) & (mask)) != 0))

#define MIPS_INSTR(NAME) void NAME(mips_instruction_t instruction)

// https://stackoverflow.com/questions/25095741/how-can-i-multiply-64-bit-operands-and-get-128-bit-result-portably/58381061#58381061
/* Prevents a partial vectorization from GCC. */
#if defined(__GNUC__) && !defined(__clang__) && defined(__i386__)
__attribute__((__target__("no-sse")))
#endif
INLINE u64 multu_64_to_128(u64 lhs, u64 rhs, u64 *high) {
    /*
     * GCC and Clang usually provide __uint128_t on 64-bit targets,
     * although Clang also defines it on WASM despite having to use
     * builtins for most purposes - including multiplication.
     */
#if defined(__SIZEOF_INT128__) && !defined(__wasm__)
    __uint128_t product = (__uint128_t)lhs * (__uint128_t)rhs;
    *high = (u64)(product >> 64);
    return (u64)(product & 0xFFFFFFFFFFFFFFFF);

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
INLINE u64 mult_64_to_128(s64 lhs, s64 rhs, u64 *high) {
    /*
     * GCC and Clang usually provide __uint128_t on 64-bit targets,
     * although Clang also defines it on WASM despite having to use
     * builtins for most purposes - including multiplication.
     */
#if defined(__SIZEOF_INT128__) && !defined(__wasm__)
    __int128_t product = (__int128_t)lhs * (__int128_t)rhs;
    *high = (s64)(product >> 64);
    return (s64)(product & 0xFFFFFFFFFFFFFFFF);

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


MIPS_INSTR(mips_nop);

MIPS_INSTR(mips_addi);
MIPS_INSTR(mips_addiu);

MIPS_INSTR(mips_daddi);

MIPS_INSTR(mips_andi);

MIPS_INSTR(mips_blez);
MIPS_INSTR(mips_blezl);
MIPS_INSTR(mips_beq);
MIPS_INSTR(mips_beql);
MIPS_INSTR(mips_bgtz);
MIPS_INSTR(mips_bgtzl);
MIPS_INSTR(mips_bne);
MIPS_INSTR(mips_bnel);

MIPS_INSTR(mips_cache);

MIPS_INSTR(mips_j);
MIPS_INSTR(mips_jal);

MIPS_INSTR(mips_slti);
MIPS_INSTR(mips_sltiu);

MIPS_INSTR(mips_mfc0);
MIPS_INSTR(mips_mtc0);
MIPS_INSTR(mips_dmfc0);
MIPS_INSTR(mips_dmtc0);

MIPS_INSTR(mips_eret);

MIPS_INSTR(mips_ld);
MIPS_INSTR(mips_lui);
MIPS_INSTR(mips_lbu);
MIPS_INSTR(mips_lhu);
MIPS_INSTR(mips_lh);
MIPS_INSTR(mips_lw);
MIPS_INSTR(mips_lwu);
MIPS_INSTR(mips_sb);
MIPS_INSTR(mips_sh);
MIPS_INSTR(mips_sd);
MIPS_INSTR(mips_sw);
MIPS_INSTR(mips_ori);

MIPS_INSTR(mips_xori);
MIPS_INSTR(mips_daddiu);

MIPS_INSTR(mips_lb);

MIPS_INSTR(mips_ldc1);
MIPS_INSTR(mips_sdc1);
MIPS_INSTR(mips_lwc1);
MIPS_INSTR(mips_swc1);
MIPS_INSTR(mips_lwl);
MIPS_INSTR(mips_lwr);
MIPS_INSTR(mips_swl);
MIPS_INSTR(mips_swr);
MIPS_INSTR(mips_ldl);
MIPS_INSTR(mips_ldr);
MIPS_INSTR(mips_sdl);
MIPS_INSTR(mips_sdr);
MIPS_INSTR(mips_ll);
MIPS_INSTR(mips_lld);
MIPS_INSTR(mips_sc);
MIPS_INSTR(mips_scd);

MIPS_INSTR(mips_spc_sll);
MIPS_INSTR(mips_spc_srl);
MIPS_INSTR(mips_spc_sra);
MIPS_INSTR(mips_spc_srav);
MIPS_INSTR(mips_spc_sllv);
MIPS_INSTR(mips_spc_srlv);
MIPS_INSTR(mips_spc_jr);
MIPS_INSTR(mips_spc_jalr);
MIPS_INSTR(mips_spc_syscall);
MIPS_INSTR(mips_spc_mfhi);
MIPS_INSTR(mips_spc_mthi);
MIPS_INSTR(mips_spc_mflo);
MIPS_INSTR(mips_spc_mtlo);
MIPS_INSTR(mips_spc_dsllv);
MIPS_INSTR(mips_spc_dsrlv);
MIPS_INSTR(mips_spc_dsrav);
MIPS_INSTR(mips_spc_mult);
MIPS_INSTR(mips_spc_multu);
MIPS_INSTR(mips_spc_div);
MIPS_INSTR(mips_spc_divu);
MIPS_INSTR(mips_spc_dmult);
MIPS_INSTR(mips_spc_dmultu);
MIPS_INSTR(mips_spc_ddiv);
MIPS_INSTR(mips_spc_ddivu);
MIPS_INSTR(mips_spc_add);
MIPS_INSTR(mips_spc_addu);
MIPS_INSTR(mips_spc_nor);
MIPS_INSTR(mips_spc_and);
MIPS_INSTR(mips_spc_sub);
MIPS_INSTR(mips_spc_subu);
MIPS_INSTR(mips_spc_or);
MIPS_INSTR(mips_spc_xor);
MIPS_INSTR(mips_spc_slt);
MIPS_INSTR(mips_spc_sltu);
MIPS_INSTR(mips_spc_dadd);
MIPS_INSTR(mips_spc_daddu);
MIPS_INSTR(mips_spc_dsub);
MIPS_INSTR(mips_spc_dsubu);
MIPS_INSTR(mips_spc_teq);
MIPS_INSTR(mips_spc_break);
MIPS_INSTR(mips_spc_tne);
MIPS_INSTR(mips_spc_dsll);
MIPS_INSTR(mips_spc_dsrl);
MIPS_INSTR(mips_spc_dsra);
MIPS_INSTR(mips_spc_dsll32);
MIPS_INSTR(mips_spc_dsrl32);
MIPS_INSTR(mips_spc_dsra32);
MIPS_INSTR(mips_spc_tge);
MIPS_INSTR(mips_spc_tgeu);
MIPS_INSTR(mips_spc_tlt);
MIPS_INSTR(mips_spc_tltu);


MIPS_INSTR(mips_ri_bltz);
MIPS_INSTR(mips_ri_bltzl);
MIPS_INSTR(mips_ri_bgez);
MIPS_INSTR(mips_ri_bgezl);
MIPS_INSTR(mips_ri_bltzal);
MIPS_INSTR(mips_ri_bgezal);
MIPS_INSTR(mips_ri_bgezall);

MIPS_INSTR(mips_ri_tgei);
MIPS_INSTR(mips_ri_tgeiu);
MIPS_INSTR(mips_ri_tlti);
MIPS_INSTR(mips_ri_tltiu);
MIPS_INSTR(mips_ri_teqi);
MIPS_INSTR(mips_ri_tnei);

MIPS_INSTR(mips_invalid);

MIPS_INSTR(mips_mfc2);
MIPS_INSTR(mips_mtc2);
MIPS_INSTR(mips_dmfc2);
MIPS_INSTR(mips_dmtc2);
MIPS_INSTR(mips_cfc2);
MIPS_INSTR(mips_ctc2);
MIPS_INSTR(mips_cp2_invalid);

#endif //N64_MIPS_INSTRUCTIONS_H
