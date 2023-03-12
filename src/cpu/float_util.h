#ifndef N64_FLOAT_UTIL_H
#define N64_FLOAT_UTIL_H

#include <fenv.h>

#define F_TO_U32(f) (*((u32*)(&(f))))
#define D_TO_U64(d) (*((u64*)(&(d))))

INLINE bool is_qnan_f(float f) {
    u32 v = F_TO_U32(f);
    return (v & 0x7FC00000) == 0x7FC00000;
}

INLINE bool is_qnan_d(double d) {
    u64 v = D_TO_U64(d);
    return (v & 0x7FF8000000000000) == 0x7FF8000000000000;
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

INLINE void set_cause_inexact_operation() {
    N64CPU.fcr31.cause_inexact_operation = true;
    if (!N64CPU.fcr31.enable_inexact_operation) {
        N64CPU.fcr31.flag_inexact_operation = true;
    }
}

INLINE void set_cause_underflow() {
    N64CPU.fcr31.cause_underflow = true;
    if (!N64CPU.fcr31.enable_underflow) {
        N64CPU.fcr31.flag_underflow = true;
    }
}

INLINE void set_cause_overflow() {
    N64CPU.fcr31.cause_overflow = true;
    if (!N64CPU.fcr31.enable_overflow) {
        N64CPU.fcr31.flag_overflow = true;
    }
}

INLINE void set_cause_division_by_zero() {
    N64CPU.fcr31.cause_division_by_zero = true;
    if (!N64CPU.fcr31.enable_division_by_zero) {
        N64CPU.fcr31.flag_division_by_zero = true;
    }
}

INLINE void set_cause_invalid_operation() {
    N64CPU.fcr31.cause_invalid_operation = true;
    if (!N64CPU.fcr31.enable_invalid_operation) {
        N64CPU.fcr31.flag_invalid_operation = true;
    }
}

INLINE void set_cause_unimplemented_operation() {
    N64CPU.fcr31.cause_unimplemented_operation = true;
}

void set_cause_fpu_raised(int raised) {
    if (raised & FE_INEXACT) {
        set_cause_inexact_operation();
    }
    if (raised & FE_DIVBYZERO) {
        set_cause_division_by_zero();
    }
    if (raised & FE_UNDERFLOW) {
        set_cause_underflow();
    }
    if (raised & FE_OVERFLOW) {
        set_cause_overflow();
    }
    if (raised & FE_INVALID) {
        set_cause_invalid_operation();
    }
}

#define fpu_op_check_except(op) do { PUSHROUND; feclearexcept(FE_ALL_EXCEPT); op; set_cause_fpu_raised(fetestexcept(FE_ALL_EXCEPT)); POPROUND; } while(0)

#endif //N64_FLOAT_UTIL_H
