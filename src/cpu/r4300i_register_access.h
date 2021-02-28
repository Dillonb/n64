#ifndef N64_R4300I_REGISTER_ACCESS_H
#define N64_R4300I_REGISTER_ACCESS_H

#include "r4300i.h"

INLINE void set_register(byte r, dword value) {
    logtrace("Setting $%s (r%d) to [0x%016lX]", register_names[r], r, value);
    if (r != 0) {
        N64CPU.gpr[r] = value;
    }
}

INLINE dword get_register(byte r) {
    dword value = N64CPU.gpr[r];
    logtrace("Reading $%s (r%d): 0x%016lX", register_names[r], r, value);
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

INLINE void set_cp0_register_word(byte r, word value) {
    switch (r) {
        case R4300I_CP0_REG_INDEX:
            N64CPU.cp0.index = value;
            break;
        case R4300I_CP0_REG_RANDOM:
            break;
        case R4300I_CP0_REG_COUNT:
            N64CPU.cp0.count = value;
            break;
        case R4300I_CP0_REG_CAUSE: {
            cp0_cause_t newcause;
            newcause.raw = value;
            N64CPU.cp0.cause.ip0 = newcause.ip0;
            N64CPU.cp0.cause.ip1 = newcause.ip1;
            break;
        }
        case R4300I_CP0_REG_TAGLO: // Used for the cache, which is unimplemented.
            N64CPU.cp0.tag_lo = value;
            break;
        case R4300I_CP0_REG_TAGHI: // Used for the cache, which is unimplemented.
            N64CPU.cp0.tag_hi = value;
            break;
        case R4300I_CP0_REG_COMPARE:
            loginfo("$Compare written with 0x%08X (count is now 0x%08lX)", value, N64CPU.cp0.count);
            N64CPU.cp0.cause.ip7 = false;
            N64CPU.cp0.compare = value;
            break;
        case R4300I_CP0_REG_STATUS: {
            N64CPU.cp0.status.raw &= ~CP0_STATUS_WRITE_MASK;
            N64CPU.cp0.status.raw |= value & CP0_STATUS_WRITE_MASK;

            unimplemented(N64CPU.cp0.status.re, "Reverse endian bit set in CP0 (this probably ");
            // TODO: make sure to fix the CPU_MODE_SUPERVISOR and CPU_MODE_USER constants when this assertion gets hit
            unimplemented(N64CPU.cp0.status.ksu, "KSU != 0, leaving kernel mode!");

            cp0_status_updated();
            log_status(N64CPU.cp0.status);

            r4300i_interrupt_update();
            break;
        }
        case R4300I_CP0_REG_ENTRYLO0:
            unimplemented(N64CPU.cp0.is_64bit_addressing, "EntryLo0 written as word in 64 bit mode");
            N64CPU.cp0.entry_lo0.raw = value;
            break;
        case R4300I_CP0_REG_ENTRYLO1:
            unimplemented(N64CPU.cp0.is_64bit_addressing, "EntryLo1 written as word in 64 bit mode");
            N64CPU.cp0.entry_lo1.raw = value;
            break;
        case 7:
            logfatal("CP0 Reg 7 write?");
            break;
        case R4300I_CP0_REG_ENTRYHI:
            unimplemented(N64CPU.cp0.is_64bit_addressing, "Entry hi written as word in 64 bit mode");
            N64CPU.cp0.entry_hi.raw = value;
            break;
        case R4300I_CP0_REG_PAGEMASK:
            N64CPU.cp0.page_mask.raw = value;
            break;
        case R4300I_CP0_REG_EPC:
            unimplemented(N64CPU.cp0.is_64bit_addressing, "EPC written as word in 64 bit mode");
            N64CPU.cp0.EPC = (sdword)((sword)value);
            break;
        case R4300I_CP0_REG_CONFIG:
            N64CPU.cp0.config &= ~CP0_CONFIG_WRITE_MASK;
            N64CPU.cp0.config |= value & CP0_CONFIG_WRITE_MASK;
            break;
        case R4300I_CP0_REG_WATCHLO:
            N64CPU.cp0.watch_lo.raw = value;
            unimplemented(N64CPU.cp0.watch_lo.r, "Read exception enabled in CP0 watch_lo!");
            unimplemented(N64CPU.cp0.watch_lo.w, "Write exception enabled in CP0 watch_lo!");
            break;
        case R4300I_CP0_REG_WATCHHI:
            N64CPU.cp0.watch_hi = value;
            break;
        case R4300I_CP0_REG_WIRED:
            N64CPU.cp0.wired = value;
            break;
        case R4300I_CP0_REG_CONTEXT:
            unimplemented(N64CPU.cp0.is_64bit_addressing, "Context written as word in 64 bit mode");
            unimplemented(value != 0, "cp0 context written with non-zero value");
            N64CPU.cp0.context = value;
            break;
        case R4300I_CP0_REG_LLADDR:
            N64CPU.cp0.lladdr = value;
            break;
        default:
            logfatal("Unsupported CP0 $%s (%d) set: 0x%08X", cp0_register_names[r], r, value);
    }

    loginfo("CP0 $%s = 0x%08X", cp0_register_names[r], value);
}

INLINE word get_cp0_count() {
    dword shifted = N64CPU.cp0.count >> 1;
    return (word)shifted;
}

INLINE word get_cp0_register_word(byte r) {
    switch (r) {
        case R4300I_CP0_REG_INDEX:
            return N64CPU.cp0.index & 0x8000003F;
        case R4300I_CP0_REG_RANDOM:
            logalways("WARNING! Stubbed read from Random!");
            return 1;
        case R4300I_CP0_REG_ENTRYLO0:
            unimplemented(N64CPU.cp0.is_64bit_addressing, "entrylo0 read as word in 64 bit mode");
            return N64CPU.cp0.entry_lo0.raw;
        case R4300I_CP0_REG_ENTRYLO1:
            unimplemented(N64CPU.cp0.is_64bit_addressing, "entrylo1 read as word in 64 bit mode");
            return N64CPU.cp0.entry_lo1.raw;
        case R4300I_CP0_REG_CONTEXT:
            unimplemented(N64CPU.cp0.is_64bit_addressing, "context read as word in 64 bit mode");
            return N64CPU.cp0.context;
        case R4300I_CP0_REG_PAGEMASK:
            return N64CPU.cp0.page_mask.raw;
        case R4300I_CP0_REG_WIRED:
            return N64CPU.cp0.wired;
        case R4300I_CP0_REG_7:
            return N64CPU.cp0.r7;
        case R4300I_CP0_REG_BADVADDR:
            unimplemented(N64CPU.cp0.is_64bit_addressing, "bad vaddr read as word in 64 bit mode");
            return N64CPU.cp0.bad_vaddr;
        case R4300I_CP0_REG_COUNT:
            return get_cp0_count();
        case R4300I_CP0_REG_ENTRYHI:
            unimplemented(N64CPU.cp0.is_64bit_addressing, "entryhi read as word in 64 bit mode");
            return N64CPU.cp0.entry_hi.raw;
        case R4300I_CP0_REG_COMPARE:
            return N64CPU.cp0.compare;
        case R4300I_CP0_REG_STATUS:
            return N64CPU.cp0.status.raw;
        case R4300I_CP0_REG_CAUSE:
            return N64CPU.cp0.cause.raw;
        case R4300I_CP0_REG_EPC:
            return N64CPU.cp0.EPC & 0xFFFFFFFF;
        case R4300I_CP0_REG_PRID:
            return N64CPU.cp0.PRId;
        case R4300I_CP0_REG_CONFIG:
            return N64CPU.cp0.config;
        case R4300I_CP0_REG_LLADDR:
            return N64CPU.cp0.lladdr;
        case R4300I_CP0_REG_WATCHLO:
            return N64CPU.cp0.watch_lo.raw;
        case R4300I_CP0_REG_WATCHHI:
            return N64CPU.cp0.watch_hi;
        case R4300I_CP0_REG_XCONTEXT:
            return N64CPU.cp0.x_context;
        case R4300I_CP0_REG_21:
            return N64CPU.cp0.r21;
        case R4300I_CP0_REG_22:
            return N64CPU.cp0.r22;
        case R4300I_CP0_REG_23:
            return N64CPU.cp0.r23;
        case R4300I_CP0_REG_24:
            return N64CPU.cp0.r24;
        case R4300I_CP0_REG_25:
            return N64CPU.cp0.r25;
        case R4300I_CP0_REG_PARITYER:
            return N64CPU.cp0.parity_error;
        case R4300I_CP0_REG_CACHEER:
            return N64CPU.cp0.cache_error;
        case R4300I_CP0_REG_TAGLO:
            return N64CPU.cp0.tag_lo;
        case R4300I_CP0_REG_TAGHI:
            return N64CPU.cp0.tag_hi;
        case R4300I_CP0_REG_ERR_EPC:
            return N64CPU.cp0.error_epc & 0xFFFFFFFF;
        case R4300I_CP0_REG_31:
            return N64CPU.cp0.r31;
        default:
            logfatal("Unsupported CP0 $%s (%d) read", cp0_register_names[r], r);
    }
}

INLINE void set_cp0_register_dword(byte r, dword value) {
    switch (r) {
        case R4300I_CP0_REG_INDEX:
            logfatal("Writing CP0 register R4300I_CP0_REG_INDEX as dword!");
        case R4300I_CP0_REG_RANDOM:
            logfatal("Writing CP0 register R4300I_CP0_REG_RANDOM as dword!");
        case R4300I_CP0_REG_ENTRYLO0:
            N64CPU.cp0.entry_lo0.raw = value;
            break;
        case R4300I_CP0_REG_ENTRYLO1:
            N64CPU.cp0.entry_lo1.raw = value;
            break;
        case R4300I_CP0_REG_CONTEXT:
            unimplemented(value != 0, "cp0 context written with non-zero value in 64 bit mode!");
            N64CPU.cp0.context_64 = value;
            break;
        case R4300I_CP0_REG_PAGEMASK:
            logfatal("Writing CP0 register R4300I_CP0_REG_PAGEMASK as dword!");
        case R4300I_CP0_REG_WIRED:
            logfatal("Writing CP0 register R4300I_CP0_REG_WIRED as dword!");
        case R4300I_CP0_REG_7:
            logfatal("Writing CP0 register R4300I_CP0_REG_7 as dword!");
        case R4300I_CP0_REG_BADVADDR:
            logfatal("Writing CP0 register R4300I_CP0_REG_BADVADDR as dword!");
        case R4300I_CP0_REG_COUNT:
            logfatal("Writing CP0 register R4300I_CP0_REG_COUNT as dword!");
        case R4300I_CP0_REG_ENTRYHI:
            N64CPU.cp0.entry_hi_64.raw = value;
            N64CPU.cp0.entry_hi_64.fill = 0;
            break;
        case R4300I_CP0_REG_COMPARE:
            logfatal("Writing CP0 register R4300I_CP0_REG_COMPARE as dword!");
        case R4300I_CP0_REG_STATUS:
            logfatal("Writing CP0 register R4300I_CP0_REG_STATUS as dword!");
        case R4300I_CP0_REG_CAUSE:
            logfatal("Writing CP0 register R4300I_CP0_REG_CAUSE as dword!");
        case R4300I_CP0_REG_EPC:
            N64CPU.cp0.EPC = value;
            break;
        case R4300I_CP0_REG_PRID:
            logfatal("Writing CP0 register R4300I_CP0_REG_PRID as dword!");
        case R4300I_CP0_REG_CONFIG:
            logfatal("Writing CP0 register R4300I_CP0_REG_CONFIG as dword!");
        case R4300I_CP0_REG_LLADDR:
            logfatal("Writing CP0 register R4300I_CP0_REG_LLADDR as dword!");
        case R4300I_CP0_REG_WATCHLO:
            logfatal("Writing CP0 register R4300I_CP0_REG_WATCHLO as dword!");
        case R4300I_CP0_REG_WATCHHI:
            logfatal("Writing CP0 register R4300I_CP0_REG_WATCHHI as dword!");
        case R4300I_CP0_REG_XCONTEXT:
            unimplemented(value != 0, "cp0 xcontext written with non-zero value in 64 bit mode!");
            N64CPU.cp0.x_context = value;
            break;
        case R4300I_CP0_REG_21:
            logfatal("Writing CP0 register R4300I_CP0_REG_21 as dword!");
        case R4300I_CP0_REG_22:
            logfatal("Writing CP0 register R4300I_CP0_REG_22 as dword!");
        case R4300I_CP0_REG_23:
            logfatal("Writing CP0 register R4300I_CP0_REG_23 as dword!");
        case R4300I_CP0_REG_24:
            logfatal("Writing CP0 register R4300I_CP0_REG_24 as dword!");
        case R4300I_CP0_REG_25:
            logfatal("Writing CP0 register R4300I_CP0_REG_25 as dword!");
        case R4300I_CP0_REG_PARITYER:
            logfatal("Writing CP0 register R4300I_CP0_REG_PARITYER as dword!");
        case R4300I_CP0_REG_CACHEER:
            logfatal("Writing CP0 register R4300I_CP0_REG_CACHEER as dword!");
        case R4300I_CP0_REG_TAGLO:
            logfatal("Writing CP0 register R4300I_CP0_REG_TAGLO as dword!");
        case R4300I_CP0_REG_TAGHI:
            logfatal("Writing CP0 register R4300I_CP0_REG_TAGHI as dword!");
        case R4300I_CP0_REG_ERR_EPC:
            logfatal("Writing CP0 register R4300I_CP0_REG_ERR_EPC as dword!");
        case R4300I_CP0_REG_31:
            logfatal("Writing CP0 register R4300I_CP0_REG_31 as dword!");
        default:
            logfatal("Unsupported CP0 $%s (%d) read", cp0_register_names[r], r);
    }
}

INLINE dword get_cp0_register_dword(byte r) {
    switch (r) {
        case R4300I_CP0_REG_INDEX:
            logfatal("Reading CP0 register R4300I_CP0_REG_INDEX as dword!");
        case R4300I_CP0_REG_RANDOM:
            logfatal("Reading CP0 register R4300I_CP0_REG_RANDOM as dword!");
        case R4300I_CP0_REG_ENTRYLO0:
            logfatal("Reading CP0 register R4300I_CP0_REG_ENTRYLO0 as dword!");
        case R4300I_CP0_REG_ENTRYLO1:
            logfatal("Reading CP0 register R4300I_CP0_REG_ENTRYLO1 as dword!");
        case R4300I_CP0_REG_CONTEXT:
            return N64CPU.cp0.context_64;
        case R4300I_CP0_REG_PAGEMASK:
            logfatal("Reading CP0 register R4300I_CP0_REG_PAGEMASK as dword!");
        case R4300I_CP0_REG_WIRED:
            logfatal("Reading CP0 register R4300I_CP0_REG_WIRED as dword!");
        case R4300I_CP0_REG_7:
            logfatal("Reading CP0 register R4300I_CP0_REG_7 as dword!");
        case R4300I_CP0_REG_BADVADDR:
            logfatal("Reading CP0 register R4300I_CP0_REG_BADVADDR as dword!");
        case R4300I_CP0_REG_COUNT:
            logfatal("Reading CP0 register R4300I_CP0_REG_COUNT as dword!");
        case R4300I_CP0_REG_ENTRYHI:
            return N64CPU.cp0.entry_hi.raw & CP0_ENTRY_HI_64_READ_MASK;
        case R4300I_CP0_REG_COMPARE:
            logfatal("Reading CP0 register R4300I_CP0_REG_COMPARE as dword!");
        case R4300I_CP0_REG_STATUS:
            logfatal("Reading CP0 register R4300I_CP0_REG_STATUS as dword!");
        case R4300I_CP0_REG_CAUSE:
            logfatal("Reading CP0 register R4300I_CP0_REG_CAUSE as dword!");
        case R4300I_CP0_REG_EPC:
            return N64CPU.cp0.EPC;
        case R4300I_CP0_REG_PRID:
            logfatal("Reading CP0 register R4300I_CP0_REG_PRID as dword!");
        case R4300I_CP0_REG_CONFIG:
            logfatal("Reading CP0 register R4300I_CP0_REG_CONFIG as dword!");
        case R4300I_CP0_REG_LLADDR:
            logfatal("Reading CP0 register R4300I_CP0_REG_LLADDR as dword!");
        case R4300I_CP0_REG_WATCHLO:
            logfatal("Reading CP0 register R4300I_CP0_REG_WATCHLO as dword!");
        case R4300I_CP0_REG_WATCHHI:
            logfatal("Reading CP0 register R4300I_CP0_REG_WATCHHI as dword!");
        case R4300I_CP0_REG_XCONTEXT:
            logfatal("Reading CP0 register R4300I_CP0_REG_XCONTEXT as dword!");
        case R4300I_CP0_REG_21:
            logfatal("Reading CP0 register R4300I_CP0_REG_21 as dword!");
        case R4300I_CP0_REG_22:
            logfatal("Reading CP0 register R4300I_CP0_REG_22 as dword!");
        case R4300I_CP0_REG_23:
            logfatal("Reading CP0 register R4300I_CP0_REG_23 as dword!");
        case R4300I_CP0_REG_24:
            logfatal("Reading CP0 register R4300I_CP0_REG_24 as dword!");
        case R4300I_CP0_REG_25:
            logfatal("Reading CP0 register R4300I_CP0_REG_25 as dword!");
        case R4300I_CP0_REG_PARITYER:
            logfatal("Reading CP0 register R4300I_CP0_REG_PARITYER as dword!");
        case R4300I_CP0_REG_CACHEER:
            logfatal("Reading CP0 register R4300I_CP0_REG_CACHEER as dword!");
        case R4300I_CP0_REG_TAGLO:
            logfatal("Reading CP0 register R4300I_CP0_REG_TAGLO as dword!");
        case R4300I_CP0_REG_TAGHI:
            logfatal("Reading CP0 register R4300I_CP0_REG_TAGHI as dword!");
        case R4300I_CP0_REG_ERR_EPC:
            logfatal("Reading CP0 register R4300I_CP0_REG_ERR_EPC as dword!");
        case R4300I_CP0_REG_31:
            logfatal("Reading CP0 register R4300I_CP0_REG_31 as dword!");
        default:
            logfatal("Unsupported CP0 $%s (%d) read", cp0_register_names[r], r);
    }
}

INLINE void set_fpu_register_dword(byte r, dword value) {
    if (!N64CPU.cp0.status.fr) {
        // When this bit is not set, accessing odd registers is not allowed.
        r &= ~1;
    }

    N64CPU.f[r].raw = value;
}

INLINE dword get_fpu_register_dword(byte r) {
    if (!N64CPU.cp0.status.fr) {
        // When this bit is not set, accessing odd registers is not allowed.
        r &= ~1;
    }

    return N64CPU.f[r].raw;
}

INLINE void set_fpu_register_word(byte r, word value) {
    if (N64CPU.cp0.status.fr) {
        N64CPU.f[r].lo = value;
    } else {
        if (r & 1) {
            N64CPU.f[r & ~1].hi = value;
        } else {
            N64CPU.f[r].lo = value;
        }
    }
}

INLINE word get_fpu_register_word(byte r) {
    if (N64CPU.cp0.status.fr) {
        return N64CPU.f[r].lo;
    } else {
        if (r & 1) {
            return N64CPU.f[r & ~1].hi;
        } else {
            return N64CPU.f[r].lo;
        }
    }
}

INLINE void set_fpu_register_double(byte r, double value) {
    static_assert(sizeof(double) == sizeof(dword), "double and dword need to both be 64 bits for this to work.");

    dword rawvalue;
    memcpy(&rawvalue, &value, sizeof(double));
    set_fpu_register_dword(r, rawvalue);
}

INLINE double get_fpu_register_double(byte r) {
    static_assert(sizeof(double) == sizeof(dword), "double and dword need to both be 64 bits for this to work.");
    double doublevalue;
    dword rawvalue = get_fpu_register_dword(r);
    memcpy(&doublevalue, &rawvalue, sizeof(double));
    return doublevalue;
}

INLINE void set_fpu_register_float(byte r, float value) {
    static_assert(sizeof(float) == sizeof(word), "float and word need to both be 32 bits for this to work.");

    word rawvalue;
    memcpy(&rawvalue, &value, sizeof(float));
    set_fpu_register_word(r, rawvalue);
}

INLINE float get_fpu_register_float(byte r) {
    static_assert(sizeof(float) == sizeof(word), "float and word need to both be 32 bits for this to work.");
    word rawvalue = get_fpu_register_word(r);
    float floatvalue;
    memcpy(&floatvalue, &rawvalue, sizeof(float));
    return floatvalue;
}

INLINE void link() {
    dword pc = N64CPU.pc + 4;
    set_register(R4300I_REG_LR, pc); // Skips the instruction in the delay slot on return
}

INLINE void branch_abs(dword address) {
    N64CPU.next_pc = address;
    N64CPU.branch = true;
}

INLINE void branch_abs_word(dword address) {
    branch_abs((sdword)((sword)address));
}

INLINE void branch_offset(shalf offset) {
    sword soffset = offset;
    soffset *= 4;
    // This is taking advantage of the fact that we add 4 to the PC after each instruction.
    // Due to the compiler expecting pipelining, the address we get here will be 4 _too early_

    branch_abs(N64CPU.pc + soffset);
}

INLINE void conditional_branch_likely(word offset, bool condition) {
    if (condition) {
        branch_offset(offset);
    } else {
        // Skip instruction in delay slot
        set_pc_dword_r4300i(N64CPU.pc + 4);
    }
}

INLINE void conditional_branch(word offset, bool condition) {
    if (condition) {
        branch_offset(offset);
    }
}

#endif //N64_R4300I_REGISTER_ACCESS_H
