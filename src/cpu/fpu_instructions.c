#include "fpu_instructions.h"

#include <util.h>
#include <fenv.h>
#include <mem/n64bus.h>
#include <math.h>

#include "r4300i_register_access.h"

#ifdef N64_WIN
#define ORDERED_S(fs, ft) do { if (isnan(fs) || isnan(ft)) { logfatal("we got some nans, time to panic"); } } while (0)
#define ORDERED_D(fs, ft) do { if (isnan(fs) || isnan(ft)) { logfatal("we got some nans, time to panic"); } } while (0)

#define UNORDERED_S(fs, ft) do { if (isnan(fs) || isnan(ft)) { logfatal("we got some nans, time to panic"); } } while (0)
#define UNORDERED_D(fs, ft) do { if (isnan(fs) || isnan(ft)) { logfatal("we got some nans, time to panic"); } } while (0)

#define checknansf(fs, ft) if (isnan(fs) || isnan(ft)) { logfatal("fs || ft == NaN!"); }
#define checknansd(fs, ft) if (isnan(fs) || isnan(ft)) { logfatal("fs || ft == NaN!"); }
#define checknanf(value) if (isnan(value)) { logfatal("value == NaN!"); }
#define checknand(value) if (isnan(value)) { logfatal("value == NaN!"); }

#define isnanf isnan

#else

#define ORDERED_S(fs, ft) do { if (isnanf(fs) || isnanf(ft)) { logfatal("we got some nans, time to panic"); } } while (0)
#define ORDERED_D(fs, ft) do { if (isnan(fs) || isnan(ft)) { logfatal("we got some nans, time to panic"); } } while (0)

#define UNORDERED_S(fs, ft) do { if (isnanf(fs) || isnanf(ft)) { logfatal("we got some nans, time to panic"); } } while (0)
#define UNORDERED_D(fs, ft) do { if (isnan(fs) || isnan(ft)) { logfatal("we got some nans, time to panic"); } } while (0)

#define checknansf(fs, ft) if (isnanf(fs) || isnanf(ft)) { logfatal("fs || ft == NaN!"); }
#define checknansd(fs, ft) if (isnan(fs) || isnan(ft)) { logfatal("fs || ft == NaN!"); }
#define checknanf(value) if (isnanf(value)) { logfatal("value == NaN!"); }
#define checknand(value) if (isnan(value)) { logfatal("value == NaN!"); }
#endif


MIPS_INSTR(mips_mfc1) {
    checkcp1;
    s32 value = get_fpu_register_word(instruction.fr.fs);
    set_register(instruction.r.rt, (s64)value);
}

MIPS_INSTR(mips_dmfc1) {
    checkcp1;
    u64 value = get_fpu_register_dword(instruction.fr.fs);
    set_register(instruction.r.rt, value);
}

MIPS_INSTR(mips_mtc1) {
    checkcp1;
    u32 value = get_register(instruction.r.rt);
    set_fpu_register_word(instruction.r.rd, value);
}

MIPS_INSTR(mips_dmtc1) {
    checkcp1;
    u64 value = get_register(instruction.r.rt);
    set_fpu_register_dword(instruction.r.rd, value);
}

MIPS_INSTR(mips_cfc1) {
    checkcp1;
    u8 fs = instruction.r.rd;
    s32 value;
    switch (fs) {
        case 0:
            logwarn("Reading FCR0 - probably returning an invalid value!");
            value = N64CPU.fcr0.raw;
            break;
        case 31:
            value = N64CPU.fcr31.raw;
            if (N64CPU.fcr31.enable_inexact_operation) {
                logwarn("FPU exception inexact operation enabled!");
            }
            if (N64CPU.fcr31.enable_overflow) {
                logwarn("FPU exception overflow enabled!");
            }
            if (N64CPU.fcr31.enable_underflow) {
                logwarn("FPU exception underflow enabled!");
            }
            break;
        default:
            logfatal("This instruction is only defined when fs == 0 or fs == 31! (Throw an exception?)");
    }

    set_register(instruction.r.rt, (s64)value);
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
    return orig_round;
}

#define PUSHROUND int orig_round = push_round_mode()
#define POPROUND fesetround(orig_round)

