#include "fpu_instructions.h"

#include <util.h>
#include <mem/n64bus.h>
#include <math.h>
#include <float.h>

#include "r4300i_register_access.h"
#include "float_util.h"

INLINE bool fire_fpu_exception() {
    // unimplemented operation is always enabled - there's not even a bit in `enable` for it.
    u32 enable = N64CPU.fcr31.enable | (1 << 5);
    if (N64CPU.fcr31.cause & enable) {
        r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_FLOATING_POINT, 0);
        return true;
    }
    return false;
}

#define check_fpu_exception() do { if (fire_fpu_exception()) { return; } } while(0)

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

#define check_fpu_arg_s(f) logfatal("unimplemented on Windows");
#define check_fpu_result_s(f) logfatal("unimplemented on Windows");

#else

#define unordered_s(a, b) (isnanf(a) || isnanf(b))
#define unordered_d(a, b) (isnan(a) || isnan(b))

#define checknanf(value) if (isnanf(value)) { logfatal("value == NaN!"); }
#define checknand(value) if (isnan(value)) { logfatal("value == NaN!"); }

INLINE void set_cause_fpu_arg_s(float f) {
    int classification = fpclassify(f);
    switch (classification) {
        case FP_NAN:
            if (is_qnan_f(f)) {
                set_cause_invalid_operation();
            } else {
                set_cause_unimplemented_operation();
            }
            break;
        case FP_SUBNORMAL:
            set_cause_unimplemented_operation();
            break;

        case FP_INFINITE:
        case FP_ZERO:
        case FP_NORMAL:
            break; // No-op, these are fine.
        default:
            logfatal("Unknown FP classification: %d", classification);
    }
}

INLINE void set_cause_fpu_arg_d(double d) {
    int classification = fpclassify(d);
    switch (classification) {
        case FP_NAN:
            if (is_qnan_d(d)) {
                set_cause_invalid_operation();
            } else {
                set_cause_unimplemented_operation();
            }
            break;
        case FP_SUBNORMAL:
            set_cause_unimplemented_operation();
            break;

        case FP_INFINITE:
        case FP_ZERO:
        case FP_NORMAL:
            break; // No-op, these are fine.
        default:
            logfatal("Unknown FP classification: %d", classification);
    }
}

#define check_fpu_arg_s(f) do { set_cause_fpu_arg_s(f); check_fpu_exception(); } while(0)
#define check_fpu_arg_d(d) do { set_cause_fpu_arg_d(d); check_fpu_exception(); } while(0)

INLINE void set_cause_fpu_result_s(float* f) {
    int classification = fpclassify(*f);
    switch (classification) {
        case FP_NAN:
            F_TO_U32(*f) = 0x7FBFFFFF; // set result to sNAN
            break;
        case FP_SUBNORMAL:
            if (!N64CPU.fcr31.flush_subnormals || N64CPU.fcr31.enable_underflow || N64CPU.fcr31.enable_inexact_operation) {
                set_cause_unimplemented_operation();
            } else {
                // Since the if statement checks for the corresponding enable bits, it's safe to turn these cause bits on here.
                set_cause_underflow();
                set_cause_inexact_operation();
                switch (N64CPU.fcr31.rounding_mode) {
                    case R4300I_CP1_ROUND_NEAREST:
                    case R4300I_CP1_ROUND_ZERO:
                        *f = copysignf(0, *f);
                        break;
                    case R4300I_CP1_ROUND_POSINF:
                        if (signbit(*f)) {
                            *f = -(float)0;
                        } else {
                            *f = FLT_MIN;
                        }
                        break;
                    case R4300I_CP1_ROUND_NEGINF:
                        if (signbit(*f)) {
                            *f = -FLT_MIN;
                        } else {
                            *f = 0;
                        }
                        break;
                }
            }
            break;
        case FP_INFINITE:
        case FP_ZERO:
        case FP_NORMAL:
            break; // No-op, these are fine.
        default:
            logfatal("Unknown FP classification: %d", classification);
    }
}

