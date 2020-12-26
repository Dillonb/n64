#ifndef N64_R4300I_REGISTER_ACCESS_H
#define N64_R4300I_REGISTER_ACCESS_H

#include "r4300i.h"

INLINE void set_register(r4300i_t* cpu, byte r, dword value) {
    logtrace("Setting $%s (r%d) to [0x%016lX]", register_names[r], r, value);
    if (r != 0) {
        cpu->gpr[r] = value;
    }
}

INLINE dword get_register(r4300i_t* cpu, byte r) {
    dword value = cpu->gpr[r];
    logtrace("Reading $%s (r%d): 0x%08lX", register_names[r], r, value);
    return value;
}

INLINE void log_status(cp0_status_t status) {
    loginfo("    CP0 status: ie:  %d", status.ie);
    loginfo("    CP0 status: exl: %d", status.exl);
    loginfo("    CP0 status: erl: %d", status.erl);
    loginfo("    CP0 status: ksu: %d", status.ksu);
    loginfo("    CP0 status: ux:  %d", status.ux);
    loginfo("    CP0 status: sx:  %d", status.sx);
    loginfo("    CP0 status: kx:  %d", status.kx);
    loginfo("    CP0 status: im:  %d", status.im);
    loginfo("    CP0 status: ds:  %d", status.ds);
    loginfo("    CP0 status: re:  %d", status.re);
    loginfo("    CP0 status: fr:  %d", status.fr);
    loginfo("    CP0 status: rp:  %d", status.rp);
    loginfo("    CP0 status: cu0: %d", status.cu0);
    loginfo("    CP0 status: cu1: %d", status.cu1);
    loginfo("    CP0 status: cu2: %d", status.cu2);
    loginfo("    CP0 status: cu3: %d", status.cu3);
}

INLINE void set_cp0_register(r4300i_t* cpu, byte r, word value) {
    switch (r) {
        case R4300I_CP0_REG_INDEX:
            cpu->cp0.index = value;
            break;
        case R4300I_CP0_REG_RANDOM:
            break;
        case R4300I_CP0_REG_COUNT:
            cpu->cp0.count = value;
            break;
        case R4300I_CP0_REG_CAUSE: {
            cp0_cause_t newcause;
            newcause.raw = value;
            cpu->cp0.cause.ip0 = newcause.ip0;
            cpu->cp0.cause.ip1 = newcause.ip1;
            break;
        }
        case R4300I_CP0_REG_TAGLO: // Used for the cache, which is unimplemented.
            cpu->cp0.tag_lo = value;
            break;
        case R4300I_CP0_REG_TAGHI: // Used for the cache, which is unimplemented.
            cpu->cp0.tag_hi = value;
            break;
        case R4300I_CP0_REG_COMPARE:
            loginfo("$Compare written with 0x%08X (count is now 0x%08lX)", value, cpu->cp0.count);
            cpu->cp0.cause.ip7 = false;
            cpu->cp0.compare = value;
            break;
        case R4300I_CP0_REG_STATUS: {
            cpu->cp0.status.raw &= ~CP0_STATUS_WRITE_MASK;
            cpu->cp0.status.raw |= value & CP0_STATUS_WRITE_MASK;

            unimplemented(cpu->cp0.status.re, "Reverse endian bit set in CP0");
            unimplemented(cpu->cp0.status.kx, "KX bit set in CP0 - 64 bit addressing in kernel mode enabled");
            unimplemented(cpu->cp0.status.sx, "SX bit set in CP0 - 64 bit addressing in supervisor mode enabled");
            unimplemented(cpu->cp0.status.ux, "UX bit set in CP0 - 64 bit addressing in user mode enabled");
            unimplemented(cpu->cp0.status.ksu, "KSU != 0, leaving kernel mode!");

            log_status(cpu->cp0.status);

            r4300i_interrupt_update(cpu);
            break;
        }
        case R4300I_CP0_REG_ENTRYLO0:
            cpu->cp0.entry_lo0.raw = value;
            break;
        case R4300I_CP0_REG_ENTRYLO1:
            cpu->cp0.entry_lo1.raw = value;
            break;
        case 7:
            logfatal("CP0 Reg 7 write?");
            break;
        case R4300I_CP0_REG_ENTRYHI:
            cpu->cp0.entry_hi.raw = value;
            break;
        case R4300I_CP0_REG_PAGEMASK:
            cpu->cp0.page_mask.raw = value;
            break;
        case R4300I_CP0_REG_EPC:
            cpu->cp0.EPC = value;
            break;
        case R4300I_CP0_REG_CONFIG:
            cpu->cp0.config &= ~CP0_CONFIG_WRITE_MASK;
            cpu->cp0.config |= value & CP0_CONFIG_WRITE_MASK;
            break;
        case R4300I_CP0_REG_WATCHLO:
            cpu->cp0.watch_lo.raw = value;
            unimplemented(cpu->cp0.watch_lo.r, "Read exception enabled in CP0 watch_lo!");
            unimplemented(cpu->cp0.watch_lo.w, "Write exception enabled in CP0 watch_lo!");
            break;
        case R4300I_CP0_REG_WATCHHI:
            cpu->cp0.watch_hi = value;
            break;
        case R4300I_CP0_REG_WIRED:
            cpu->cp0.wired = value;
            break;
        case R4300I_CP0_REG_CONTEXT:
            unimplemented(value != 0, "cp0 context written with non-zero value");
            cpu->cp0.context = value;
            break;
        default:
            logfatal("Unsupported CP0 $%s (%d) set: 0x%08X", cp0_register_names[r], r, value);
    }

    loginfo("CP0 $%s = 0x%08X", cp0_register_names[r], value);
}

