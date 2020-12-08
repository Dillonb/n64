#include "mips_instructions.h"
#include "r4300i_register_access.h"

#include <math.h>

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
    sword pc = cpu->pc + 4;
    set_register(cpu, R4300I_REG_LR, (sdword)pc); // Skips the instruction in the delay slot on return
}

INLINE void branch_abs(r4300i_t* cpu, word address) {
    cpu->next_pc = address;
    cpu->branch = true;
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
        set_pc_r4300i(cpu, cpu->pc + 4);
    }
}

INLINE void conditional_branch(r4300i_t* cpu, word offset, bool condition) {
    if (condition) {
        branch_offset(cpu, offset);
    }
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

    branch_abs(cpu, target);
}

MIPS_INSTR(mips_jal) {
    link(cpu);

    word target = instruction.j.target;
    target <<= 2;
    target |= ((cpu->pc - 4) & 0xF0000000); // PC is 4 ahead

    branch_abs(cpu, target);
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
    sword value = get_cp0_register(cpu, instruction.r.rd);
    set_register(cpu, instruction.r.rt, (sdword)value);
}


MIPS_INSTR(mips_mtc0) {
    word value = get_register(cpu, instruction.r.rt);
    set_cp0_register(cpu, instruction.r.rd, value);
}

#define checkcp1 do { if (!cpu->cp0.status.cu1) { r4300i_handle_exception(cpu, cpu->prev_pc, EXCEPTION_COPROCESSOR_UNUSABLE, 1); } } while(0)

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
        set_pc_r4300i(cpu, cpu->cp0.error_epc);
        cpu->cp0.status.erl = false;
    } else {
        set_pc_r4300i(cpu, cpu->cp0.EPC);
        cpu->cp0.status.exl = false;
    }
}