INLINE void set_cause_fpu_result_d(double* d) {
    int classification = fpclassify(*d);
    switch (classification) {
        case FP_NAN:
            D_TO_U64(*d) = 0x7FF7FFFFFFFFFFFF; // set result to sNAN
            break;
        case FP_SUBNORMAL:
            if (!N64CPU.fcr31.flush_subnormals || N64CPU.fcr31.enable_underflow || N64CPU.fcr31.enable_inexact_operation) {
                set_cause_unimplemented_operation();
            } else {
                // Since the if statement checks for the corresponding enable bits, it's safe to turn these cause bits on here.
                set_cause_underflow();
                set_cause_inexact_operation();
                switch (N64CPU.fcr31.rounding_mode) {
                    case R4300I_CP1_ROUND_NEAREST:
                    case R4300I_CP1_ROUND_ZERO:
                        *d = copysign(0, *d);
                        break;
                    case R4300I_CP1_ROUND_POSINF:
                        if (signbit(*d)) {
                            *d = -(double)0;
                        } else {
                            *d = DBL_MIN;
                        }
                        break;
                    case R4300I_CP1_ROUND_NEGINF:
                        if (signbit(*d)) {
                            *d = -DBL_MIN;
                        } else {
                            *d = 0;
                        }
                        break;
                }
            }
            break;
        case FP_INFINITE:
        case FP_ZERO:
        case FP_NORMAL:
            break; // No-op, these are fine.
        default:
            logfatal("Unknown FP classification: %d", classification);
    }
}

#define check_fpu_result_s(f) do { set_cause_fpu_result_s(&(f)); check_fpu_exception(); } while(0)
#define check_fpu_result_d(d) do { set_cause_fpu_result_d(&(d)); check_fpu_exception(); } while(0)
#endif

#define FPU_OP_S(op) do {                                    \
    checkcp1;                                                \
    float fs = get_fpu_register_float_fs(instruction.fr.fs); \
    float ft = get_fpu_register_float_ft(instruction.fr.ft); \
    check_fpu_arg_s(fs);                                     \
    check_fpu_arg_s(ft);                                     \
    float result;                                            \
    fpu_op_check_except({ result = (op); });                 \
    check_fpu_result_s(result);                              \
    set_fpu_register_float(instruction.fr.fd, result);       \
} while(0)

#define FPU_OP_D(op) do {                                      \
    checkcp1;                                                  \
    double fs = get_fpu_register_double_fs(instruction.fr.fs); \
    double ft = get_fpu_register_double_ft(instruction.fr.ft); \
    check_fpu_arg_d(fs);                                       \
    check_fpu_arg_d(ft);                                       \
    double result;                                             \
    fpu_op_check_except({ result = (op); });                   \
    check_fpu_result_d(result);                                \
    set_fpu_register_double(instruction.fr.fd, result);        \
} while(0)

MIPS_INSTR(mips_mfc1) {
    checkcp1;
    s32 value = get_fpu_register_word_fr(instruction.fr.fs);
    set_register(instruction.r.rt, (s64)value);
}

MIPS_INSTR(mips_dmfc1) {
    checkcp1;
    u64 value = get_fpu_register_dword_fr(instruction.fr.fs);
    set_register(instruction.r.rt, value);
}

MIPS_INSTR(mips_mtc1) {
    checkcp1;
    u32 value = get_register(instruction.r.rt);
    set_fpu_register_word_fr(instruction.r.rd, value);
}

MIPS_INSTR(mips_dmtc1) {
    checkcp1;
    u64 value = get_register(instruction.r.rt);
    set_fpu_register_dword_fr(instruction.r.rd, value);
}

