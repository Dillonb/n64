#include "fpu_instructions.h"

#include <util.h>
#include <fenv.h>
#include <mem/n64bus.h>
#include <math.h>

#include "r4300i_register_access.h"

#define checkcp1 do { if (!N64CPU.cp0.status.cu1) { r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_COPROCESSOR_UNUSABLE, 1); return; } } while(0)

#define ORDERED_S(fs, ft) do { if (isnanf(fs) || isnanf(ft)) { logfatal("we got some nans, time to panic"); } } while (0)
#define ORDERED_D(fs, ft) do { if (isnan(fs) || isnan(ft)) { logfatal("we got some nans, time to panic"); } } while (0)

#define UNORDERED_S(fs, ft) do { if (isnanf(fs) || isnanf(ft)) { logfatal("we got some nans, time to panic"); } } while (0)
#define UNORDERED_D(fs, ft) do { if (isnan(fs) || isnan(ft)) { logfatal("we got some nans, time to panic"); } } while (0)


MIPS_INSTR(mips_mfc1) {
        checkcp1;
        sword value = get_fpu_register_word(instruction.fr.fs);
        set_register(instruction.r.rt, (sdword)value);
}

MIPS_INSTR(mips_dmfc1) {
        checkcp1;
        dword value = get_fpu_register_dword(instruction.fr.fs);
        set_register(instruction.r.rt, value);
}

MIPS_INSTR(mips_mtc1) {
        checkcp1;
        word value = get_register(instruction.r.rt);
        set_fpu_register_word(instruction.r.rd, value);
}

