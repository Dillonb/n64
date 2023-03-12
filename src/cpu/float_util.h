#ifndef N64_FLOAT_UTIL_H
#define N64_FLOAT_UTIL_H

#include <fenv.h>

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

void set_cause_fpu_raised(int raised) {
    if (raised & FE_INEXACT) {
        N64CPU.fcr31.cause_inexact_operation = true;
        if (!N64CPU.fcr31.enable_inexact_operation) {
            N64CPU.fcr31.flag_inexact_operation = true;
        }
    }
    if (raised & FE_DIVBYZERO) {
        N64CPU.fcr31.cause_division_by_zero = true;
        if (!N64CPU.fcr31.enable_division_by_zero) {
            N64CPU.fcr31.flag_division_by_zero = true;
        }
    }
    if (raised & FE_UNDERFLOW) {
        N64CPU.fcr31.cause_underflow = true;
        if (!N64CPU.fcr31.enable_underflow) {
            N64CPU.fcr31.flag_underflow = true;
        }
    }
    if (raised & FE_OVERFLOW) {
        N64CPU.fcr31.cause_overflow = true;
        if (!N64CPU.fcr31.enable_overflow) {
            N64CPU.fcr31.flag_overflow = true;
        }
    }
    if (raised & FE_INVALID) {
        N64CPU.fcr31.cause_invalid_operation = true;
        if (!N64CPU.fcr31.enable_invalid_operation) {
            N64CPU.fcr31.flag_invalid_operation = true;
        }
    }
}

#define fpu_op_check_except(op) do { feclearexcept(FE_ALL_EXCEPT); op; set_cause_fpu_raised(fetestexcept(FE_ALL_EXCEPT)); } while(0)

#endif //N64_FLOAT_UTIL_H