MIPS_INSTR(mips_cfc1) {
    checkcp1;
    byte fs = instruction.r.rd;
    sword value;
    switch (fs) {
        case 0:
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

MIPS_INSTR(mips_ctc1) {
    checkcp1;
    byte fs = instruction.r.rd;
    word value = get_register(cpu, instruction.r.rt);
    switch (fs) {
        case 0:
            cpu->fcr0.raw = value;
            break;
        case 31:
            cpu->fcr31.raw = value;
            loginfo("TODO: possible exception here. See manual for CTC1");
            break;
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
    loginfo("mul.d: 0x%08X with fmt %d: %f * %f = %f", instruction.raw, instruction.fr.fmt, fs, ft, result);
}

MIPS_INSTR(mips_cp_mul_s) {
    checkcp1;
    float fs = get_fpu_register_float(cpu, instruction.fr.fs);
    float ft = get_fpu_register_float(cpu, instruction.fr.ft);
    float result = fs * ft;
    set_fpu_register_float(cpu, instruction.fr.fd, result);
    loginfo("mul.s: 0x%08X with fmt %d: %f * %f = %f", instruction.raw, instruction.fr.fmt, fs, ft, result);
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

MIPS_INSTR(mips_cp_trunc_l_s) {
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

MIPS_INSTR(mips_cp_trunc_w_s) {
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
    cpu->fcr31.compare = fs == ft;
}
MIPS_INSTR(mips_cp_c_ueq_s) {
    checkcp1;
    /*
    float fs = get_fpu_register_float(cpu, instruction.fr.fs);
    float ft = get_fpu_register_float(cpu, instruction.fr.ft);
     */
    logfatal("Unimplemented: mips_cp_c_ueq_s");
}
MIPS_INSTR(mips_cp_c_olt_s) {
    checkcp1;
    /*
    float fs = get_fpu_register_float(cpu, instruction.fr.fs);
    float ft = get_fpu_register_float(cpu, instruction.fr.ft);
     */
    logfatal("Unimplemented: mips_cp_c_olt_s");
}
MIPS_INSTR(mips_cp_c_ult_s) {
    checkcp1;
    /*
    float fs = get_fpu_register_float(cpu, instruction.fr.fs);
    float ft = get_fpu_register_float(cpu, instruction.fr.ft);
     */
    logfatal("Unimplemented: mips_cp_c_ult_s");
}
MIPS_INSTR(mips_cp_c_ole_s) {
    checkcp1;
    /*
    float fs = get_fpu_register_float(cpu, instruction.fr.fs);
    float ft = get_fpu_register_float(cpu, instruction.fr.ft);
     */
    logfatal("Unimplemented: mips_cp_c_ole_s");
}
MIPS_INSTR(mips_cp_c_ule_s) {
    checkcp1;
    /*
    float fs = get_fpu_register_float(cpu, instruction.fr.fs);
    float ft = get_fpu_register_float(cpu, instruction.fr.ft);
     */
    logfatal("Unimplemented: mips_cp_c_ule_s");
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
    cpu->fcr31.compare = fs == ft;
}
MIPS_INSTR(mips_cp_c_ueq_d) {
    checkcp1;
    /*
    double fs = get_fpu_register_double(cpu, instruction.fr.fs);
    double ft = get_fpu_register_double(cpu, instruction.fr.ft);
     */
    logfatal("Unimplemented: mips_cp_c_ueq_d");
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
    /*
    double fs = get_fpu_register_double(cpu, instruction.fr.fs);
    double ft = get_fpu_register_double(cpu, instruction.fr.ft);
     */
    logfatal("Unimplemented: mips_cp_c_ult_d");
}
MIPS_INSTR(mips_cp_c_ole_d) {
    checkcp1;
    /*
    double fs = get_fpu_register_double(cpu, instruction.fr.fs);
    double ft = get_fpu_register_double(cpu, instruction.fr.ft);
     */
    logfatal("Unimplemented: mips_cp_c_ole_d");
}
MIPS_INSTR(mips_cp_c_ule_d) {
    checkcp1;
    /*
    double fs = get_fpu_register_double(cpu, instruction.fr.fs);
    double ft = get_fpu_register_double(cpu, instruction.fr.ft);
     */
    logfatal("Unimplemented: mips_cp_c_ule_d");
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
    word address = get_register(cpu, instruction.i.rs) + offset;
    dword result = cpu->read_dword(address);
    if ((address & 0b111) > 0) {
        logfatal("TODO: throw an 'address error' exception! Tried to load from unaligned address 0x%08X", address);
    }
    set_register(cpu, instruction.i.rt, result);
}

MIPS_INSTR(mips_lui) {
    sword immediate = instruction.i.immediate << 16;
    set_register(cpu, instruction.i.rt, (sdword)immediate);
}

MIPS_INSTR(mips_lbu) {
    shalf offset = instruction.i.immediate;
    logtrace("LBU offset: %d", offset);
    word address = get_register(cpu, instruction.i.rs) + offset;
    byte value   = cpu->read_byte(address);

    set_register(cpu, instruction.i.rt, value); // zero extend
}

MIPS_INSTR(mips_lhu) {
    shalf offset = instruction.i.immediate;
    logtrace("LHU offset: %d", offset);
    word address = get_register(cpu, instruction.i.rs) + offset;
    half value   = cpu->read_half(address);
    if ((address & 0b1) > 0) {
        logfatal("TODO: throw an 'address error' exception! Tried to load from unaligned address 0x%08X", address);
    }

    set_register(cpu, instruction.i.rt, value); // zero extend
}

MIPS_INSTR(mips_lh) {
    shalf offset = instruction.i.immediate;
    word address = get_register(cpu, instruction.i.rs) + offset;
    shalf value   = cpu->read_half(address);
    if ((address & 0b1) > 0) {
        logfatal("TODO: throw an 'address error' exception! Tried to load from unaligned address 0x%08X", address);
    }

    set_register(cpu, instruction.i.rt, (sdword)value);
}

MIPS_INSTR(mips_lw) {
    shalf offset = instruction.i.immediate;
    word address = get_register(cpu, instruction.i.rs) + offset;
    if ((address & 0b11) > 0) {
        logfatal("TODO: throw an 'address error' exception! Tried to load from unaligned address 0x%08X", address);
    }

    sword value = cpu->read_word(address);
    set_register(cpu, instruction.i.rt, (sdword)value);
}

MIPS_INSTR(mips_lwu) {
    shalf offset = instruction.i.immediate;
    word address = get_register(cpu, instruction.i.rs) + offset;
    if ((address & 0b11) > 0) {
        logfatal("TODO: throw an 'address error' exception! Tried to load from unaligned address 0x%08X", address);
    }

    word value = cpu->read_word(address);
    set_register(cpu, instruction.i.rt, value);
}

MIPS_INSTR(mips_sb) {
    shalf offset = instruction.i.immediate;
    word address = get_register(cpu, instruction.i.rs);
    address += offset;
    byte value = get_register(cpu, instruction.i.rt) & 0xFF;
    cpu->write_byte(address, value);
}

MIPS_INSTR(mips_sh) {
    shalf offset = instruction.i.immediate;
    word address = get_register(cpu, instruction.i.rs);
    address += offset;
    half value = get_register(cpu, instruction.i.rt);
    cpu->write_half(address, value);
}

MIPS_INSTR(mips_sw) {
    shalf offset = instruction.i.immediate;
    word address = get_register(cpu, instruction.i.rs);
    address += offset;
    cpu->write_word(address, get_register(cpu, instruction.i.rt));
}

MIPS_INSTR(mips_sd) {
    shalf offset = instruction.i.immediate;
    word address = get_register(cpu, instruction.i.rs) + offset;
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
    check_sdword_add_overflow(addend1, addend2, result);
    set_register(cpu, instruction.i.rt, result);
}

MIPS_INSTR(mips_lb) {
    shalf offset    = instruction.i.immediate;
    word address    = get_register(cpu, instruction.i.rs) + offset;
    sbyte value     = cpu->read_byte(address);

    set_register(cpu, instruction.i.rt, (sdword)value);
}

MIPS_INSTR(mips_ldc1) {
    shalf offset    = instruction.i.immediate;
    word address    = get_register(cpu, instruction.i.rs) + offset;
    if (address & 0b111) {
        logfatal("Address error exception: misaligned dword read!");
    }

    dword value = cpu->read_dword(address);
    set_fpu_register_dword(cpu, instruction.i.rt, value);
}

MIPS_INSTR(mips_sdc1) {
    shalf offset    = instruction.fi.offset;
    word address    = get_register(cpu, instruction.fi.base) + offset;
    dword value     = get_fpu_register_dword(cpu, instruction.fi.ft);

    cpu->write_dword(address, value);
}

MIPS_INSTR(mips_lwc1) {
    shalf offset = instruction.fi.offset;
    word address = get_register(cpu, instruction.fi.base) + offset;
    word value   = cpu->read_word(address);

    set_fpu_register_word(cpu, instruction.fi.ft, value);
}

MIPS_INSTR(mips_swc1) {
    shalf offset = instruction.fi.offset;
    word address = get_register(cpu, instruction.fi.base) + offset;
    word value   = get_fpu_register_word(cpu, instruction.fi.ft);

    cpu->write_word(address, value);
}

MIPS_INSTR(mips_lwl) {
    shalf offset = instruction.fi.offset;
    word address = get_register(cpu, instruction.fi.base) + offset;

    word shift = 8 * ((address ^ 0) & 3);
    word mask = 0xFFFFFFFF << shift;
    word data = cpu->read_word(address & ~3);
    sword result = (get_register(cpu, instruction.i.rt) & ~mask) | data << shift;
    set_register(cpu, instruction.i.rt, (sdword)result);
}

MIPS_INSTR(mips_lwr) {
    shalf offset = instruction.fi.offset;
    word address = get_register(cpu, instruction.fi.base) + offset;

    word shift = 8 * ((address ^ 3) & 3);

    word mask = 0xFFFFFFFF >> shift;
    word data = cpu->read_word(address & ~3);
    sword result = (get_register(cpu, instruction.i.rt) & ~mask) | data >> shift;
    set_register(cpu, instruction.i.rt, (sdword)result);
}

MIPS_INSTR(mips_swl) {
    shalf offset = instruction.fi.offset;
    word address = get_register(cpu, instruction.fi.base) + offset;

    word shift = 8 * ((address ^ 0) & 3);
    word mask = 0xFFFFFFFF >> shift;
    word data = cpu->read_word(address & ~3);
    word oldreg = get_register(cpu, instruction.i.rt);
    cpu->write_word(address & ~3, (data & ~mask) | (oldreg >> shift));
}

MIPS_INSTR(mips_swr) {
    shalf offset = instruction.fi.offset;
    word address = get_register(cpu, instruction.fi.base) + offset;

    word shift = 8 * ((address ^ 3) & 3);
    word mask = 0xFFFFFFFF << shift;
    word data = cpu->read_word(address & ~3);
    word oldreg = get_register(cpu, instruction.i.rt);
    cpu->write_word(address & ~3, (data & ~mask) | oldreg << shift);
}

MIPS_INSTR(mips_ldl) {
    shalf offset = instruction.fi.offset;
    word address = get_register(cpu, instruction.fi.base) + offset;
    int shift = 8 * ((address ^ 0) & 7);
    dword mask = (dword)0xFFFFFFFFFFFFFFFF << shift;
    dword data = cpu->read_dword(address & ~7);
    dword oldreg = get_register(cpu, instruction.i.rt);

    set_register(cpu, instruction.i.rt, (oldreg & ~mask) | (data << shift));
}

MIPS_INSTR(mips_ldr) {
    shalf offset = instruction.fi.offset;
    word address = get_register(cpu, instruction.fi.base) + offset;
    int shift = 8 * ((address ^ 7) & 7);
    dword mask = (dword)0xFFFFFFFFFFFFFFFF >> shift;
    dword data = cpu->read_dword(address & ~7);
    dword oldreg = get_register(cpu, instruction.i.rt);

    set_register(cpu, instruction.i.rt, (oldreg & ~mask) | (data >> shift));
}

MIPS_INSTR(mips_sdl) {
    shalf offset = instruction.fi.offset;
    word address = get_register(cpu, instruction.fi.base) + offset;

    int shift = 8 * ((address ^ 0) & 7);
    dword mask = 0xFFFFFFFFFFFFFFFF;
    mask >>= shift;
    dword data = cpu->read_dword(address & ~7);
    dword oldreg = get_register(cpu, instruction.i.rt);
    cpu->write_dword(address & ~7, (data & ~mask) | (oldreg >> shift));
}

MIPS_INSTR(mips_sdr) {
    shalf offset = instruction.fi.offset;
    word address = get_register(cpu, instruction.fi.base) + offset;

    int shift = 8 * ((address ^ 7) & 7);
    dword mask = 0xFFFFFFFFFFFFFFFF ;
    mask <<= shift;
    dword data = cpu->read_dword(address & ~7);
    dword oldreg = get_register(cpu, instruction.i.rt);
    cpu->write_dword(address & ~7, (data & ~mask) | (oldreg << shift));
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
        logwarn("Undefined behavior! No exception thrown, but a divide by zero happened.");
        cpu->mult_lo = 0;
        cpu->mult_hi = 0;
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

MIPS_INSTR(mips_spc_dmultu) {
    __uint128_t multiplicand_1 = get_register(cpu, instruction.r.rs) & 0xFFFFFFFFFFFFFFFF;
    __uint128_t multiplicand_2 = get_register(cpu, instruction.r.rt) & 0xFFFFFFFFFFFFFFFF;

    __uint128_t result = multiplicand_1 * multiplicand_2;

    sdword result_lower = result         & 0xFFFFFFFFFFFFFFFF;
    sdword result_upper = (result >> 64) & 0xFFFFFFFFFFFFFFFF;

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

MIPS_INSTR(mips_ri_bgezal) {
    link(cpu);
    sdword reg = get_register(cpu, instruction.i.rs);
    conditional_branch(cpu, instruction.i.immediate, reg >= 0);
}
