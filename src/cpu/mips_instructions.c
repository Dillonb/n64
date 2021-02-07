#include "mips_instructions.h"
#include "r4300i_register_access.h"

#include <math.h>

#define ORDERED_S(fs, ft) do { if (isnanf(fs) || isnanf(ft)) { logfatal("we got some nans, time to panic"); } } while (0)
#define ORDERED_D(fs, ft) do { if (isnan(fs) || isnan(ft)) { logfatal("we got some nans, time to panic"); } } while (0)

#define UNORDERED_S(fs, ft) do { if (isnanf(fs) || isnanf(ft)) { logfatal("we got some nans, time to panic"); } } while (0)
#define UNORDERED_D(fs, ft) do { if (isnan(fs) || isnan(ft)) { logfatal("we got some nans, time to panic"); } } while (0)

void check_sword_add_overflow(sword addend1, sword addend2, sword result) {
    if (addend1 > 0 && addend2 > 0) {
        if (result < 0) {
            logfatal("Integer overflow exception");
        }
    } else if (addend1 < 0 && addend2 < 0) {
        if (result > 0) {
            logfatal("Integer overflow exception");
        }
    }
}

void check_sdword_add_overflow(sdword addend1, sdword addend2, sdword result) {
    if (addend1 > 0 && addend2 > 0) {
        if (result < 0) {
            logfatal("Integer overflow exception");
        }
    } else if (addend1 < 0 && addend2 < 0) {
        if (result > 0) {
            logfatal("Integer overflow exception");
        }
    }
}

INLINE void link(r4300i_t* cpu) {
    dword pc = cpu->pc + 4;
    set_register(cpu, R4300I_REG_LR, pc); // Skips the instruction in the delay slot on return
}

INLINE void branch_abs(r4300i_t* cpu, dword address) {
    cpu->next_pc = address;
    cpu->branch = true;
}

INLINE void branch_abs_word(r4300i_t* cpu, dword address) {
    branch_abs(cpu, (sdword)((sword)address));
}

INLINE void branch_offset(r4300i_t* cpu, shalf offset) {
    sword soffset = offset;
    soffset *= 4;
    // This is taking advantage of the fact that we add 4 to the PC after each instruction.
    // Due to the compiler expecting pipelining, the address we get here will be 4 _too early_

    branch_abs(cpu, cpu->pc + soffset);
}

INLINE void conditional_branch_likely(r4300i_t* cpu, word offset, bool condition) {
    if (condition) {
        branch_offset(cpu, offset);
    } else {
        // Skip instruction in delay slot
        set_pc_dword_r4300i(cpu, cpu->pc + 4);
    }
}

INLINE void conditional_branch(r4300i_t* cpu, word offset, bool condition) {
    if (condition) {
        branch_offset(cpu, offset);
    }
}