INLINE word get_cp0_register(r4300i_t* cpu, byte r) {
    switch (r) {
        case R4300I_CP0_REG_INDEX:
            return cpu->cp0.index & 0x8000003F;
        case R4300I_CP0_REG_RANDOM:
            logalways("WARNING! Stubbed read from Random!");
            return 1;
        case R4300I_CP0_REG_ENTRYLO0:
            return cpu->cp0.entry_lo0.raw;
        case R4300I_CP0_REG_ENTRYLO1:
            return cpu->cp0.entry_lo1.raw;
        case R4300I_CP0_REG_CONTEXT:
            return cpu->cp0.context;
        case R4300I_CP0_REG_PAGEMASK:
            return cpu->cp0.page_mask.raw;
        case R4300I_CP0_REG_WIRED:
            return cpu->cp0.wired;
        case R4300I_CP0_REG_7:
            return cpu->cp0.r7;
        case R4300I_CP0_REG_BADVADDR:
            return cpu->cp0.bad_vaddr;
        case R4300I_CP0_REG_COUNT: {
            dword shifted = cpu->cp0.count >> 1;
            return (word)shifted;
        }
        case R4300I_CP0_REG_ENTRYHI:
            return cpu->cp0.entry_hi.raw;
        case R4300I_CP0_REG_COMPARE:
            return cpu->cp0.compare;
        case R4300I_CP0_REG_STATUS:
            return cpu->cp0.status.raw;
        case R4300I_CP0_REG_CAUSE:
            return cpu->cp0.cause.raw;
        case R4300I_CP0_REG_EPC:
            return cpu->cp0.EPC;
        case R4300I_CP0_REG_PRID:
            return cpu->cp0.PRId;
        case R4300I_CP0_REG_CONFIG:
            return cpu->cp0.config;
        case R4300I_CP0_REG_LLADDR:
            return cpu->cp0.lladdr;
        case R4300I_CP0_REG_WATCHLO:
            return cpu->cp0.watch_lo.raw;
        case R4300I_CP0_REG_WATCHHI:
            return cpu->cp0.watch_hi;
        case R4300I_CP0_REG_XCONTEXT:
            return cpu->cp0.x_context;
        case R4300I_CP0_REG_21:
            return cpu->cp0.r21;
        case R4300I_CP0_REG_22:
            return cpu->cp0.r22;
        case R4300I_CP0_REG_23:
            return cpu->cp0.r23;
        case R4300I_CP0_REG_24:
            return cpu->cp0.r24;
        case R4300I_CP0_REG_25:
            return cpu->cp0.r25;
        case R4300I_CP0_REG_PARITYER:
            return cpu->cp0.parity_error;
        case R4300I_CP0_REG_CACHEER:
            return cpu->cp0.cache_error;
        case R4300I_CP0_REG_TAGLO:
            return cpu->cp0.tag_lo;
        case R4300I_CP0_REG_TAGHI:
            return cpu->cp0.tag_hi;
        case R4300I_CP0_REG_ERR_EPC:
            return cpu->cp0.error_epc;
        case R4300I_CP0_REG_31:
            return cpu->cp0.r31;
        default:
            logfatal("Unsupported CP0 $%s (%d) read", cp0_register_names[r], r);
    }
}