MIPS_INSTR(mips_cfc1) {
    checkcp1_preservecause;
    u8 fs = instruction.r.rd;
    s32 value;
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

    set_register(instruction.r.rt, (s64)value);
}

MIPS_INSTR(mips_ctc1) {
    checkcp1_preservecause;
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
    FPU_OP_D(fs * ft);
}

MIPS_INSTR(mips_cp_mul_s) {
    FPU_OP_S(fs * ft);
}

MIPS_INSTR(mips_cp_div_d) {
    FPU_OP_D(fs / ft);
}

MIPS_INSTR(mips_cp_div_s) {
    FPU_OP_S(fs / ft);
}

MIPS_INSTR(mips_cp_add_d) {
    FPU_OP_D(fs + ft);
}

MIPS_INSTR(mips_cp_add_s) {
    FPU_OP_S(fs + ft);
}

MIPS_INSTR(mips_cp_sub_d) {
    FPU_OP_D(fs - ft);
}

MIPS_INSTR(mips_cp_sub_s) {
    FPU_OP_S(fs - ft);
}

MIPS_INSTR(mips_cp_trunc_l_d) {
    checkcp1;
    double value = get_fpu_register_double_fs(instruction.fr.fs);
    checknand(value);
    u64 truncated = trunc(value);
    set_fpu_register_dword(instruction.fr.fd, truncated);
}

MIPS_INSTR(mips_cp_round_l_d) {
    checkcp1;
    double value = get_fpu_register_double_fs(instruction.fr.fs);
    PUSHROUND;
    u64 truncated = nearbyint(value);
    POPROUND;
    set_fpu_register_dword(instruction.fr.fd, truncated);
}

MIPS_INSTR(mips_cp_trunc_l_s) {
    checkcp1;
    float value = get_fpu_register_float_fs(instruction.fr.fs);
    checknanf(value);
    u64 truncated = truncf(value);
    set_fpu_register_dword(instruction.fr.fd, truncated);
}

MIPS_INSTR(mips_cp_round_l_s) {
    checkcp1;
    float value = get_fpu_register_float_fs(instruction.fr.fs);
    checknanf(value);
    PUSHROUND;
    u64 truncated = nearbyintf(value);
    POPROUND;
    set_fpu_register_dword(instruction.fr.fd, truncated);
}

MIPS_INSTR(mips_cp_trunc_w_d) {
    checkcp1;
    double value = get_fpu_register_double_fs(instruction.fr.fs);
    u32 truncated = trunc(value);
    set_fpu_register_word(instruction.fr.fd, truncated);
}

MIPS_INSTR(mips_cp_round_w_d) {
    checkcp1;
    double value = get_fpu_register_double_fs(instruction.fr.fs);
    PUSHROUND;
    u32 truncated = nearbyint(value);
    POPROUND;
    set_fpu_register_word(instruction.fr.fd, truncated);
}

MIPS_INSTR(mips_cp_trunc_w_s) {
    checkcp1;
    float value = get_fpu_register_float_fs(instruction.fr.fs);
    checknanf(value);
    s32 truncated = truncf(value);
    set_fpu_register_word(instruction.fr.fd, truncated);
}

MIPS_INSTR(mips_cp_ceil_l_d) {
    checkcp1;
    double value = get_fpu_register_double_fs(instruction.fr.fs);
    checknand(value);
    u64 truncated = ceil(value);
    set_fpu_register_dword(instruction.fr.fd, truncated);
}

MIPS_INSTR(mips_cp_ceil_l_s) {
    checkcp1;
    float value = get_fpu_register_float_fs(instruction.fr.fs);
    checknand(value);
    u64 truncated = ceilf(value);
    set_fpu_register_dword(instruction.fr.fd, truncated);
}

MIPS_INSTR(mips_cp_ceil_w_d) {
    checkcp1;
    double value = get_fpu_register_double_fs(instruction.fr.fs);
    checknanf(value);
    s32 truncated = ceil(value);
    set_fpu_register_word(instruction.fr.fd, truncated);
}