MIPS_INSTR(mips_ctc1) {
    checkcp1;
    u8 fs = instruction.r.rd;
    u32 value = get_register(instruction.r.rt);
    switch (fs) {
        case 0:
            logwarn("CTC1 FCR0: Wrote %08X to read-only register FCR0!", value);
            break;
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
    checknansd(fs, ft);
    double result = fs * ft;
    set_fpu_register_double(instruction.fr.fd, result);
}

MIPS_INSTR(mips_cp_mul_s) {
    checkcp1;
    float fs = get_fpu_register_float(instruction.fr.fs);
    float ft = get_fpu_register_float(instruction.fr.ft);
    checknansf(fs, ft);
    float result = fs * ft;
    set_fpu_register_float(instruction.fr.fd, result);
}

MIPS_INSTR(mips_cp_div_d) {
    checkcp1;
    double fs = get_fpu_register_double(instruction.fr.fs);
    double ft = get_fpu_register_double(instruction.fr.ft);
    checknansd(fs, ft);
    if (ft == 0) {
        N64CPU.fcr31.cause_division_by_zero = true;
        check_fpu_exception();
    }
    double result = fs / ft;
    set_fpu_register_double(instruction.fr.fd, result);
}

MIPS_INSTR(mips_cp_div_s) {
    checkcp1;
    float fs = get_fpu_register_float(instruction.fr.fs);
    float ft = get_fpu_register_float(instruction.fr.ft);
    checknansf(fs, ft);
    if (ft == 0) {
        N64CPU.fcr31.cause_division_by_zero = true;
        check_fpu_exception();
    }
    float result = fs / ft;
    set_fpu_register_float(instruction.fr.fd, result);
}

MIPS_INSTR(mips_cp_add_d) {
    checkcp1;
    double fs = get_fpu_register_double(instruction.fr.fs);
    double ft = get_fpu_register_double(instruction.fr.ft);
    checknansd(fs, ft);
    double result = fs + ft;
    set_fpu_register_double(instruction.fr.fd, result);
}

MIPS_INSTR(mips_cp_add_s) {
    checkcp1;
    float fs = get_fpu_register_float(instruction.fr.fs);
    float ft = get_fpu_register_float(instruction.fr.ft);
    checknansf(fs, ft);
    float result = fs + ft;
    set_fpu_register_float(instruction.fr.fd, result);
}

MIPS_INSTR(mips_cp_sub_d) {
    checkcp1;
    double fs = get_fpu_register_double(instruction.fr.fs);
    double ft = get_fpu_register_double(instruction.fr.ft);
    checknansd(fs, ft);
    double result = fs - ft;
    set_fpu_register_double(instruction.fr.fd, result);
}

MIPS_INSTR(mips_cp_sub_s) {
    checkcp1;
    float fs = get_fpu_register_float(instruction.fr.fs);
    float ft = get_fpu_register_float(instruction.fr.ft);
    checknansf(fs, ft);
    float result = fs - ft;
    set_fpu_register_float(instruction.fr.fd, result);
}

MIPS_INSTR(mips_cp_trunc_l_d) {
    checkcp1;
    double value = get_fpu_register_double(instruction.fr.fs);
    checknand(value);
    u64 truncated = trunc(value);
    set_fpu_register_dword(instruction.fr.fd, truncated);
}

MIPS_INSTR(mips_cp_round_l_d) {
    checkcp1;
    double value = get_fpu_register_double(instruction.fr.fs);
    PUSHROUND;
    u64 truncated = nearbyint(value);
    POPROUND;
    set_fpu_register_dword(instruction.fr.fd, truncated);
}

MIPS_INSTR(mips_cp_trunc_l_s) {
    checkcp1;
    float value = get_fpu_register_float(instruction.fr.fs);
    checknanf(value);
    u64 truncated = truncf(value);
    set_fpu_register_dword(instruction.fr.fd, truncated);
}

MIPS_INSTR(mips_cp_round_l_s) {
    checkcp1;
    float value = get_fpu_register_float(instruction.fr.fs);
    checknanf(value);
    PUSHROUND;
    u64 truncated = nearbyintf(value);
    POPROUND;
    set_fpu_register_dword(instruction.fr.fd, truncated);
}

MIPS_INSTR(mips_cp_trunc_w_d) {
    checkcp1;
    double value = get_fpu_register_double(instruction.fr.fs);
    u32 truncated = trunc(value);
    set_fpu_register_word(instruction.fr.fd, truncated);
}

MIPS_INSTR(mips_cp_round_w_d) {
    checkcp1;
    double value = get_fpu_register_double(instruction.fr.fs);
    PUSHROUND;
    u32 truncated = nearbyint(value);
    POPROUND;
    set_fpu_register_word(instruction.fr.fd, truncated);
}

MIPS_INSTR(mips_cp_trunc_w_s) {
    checkcp1;
    float value = get_fpu_register_float(instruction.fr.fs);
    checknanf(value);
    s32 truncated = truncf(value);
    set_fpu_register_word(instruction.fr.fd, truncated);
}

MIPS_INSTR(mips_cp_floor_w_d) {
    logfatal("mips_cp_floor_w_d");
}

MIPS_INSTR(mips_cp_floor_w_s) {
    checkcp1;
    float value = get_fpu_register_float(instruction.fr.fs);
    checknanf(value);
    s32 truncated = floorf(value);
    set_fpu_register_word(instruction.fr.fd, truncated);
}

MIPS_INSTR(mips_cp_round_w_s) {
    checkcp1;
    float value = get_fpu_register_float(instruction.fr.fs);
    checknanf(value);
    PUSHROUND;
    s32 truncated = nearbyintf(value);
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
    s32 fs = get_fpu_register_word(instruction.fr.fs);
    double converted = fs;
    set_fpu_register_double(instruction.fr.fd, converted);
}

MIPS_INSTR(mips_cp_cvt_d_l) {
    checkcp1;
    s64 fs = get_fpu_register_dword(instruction.fr.fs);
    double converted = fs;
    set_fpu_register_double(instruction.fr.fd, converted);
}

MIPS_INSTR(mips_cp_cvt_l_s) {
    checkcp1;
    float fs = get_fpu_register_float(instruction.fr.fs);
    s64 converted = fs;
    set_fpu_register_dword(instruction.fr.fd, converted);
}

MIPS_INSTR(mips_cp_cvt_l_d) {
    checkcp1;
    double fs = get_fpu_register_double(instruction.fr.fs);
    s64 converted = fs;
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
    s32 fs = get_fpu_register_word(instruction.fr.fs);
    float converted = fs;
    set_fpu_register_float(instruction.fr.fd, converted);
}

MIPS_INSTR(mips_cp_cvt_s_l) {
    checkcp1;
    s64 fs = get_fpu_register_dword(instruction.fr.fs);
    float converted = fs;
    set_fpu_register_float(instruction.fr.fd, converted);
}

MIPS_INSTR(mips_cp_cvt_w_s) {
    checkcp1;
    float fs = get_fpu_register_float(instruction.fr.fs);
    s32 converted = fs;
    set_fpu_register_word(instruction.fr.fd, converted);
}

MIPS_INSTR(mips_cp_cvt_w_d) {
    checkcp1;
    double fs = get_fpu_register_double(instruction.fr.fs);
    s32 converted = fs;
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
     checknansf(fs, ft);
     */
    logfatal("Unimplemented: mips_cp_c_f_s");
}
MIPS_INSTR(mips_cp_c_un_s) {
    checkcp1;
    float fs = get_fpu_register_float(instruction.fr.fs);
    float ft = get_fpu_register_float(instruction.fr.ft);
    N64CPU.fcr31.compare = isnanf(fs) || isnanf(ft);
}
MIPS_INSTR(mips_cp_c_eq_s) {
    checkcp1;
    float fs = get_fpu_register_float(instruction.fr.fs);
    float ft = get_fpu_register_float(instruction.fr.ft);
    checknansf(fs, ft);

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
     checknansf(fs, ft);
     */
    logfatal("Unimplemented: mips_cp_c_sf_s");
}
MIPS_INSTR(mips_cp_c_ngle_s) {
    checkcp1;
    /*
    float fs = get_fpu_register_float(instruction.fr.fs);
    float ft = get_fpu_register_float(instruction.fr.ft);
     checknansf(fs, ft);
     */
    logfatal("Unimplemented: mips_cp_c_ngle_s");
}
MIPS_INSTR(mips_cp_c_seq_s) {
    checkcp1;
    /*
    float fs = get_fpu_register_float(instruction.fr.fs);
    float ft = get_fpu_register_float(instruction.fr.ft);
     checknansf(fs, ft);
     */
    logfatal("Unimplemented: mips_cp_c_seq_s");
}
MIPS_INSTR(mips_cp_c_ngl_s) {
    checkcp1;
    /*
    float fs = get_fpu_register_float(instruction.fr.fs);
    float ft = get_fpu_register_float(instruction.fr.ft);
     checknansf(fs, ft);
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
     checknansd(fs, ft);
     */
    logfatal("Unimplemented: mips_cp_c_f_d");
}
MIPS_INSTR(mips_cp_c_un_d) {
    checkcp1;
    double fs = get_fpu_register_double(instruction.fr.fs);
    double ft = get_fpu_register_double(instruction.fr.ft);
    N64CPU.fcr31.compare = isnan(fs) || isnan(ft);
}
MIPS_INSTR(mips_cp_c_eq_d) {
    checkcp1;
    double fs = get_fpu_register_double(instruction.fr.fs);
    double ft = get_fpu_register_double(instruction.fr.ft);
    checknansd(fs, ft);

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
    double fs = get_fpu_register_double(instruction.fr.fs);
    double ft = get_fpu_register_double(instruction.fr.ft);
    checknansd(fs, ft);
    ORDERED_D(fs, ft);

    N64CPU.fcr31.compare = fs < ft;
}
MIPS_INSTR(mips_cp_c_ult_d) {
    checkcp1;
    double fs = get_fpu_register_double(instruction.fr.fs);
    double ft = get_fpu_register_double(instruction.fr.ft);
    UNORDERED_D(fs, ft);
    N64CPU.fcr31.compare = fs < ft;
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
     checknansd(fs, ft);
     */
    logfatal("Unimplemented: mips_cp_c_sf_d");
}
MIPS_INSTR(mips_cp_c_ngle_d) {
    checkcp1;
    /*
    double fs = get_fpu_register_double(instruction.fr.fs);
    double ft = get_fpu_register_double(instruction.fr.ft);
     checknansd(fs, ft);
     */
    logfatal("Unimplemented: mips_cp_c_ngle_d");
}
MIPS_INSTR(mips_cp_c_seq_d) {
    checkcp1;
    /*
    double fs = get_fpu_register_double(instruction.fr.fs);
    double ft = get_fpu_register_double(instruction.fr.ft);
     checknansd(fs, ft);
     */
    logfatal("Unimplemented: mips_cp_c_seq_d");
}
MIPS_INSTR(mips_cp_c_ngl_d) {
    checkcp1;
    /*
    double fs = get_fpu_register_double(instruction.fr.fs);
    double ft = get_fpu_register_double(instruction.fr.ft);
     checknansd(fs, ft);
     */
    logfatal("Unimplemented: mips_cp_c_ngl_d");
}
MIPS_INSTR(mips_cp_c_lt_d) {
    checkcp1;
    double fs = get_fpu_register_double(instruction.fr.fs);
    double ft = get_fpu_register_double(instruction.fr.ft);
    checknansd(fs, ft);

    N64CPU.fcr31.compare = fs < ft;
}
MIPS_INSTR(mips_cp_c_nge_d) {
    checkcp1;
    double fs = get_fpu_register_double(instruction.fr.fs);
    double ft = get_fpu_register_double(instruction.fr.ft);
    checknansd(fs, ft);
    N64CPU.fcr31.compare = !(fs >= ft);
}
MIPS_INSTR(mips_cp_c_le_d) {
    checkcp1;
    double fs = get_fpu_register_double(instruction.fr.fs);
    double ft = get_fpu_register_double(instruction.fr.ft);
    checknansd(fs, ft);

    N64CPU.fcr31.compare = fs <= ft;
}
MIPS_INSTR(mips_cp_c_ngt_d) {
    checkcp1;
    double fs = get_fpu_register_double(instruction.fr.fs);
    double ft = get_fpu_register_double(instruction.fr.ft);
    checknansd(fs, ft);
    N64CPU.fcr31.compare = !(fs > ft);
}

MIPS_INSTR(mips_cp_mov_s) {
    checkcp1;
    float value = get_fpu_register_float(instruction.fr.fs);
    checknanf(value);
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
    checknanf(value);
    set_fpu_register_float(instruction.fr.fd, -value);
}

MIPS_INSTR(mips_cp_neg_d) {
    checkcp1;
    double value = get_fpu_register_double(instruction.fr.fs);
    set_fpu_register_double(instruction.fr.fd, -value);
}

MIPS_INSTR(mips_ldc1) {
    checkcp1;
    s16 offset  = instruction.i.immediate;
    u64 address = get_register(instruction.i.rs) + offset;
    if (address & 0b111) {
        logfatal("Address error exception: misaligned dword read!");
    }

    u32 physical;
    if (!resolve_virtual_address(address, BUS_LOAD, &physical)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_LOAD), 0);
    } else {
        u64 value = n64_read_physical_dword(physical);
        set_fpu_register_dword(instruction.i.rt, value);
    }
}