INLINE void set_fpu_register_dword(r4300i_t* cpu, byte r, dword value) {
    if (!cpu->cp0.status.fr) {
        // When this bit is not set, accessing odd registers is not allowed.
        r &= ~1;
    }

    cpu->f[r].raw = value;
}

INLINE dword get_fpu_register_dword(r4300i_t* cpu, byte r) {
    if (!cpu->cp0.status.fr) {
        // When this bit is not set, accessing odd registers is not allowed.
        r &= ~1;
    }

    return cpu->f[r].raw;
}

INLINE void set_fpu_register_word(r4300i_t* cpu, byte r, word value) {
    if (cpu->cp0.status.fr) {
        cpu->f[r].lo = value;
    } else {
        if (r & 1) {
            cpu->f[r & ~1].hi = value;
        } else {
            cpu->f[r].lo = value;
        }
    }
}

INLINE word get_fpu_register_word(r4300i_t* cpu, byte r) {
    if (cpu->cp0.status.fr) {
        return cpu->f[r].lo;
    } else {
        if (r & 1) {
            return cpu->f[r & ~1].hi;
        } else {
            return cpu->f[r].lo;
        }
    }
}

INLINE void set_fpu_register_double(r4300i_t* cpu, byte r, double value) {
    static_assert(sizeof(double) == sizeof(dword), "double and dword need to both be 64 bits for this to work.");

    dword rawvalue;
    memcpy(&rawvalue, &value, sizeof(double));
    set_fpu_register_dword(cpu, r, rawvalue);
}

INLINE double get_fpu_register_double(r4300i_t* cpu, byte r) {
    static_assert(sizeof(double) == sizeof(dword), "double and dword need to both be 64 bits for this to work.");
    double doublevalue;
    dword rawvalue = get_fpu_register_dword(cpu, r);
    memcpy(&doublevalue, &rawvalue, sizeof(double));
    return doublevalue;
}

INLINE void set_fpu_register_float(r4300i_t* cpu, byte r, float value) {
    static_assert(sizeof(float) == sizeof(word), "float and word need to both be 32 bits for this to work.");

    word rawvalue;
    memcpy(&rawvalue, &value, sizeof(float));
    set_fpu_register_word(cpu, r, rawvalue);
}

INLINE float get_fpu_register_float(r4300i_t* cpu, byte r) {
    static_assert(sizeof(float) == sizeof(word), "float and word need to both be 32 bits for this to work.");
    word rawvalue = get_fpu_register_word(cpu, r);
    float floatvalue;
    memcpy(&floatvalue, &rawvalue, sizeof(float));
    return floatvalue;
}

#endif //N64_R4300I_REGISTER_ACCESS_H