MIPS_INSTR(mips_cp_ceil_w_s) {
    checkcp1;
    float value = get_fpu_register_float_fs(instruction.fr.fs);
    checknanf(value);
    s32 truncated = ceilf(value);
    set_fpu_register_word(instruction.fr.fd, truncated);
}

MIPS_INSTR(mips_cp_floor_l_d) {
    checkcp1;
    double value = get_fpu_register_double_fs(instruction.fr.fs);
    checknand(value);
    u64 truncated = floor(value);
    set_fpu_register_dword(instruction.fr.fd, truncated);
}

MIPS_INSTR(mips_cp_floor_l_s) {
    checkcp1;
    float value = get_fpu_register_float_fs(instruction.fr.fs);
    checknand(value);
    u64 truncated = floorf(value);
    set_fpu_register_dword(instruction.fr.fd, truncated);
}

MIPS_INSTR(mips_cp_floor_w_d) {
    checkcp1;
    double value = get_fpu_register_double_fs(instruction.fr.fs);
    checknanf(value);
    s32 truncated = floor(value);
    set_fpu_register_word(instruction.fr.fd, truncated);
}

MIPS_INSTR(mips_cp_floor_w_s) {
    checkcp1;
    float value = get_fpu_register_float_fs(instruction.fr.fs);
    checknanf(value);
    s32 truncated = floorf(value);
    set_fpu_register_word(instruction.fr.fd, truncated);
}

MIPS_INSTR(mips_cp_round_w_s) {
    checkcp1;
    float value = get_fpu_register_float_fs(instruction.fr.fs);
    checknanf(value);
    PUSHROUND;
    s32 truncated = nearbyintf(value);
    POPROUND;
    set_fpu_register_word(instruction.fr.fd, truncated);
}

MIPS_INSTR(mips_cp_cvt_d_s) {
    checkcp1;
    float fs = get_fpu_register_float_fs(instruction.fr.fs);
    double converted = fs;
    set_fpu_register_double(instruction.fr.fd, converted);
}

MIPS_INSTR(mips_cp_cvt_d_w) {
    checkcp1;
    s32 fs = get_fpu_register_word_fs(instruction.fr.fs);
    double converted = fs;
    set_fpu_register_double(instruction.fr.fd, converted);
}

MIPS_INSTR(mips_cp_cvt_d_l) {
    checkcp1;
    s64 fs = get_fpu_register_dword_fr(instruction.fr.fs);
    double converted = fs;
    set_fpu_register_double(instruction.fr.fd, converted);
}

MIPS_INSTR(mips_cp_cvt_l_s) {
    checkcp1;
    float fs = get_fpu_register_float_fs(instruction.fr.fs);
    s64 converted = fs;
    set_fpu_register_dword(instruction.fr.fd, converted);
}

MIPS_INSTR(mips_cp_cvt_l_d) {
    checkcp1;
    double fs = get_fpu_register_double_fs(instruction.fr.fs);
    s64 converted = fs;
    set_fpu_register_dword(instruction.fr.fd, converted);
}

MIPS_INSTR(mips_cp_cvt_s_d) {
    checkcp1;
    double fs = get_fpu_register_double_fs(instruction.fr.fs);
    float converted = fs;
    set_fpu_register_float(instruction.fr.fd, converted);
}

MIPS_INSTR(mips_cp_cvt_s_w) {
    checkcp1;
    s32 fs = get_fpu_register_word_fs(instruction.fr.fs);
    float converted = fs;
    set_fpu_register_float(instruction.fr.fd, converted);
}

MIPS_INSTR(mips_cp_cvt_s_l) {
    checkcp1;
    s64 fs = get_fpu_register_dword_fr(instruction.fr.fs);
    float converted = fs;
    set_fpu_register_float(instruction.fr.fd, converted);
}