MIPS_INSTR(mips_sdc1) {
    checkcp1;
    s16 offset  = instruction.fi.offset;
    u64 address = get_register(instruction.fi.base) + offset;
    u64 value   = get_fpu_register_dword(instruction.fi.ft);

    u32 physical;
    if (!resolve_virtual_address(address, BUS_LOAD, &physical)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_STORE), 0);
    } else {
        n64_write_physical_dword(physical, value);
    }
}

MIPS_INSTR(mips_lwc1) {
    checkcp1;
    s16 offset  = instruction.fi.offset;
    u64 address = get_register(instruction.fi.base) + offset;

    u32 physical;
    if (!resolve_virtual_address(address, BUS_LOAD, &physical)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_LOAD), 0);
    } else {
        u32 value = n64_read_physical_word(physical);
        set_fpu_register_word(instruction.fi.ft, value);
    }
}

MIPS_INSTR(mips_swc1) {
    checkcp1;
    s16 offset  = instruction.fi.offset;
    u64 address = get_register(instruction.fi.base) + offset;
    u32 value    = get_fpu_register_word(instruction.fi.ft);

    n64_write_word(address, value);
}

MIPS_INSTR(mips_cp1_invalid) {
    checkcp1;
    r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_FLOATING_POINT, 0);
}