MIPS_INSTR(mips_dmtc1) {
        checkcp1;
        dword value = get_register(instruction.r.rt);
        set_fpu_register_dword(instruction.r.rd, value);
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

MIPS_INSTR(mips_cfc1) {
        checkcp1;
        byte fs = instruction.r.rd;
        sword value;
        switch (fs) {
            case 0:
                logwarn("Reading FCR0 - probably returning an invalid value!");
            value = N64CPU.fcr0.raw;
            break;
            case 31:
                value = N64CPU.fcr31.raw;
            break;
            default:
                logfatal("This instruction is only defined when fs == 0 or fs == 31! (Throw an exception?)");
        }

        set_register(instruction.r.rt, (sdword)value);
}

INLINE void check_fpu_exception() {
    if (N64CPU.fcr31.cause & (0b100000 | N64CPU.fcr31.enable)) {
        logfatal("FPU exception");
    }
}

INLINE int push_round_mode() {
    int orig_round = fegetround();
    switch (N64CPU.fcr31.rounding_mode) {
        case R4300I_CP1_ROUND_NEAREST:
            fesetround(FE_TONEAREST);
            break;
        case R4300I_CP1_ROUND_ZERO:
            fesetround(FE_TOWARDZERO);
            break;
        case R4300I_CP1_ROUND_POSINF:
            fesetround(FE_UPWARD);
            break;
        case R4300I_CP1_ROUND_NEGINF:
            fesetround(FE_DOWNWARD);
            break;
        default:
            logfatal("Unknown rounding mode %d", N64CPU.fcr31.rounding_mode);
    }
    //fesetround(FE_TOWARDZERO);
    return orig_round;
}

#define PUSHROUND int orig_round = push_round_mode()
#define POPROUND fesetround(orig_round)

MIPS_INSTR(mips_ctc1) {
        checkcp1;
        byte fs = instruction.r.rd;
        word value = get_register(instruction.r.rt);
        switch (fs) {
            case 0:
                logfatal("CTC1 FCR0: FCR0 is read only!");
            case 31: {
                value &= 0x183ffff; // mask out bits held 0
                N64CPU.fcr31.raw = value;
                check_fpu_exception();
                break;
            }
            default:
                logfatal("This instruction is only defined when fs == 0 or fs == 31! (Throw an exception?)");
        }
}

MIPS_INSTR(mips_cp_bc1f) {
        checkcp1;
        conditional_branch(instruction.i.immediate, !N64CPU.fcr31.compare);
}

MIPS_INSTR(mips_cp_bc1fl) {
        checkcp1;
        conditional_branch_likely(instruction.i.immediate, !N64CPU.fcr31.compare);
}

MIPS_INSTR(mips_cp_bc1t) {
        checkcp1;
        conditional_branch(instruction.i.immediate, N64CPU.fcr31.compare);
}
MIPS_INSTR(mips_cp_bc1tl) {
        checkcp1;
        conditional_branch_likely(instruction.i.immediate, N64CPU.fcr31.compare);
}

MIPS_INSTR(mips_cp_mul_d) {
        checkcp1;
        double fs = get_fpu_register_double(instruction.fr.fs);
        double ft = get_fpu_register_double(instruction.fr.ft);
        double result = fs * ft;
        set_fpu_register_double(instruction.fr.fd, result);
}

MIPS_INSTR(mips_cp_mul_s) {
        checkcp1;
        float fs = get_fpu_register_float(instruction.fr.fs);
        float ft = get_fpu_register_float(instruction.fr.ft);
        float result = fs * ft;
        set_fpu_register_float(instruction.fr.fd, result);
}

MIPS_INSTR(mips_cp_div_d) {
        checkcp1;
        double fs = get_fpu_register_double(instruction.fr.fs);
        double ft = get_fpu_register_double(instruction.fr.ft);
        double result = fs / ft;
        set_fpu_register_double(instruction.fr.fd, result);
}

MIPS_INSTR(mips_cp_div_s) {
        checkcp1;
        float fs = get_fpu_register_float(instruction.fr.fs);
        float ft = get_fpu_register_float(instruction.fr.ft);
        float result = fs / ft;
        set_fpu_register_float(instruction.fr.fd, result);
}

MIPS_INSTR(mips_cp_add_d) {
        checkcp1;
        double fs = get_fpu_register_double(instruction.fr.fs);
        double ft = get_fpu_register_double(instruction.fr.ft);
        double result = fs + ft;
        set_fpu_register_double(instruction.fr.fd, result);
}

MIPS_INSTR(mips_cp_add_s) {
        checkcp1;
        float fs = get_fpu_register_float(instruction.fr.fs);
        float ft = get_fpu_register_float(instruction.fr.ft);
        float result = fs + ft;
        set_fpu_register_float(instruction.fr.fd, result);
}

MIPS_INSTR(mips_cp_sub_d) {
        checkcp1;
        double fs = get_fpu_register_double(instruction.fr.fs);
        double ft = get_fpu_register_double(instruction.fr.ft);
        double result = fs - ft;
        set_fpu_register_double(instruction.fr.fd, result);
}

MIPS_INSTR(mips_cp_sub_s) {
        checkcp1;
        float fs = get_fpu_register_float(instruction.fr.fs);
        float ft = get_fpu_register_float(instruction.fr.ft);
        float result = fs - ft;
        set_fpu_register_float(instruction.fr.fd, result);
}

MIPS_INSTR(mips_cp_trunc_l_d) {
        checkcp1;
        double value = get_fpu_register_double(instruction.fr.fs);
        dword truncated = trunc(value);
        set_fpu_register_dword(instruction.fr.fd, truncated);
}

MIPS_INSTR(mips_cp_round_l_d) {
        logwarn("mips_cp_round_l_d used: this instruction is known to be buggy!");
        checkcp1;
        double value = get_fpu_register_double(instruction.fr.fs);
        PUSHROUND;
        dword truncated = round(value);
        POPROUND;
        set_fpu_register_dword(instruction.fr.fd, truncated);
}

MIPS_INSTR(mips_cp_trunc_l_s) {
        checkcp1;
        float value = get_fpu_register_float(instruction.fr.fs);
        dword truncated = truncf(value);
        set_fpu_register_dword(instruction.fr.fd, truncated);
}

MIPS_INSTR(mips_cp_round_l_s) {
        logwarn("mips_cp_round_l_s used: this instruction is known to be buggy!");
        checkcp1;
        float value = get_fpu_register_float(instruction.fr.fs);
        PUSHROUND;
        dword truncated = roundf(value);
        POPROUND;
        set_fpu_register_dword(instruction.fr.fd, truncated);
}

MIPS_INSTR(mips_cp_trunc_w_d) {
        checkcp1;
        double value = get_fpu_register_double(instruction.fr.fs);
        word truncated = trunc(value);
        set_fpu_register_word(instruction.fr.fd, truncated);
}

MIPS_INSTR(mips_cp_round_w_d) {
        logwarn("mips_cp_round_w_d used: this instruction is known to be buggy!");
        checkcp1;
        double value = get_fpu_register_double(instruction.fr.fs);
        PUSHROUND;
        word truncated = round(value);
        POPROUND;
        set_fpu_register_word(instruction.fr.fd, truncated);
}

MIPS_INSTR(mips_cp_trunc_w_s) {
        checkcp1;
        float value = get_fpu_register_float(instruction.fr.fs);
        sword truncated = truncf(value);
        set_fpu_register_word(instruction.fr.fd, truncated);
}

MIPS_INSTR(mips_cp_floor_w_d) {
        logfatal("mips_cp_floor_w_d");
}

MIPS_INSTR(mips_cp_floor_w_s) {
        checkcp1;
        float value = get_fpu_register_float(instruction.fr.fs);
        sword truncated = floorf(value);
        set_fpu_register_word(instruction.fr.fd, truncated);
}

MIPS_INSTR(mips_cp_round_w_s) {
        logwarn("mips_cp_round_w_s used: this instruction is known to be buggy!");
        checkcp1;
        float value = get_fpu_register_float(instruction.fr.fs);
        PUSHROUND;
        sword truncated = roundf(value);
        POPROUND;
        set_fpu_register_word(instruction.fr.fd, truncated);
}

MIPS_INSTR(mips_cp_cvt_d_s) {
        checkcp1;
        float fs = get_fpu_register_float(instruction.fr.fs);
        double converted = fs;
        set_fpu_register_double(instruction.fr.fd, converted);
}

MIPS_INSTR(mips_cp_cvt_d_w) {
        checkcp1;
        sword fs = get_fpu_register_word(instruction.fr.fs);
        double converted = fs;
        set_fpu_register_double(instruction.fr.fd, converted);
}

MIPS_INSTR(mips_cp_cvt_d_l) {
        checkcp1;
        sdword fs = get_fpu_register_dword(instruction.fr.fs);
        double converted = fs;
        set_fpu_register_double(instruction.fr.fd, converted);
}

MIPS_INSTR(mips_cp_cvt_l_s) {
        checkcp1;
        float fs = get_fpu_register_float(instruction.fr.fs);
        sdword converted = fs;
        set_fpu_register_dword(instruction.fr.fd, converted);
}

MIPS_INSTR(mips_cp_cvt_l_d) {
        checkcp1;
        double fs = get_fpu_register_double(instruction.fr.fs);
        sdword converted = fs;
        set_fpu_register_dword(instruction.fr.fd, converted);
}

MIPS_INSTR(mips_cp_cvt_s_d) {
        checkcp1;
        double fs = get_fpu_register_double(instruction.fr.fs);
        float converted = fs;
        set_fpu_register_float(instruction.fr.fd, converted);
}

MIPS_INSTR(mips_cp_cvt_s_w) {
        checkcp1;
        sword fs = get_fpu_register_word(instruction.fr.fs);
        float converted = fs;
        set_fpu_register_float(instruction.fr.fd, converted);
}

MIPS_INSTR(mips_cp_cvt_s_l) {
        checkcp1;
        sdword fs = get_fpu_register_dword(instruction.fr.fs);
        float converted = fs;
        set_fpu_register_float(instruction.fr.fd, converted);
}

MIPS_INSTR(mips_cp_cvt_w_s) {
        checkcp1;
        float fs = get_fpu_register_float(instruction.fr.fs);
        sword converted = fs;
        set_fpu_register_word(instruction.fr.fd, converted);
}

MIPS_INSTR(mips_cp_cvt_w_d) {
        checkcp1;
        double fs = get_fpu_register_double(instruction.fr.fs);
        sword converted = fs;
        set_fpu_register_word(instruction.fr.fd, converted);
}

MIPS_INSTR(mips_cp_sqrt_s) {
        checkcp1;
        float fs = get_fpu_register_float(instruction.fr.fs);
        float root = sqrt(fs);
        set_fpu_register_float(instruction.fr.fd, root);
}

MIPS_INSTR(mips_cp_sqrt_d) {
        checkcp1;
        double fs = get_fpu_register_double(instruction.fr.fs);
        double root = sqrt(fs);
        set_fpu_register_double(instruction.fr.fd, root);
}

MIPS_INSTR(mips_cp_abs_s) {
        checkcp1;
        float fs = get_fpu_register_float(instruction.fr.fs);
        if (fs < 0) {
            fs = -fs;
        }
        set_fpu_register_float(instruction.fr.fd, fs);
}

MIPS_INSTR(mips_cp_abs_d) {
        checkcp1;
        double fs = get_fpu_register_double(instruction.fr.fs);
        if (fs < 0) {
            fs = -fs;
        }
        set_fpu_register_double(instruction.fr.fd, fs);
}

MIPS_INSTR(mips_cp_c_f_s) {
        checkcp1;
        /*
        float fs = get_fpu_register_float(instruction.fr.fs);
        float ft = get_fpu_register_float(instruction.fr.ft);
         */
        logfatal("Unimplemented: mips_cp_c_f_s");
}
MIPS_INSTR(mips_cp_c_un_s) {
        checkcp1;
        /*
        float fs = get_fpu_register_float(instruction.fr.fs);
        float ft = get_fpu_register_float(instruction.fr.ft);
         */
        logfatal("Unimplemented: mips_cp_c_un_s");
}
MIPS_INSTR(mips_cp_c_eq_s) {
    checkcp1;
    float fs = get_fpu_register_float(instruction.fr.fs);
    float ft = get_fpu_register_float(instruction.fr.ft);

    N64CPU.fcr31.compare = fs == ft;
}
MIPS_INSTR(mips_cp_c_ueq_s) {
        checkcp1;
        float fs = get_fpu_register_float(instruction.fr.fs);
        float ft = get_fpu_register_float(instruction.fr.ft);
        UNORDERED_S(fs, ft);

        N64CPU.fcr31.compare = fs == ft;
}

MIPS_INSTR(mips_cp_c_olt_s) {
        checkcp1;
        float fs = get_fpu_register_float(instruction.fr.fs);
        float ft = get_fpu_register_float(instruction.fr.ft);
        ORDERED_S(fs, ft);

        N64CPU.fcr31.compare = fs < ft;
}

MIPS_INSTR(mips_cp_c_ult_s) {
        checkcp1;
        float fs = get_fpu_register_float(instruction.fr.fs);
        float ft = get_fpu_register_float(instruction.fr.ft);
        UNORDERED_S(fs, ft);
        N64CPU.fcr31.compare = fs < ft;
}
MIPS_INSTR(mips_cp_c_ole_s) {
        checkcp1;
        float fs = get_fpu_register_float(instruction.fr.fs);
        float ft = get_fpu_register_float(instruction.fr.ft);
        ORDERED_S(fs, ft);

        N64CPU.fcr31.compare = fs <= ft;
}
MIPS_INSTR(mips_cp_c_ule_s) {
        checkcp1;
        float fs = get_fpu_register_float(instruction.fr.fs);
        float ft = get_fpu_register_float(instruction.fr.ft);
        UNORDERED_S(fs, ft);
        N64CPU.fcr31.compare = fs <= ft;
}
MIPS_INSTR(mips_cp_c_sf_s) {
        checkcp1;
        /*
        float fs = get_fpu_register_float(instruction.fr.fs);
        float ft = get_fpu_register_float(instruction.fr.ft);
         */
        logfatal("Unimplemented: mips_cp_c_sf_s");
}
MIPS_INSTR(mips_cp_c_ngle_s) {
        checkcp1;
        /*
        float fs = get_fpu_register_float(instruction.fr.fs);
        float ft = get_fpu_register_float(instruction.fr.ft);
         */
        logfatal("Unimplemented: mips_cp_c_ngle_s");
}
MIPS_INSTR(mips_cp_c_seq_s) {
        checkcp1;
        /*
        float fs = get_fpu_register_float(instruction.fr.fs);
        float ft = get_fpu_register_float(instruction.fr.ft);
         */
        logfatal("Unimplemented: mips_cp_c_seq_s");
}
MIPS_INSTR(mips_cp_c_ngl_s) {
        checkcp1;
        /*
        float fs = get_fpu_register_float(instruction.fr.fs);
        float ft = get_fpu_register_float(instruction.fr.ft);
         */
        logfatal("Unimplemented: mips_cp_c_ngl_s");
}
MIPS_INSTR(mips_cp_c_lt_s) {
    checkcp1;
    float fs = get_fpu_register_float(instruction.fr.fs);
    float ft = get_fpu_register_float(instruction.fr.ft);

    N64CPU.fcr31.compare = fs < ft;

    if (isnan(fs) || isnan(ft)) {
        N64CPU.fcr31.cause_invalid_operation = true;
        N64CPU.fcr31.flag_invalid_operation = true;
        check_fpu_exception();
    }
}
MIPS_INSTR(mips_cp_c_nge_s) {
    checkcp1;
    float fs = get_fpu_register_float(instruction.fr.fs);
    float ft = get_fpu_register_float(instruction.fr.ft);

    N64CPU.fcr31.compare = !(fs >= ft);

    if (isnan(fs) || isnan(ft)) {
        N64CPU.fcr31.cause_invalid_operation = true;
        N64CPU.fcr31.flag_invalid_operation = true;
        check_fpu_exception();
    }
}
MIPS_INSTR(mips_cp_c_le_s) {
    checkcp1;
    float fs = get_fpu_register_float(instruction.fr.fs);
    float ft = get_fpu_register_float(instruction.fr.ft);

    N64CPU.fcr31.compare = fs <= ft;

    if (isnan(fs) || isnan(ft)) {
        N64CPU.fcr31.cause_invalid_operation = true;
        N64CPU.fcr31.flag_invalid_operation = true;
        check_fpu_exception();
    }
}
MIPS_INSTR(mips_cp_c_ngt_s) {
    checkcp1;
    float fs = get_fpu_register_float(instruction.fr.fs);
    float ft = get_fpu_register_float(instruction.fr.ft);

    N64CPU.fcr31.compare = !(fs > ft);

    if (isnan(fs) || isnan(ft)) {
        N64CPU.fcr31.cause_invalid_operation = true;
        N64CPU.fcr31.flag_invalid_operation = true;
        check_fpu_exception();
    }
}

MIPS_INSTR(mips_cp_c_f_d) {
        checkcp1;
        /*
        double fs = get_fpu_register_double(instruction.fr.fs);
        double ft = get_fpu_register_double(instruction.fr.ft);
         */
        logfatal("Unimplemented: mips_cp_c_f_d");
}
MIPS_INSTR(mips_cp_c_un_d) {
        checkcp1;
        /*
        double fs = get_fpu_register_double(instruction.fr.fs);
        double ft = get_fpu_register_double(instruction.fr.ft);
         */
        logfatal("Unimplemented: mips_cp_c_un_d");
}
MIPS_INSTR(mips_cp_c_eq_d) {
        checkcp1;
        double fs = get_fpu_register_double(instruction.fr.fs);
        double ft = get_fpu_register_double(instruction.fr.ft);

        unimplemented(isnan(fs), "fs is nan");
        unimplemented(isnan(ft), "ft is nan");

        N64CPU.fcr31.compare = fs == ft;
}
MIPS_INSTR(mips_cp_c_ueq_d) {
        checkcp1;
        double fs = get_fpu_register_double(instruction.fr.fs);
        double ft = get_fpu_register_double(instruction.fr.ft);

        UNORDERED_D(fs, ft);

        N64CPU.fcr31.compare = fs == ft;
}
MIPS_INSTR(mips_cp_c_olt_d) {
        checkcp1;
        /*
        double fs = get_fpu_register_double(instruction.fr.fs);
        double ft = get_fpu_register_double(instruction.fr.ft);
         */
        logfatal("Unimplemented: mips_cp_c_olt_d");
}
MIPS_INSTR(mips_cp_c_ult_d) {
        checkcp1;
        double fs = get_fpu_register_double(instruction.fr.fs);
        double ft = get_fpu_register_double(instruction.fr.ft);
        UNORDERED_D(fs, ft);
        N64CPU.fcr31.compare = fs < ft;
        logfatal("Unimplemented: mips_cp_c_ult_d");
}
MIPS_INSTR(mips_cp_c_ole_d) {
        checkcp1;
        double fs = get_fpu_register_double(instruction.fr.fs);
        double ft = get_fpu_register_double(instruction.fr.ft);
        ORDERED_D(fs, ft);
        N64CPU.fcr31.compare = fs <= ft;
}
MIPS_INSTR(mips_cp_c_ule_d) {
        checkcp1;
        double fs = get_fpu_register_double(instruction.fr.fs);
        double ft = get_fpu_register_double(instruction.fr.ft);

        UNORDERED_D(fs, ft);

        N64CPU.fcr31.compare = fs <= ft;
}
MIPS_INSTR(mips_cp_c_sf_d) {
        checkcp1;
        /*
        double fs = get_fpu_register_double(instruction.fr.fs);
        double ft = get_fpu_register_double(instruction.fr.ft);
         */
        logfatal("Unimplemented: mips_cp_c_sf_d");
}
MIPS_INSTR(mips_cp_c_ngle_d) {
        checkcp1;
        /*
        double fs = get_fpu_register_double(instruction.fr.fs);
        double ft = get_fpu_register_double(instruction.fr.ft);
         */
        logfatal("Unimplemented: mips_cp_c_ngle_d");
}
MIPS_INSTR(mips_cp_c_seq_d) {
        checkcp1;
        /*
        double fs = get_fpu_register_double(instruction.fr.fs);
        double ft = get_fpu_register_double(instruction.fr.ft);
         */
        logfatal("Unimplemented: mips_cp_c_seq_d");
}
MIPS_INSTR(mips_cp_c_ngl_d) {
        checkcp1;
        /*
        double fs = get_fpu_register_double(instruction.fr.fs);
        double ft = get_fpu_register_double(instruction.fr.ft);
         */
        logfatal("Unimplemented: mips_cp_c_ngl_d");
}
MIPS_INSTR(mips_cp_c_lt_d) {
        checkcp1;
        double fs = get_fpu_register_double(instruction.fr.fs);
        double ft = get_fpu_register_double(instruction.fr.ft);

        unimplemented(isnan(fs), "fs is nan");
        unimplemented(isnan(ft), "ft is nan");

        N64CPU.fcr31.compare = fs < ft;
}
MIPS_INSTR(mips_cp_c_nge_d) {
        checkcp1;
        double fs = get_fpu_register_double(instruction.fr.fs);
        double ft = get_fpu_register_double(instruction.fr.ft);
        N64CPU.fcr31.compare = !(fs >= ft);
}
MIPS_INSTR(mips_cp_c_le_d) {
        checkcp1;
        double fs = get_fpu_register_double(instruction.fr.fs);
        double ft = get_fpu_register_double(instruction.fr.ft);

        unimplemented(isnan(fs), "fs is nan");
        unimplemented(isnan(ft), "ft is nan");

        N64CPU.fcr31.compare = fs <= ft;
}
MIPS_INSTR(mips_cp_c_ngt_d) {
        checkcp1;
        double fs = get_fpu_register_double(instruction.fr.fs);
        double ft = get_fpu_register_double(instruction.fr.ft);
        N64CPU.fcr31.compare = !(fs > ft);
}

MIPS_INSTR(mips_cp_mov_s) {
        checkcp1;
        float value = get_fpu_register_float(instruction.fr.fs);
        set_fpu_register_float(instruction.fr.fd, value);
}

MIPS_INSTR(mips_cp_mov_d) {
        checkcp1;
        double value = get_fpu_register_double(instruction.fr.fs);
        set_fpu_register_double(instruction.fr.fd, value);
}

MIPS_INSTR(mips_cp_neg_s) {
        checkcp1;
        float value = get_fpu_register_float(instruction.fr.fs);
        set_fpu_register_float(instruction.fr.fd, -value);
}

MIPS_INSTR(mips_cp_neg_d) {
        checkcp1;
        double value = get_fpu_register_double(instruction.fr.fs);
        set_fpu_register_double(instruction.fr.fd, -value);
}

MIPS_INSTR(mips_ldc1) {
        checkcp1;
        shalf offset  = instruction.i.immediate;
        dword address = get_register(instruction.i.rs) + offset;
        if (address & 0b111) {
            logfatal("Address error exception: misaligned dword read!");
        }

        dword value = n64_read_dword(address);
        set_fpu_register_dword(instruction.i.rt, value);
}

MIPS_INSTR(mips_sdc1) {
        checkcp1;
        shalf offset  = instruction.fi.offset;
        dword address = get_register(instruction.fi.base) + offset;
        dword value   = get_fpu_register_dword(instruction.fi.ft);

        n64_write_dword(address, value);
}

MIPS_INSTR(mips_lwc1) {
        checkcp1;
        shalf offset  = instruction.fi.offset;
        dword address = get_register(instruction.fi.base) + offset;
        word value    = n64_read_word(address);

        set_fpu_register_word(instruction.fi.ft, value);
}

MIPS_INSTR(mips_swc1) {
        checkcp1;
        shalf offset  = instruction.fi.offset;
        dword address = get_register(instruction.fi.base) + offset;
        word value    = get_fpu_register_word(instruction.fi.ft);

        n64_write_word(address, value);
}