MIPS_INSTR(mips_cp_cvt_w_s) {
    checkcp1;
    float fs = get_fpu_register_float_fs(instruction.fr.fs);
    s32 converted = fs;
    set_fpu_register_word(instruction.fr.fd, converted);
}

MIPS_INSTR(mips_cp_cvt_w_d) {
    checkcp1;
    double fs = get_fpu_register_double_fs(instruction.fr.fs);
    s32 converted = fs;
    set_fpu_register_word(instruction.fr.fd, converted);
}

MIPS_INSTR(mips_cp_sqrt_s) {
    FPU_OP_S(sqrtf(fs));
}

MIPS_INSTR(mips_cp_sqrt_d) {
    FPU_OP_D(sqrt(fs));
}

MIPS_INSTR(mips_cp_abs_s) {
    FPU_OP_S(fabsf(fs));
}

MIPS_INSTR(mips_cp_abs_d) {
    FPU_OP_D(fabs(fs));
}


#define assert_is_float(f) do { static_assert(sizeof(f) == sizeof(float), #f " is not a float!");} while(0)
#define assert_is_double(d) do { static_assert(sizeof(d) == sizeof(double), #d " is not a double!");} while(0)

#define check_qnans_f(fs, ft) do { assert_is_float(fs); assert_is_float(ft); if (is_qnan_f(fs) || is_qnan_f(ft)) { set_cause_invalid_operation(); check_fpu_exception(); } } while (0)
#define check_qnans_d(fs, ft) do { assert_is_double(fs); assert_is_double(ft); if (is_qnan_d(fs) || is_qnan_d(ft)) { set_cause_invalid_operation(); check_fpu_exception(); } } while (0)

#define check_nans_f(fs, ft) do { assert_is_float(fs); assert_is_float(ft); if (is_nan_f(fs) || is_nan_f(ft)) { set_cause_invalid_operation(); check_fpu_exception(); } } while (0)
#define check_nans_d(fs, ft) do { assert_is_double(fs); assert_is_double(ft); if (is_nan_d(fs) || is_nan_d(ft)) { set_cause_invalid_operation(); check_fpu_exception(); } } while (0)

MIPS_INSTR(mips_cp_c_f_s) {
    checkcp1;
    float fs = get_fpu_register_float_fs(instruction.fr.fs);
    float ft = get_fpu_register_float_ft(instruction.fr.ft);
    check_qnans_f(fs, ft);
    N64CPU.fcr31.compare = false;
}

MIPS_INSTR(mips_cp_c_f_d) {
    checkcp1;
    double fs = get_fpu_register_double_fs(instruction.fr.fs);
    double ft = get_fpu_register_double_ft(instruction.fr.ft);
    check_qnans_d(fs, ft);
    N64CPU.fcr31.compare = false;
}

MIPS_INSTR(mips_cp_c_un_s) {
    checkcp1;
    float fs = get_fpu_register_float_fs(instruction.fr.fs);
    float ft = get_fpu_register_float_ft(instruction.fr.ft);
    check_qnans_f(fs, ft);
    N64CPU.fcr31.compare = unordered_s(fs, ft);
}

MIPS_INSTR(mips_cp_c_un_d) {
    checkcp1;
    double fs = get_fpu_register_double_fs(instruction.fr.fs);
    double ft = get_fpu_register_double_ft(instruction.fr.ft);
    check_qnans_d(fs, ft);
    N64CPU.fcr31.compare = unordered_d(fs, ft);
}

MIPS_INSTR(mips_cp_c_eq_s) {
    checkcp1;
    float fs = get_fpu_register_float_fs(instruction.fr.fs);
    float ft = get_fpu_register_float_ft(instruction.fr.ft);
    check_qnans_f(fs, ft);
    N64CPU.fcr31.compare = fs == ft;
}