// https://stackoverflow.com/questions/25095741/how-can-i-multiply-64-bit-operands-and-get-128-bit-result-portably/58381061#58381061
/* Prevents a partial vectorization from GCC. */
#if defined(__GNUC__) && !defined(__clang__) && defined(__i386__)
__attribute__((__target__("no-sse")))
#endif
INLINE dword mult_64_to_128(dword lhs, dword rhs, dword *high) {
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

MIPS_INSTR(mips_nop) {}

MIPS_INSTR(mips_addi) {
    sword reg_addend = get_register(cpu, instruction.i.rs);
    shalf imm_addend = instruction.i.immediate;
    sword result = imm_addend + reg_addend;
    check_sword_add_overflow(imm_addend, reg_addend, result);
    set_register(cpu, instruction.i.rt, (sdword)result);
}

MIPS_INSTR(mips_addiu) {
    word reg_addend = get_register(cpu, instruction.i.rs);
    shalf addend = instruction.i.immediate;
    sword result = reg_addend + addend;

    set_register(cpu, instruction.i.rt, (sdword)result);
}

MIPS_INSTR(mips_daddi) {
    shalf  addend1 = instruction.i.immediate;
    sdword addend2 = get_register(cpu, instruction.i.rs);
    sdword result = addend1 + addend2;
    check_sdword_add_overflow(addend1, addend2, result);
    set_register(cpu, instruction.i.rt, result);
}


MIPS_INSTR(mips_andi) {
    dword immediate = instruction.i.immediate;
    dword result = immediate & get_register(cpu, instruction.i.rs);
    set_register(cpu, instruction.i.rt, result);
}

MIPS_INSTR(mips_beq) {
    conditional_branch(cpu, instruction.i.immediate, get_register(cpu, instruction.i.rs) == get_register(cpu, instruction.i.rt));
}

MIPS_INSTR(mips_beql) {
    conditional_branch_likely(cpu, instruction.i.immediate, get_register(cpu, instruction.i.rs) == get_register(cpu, instruction.i.rt));
}

MIPS_INSTR(mips_bgtz) {
    sdword reg = get_register(cpu, instruction.i.rs);
    conditional_branch(cpu, instruction.i.immediate,  reg > 0);
}

MIPS_INSTR(mips_bgtzl) {
    sdword reg = get_register(cpu, instruction.i.rs);
    conditional_branch_likely(cpu, instruction.i.immediate,  reg > 0);
}

MIPS_INSTR(mips_blez) {
    sdword reg = get_register(cpu, instruction.i.rs);
    conditional_branch(cpu, instruction.i.immediate, reg <= 0);
}

MIPS_INSTR(mips_blezl) {
    sdword reg = get_register(cpu, instruction.i.rs);
    conditional_branch_likely(cpu, instruction.i.immediate, reg <= 0);
}

MIPS_INSTR(mips_bne) {
    conditional_branch(cpu, instruction.i.immediate, get_register(cpu, instruction.i.rs) != get_register(cpu, instruction.i.rt));
}

MIPS_INSTR(mips_bnel) {
    dword rs = get_register(cpu, instruction.i.rs);
    dword rt = get_register(cpu, instruction.i.rt);
    logtrace("Branch if: 0x%08lX != 0x%08lX", rs, rt);
    conditional_branch_likely(cpu, instruction.i.immediate, rs != rt);
}


MIPS_INSTR(mips_cache) {
    return; // No need to emulate the cache. Might be fun to do someday for accuracy.
}

MIPS_INSTR(mips_j) {
    word target = instruction.j.target;
    target <<= 2;
    target |= ((cpu->pc - 4) & 0xF0000000); // PC is 4 ahead

    branch_abs_word(cpu, target);
}

MIPS_INSTR(mips_jal) {
    link(cpu);

    word target = instruction.j.target;
    target <<= 2;
    target |= ((cpu->pc - 4) & 0xF0000000); // PC is 4 ahead

    branch_abs_word(cpu, target);
}

MIPS_INSTR(mips_slti) {
    shalf immediate = instruction.i.immediate;
    logtrace("Set if %ld < %d", get_register(cpu, instruction.i.rs), immediate);
    sdword reg = get_register(cpu, instruction.i.rs);
    if (reg < immediate) {
        set_register(cpu, instruction.i.rt, 1);
    } else {
        set_register(cpu, instruction.i.rt, 0);
    }
}

MIPS_INSTR(mips_sltiu) {
    shalf immediate = instruction.i.immediate;
    logtrace("Set if %ld < %d", get_register(cpu, instruction.i.rs), immediate);
    if (get_register(cpu, instruction.i.rs) < immediate) {
        set_register(cpu, instruction.i.rt, 1);
    } else {
        set_register(cpu, instruction.i.rt, 0);
    }
}

MIPS_INSTR(mips_mfc0) {
    sword value = get_cp0_register_word(cpu, instruction.r.rd);
    set_register(cpu, instruction.r.rt, (sdword)value);
}

MIPS_INSTR(mips_mtc0) {
    word value = get_register(cpu, instruction.r.rt);
    set_cp0_register_word(cpu, instruction.r.rd, value);
}

MIPS_INSTR(mips_dmfc0) {
    dword value = get_cp0_register_dword(cpu, instruction.r.rd);
    set_register(cpu, instruction.r.rt, value);
}

MIPS_INSTR(mips_dmtc0) {
    dword value = get_register(cpu, instruction.r.rt);
    set_cp0_register_dword(cpu, instruction.r.rd, value);
}

#define checkcp1 do { if (!cpu->cp0.status.cu1) { r4300i_handle_exception(cpu, cpu->prev_pc, EXCEPTION_COPROCESSOR_UNUSABLE, 1); return; } } while(0)

MIPS_INSTR(mips_mfc1) {
    checkcp1;
    sword value = get_fpu_register_word(cpu, instruction.fr.fs);
    set_register(cpu, instruction.r.rt, (sdword)value);
}

MIPS_INSTR(mips_dmfc1) {
    checkcp1;
    dword value = get_fpu_register_dword(cpu, instruction.fr.fs);
    set_register(cpu, instruction.r.rt, value);
}

MIPS_INSTR(mips_mtc1) {
    checkcp1;
    word value = get_register(cpu, instruction.r.rt);
    set_fpu_register_word(cpu, instruction.r.rd, value);
}

MIPS_INSTR(mips_dmtc1) {
    checkcp1;
    dword value = get_register(cpu, instruction.r.rt);
    set_fpu_register_dword(cpu, instruction.r.rd, value);
}

MIPS_INSTR(mips_eret) {
    if (cpu->cp0.status.erl) {
        set_pc_dword_r4300i(cpu, cpu->cp0.error_epc);
        cpu->cp0.status.erl = false;
    } else {
        set_pc_dword_r4300i(cpu, cpu->cp0.EPC);
        cpu->cp0.status.exl = false;
    }
    cp0_status_updated(cpu);
    cpu->llbit = false;
}

MIPS_INSTR(mips_cfc1) {
    checkcp1;
    byte fs = instruction.r.rd;
    sword value;
    switch (fs) {
        case 0:
            logwarn("Reading FCR0 - probably returning an invalid value!");
            value = cpu->fcr0.raw;
            break;
        case 31:
            value = cpu->fcr31.raw;
            break;
        default:
            logfatal("This instruction is only defined when fs == 0 or fs == 31! (Throw an exception?)");
    }

    set_register(cpu, instruction.r.rt, (sdword)value);
}

INLINE void check_fpu_exception(r4300i_t* cpu) {
    if (cpu->fcr31.cause & (0b100000 | cpu->fcr31.enable)) {
        logfatal("FPU exception");
    }
}

MIPS_INSTR(mips_ctc1) {
    checkcp1;
    byte fs = instruction.r.rd;
    word value = get_register(cpu, instruction.r.rt);
    switch (fs) {
        case 0:
            logfatal("CTC1 FCR0: FCR0 is read only!");
        case 31: {
            value &= 0x183ffff; // mask out bits held 0
            cpu->fcr31.raw = value;
            check_fpu_exception(cpu);
            break;
        }
        default:
            logfatal("This instruction is only defined when fs == 0 or fs == 31! (Throw an exception?)");
    }
}

MIPS_INSTR(mips_cp_bc1f) {
    checkcp1;
    conditional_branch(cpu, instruction.i.immediate, !cpu->fcr31.compare);
}

MIPS_INSTR(mips_cp_bc1fl) {
    checkcp1;
    conditional_branch_likely(cpu, instruction.i.immediate, !cpu->fcr31.compare);
}

MIPS_INSTR(mips_cp_bc1t) {
    checkcp1;
    conditional_branch(cpu, instruction.i.immediate, cpu->fcr31.compare);
}
MIPS_INSTR(mips_cp_bc1tl) {
    checkcp1;
    conditional_branch_likely(cpu, instruction.i.immediate, cpu->fcr31.compare);
}

MIPS_INSTR(mips_cp_mul_d) {
    checkcp1;
    double fs = get_fpu_register_double(cpu, instruction.fr.fs);
    double ft = get_fpu_register_double(cpu, instruction.fr.ft);
    double result = fs * ft;
    set_fpu_register_double(cpu, instruction.fr.fd, result);
}

MIPS_INSTR(mips_cp_mul_s) {
    checkcp1;
    float fs = get_fpu_register_float(cpu, instruction.fr.fs);
    float ft = get_fpu_register_float(cpu, instruction.fr.ft);
    float result = fs * ft;
    set_fpu_register_float(cpu, instruction.fr.fd, result);
}

MIPS_INSTR(mips_cp_div_d) {
    checkcp1;
    double fs = get_fpu_register_double(cpu, instruction.fr.fs);
    double ft = get_fpu_register_double(cpu, instruction.fr.ft);
    double result = fs / ft;
    set_fpu_register_double(cpu, instruction.fr.fd, result);
}

MIPS_INSTR(mips_cp_div_s) {
    checkcp1;
    float fs = get_fpu_register_float(cpu, instruction.fr.fs);
    float ft = get_fpu_register_float(cpu, instruction.fr.ft);
    float result = fs / ft;
    set_fpu_register_float(cpu, instruction.fr.fd, result);
}

MIPS_INSTR(mips_cp_add_d) {
    checkcp1;
    double fs = get_fpu_register_double(cpu, instruction.fr.fs);
    double ft = get_fpu_register_double(cpu, instruction.fr.ft);
    double result = fs + ft;
    set_fpu_register_double(cpu, instruction.fr.fd, result);
}

MIPS_INSTR(mips_cp_add_s) {
    checkcp1;
    float fs = get_fpu_register_float(cpu, instruction.fr.fs);
    float ft = get_fpu_register_float(cpu, instruction.fr.ft);
    float result = fs + ft;
    set_fpu_register_float(cpu, instruction.fr.fd, result);
}

MIPS_INSTR(mips_cp_sub_d) {
    checkcp1;
    double fs = get_fpu_register_double(cpu, instruction.fr.fs);
    double ft = get_fpu_register_double(cpu, instruction.fr.ft);
    double result = fs - ft;
    set_fpu_register_double(cpu, instruction.fr.fd, result);
}

MIPS_INSTR(mips_cp_sub_s) {
    checkcp1;
    float fs = get_fpu_register_float(cpu, instruction.fr.fs);
    float ft = get_fpu_register_float(cpu, instruction.fr.ft);
    float result = fs - ft;
    set_fpu_register_float(cpu, instruction.fr.fd, result);
}

MIPS_INSTR(mips_cp_trunc_l_d) {
    checkcp1;
    double value = get_fpu_register_double(cpu, instruction.fr.fs);
    dword truncated = value;
    set_fpu_register_dword(cpu, instruction.fr.fd, truncated);
}

MIPS_INSTR(mips_cp_round_l_d) {
    logwarn("mips_cp_round_l_d used: this instruction is known to be buggy!");
    checkcp1;
    double value = get_fpu_register_double(cpu, instruction.fr.fs);
    dword truncated = value;
    set_fpu_register_dword(cpu, instruction.fr.fd, truncated);
}

MIPS_INSTR(mips_cp_trunc_l_s) {
    checkcp1;
    float value = get_fpu_register_float(cpu, instruction.fr.fs);
    dword truncated = value;
    set_fpu_register_dword(cpu, instruction.fr.fd, truncated);
}

MIPS_INSTR(mips_cp_round_l_s) {
    logwarn("mips_cp_round_l_s used: this instruction is known to be buggy!");
    checkcp1;
    float value = get_fpu_register_float(cpu, instruction.fr.fs);
    dword truncated = value;
    set_fpu_register_dword(cpu, instruction.fr.fd, truncated);
}

MIPS_INSTR(mips_cp_trunc_w_d) {
    checkcp1;
    double value = get_fpu_register_double(cpu, instruction.fr.fs);
    word truncated = value;
    set_fpu_register_word(cpu, instruction.fr.fd, truncated);
}

MIPS_INSTR(mips_cp_round_w_d) {
    logwarn("mips_cp_round_w_d used: this instruction is known to be buggy!");
    checkcp1;
    double value = get_fpu_register_double(cpu, instruction.fr.fs);
    word truncated = value;
    set_fpu_register_word(cpu, instruction.fr.fd, truncated);
}

MIPS_INSTR(mips_cp_trunc_w_s) {
    checkcp1;
    float value = get_fpu_register_float(cpu, instruction.fr.fs);
    sword truncated = truncf(value);
    set_fpu_register_word(cpu, instruction.fr.fd, truncated);
}

MIPS_INSTR(mips_cp_round_w_s) {
    logwarn("mips_cp_round_w_s used: this instruction is known to be buggy!");
    checkcp1;
    float value = get_fpu_register_float(cpu, instruction.fr.fs);
    sword truncated = truncf(value);
    set_fpu_register_word(cpu, instruction.fr.fd, truncated);
}

MIPS_INSTR(mips_cp_cvt_d_s) {
    checkcp1;
    float fs = get_fpu_register_float(cpu, instruction.fr.fs);
    double converted = fs;
    set_fpu_register_double(cpu, instruction.fr.fd, converted);
}

MIPS_INSTR(mips_cp_cvt_d_w) {
    checkcp1;
    sword fs = get_fpu_register_word(cpu, instruction.fr.fs);
    double converted = fs;
    set_fpu_register_double(cpu, instruction.fr.fd, converted);
}

MIPS_INSTR(mips_cp_cvt_d_l) {
    checkcp1;
    sdword fs = get_fpu_register_dword(cpu, instruction.fr.fs);
    double converted = fs;
    set_fpu_register_double(cpu, instruction.fr.fd, converted);
}

MIPS_INSTR(mips_cp_cvt_l_s) {
    checkcp1;
    float fs = get_fpu_register_float(cpu, instruction.fr.fs);
    sdword converted = fs;
    set_fpu_register_dword(cpu, instruction.fr.fd, converted);
}

MIPS_INSTR(mips_cp_cvt_l_d) {
    checkcp1;
    double fs = get_fpu_register_double(cpu, instruction.fr.fs);
    sdword converted = fs;
    set_fpu_register_dword(cpu, instruction.fr.fd, converted);
}

MIPS_INSTR(mips_cp_cvt_s_d) {
    checkcp1;
    double fs = get_fpu_register_double(cpu, instruction.fr.fs);
    float converted = fs;
    set_fpu_register_float(cpu, instruction.fr.fd, converted);
}

MIPS_INSTR(mips_cp_cvt_s_w) {
    checkcp1;
    sword fs = get_fpu_register_word(cpu, instruction.fr.fs);
    float converted = fs;
    set_fpu_register_float(cpu, instruction.fr.fd, converted);
}

MIPS_INSTR(mips_cp_cvt_s_l) {
    checkcp1;
    sdword fs = get_fpu_register_dword(cpu, instruction.fr.fs);
    float converted = fs;
    set_fpu_register_float(cpu, instruction.fr.fd, converted);
}

MIPS_INSTR(mips_cp_cvt_w_s) {
    checkcp1;
    float fs = get_fpu_register_float(cpu, instruction.fr.fs);
    sword converted = fs;
    set_fpu_register_word(cpu, instruction.fr.fd, converted);
}

MIPS_INSTR(mips_cp_cvt_w_d) {
    checkcp1;
    double fs = get_fpu_register_double(cpu, instruction.fr.fs);
    sword converted = fs;
    set_fpu_register_word(cpu, instruction.fr.fd, converted);
}

MIPS_INSTR(mips_cp_sqrt_s) {
    checkcp1;
    float fs = get_fpu_register_float(cpu, instruction.fr.fs);
    float root = sqrt(fs);
    set_fpu_register_float(cpu, instruction.fr.fd, root);
}

MIPS_INSTR(mips_cp_sqrt_d) {
    checkcp1;
    double fs = get_fpu_register_double(cpu, instruction.fr.fs);
    double root = sqrt(fs);
    set_fpu_register_double(cpu, instruction.fr.fd, root);
}

MIPS_INSTR(mips_cp_abs_s) {
    checkcp1;
    float fs = get_fpu_register_float(cpu, instruction.fr.fs);
    if (fs < 0) {
        fs = -fs;
    }
    set_fpu_register_float(cpu, instruction.fr.fd, fs);
}

MIPS_INSTR(mips_cp_abs_d) {
    checkcp1;
    double fs = get_fpu_register_double(cpu, instruction.fr.fs);
    if (fs < 0) {
        fs = -fs;
    }
    set_fpu_register_double(cpu, instruction.fr.fd, fs);
}

MIPS_INSTR(mips_cp_c_f_s) {
    checkcp1;
    /*
    float fs = get_fpu_register_float(cpu, instruction.fr.fs);
    float ft = get_fpu_register_float(cpu, instruction.fr.ft);
     */
    logfatal("Unimplemented: mips_cp_c_f_s");
}
MIPS_INSTR(mips_cp_c_un_s) {
    checkcp1;
    /*
    float fs = get_fpu_register_float(cpu, instruction.fr.fs);
    float ft = get_fpu_register_float(cpu, instruction.fr.ft);
     */
    logfatal("Unimplemented: mips_cp_c_un_s");
}
MIPS_INSTR(mips_cp_c_eq_s) {
    checkcp1;
    float fs = get_fpu_register_float(cpu, instruction.fr.fs);
    float ft = get_fpu_register_float(cpu, instruction.fr.ft);

    unimplemented(isnanf(fs), "fs is nan");
    unimplemented(isnanf(ft), "ft is nan");

    cpu->fcr31.compare = fs == ft;
}
MIPS_INSTR(mips_cp_c_ueq_s) {
    checkcp1;
    float fs = get_fpu_register_float(cpu, instruction.fr.fs);
    float ft = get_fpu_register_float(cpu, instruction.fr.ft);
    UNORDERED_S(fs, ft);

    cpu->fcr31.compare = fs == ft;
}

MIPS_INSTR(mips_cp_c_olt_s) {
    checkcp1;
    float fs = get_fpu_register_float(cpu, instruction.fr.fs);
    float ft = get_fpu_register_float(cpu, instruction.fr.ft);
    ORDERED_S(fs, ft);

    cpu->fcr31.compare = fs < ft;
}

MIPS_INSTR(mips_cp_c_ult_s) {
    checkcp1;
    float fs = get_fpu_register_float(cpu, instruction.fr.fs);
    float ft = get_fpu_register_float(cpu, instruction.fr.ft);
    UNORDERED_S(fs, ft);
    cpu->fcr31.compare = fs < ft;
}
MIPS_INSTR(mips_cp_c_ole_s) {
    checkcp1;
    float fs = get_fpu_register_float(cpu, instruction.fr.fs);
    float ft = get_fpu_register_float(cpu, instruction.fr.ft);
    ORDERED_S(fs, ft);

    cpu->fcr31.compare = fs <= ft;
}
MIPS_INSTR(mips_cp_c_ule_s) {
    checkcp1;
    float fs = get_fpu_register_float(cpu, instruction.fr.fs);
    float ft = get_fpu_register_float(cpu, instruction.fr.ft);
    UNORDERED_S(fs, ft);
    cpu->fcr31.compare = fs <= ft;
}
MIPS_INSTR(mips_cp_c_sf_s) {
    checkcp1;
    /*
    float fs = get_fpu_register_float(cpu, instruction.fr.fs);
    float ft = get_fpu_register_float(cpu, instruction.fr.ft);
     */
    logfatal("Unimplemented: mips_cp_c_sf_s");
}
MIPS_INSTR(mips_cp_c_ngle_s) {
    checkcp1;
    /*
    float fs = get_fpu_register_float(cpu, instruction.fr.fs);
    float ft = get_fpu_register_float(cpu, instruction.fr.ft);
     */
    logfatal("Unimplemented: mips_cp_c_ngle_s");
}
MIPS_INSTR(mips_cp_c_seq_s) {
    checkcp1;
    /*
    float fs = get_fpu_register_float(cpu, instruction.fr.fs);
    float ft = get_fpu_register_float(cpu, instruction.fr.ft);
     */
    logfatal("Unimplemented: mips_cp_c_seq_s");
}
MIPS_INSTR(mips_cp_c_ngl_s) {
    checkcp1;
    /*
    float fs = get_fpu_register_float(cpu, instruction.fr.fs);
    float ft = get_fpu_register_float(cpu, instruction.fr.ft);
     */
    logfatal("Unimplemented: mips_cp_c_ngl_s");
}
MIPS_INSTR(mips_cp_c_lt_s) {
    checkcp1;
    float fs = get_fpu_register_float(cpu, instruction.fr.fs);
    float ft = get_fpu_register_float(cpu, instruction.fr.ft);

    unimplemented(isnanf(fs), "fs is nan");
    unimplemented(isnanf(ft), "ft is nan");

    cpu->fcr31.compare = fs < ft;
}
MIPS_INSTR(mips_cp_c_nge_s) {
    checkcp1;
    /*
    float fs = get_fpu_register_float(cpu, instruction.fr.fs);
    float ft = get_fpu_register_float(cpu, instruction.fr.ft);
     */
    logfatal("Unimplemented: mips_cp_c_nge_s");
}
MIPS_INSTR(mips_cp_c_le_s) {
    checkcp1;
    float fs = get_fpu_register_float(cpu, instruction.fr.fs);
    float ft = get_fpu_register_float(cpu, instruction.fr.ft);

    unimplemented(isnanf(fs), "fs is nan");
    unimplemented(isnanf(ft), "ft is nan");

    cpu->fcr31.compare = fs <= ft;
}
MIPS_INSTR(mips_cp_c_ngt_s) {
    checkcp1;
    /*
    float fs = get_fpu_register_float(cpu, instruction.fr.fs);
    float ft = get_fpu_register_float(cpu, instruction.fr.ft);
     */
    logfatal("Unimplemented: mips_cp_c_ngt_s");
}

MIPS_INSTR(mips_cp_c_f_d) {
    checkcp1;
    /*
    double fs = get_fpu_register_double(cpu, instruction.fr.fs);
    double ft = get_fpu_register_double(cpu, instruction.fr.ft);
     */
    logfatal("Unimplemented: mips_cp_c_f_d");
}
MIPS_INSTR(mips_cp_c_un_d) {
    checkcp1;
    /*
    double fs = get_fpu_register_double(cpu, instruction.fr.fs);
    double ft = get_fpu_register_double(cpu, instruction.fr.ft);
     */
    logfatal("Unimplemented: mips_cp_c_un_d");
}
MIPS_INSTR(mips_cp_c_eq_d) {
    checkcp1;
    double fs = get_fpu_register_double(cpu, instruction.fr.fs);
    double ft = get_fpu_register_double(cpu, instruction.fr.ft);

    unimplemented(isnan(fs), "fs is nan");
    unimplemented(isnan(ft), "ft is nan");

    cpu->fcr31.compare = fs == ft;
}
MIPS_INSTR(mips_cp_c_ueq_d) {
    checkcp1;
    double fs = get_fpu_register_double(cpu, instruction.fr.fs);
    double ft = get_fpu_register_double(cpu, instruction.fr.ft);

    UNORDERED_D(fs, ft);

    cpu->fcr31.compare = fs == ft;
}
MIPS_INSTR(mips_cp_c_olt_d) {
    checkcp1;
    /*
    double fs = get_fpu_register_double(cpu, instruction.fr.fs);
    double ft = get_fpu_register_double(cpu, instruction.fr.ft);
     */
    logfatal("Unimplemented: mips_cp_c_olt_d");
}
MIPS_INSTR(mips_cp_c_ult_d) {
    checkcp1;
    double fs = get_fpu_register_double(cpu, instruction.fr.fs);
    double ft = get_fpu_register_double(cpu, instruction.fr.ft);
    UNORDERED_D(fs, ft);
    cpu->fcr31.compare = fs < ft;
    logfatal("Unimplemented: mips_cp_c_ult_d");
}
MIPS_INSTR(mips_cp_c_ole_d) {
    checkcp1;
    double fs = get_fpu_register_double(cpu, instruction.fr.fs);
    double ft = get_fpu_register_double(cpu, instruction.fr.ft);
    ORDERED_D(fs, ft);
    cpu->fcr31.compare = fs <= ft;
}
MIPS_INSTR(mips_cp_c_ule_d) {
    checkcp1;
    double fs = get_fpu_register_double(cpu, instruction.fr.fs);
    double ft = get_fpu_register_double(cpu, instruction.fr.ft);

    UNORDERED_D(fs, ft);

    cpu->fcr31.compare = fs <= ft;
}
MIPS_INSTR(mips_cp_c_sf_d) {
    checkcp1;
    /*
    double fs = get_fpu_register_double(cpu, instruction.fr.fs);
    double ft = get_fpu_register_double(cpu, instruction.fr.ft);
     */
    logfatal("Unimplemented: mips_cp_c_sf_d");
}
MIPS_INSTR(mips_cp_c_ngle_d) {
    checkcp1;
    /*
    double fs = get_fpu_register_double(cpu, instruction.fr.fs);
    double ft = get_fpu_register_double(cpu, instruction.fr.ft);
     */
    logfatal("Unimplemented: mips_cp_c_ngle_d");
}
MIPS_INSTR(mips_cp_c_seq_d) {
    checkcp1;
    /*
    double fs = get_fpu_register_double(cpu, instruction.fr.fs);
    double ft = get_fpu_register_double(cpu, instruction.fr.ft);
     */
    logfatal("Unimplemented: mips_cp_c_seq_d");
}
MIPS_INSTR(mips_cp_c_ngl_d) {
    checkcp1;
    /*
    double fs = get_fpu_register_double(cpu, instruction.fr.fs);
    double ft = get_fpu_register_double(cpu, instruction.fr.ft);
     */
    logfatal("Unimplemented: mips_cp_c_ngl_d");
}
MIPS_INSTR(mips_cp_c_lt_d) {
    checkcp1;
    double fs = get_fpu_register_double(cpu, instruction.fr.fs);
    double ft = get_fpu_register_double(cpu, instruction.fr.ft);

    unimplemented(isnan(fs), "fs is nan");
    unimplemented(isnan(ft), "ft is nan");

    cpu->fcr31.compare = fs < ft;
}
MIPS_INSTR(mips_cp_c_nge_d) {
    checkcp1;
    /*
    double fs = get_fpu_register_double(cpu, instruction.fr.fs);
    double ft = get_fpu_register_double(cpu, instruction.fr.ft);
     */
    logfatal("Unimplemented: mips_cp_c_nge_d");
}
MIPS_INSTR(mips_cp_c_le_d) {
    checkcp1;
    double fs = get_fpu_register_double(cpu, instruction.fr.fs);
    double ft = get_fpu_register_double(cpu, instruction.fr.ft);

    unimplemented(isnan(fs), "fs is nan");
    unimplemented(isnan(ft), "ft is nan");

    cpu->fcr31.compare = fs <= ft;
}
MIPS_INSTR(mips_cp_c_ngt_d) {
    checkcp1;
    /*
    double fs = get_fpu_register_double(cpu, instruction.fr.fs);
    double ft = get_fpu_register_double(cpu, instruction.fr.ft);
     */
    logfatal("Unimplemented: mips_cp_c_ngt_d");
}

MIPS_INSTR(mips_cp_mov_s) {
    checkcp1;
    float value = get_fpu_register_float(cpu, instruction.fr.fs);
    set_fpu_register_float(cpu, instruction.fr.fd, value);
}

MIPS_INSTR(mips_cp_mov_d) {
    checkcp1;
    double value = get_fpu_register_double(cpu, instruction.fr.fs);
    set_fpu_register_double(cpu, instruction.fr.fd, value);
}

MIPS_INSTR(mips_cp_neg_s) {
    checkcp1;
    float value = get_fpu_register_float(cpu, instruction.fr.fs);
    set_fpu_register_float(cpu, instruction.fr.fd, -value);
}

MIPS_INSTR(mips_cp_neg_d) {
    checkcp1;
    double value = get_fpu_register_double(cpu, instruction.fr.fs);
    set_fpu_register_double(cpu, instruction.fr.fd, -value);
}

MIPS_INSTR(mips_ld) {
    shalf offset = instruction.i.immediate;
    dword address = get_register(cpu, instruction.i.rs) + offset;
    dword result  = cpu->read_dword(address);
    if ((address & 0b111) > 0) {
        logfatal("TODO: throw an 'address error' exception! Tried to load from unaligned address 0x%016lX", address);
    }
    set_register(cpu, instruction.i.rt, result);
}

MIPS_INSTR(mips_lui) {
    // Done this way to avoid the undefined behavior of left shifting a signed integer
    // Should compile to a left shift by 16.
    sdword value = (shalf)instruction.i.immediate;
    value *= 65536;

    set_register(cpu, instruction.i.rt, value);
}

MIPS_INSTR(mips_lbu) {
    shalf offset = instruction.i.immediate;
    logtrace("LBU offset: %d", offset);
    dword address = get_register(cpu, instruction.i.rs) + offset;
    byte value    = cpu->read_byte(address);

    set_register(cpu, instruction.i.rt, value); // zero extend
}

MIPS_INSTR(mips_lhu) {
    shalf offset = instruction.i.immediate;
    logtrace("LHU offset: %d", offset);
    dword address = get_register(cpu, instruction.i.rs) + offset;
    half value    = cpu->read_half(address);
    if ((address & 0b1) > 0) {
        logfatal("TODO: throw an 'address error' exception! Tried to load from unaligned address 0x%016lX", address);
    }

    set_register(cpu, instruction.i.rt, value); // zero extend
}

MIPS_INSTR(mips_lh) {
    shalf offset = instruction.i.immediate;
    dword address = get_register(cpu, instruction.i.rs) + offset;
    shalf value   = cpu->read_half(address);
    if ((address & 0b1) > 0) {
        logfatal("TODO: throw an 'address error' exception! Tried to load from unaligned address 0x%016lX", address);
    }

    set_register(cpu, instruction.i.rt, (sdword)value);
}

MIPS_INSTR(mips_lw) {
    shalf offset  = instruction.i.immediate;
    dword address = get_register(cpu, instruction.i.rs) + offset;
    if ((address & 0b11) > 0) {
        logfatal("TODO: throw an 'address error' exception! Tried to load from unaligned address 0x%016lX", address);
    }

    sword value = cpu->read_word(address);
    set_register(cpu, instruction.i.rt, (sdword)value);
}

MIPS_INSTR(mips_lwu) {
    shalf offset  = instruction.i.immediate;
    dword address = get_register(cpu, instruction.i.rs) + offset;
    if ((address & 0b11) > 0) {
        logfatal("TODO: throw an 'address error' exception! Tried to load from unaligned address 0x%016lX", address);
    }

    word value = cpu->read_word(address);
    set_register(cpu, instruction.i.rt, value);
}

MIPS_INSTR(mips_sb) {
    shalf offset  = instruction.i.immediate;
    dword address = get_register(cpu, instruction.i.rs);
    address += offset;
    byte value = get_register(cpu, instruction.i.rt) & 0xFF;
    cpu->write_byte(address, value);
}

MIPS_INSTR(mips_sh) {
    shalf offset  = instruction.i.immediate;
    dword address = get_register(cpu, instruction.i.rs);
    address += offset;
    half value = get_register(cpu, instruction.i.rt);
    cpu->write_half(address, value);
}

MIPS_INSTR(mips_sw) {
    shalf offset  = instruction.i.immediate;
    dword address = get_register(cpu, instruction.i.rs);
    address += offset;
    cpu->write_word(address, get_register(cpu, instruction.i.rt));
}

MIPS_INSTR(mips_sd) {
    shalf offset  = instruction.i.immediate;
    dword address = get_register(cpu, instruction.i.rs) + offset;
    dword value = get_register(cpu, instruction.i.rt);
    cpu->write_dword(address, value);
}

MIPS_INSTR(mips_ori) {
    set_register(cpu, instruction.i.rt, instruction.i.immediate | get_register(cpu, instruction.i.rs));
}

MIPS_INSTR(mips_xori) {
    set_register(cpu, instruction.i.rt, instruction.i.immediate ^ get_register(cpu, instruction.i.rs));
}

MIPS_INSTR(mips_daddiu) {
    shalf  addend1 = instruction.i.immediate;
    sdword addend2 = get_register(cpu, instruction.i.rs);
    sdword result = addend1 + addend2;
    set_register(cpu, instruction.i.rt, result);
}

MIPS_INSTR(mips_lb) {
    shalf offset  = instruction.i.immediate;
    dword address = get_register(cpu, instruction.i.rs) + offset;
    sbyte value   = cpu->read_byte(address);

    set_register(cpu, instruction.i.rt, (sdword)value);
}

MIPS_INSTR(mips_ldc1) {
    checkcp1;
    shalf offset  = instruction.i.immediate;
    dword address = get_register(cpu, instruction.i.rs) + offset;
    if (address & 0b111) {
        logfatal("Address error exception: misaligned dword read!");
    }

    dword value = cpu->read_dword(address);
    set_fpu_register_dword(cpu, instruction.i.rt, value);
}

MIPS_INSTR(mips_sdc1) {
    checkcp1;
    shalf offset  = instruction.fi.offset;
    dword address = get_register(cpu, instruction.fi.base) + offset;
    dword value   = get_fpu_register_dword(cpu, instruction.fi.ft);

    cpu->write_dword(address, value);
}

MIPS_INSTR(mips_lwc1) {
    checkcp1;
    shalf offset  = instruction.fi.offset;
    dword address = get_register(cpu, instruction.fi.base) + offset;
    word value    = cpu->read_word(address);

    set_fpu_register_word(cpu, instruction.fi.ft, value);
}

MIPS_INSTR(mips_swc1) {
    checkcp1;
    shalf offset  = instruction.fi.offset;
    dword address = get_register(cpu, instruction.fi.base) + offset;
    word value    = get_fpu_register_word(cpu, instruction.fi.ft);

    cpu->write_word(address, value);
}

MIPS_INSTR(mips_lwl) {
    shalf offset  = instruction.fi.offset;
    dword address = get_register(cpu, instruction.fi.base) + offset;

    word shift = 8 * ((address ^ 0) & 3);
    word mask = 0xFFFFFFFF << shift;
    word data = cpu->read_word(address & ~3);
    sword result = (get_register(cpu, instruction.i.rt) & ~mask) | data << shift;
    set_register(cpu, instruction.i.rt, (sdword)result);
}

MIPS_INSTR(mips_lwr) {
    shalf offset  = instruction.fi.offset;
    dword address = get_register(cpu, instruction.fi.base) + offset;

    word shift = 8 * ((address ^ 3) & 3);

    word mask = 0xFFFFFFFF >> shift;
    word data = cpu->read_word(address & ~3);
    sword result = (get_register(cpu, instruction.i.rt) & ~mask) | data >> shift;
    set_register(cpu, instruction.i.rt, (sdword)result);
}

MIPS_INSTR(mips_swl) {
    shalf offset  = instruction.fi.offset;
    dword address = get_register(cpu, instruction.fi.base) + offset;

    word shift = 8 * ((address ^ 0) & 3);
    word mask = 0xFFFFFFFF >> shift;
    word data = cpu->read_word(address & ~3);
    word oldreg = get_register(cpu, instruction.i.rt);
    cpu->write_word(address & ~3, (data & ~mask) | (oldreg >> shift));
}

MIPS_INSTR(mips_swr) {
    shalf offset  = instruction.fi.offset;
    dword address = get_register(cpu, instruction.fi.base) + offset;

    word shift = 8 * ((address ^ 3) & 3);
    word mask = 0xFFFFFFFF << shift;
    word data = cpu->read_word(address & ~3);
    word oldreg = get_register(cpu, instruction.i.rt);
    cpu->write_word(address & ~3, (data & ~mask) | oldreg << shift);
}

MIPS_INSTR(mips_ldl) {
    shalf offset = instruction.fi.offset;
    dword address = get_register(cpu, instruction.fi.base) + offset;
    int shift = 8 * ((address ^ 0) & 7);
    dword mask = (dword)0xFFFFFFFFFFFFFFFF << shift;
    dword data = cpu->read_dword(address & ~7);
    dword oldreg = get_register(cpu, instruction.i.rt);

    set_register(cpu, instruction.i.rt, (oldreg & ~mask) | (data << shift));
}

MIPS_INSTR(mips_ldr) {
    shalf offset = instruction.fi.offset;
    dword address = get_register(cpu, instruction.fi.base) + offset;
    int shift = 8 * ((address ^ 7) & 7);
    dword mask = (dword)0xFFFFFFFFFFFFFFFF >> shift;
    dword data = cpu->read_dword(address & ~7);
    dword oldreg = get_register(cpu, instruction.i.rt);

    set_register(cpu, instruction.i.rt, (oldreg & ~mask) | (data >> shift));
}

MIPS_INSTR(mips_sdl) {
    shalf offset = instruction.fi.offset;
    dword address = get_register(cpu, instruction.fi.base) + offset;

    int shift = 8 * ((address ^ 0) & 7);
    dword mask = 0xFFFFFFFFFFFFFFFF;
    mask >>= shift;
    dword data = cpu->read_dword(address & ~7);
    dword oldreg = get_register(cpu, instruction.i.rt);
    cpu->write_dword(address & ~7, (data & ~mask) | (oldreg >> shift));
}

MIPS_INSTR(mips_sdr) {
    shalf offset = instruction.fi.offset;
    dword address = get_register(cpu, instruction.fi.base) + offset;

    int shift = 8 * ((address ^ 7) & 7);
    dword mask = 0xFFFFFFFFFFFFFFFF ;
    mask <<= shift;
    dword data = cpu->read_dword(address & ~7);
    dword oldreg = get_register(cpu, instruction.i.rt);
    cpu->write_dword(address & ~7, (data & ~mask) | (oldreg << shift));
}

MIPS_INSTR(mips_ll) {
    // Identical to lw
    shalf offset = instruction.i.immediate;
    dword address = get_register(cpu, instruction.i.rs) + offset;
    sword result  = cpu->read_word(address);

    if ((address & 0b11) > 0) {
        logfatal("TODO: throw an 'address error' exception! Tried to load from unaligned address 0x%016lX", address);
    }

    set_register(cpu, instruction.i.rt, (sdword)result);

    // Unique to ll
    cpu->cp0.lladdr = cpu->resolve_virtual_address(address, &cpu->cp0);
    cpu->llbit = true;
}

MIPS_INSTR(mips_lld) {
    // Instruction is undefined outside of 64 bit mode and 32 bit kernel mode.
    // Throw an exception if we're not in 64 bit mode AND not in kernel mode.
    if (!cpu->cp0.is_64bit_addressing && cpu->cp0.kernel_mode) {
        logfatal("LLD is undefined outside of 64 bit mode and 32 bit kernel mode. Throw a reserved instruction exception!");
    }

    // Identical to ld
    shalf offset = instruction.i.immediate;
    dword address = get_register(cpu, instruction.i.rs) + offset;
    dword result  = cpu->read_dword(address);

    if ((address & 0b111) > 0) {
        logfatal("TODO: throw an 'address error' exception! Tried to load from unaligned address 0x%016lX", address);
    }
    set_register(cpu, instruction.i.rt, result);

    // Unique to lld
    cpu->cp0.lladdr = cpu->resolve_virtual_address(address, &cpu->cp0);
    cpu->llbit = true;
}

MIPS_INSTR(mips_sc) {
    // Identical to sw
    shalf offset          = instruction.i.immediate;
    dword address         = get_register(cpu, instruction.i.rs) + offset;

    // Exception takes precedence over the instruction failing
    if ((address & 0b11) > 0) {
        logfatal("TODO: throw an 'address error' exception! Tried to store to unaligned address 0x%016lX", address);
    }

    if (cpu->llbit) {
        word physical_address = cpu->resolve_virtual_address(address, &cpu->cp0);

        if (physical_address != cpu->cp0.lladdr) {
            logfatal("Undefined: SC physical address is NOT EQUAL to last lladdr!\n");
        }

        word value = get_register(cpu, instruction.i.rt);
        cpu->write_word(address, value);
        set_register(cpu, instruction.i.rt, 1); // Success!

    } else {
        set_register(cpu, instruction.i.rt, 0); // Failure.
    }
}

MIPS_INSTR(mips_scd) {
    // Instruction is undefined outside of 64 bit mode and 32 bit kernel mode.
    // Throw an exception if we're not in 64 bit mode AND not in kernel mode.
    if (!cpu->cp0.is_64bit_addressing && cpu->cp0.kernel_mode) {
        logfatal("SCD is undefined outside of 64 bit mode and 32 bit kernel mode. Throw a reserved instruction exception!");
    }

    // Identical to sd
    shalf offset          = instruction.i.immediate;
    dword address         = get_register(cpu, instruction.i.rs) + offset;

    // Exception takes precedence over the instruction failing
    if ((address & 0b111) > 0) {
        logfatal("TODO: throw an 'address error' exception! Tried to store to unaligned address 0x%016lX", address);
    }

    if (cpu->llbit) {
        word physical_address = cpu->resolve_virtual_address(address, &cpu->cp0);

        if (physical_address != cpu->cp0.lladdr) {
            logfatal("Undefined: SCD physical address is NOT EQUAL to last lladdr!\n");
        }

        dword value = get_register(cpu, instruction.i.rt);
        cpu->write_dword(address, value);
        set_register(cpu, instruction.i.rt, 1); // Success!

    } else {
        set_register(cpu, instruction.i.rt, 0); // Failure.
    }
}

MIPS_INSTR(mips_spc_sll) {
    sword result = get_register(cpu, instruction.r.rt) << instruction.r.sa;
    set_register(cpu, instruction.r.rd, (sdword)result);
}

MIPS_INSTR(mips_spc_srl) {
    word value = get_register(cpu, instruction.r.rt);
    sword result = value >> instruction.r.sa;
    set_register(cpu, instruction.r.rd, (sdword) result);
}

MIPS_INSTR(mips_spc_sra) {
    sword value = get_register(cpu, instruction.r.rt);
    sword result = value >> instruction.r.sa;
    set_register(cpu, instruction.r.rd, (sdword) result);
}

MIPS_INSTR(mips_spc_srav) {
    sword value = get_register(cpu, instruction.r.rt);
    sword result = value >> (get_register(cpu, instruction.r.rs) & 0b11111);
    set_register(cpu, instruction.r.rd, (sdword) result);
}

MIPS_INSTR(mips_spc_sllv) {
    word value = get_register(cpu, instruction.r.rt);
    sword result = value << (get_register(cpu, instruction.r.rs) & 0b11111);
    set_register(cpu, instruction.r.rd, (sdword)result);
}

MIPS_INSTR(mips_spc_srlv) {
    word value = get_register(cpu, instruction.r.rt);
    sword result = value >> (get_register(cpu, instruction.r.rs) & 0b11111);
    set_register(cpu, instruction.r.rd, (sdword)result);
}

MIPS_INSTR(mips_spc_jr) {
    branch_abs(cpu, get_register(cpu, instruction.r.rs));
}

MIPS_INSTR(mips_spc_jalr) {
    link(cpu);
    branch_abs(cpu, get_register(cpu, instruction.r.rs));
}

MIPS_INSTR(mips_spc_mfhi) {
    set_register(cpu, instruction.r.rd, cpu->mult_hi);
}

MIPS_INSTR(mips_spc_mthi) {
    cpu->mult_hi = get_register(cpu, instruction.r.rs);
}

MIPS_INSTR(mips_spc_mflo) {
    set_register(cpu, instruction.r.rd, cpu->mult_lo);
}

MIPS_INSTR(mips_spc_mtlo) {
    cpu->mult_lo = get_register(cpu, instruction.r.rs);
}

MIPS_INSTR(mips_spc_dsllv) {
    dword val = get_register(cpu, instruction.r.rt);
    val <<= (get_register(cpu, instruction.r.rs) & 0b111111);
    set_register(cpu, instruction.r.rd, val);
}

MIPS_INSTR(mips_spc_dsrlv) {
    dword val = get_register(cpu, instruction.r.rt);
    val >>= (get_register(cpu, instruction.r.rs) & 0b111111);
    set_register(cpu, instruction.r.rd, val);
}

MIPS_INSTR(mips_spc_mult) {
    sword multiplicand_1 = get_register(cpu, instruction.r.rs);
    sword multiplicand_2 = get_register(cpu, instruction.r.rt);

    sdword dmultiplicand_1 = multiplicand_1;
    sdword dmultiplicand_2 = multiplicand_2;

    sdword result = dmultiplicand_1 * dmultiplicand_2;

    sword result_lower = result         & 0xFFFFFFFF;
    sword result_upper = (result >> 32) & 0xFFFFFFFF;

    cpu->mult_lo = (sdword)result_lower;
    cpu->mult_hi = (sdword)result_upper;
}

MIPS_INSTR(mips_spc_multu) {
    dword multiplicand_1 = get_register(cpu, instruction.r.rs) & 0xFFFFFFFF;
    dword multiplicand_2 = get_register(cpu, instruction.r.rt) & 0xFFFFFFFF;

    dword result = multiplicand_1 * multiplicand_2;

    sword result_lower = result         & 0xFFFFFFFF;
    sword result_upper = (result >> 32) & 0xFFFFFFFF;

    cpu->mult_lo = (sdword)result_lower;
    cpu->mult_hi = (sdword)result_upper;
}

MIPS_INSTR(mips_spc_div) {
    sdword dividend = get_register(cpu, instruction.r.rs);
    sdword divisor  = get_register(cpu, instruction.r.rt);

    if (divisor == 0) {
        logwarn("Divide by zero");
        cpu->mult_hi = dividend;
        if (dividend >= 0) {
            cpu->mult_lo = (sdword)-1;
        } else {
            cpu->mult_lo = (sdword)1;
        }
    } else {
        sdword quotient  = dividend / divisor;
        sdword remainder = dividend % divisor;

        cpu->mult_lo = quotient;
        cpu->mult_hi = remainder;
    }

}

MIPS_INSTR(mips_spc_divu) {
    dword dividend = get_register(cpu, instruction.r.rs);
    dword divisor  = get_register(cpu, instruction.r.rt);

    unimplemented(divisor == 0, "Divide by zero exception");

    sword quotient  = dividend / divisor;
    sword remainder = dividend % divisor;

    cpu->mult_lo = quotient;
    cpu->mult_hi = remainder;
}

MIPS_INSTR(mips_spc_dmult) {
    dword result_upper;
    dword result_lower = mult_64_to_128(get_register(cpu, instruction.r.rs), get_register(cpu, instruction.r.rt), &result_upper);

    cpu->mult_lo = result_lower;
    cpu->mult_hi = result_upper;
}

MIPS_INSTR(mips_spc_dmultu) {
    dword result_upper;
    dword result_lower = mult_64_to_128(get_register(cpu, instruction.r.rs), get_register(cpu, instruction.r.rt), &result_upper);

    cpu->mult_lo = result_lower;
    cpu->mult_hi = result_upper;
}

MIPS_INSTR(mips_spc_ddiv) {
    dword dividend = get_register(cpu, instruction.r.rs);
    dword divisor  = get_register(cpu, instruction.r.rt);

    unimplemented(divisor == 0, "Divide by zero exception");

    dword quotient  = dividend / divisor;
    dword remainder = dividend % divisor;

    cpu->mult_lo = quotient;
    cpu->mult_hi = remainder;
}

MIPS_INSTR(mips_spc_ddivu) {
    dword dividend = get_register(cpu, instruction.r.rs);
    dword divisor  = get_register(cpu, instruction.r.rt);

    unimplemented(divisor == 0, "Divide by zero exception");

    dword quotient  = dividend / divisor;
    dword remainder = dividend % divisor;

    cpu->mult_lo = quotient;
    cpu->mult_hi = remainder;
}

MIPS_INSTR(mips_spc_add) {

    sword addend1 = get_register(cpu, instruction.r.rs);
    sword addend2 = get_register(cpu, instruction.r.rt);

    sword result = addend1 + addend2;
    check_sword_add_overflow(addend1, addend2, result);

    set_register(cpu, instruction.r.rd, (sdword)result);
}

MIPS_INSTR(mips_spc_addu) {
    word rs = get_register(cpu, instruction.r.rs);
    word rt = get_register(cpu, instruction.r.rt);
    sword result = rs + rt;
    set_register(cpu, instruction.r.rd, (sdword)result);
}

MIPS_INSTR(mips_spc_and) {
    dword result = get_register(cpu, instruction.r.rs) & get_register(cpu, instruction.r.rt);
    set_register(cpu, instruction.r.rd, result);
}

MIPS_INSTR(mips_spc_nor) {
    dword result = ~(get_register(cpu, instruction.r.rs) | get_register(cpu, instruction.r.rt));
    set_register(cpu, instruction.r.rd, result);
}

MIPS_INSTR(mips_spc_sub) {
    sword operand1 = get_register(cpu, instruction.r.rs);
    sword operand2 = get_register(cpu, instruction.r.rt);

    sword result = operand1 - operand2;
    set_register(cpu, instruction.r.rd, (sdword)result);
}

MIPS_INSTR(mips_spc_subu) {
    word operand1 = get_register(cpu, instruction.r.rs);
    word operand2 = get_register(cpu, instruction.r.rt);

    sword result = operand1 - operand2;
    set_register(cpu, instruction.r.rd, (sdword)result);
}

MIPS_INSTR(mips_spc_or) {
    set_register(cpu, instruction.r.rd, get_register(cpu, instruction.r.rs) | get_register(cpu, instruction.r.rt));
}

MIPS_INSTR(mips_spc_xor) {
    set_register(cpu, instruction.r.rd, get_register(cpu, instruction.r.rs) ^ get_register(cpu, instruction.r.rt));
}

MIPS_INSTR(mips_spc_slt) {
    sdword op1 = get_register(cpu, instruction.r.rs);
    sdword op2 = get_register(cpu, instruction.r.rt);

    // RS - RT
    sdword result = op1 - op2;
    // if RS is LESS than RT
    // aka, if result is negative

    logtrace("Set if %ld < %ld", op1, op2);
    if (result < 0) {
        set_register(cpu, instruction.r.rd, 1);
    } else {
        set_register(cpu, instruction.r.rd, 0);
    }
}

MIPS_INSTR(mips_spc_sltu) {
    dword op1 = get_register(cpu, instruction.r.rs);
    dword op2 = get_register(cpu, instruction.r.rt);

    logtrace("Set if %lu < %lu", op1, op2);
    if (op1 < op2) {
        set_register(cpu, instruction.r.rd, 1);
    } else {
        set_register(cpu, instruction.r.rd, 0);
    }
}

MIPS_INSTR(mips_spc_dadd) {
    sdword addend1 = get_register(cpu, instruction.r.rs);
    sdword addend2 = get_register(cpu, instruction.r.rt);
    sdword result = addend1 + addend2;
    check_sdword_add_overflow(addend1, addend2, result);
    set_register(cpu, instruction.r.rd, result);
}

MIPS_INSTR(mips_spc_daddu) {
    sdword addend1 = get_register(cpu, instruction.r.rs);
    sdword addend2 = get_register(cpu, instruction.r.rt);
    sdword result = addend1 + addend2;
    set_register(cpu, instruction.r.rd, result);
}

MIPS_INSTR(mips_spc_dsubu) {
    sdword minuend = get_register(cpu, instruction.r.rs);
    sdword subtrahend = get_register(cpu, instruction.r.rt);
    sdword difference = minuend - subtrahend;
    set_register(cpu, instruction.r.rd, difference);
}

MIPS_INSTR(mips_spc_teq) {
    dword rs = get_register(cpu, instruction.r.rs);
    dword rt = get_register(cpu, instruction.r.rt);

    if (rs == rt) {
        r4300i_handle_exception(cpu, cpu->prev_pc, EXCEPTION_TRAP, -1);
    }
}

MIPS_INSTR(mips_spc_tne) {
    dword rs = get_register(cpu, instruction.r.rs);
    dword rt = get_register(cpu, instruction.r.rt);

    if (rs != rt) {
        r4300i_handle_exception(cpu, cpu->prev_pc, EXCEPTION_TRAP, -1);
    }
}

MIPS_INSTR(mips_spc_dsll) {
    dword value = get_register(cpu, instruction.r.rt);
    value <<= instruction.r.sa;
    set_register(cpu, instruction.r.rd, value);
}

MIPS_INSTR(mips_spc_dsrl) {
    dword value = get_register(cpu, instruction.r.rt);
    value >>= instruction.r.sa;
    set_register(cpu, instruction.r.rd, value);
}

MIPS_INSTR(mips_spc_dsra) {
    sdword value = get_register(cpu, instruction.r.rt);
    value >>= instruction.r.sa;
    set_register(cpu, instruction.r.rd, value);
}

MIPS_INSTR(mips_spc_dsll32) {
    dword value = get_register(cpu, instruction.r.rt);
    value <<= (instruction.r.sa + 32);
    set_register(cpu, instruction.r.rd, value);
}

MIPS_INSTR(mips_spc_dsrl32) {
    dword value = get_register(cpu, instruction.r.rt);
    value >>= (instruction.r.sa + 32);
    set_register(cpu, instruction.r.rd, value);
}

MIPS_INSTR(mips_spc_dsra32) {
    sdword value = get_register(cpu, instruction.r.rt);
    value >>= (instruction.r.sa + 32);
    set_register(cpu, instruction.r.rd, value);
}

MIPS_INSTR(mips_ri_bltz) {
    sdword reg = get_register(cpu, instruction.i.rs);
    conditional_branch(cpu, instruction.i.immediate, reg < 0);
}

MIPS_INSTR(mips_ri_bltzl) {
    sdword reg = get_register(cpu, instruction.i.rs);
    conditional_branch_likely(cpu, instruction.i.immediate, reg < 0);
}

MIPS_INSTR(mips_ri_bgez) {
    sdword reg = get_register(cpu, instruction.i.rs);
    conditional_branch(cpu, instruction.i.immediate, reg >= 0);
}

MIPS_INSTR(mips_ri_bgezl) {
    sdword reg = get_register(cpu, instruction.i.rs);
    conditional_branch_likely(cpu, instruction.i.immediate, reg >= 0);
}

MIPS_INSTR(mips_ri_bltzal) {
    link(cpu);
    sdword reg = get_register(cpu, instruction.i.rs);
    conditional_branch(cpu, instruction.i.immediate, reg < 0);
}

MIPS_INSTR(mips_ri_bgezal) {
    link(cpu);
    sdword reg = get_register(cpu, instruction.i.rs);
    conditional_branch(cpu, instruction.i.immediate, reg >= 0);
}