MIPS_INSTR(mips_cp_c_eq_d) {
    checkcp1;
    double fs = get_fpu_register_double_fs(instruction.fr.fs);
    double ft = get_fpu_register_double_ft(instruction.fr.ft);
    check_qnans_d(fs, ft);
    N64CPU.fcr31.compare = fs == ft;
}

MIPS_INSTR(mips_cp_c_ueq_s) {
    checkcp1;
    float fs = get_fpu_register_float_fs(instruction.fr.fs);
    float ft = get_fpu_register_float_ft(instruction.fr.ft);
    check_qnans_f(fs, ft);
    N64CPU.fcr31.compare = fs == ft || unordered_s(fs, ft);
}

MIPS_INSTR(mips_cp_c_ueq_d) {
    checkcp1;
    double fs = get_fpu_register_double_fs(instruction.fr.fs);
    double ft = get_fpu_register_double_ft(instruction.fr.ft);
    check_qnans_d(fs, ft);
    N64CPU.fcr31.compare = fs == ft || unordered_d(fs, ft);
}

MIPS_INSTR(mips_cp_c_olt_s) {
    checkcp1;
    float fs = get_fpu_register_float_fs(instruction.fr.fs);
    float ft = get_fpu_register_float_ft(instruction.fr.ft);
    check_qnans_f(fs, ft);
    N64CPU.fcr31.compare = fs < ft;
}

MIPS_INSTR(mips_cp_c_olt_d) {
    checkcp1;
    double fs = get_fpu_register_double_fs(instruction.fr.fs);
    double ft = get_fpu_register_double_ft(instruction.fr.ft);
    check_qnans_d(fs, ft);
    N64CPU.fcr31.compare = fs < ft;
}

MIPS_INSTR(mips_cp_c_ult_s) {
    checkcp1;
    float fs = get_fpu_register_float_fs(instruction.fr.fs);
    float ft = get_fpu_register_float_ft(instruction.fr.ft);
    check_qnans_f(fs, ft);
    N64CPU.fcr31.compare = fs < ft || unordered_s(fs, ft);
}

MIPS_INSTR(mips_cp_c_ult_d) {
    checkcp1;
    double fs = get_fpu_register_double_fs(instruction.fr.fs);
    double ft = get_fpu_register_double_ft(instruction.fr.ft);
    check_qnans_d(fs, ft);
    N64CPU.fcr31.compare = fs < ft || unordered_d(fs, ft);
}

MIPS_INSTR(mips_cp_c_ole_s) {
    checkcp1;
    float fs = get_fpu_register_float_fs(instruction.fr.fs);
    float ft = get_fpu_register_float_ft(instruction.fr.ft);
    check_qnans_f(fs, ft);
    N64CPU.fcr31.compare = fs <= ft;
}

MIPS_INSTR(mips_cp_c_ole_d) {
    checkcp1;
    double fs = get_fpu_register_double_fs(instruction.fr.fs);
    double ft = get_fpu_register_double_ft(instruction.fr.ft);
    check_qnans_d(fs, ft);
    N64CPU.fcr31.compare = fs <= ft;
}

MIPS_INSTR(mips_cp_c_ule_s) {
    checkcp1;
    float fs = get_fpu_register_float_fs(instruction.fr.fs);
    float ft = get_fpu_register_float_ft(instruction.fr.ft);
    check_qnans_f(fs, ft);
    N64CPU.fcr31.compare = fs <= ft || unordered_s(fs, ft);
}

MIPS_INSTR(mips_cp_c_ule_d) {
    checkcp1;
    double fs = get_fpu_register_double_fs(instruction.fr.fs);
    double ft = get_fpu_register_double_ft(instruction.fr.ft);
    check_qnans_d(fs, ft);
    N64CPU.fcr31.compare = fs <= ft || unordered_d(fs, ft);
}

MIPS_INSTR(mips_cp_c_sf_s) {
    checkcp1;
    float fs = get_fpu_register_float_fs(instruction.fr.fs);
    float ft = get_fpu_register_float_ft(instruction.fr.ft);
    check_nans_f(fs, ft);
    N64CPU.fcr31.compare = false;
}

MIPS_INSTR(mips_cp_c_sf_d) {
    checkcp1;
    double fs = get_fpu_register_double_fs(instruction.fr.fs);
    double ft = get_fpu_register_double_ft(instruction.fr.ft);
    check_nans_d(fs, ft);
    N64CPU.fcr31.compare = false;
}

MIPS_INSTR(mips_cp_c_ngle_s) {
    checkcp1;
    float fs = get_fpu_register_float_fs(instruction.fr.fs);
    float ft = get_fpu_register_float_ft(instruction.fr.ft);
    check_nans_f(fs, ft);
    N64CPU.fcr31.compare = unordered_s(fs, ft);
}

MIPS_INSTR(mips_cp_c_ngle_d) {
    checkcp1;
    double fs = get_fpu_register_double_fs(instruction.fr.fs);
    double ft = get_fpu_register_double_ft(instruction.fr.ft);
    check_nans_d(fs, ft);
    N64CPU.fcr31.compare = unordered_d(fs, ft);
}

MIPS_INSTR(mips_cp_c_seq_s) {
    checkcp1;
    float fs = get_fpu_register_float_fs(instruction.fr.fs);
    float ft = get_fpu_register_float_ft(instruction.fr.ft);
    check_nans_f(fs, ft);
    N64CPU.fcr31.compare = fs == ft;
}

MIPS_INSTR(mips_cp_c_seq_d) {
    checkcp1;
    double fs = get_fpu_register_double_fs(instruction.fr.fs);
    double ft = get_fpu_register_double_ft(instruction.fr.ft);
    check_nans_d(fs, ft);
    N64CPU.fcr31.compare = fs == ft;
}

MIPS_INSTR(mips_cp_c_ngl_s) {
    checkcp1;
    float fs = get_fpu_register_float_fs(instruction.fr.fs);
    float ft = get_fpu_register_float_ft(instruction.fr.ft);
    check_nans_f(fs, ft);
    N64CPU.fcr31.compare = fs == ft || unordered_s(fs, ft);
}

MIPS_INSTR(mips_cp_c_ngl_d) {
    checkcp1;
    double fs = get_fpu_register_double_fs(instruction.fr.fs);
    double ft = get_fpu_register_double_ft(instruction.fr.ft);
    check_nans_d(fs, ft);
    N64CPU.fcr31.compare = fs == ft || unordered_d(fs, ft);
}

MIPS_INSTR(mips_cp_c_lt_s) {
    checkcp1;
    float fs = get_fpu_register_float_fs(instruction.fr.fs);
    float ft = get_fpu_register_float_ft(instruction.fr.ft);
    check_nans_f(fs, ft);
    N64CPU.fcr31.compare = fs < ft;
}

MIPS_INSTR(mips_cp_c_lt_d) {
    checkcp1;
    double fs = get_fpu_register_double_fs(instruction.fr.fs);
    double ft = get_fpu_register_double_ft(instruction.fr.ft);
    check_nans_d(fs, ft);
    N64CPU.fcr31.compare = fs < ft;
}

MIPS_INSTR(mips_cp_c_nge_s) {
    checkcp1;
    float fs = get_fpu_register_float_fs(instruction.fr.fs);
    float ft = get_fpu_register_float_ft(instruction.fr.ft);
    check_nans_f(fs, ft);
    N64CPU.fcr31.compare = fs < ft || unordered_s(fs, ft);
}

MIPS_INSTR(mips_cp_c_nge_d) {
    checkcp1;
    double fs = get_fpu_register_double_fs(instruction.fr.fs);
    double ft = get_fpu_register_double_ft(instruction.fr.ft);
    check_nans_d(fs, ft);
    N64CPU.fcr31.compare = fs < ft || unordered_s(fs, ft);
}

MIPS_INSTR(mips_cp_c_le_s) {
    checkcp1;
    float fs = get_fpu_register_float_fs(instruction.fr.fs);
    float ft = get_fpu_register_float_ft(instruction.fr.ft);
    check_nans_f(fs, ft);
    N64CPU.fcr31.compare = fs <= ft;
}

MIPS_INSTR(mips_cp_c_le_d) {
    checkcp1;
    double fs = get_fpu_register_double_fs(instruction.fr.fs);
    double ft = get_fpu_register_double_ft(instruction.fr.ft);
    check_nans_d(fs, ft);
    N64CPU.fcr31.compare = fs <= ft;
}

MIPS_INSTR(mips_cp_c_ngt_s) {
    checkcp1;
    float fs = get_fpu_register_float_fs(instruction.fr.fs);
    float ft = get_fpu_register_float_ft(instruction.fr.ft);
    check_nans_f(fs, ft);
    N64CPU.fcr31.compare = fs <= ft || unordered_s(fs, ft);
}

MIPS_INSTR(mips_cp_c_ngt_d) {
    checkcp1;
    double fs = get_fpu_register_double_fs(instruction.fr.fs);
    double ft = get_fpu_register_double_ft(instruction.fr.ft);
    check_nans_d(fs, ft);
    N64CPU.fcr31.compare = fs <= ft || unordered_s(fs, ft);
}

MIPS_INSTR(mips_cp_mov_s) {
    checkcp1_preservecause;
    u64 value = get_fpu_register_dword_fr(instruction.fr.fs);
    set_fpu_register_dword(instruction.fr.fd, value);
}

MIPS_INSTR(mips_cp_mov_d) {
    checkcp1_preservecause;
    double value = get_fpu_register_double_fs(instruction.fr.fs);
    set_fpu_register_double(instruction.fr.fd, value);
}

MIPS_INSTR(mips_cp_neg_s) {
    FPU_OP_S(-fs);
}

MIPS_INSTR(mips_cp_neg_d) {
    FPU_OP_D(-fs);
}

MIPS_INSTR(mips_ldc1) {
    checkcp1_preservecause;
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
        set_fpu_register_dword_fr(instruction.i.rt, value);
    }
}

MIPS_INSTR(mips_sdc1) {
    checkcp1_preservecause;
    s16 offset  = instruction.fi.offset;
    u64 address = get_register(instruction.fi.base) + offset;
    u64 value   = get_fpu_register_dword_fr(instruction.fi.ft);

    u32 physical;
    if (!resolve_virtual_address(address, BUS_LOAD, &physical)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_STORE), 0);
    } else {
        n64_write_physical_dword(physical, value);
    }
}

MIPS_INSTR(mips_lwc1) {
    checkcp1_preservecause;
    s16 offset  = instruction.fi.offset;
    u64 address = get_register(instruction.fi.base) + offset;

    u32 physical;
    if (!resolve_virtual_address(address, BUS_LOAD, &physical)) {
        on_tlb_exception(address);
        r4300i_handle_exception(N64CPU.prev_pc, get_tlb_exception_code(N64CP0.tlb_error, BUS_LOAD), 0);
    } else {
        u32 value = n64_read_physical_word(physical);
        set_fpu_register_word_fr(instruction.fi.ft, value);
    }
}

MIPS_INSTR(mips_swc1) {
    checkcp1_preservecause;
    s16 offset  = instruction.fi.offset;
    u64 address = get_register(instruction.fi.base) + offset;
    u32 value    = get_fpu_register_word_fr(instruction.fi.ft);

    n64_write_word(address, value);
}

MIPS_INSTR(mips_cp1_invalid) {
    checkcp1;
    r4300i_handle_exception(N64CPU.prev_pc, EXCEPTION_FLOATING_POINT, 0);
}
