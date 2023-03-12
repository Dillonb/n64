#ifndef N64_R4300I_REGISTER_ACCESS_H
#define N64_R4300I_REGISTER_ACCESS_H

#include "r4300i.h"

INLINE void set_register(u8 r, u64 value) {
    logtrace("Setting $%s (r%d) to [0x%016lX]", register_names[r], r, value);
    N64CPU.gpr[r] = value;
    N64CPU.gpr[0] = 0;
}

INLINE u64 get_register(u8 r) {
    u64 value = N64CPU.gpr[r];
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

INLINE void set_cp0_register_word(u8 r, u32 value) {
    N64CP0.open_bus = value;
    switch (r) {
        case R4300I_CP0_REG_INDEX:
            N64CPU.cp0.index = value;
            break;
        case R4300I_CP0_REG_RANDOM:
            break;
        case R4300I_CP0_REG_COUNT:
            N64CPU.cp0.count = (u64)value << 1;
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

            unimplemented(N64CPU.cp0.status.re, "Reverse endian bit set in CP0 (this probably doesn't actually do anything)");
            unimplemented(N64CP0.user_mode && !N64CP0.is_64bit_addressing, "user mode without 64 bit ops, need to implement reserved instruction exceptions for 64 bit instructions!");

            cp0_status_updated();
            log_status(N64CPU.cp0.status);

            r4300i_interrupt_update();
            break;
        }
        case R4300I_CP0_REG_ENTRYLO0:
            N64CPU.cp0.entry_lo0.raw = value & CP0_ENTRY_LO_WRITE_MASK;
            break;
        case R4300I_CP0_REG_ENTRYLO1:
            N64CPU.cp0.entry_lo1.raw = value & CP0_ENTRY_LO_WRITE_MASK;
            break;
        case R4300I_CP0_REG_ENTRYHI:
            N64CPU.cp0.entry_hi.raw = se_32_64(value) & CP0_ENTRY_HI_WRITE_MASK;
            break;
        case R4300I_CP0_REG_PAGEMASK:
            N64CPU.cp0.page_mask.raw = value & CP0_PAGEMASK_WRITE_MASK;
            break;
        case R4300I_CP0_REG_EPC:
            N64CPU.cp0.EPC = (s64)((s32)value);
            break;
        case R4300I_CP0_REG_CONFIG:
            N64CPU.cp0.config &= ~CP0_CONFIG_WRITE_MASK;
            N64CPU.cp0.config |= (value & CP0_CONFIG_WRITE_MASK);
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
            N64CPU.cp0.wired = value & 63;
            break;
        case R4300I_CP0_REG_CONTEXT:
            N64CPU.cp0.context.raw = (((s64)(s32)value) & 0xFFFFFFFFFF800000) | (N64CPU.cp0.context.raw & 0x7FFFFF);
            break;
        case R4300I_CP0_REG_XCONTEXT:
            N64CPU.cp0.x_context.raw = (((s64)(s32)value) & 0xFFFFFFFE00000000) | (N64CPU.cp0.x_context.raw & 0x1FFFFFFFF);
            break;
        case R4300I_CP0_REG_LLADDR:
            N64CPU.cp0.lladdr = value;
            break;
        case R4300I_CP0_REG_ERR_EPC:
            N64CP0.error_epc = (s64)((s32)value);
            break;
        case R4300I_CP0_REG_PRID:
            break; // Read only
        case R4300I_CP0_REG_PARITYER:
            N64CP0.parity_error = value & 0xFF; // ???
            break;
        case R4300I_CP0_REG_CACHEER:
            break; // Read only?
        case R4300I_CP0_REG_7:
        case R4300I_CP0_REG_21:
        case R4300I_CP0_REG_22:
        case R4300I_CP0_REG_23:
        case R4300I_CP0_REG_24:
        case R4300I_CP0_REG_25:
        case R4300I_CP0_REG_31:
            break;
        default:
            logfatal("Unsupported CP0 $%s (%d) set: 0x%08X", cp0_register_names[r], r, value);
    }

    loginfo("CP0 $%s = 0x%08X", cp0_register_names[r], value);
}

INLINE u32 get_cp0_count() {
    u64 shifted = N64CPU.cp0.count >> 1;
    return (u32)shifted;
}

INLINE u32 get_cp0_wired() {
    return N64CP0.wired & 0b111111;
}

INLINE u32 get_cp0_random() {
    int val = rand();
    int wired = get_cp0_wired();

    int lower;
    int upper;

    // Edge case: when wired is out of bounds, the full range of [0..63] can be generated by Random
    if (wired > 31) {
        lower = 0;
        upper = 64;
    } else {
        lower = wired;
        upper = 32 - wired;
    }

    val = (val % upper) + lower;

    return val;
}

INLINE u32 get_cp0_register_word(u8 r) {
    switch (r) {
        case R4300I_CP0_REG_INDEX:
            return N64CPU.cp0.index & 0x8000003F;
        case R4300I_CP0_REG_RANDOM:
            logwarn("Stubbed read from Random!");
            return get_cp0_random();
        case R4300I_CP0_REG_ENTRYLO0:
            return N64CPU.cp0.entry_lo0.raw;
        case R4300I_CP0_REG_ENTRYLO1:
            return N64CPU.cp0.entry_lo1.raw;
        case R4300I_CP0_REG_CONTEXT:
            return N64CPU.cp0.context.raw;
        case R4300I_CP0_REG_PAGEMASK:
            return N64CPU.cp0.page_mask.raw;
        case R4300I_CP0_REG_WIRED:
            return N64CPU.cp0.wired;
        case R4300I_CP0_REG_BADVADDR:
            return N64CPU.cp0.bad_vaddr;
        case R4300I_CP0_REG_COUNT:
            return get_cp0_count();
        case R4300I_CP0_REG_ENTRYHI:
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
            return N64CPU.cp0.x_context.raw;
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
        case R4300I_CP0_REG_7:
        case R4300I_CP0_REG_21:
        case R4300I_CP0_REG_22:
        case R4300I_CP0_REG_23:
        case R4300I_CP0_REG_24:
        case R4300I_CP0_REG_25:
        case R4300I_CP0_REG_31:
            return N64CP0.open_bus;
        default:
            logfatal("Unsupported CP0 $%s (%d) read", cp0_register_names[r], r);
    }
}

INLINE void set_cp0_register_dword(u8 r, u64 value) {
    N64CP0.open_bus = value;
    switch (r) {
        case R4300I_CP0_REG_INDEX:
            logfatal("Writing CP0 register R4300I_CP0_REG_INDEX as dword!");
        case R4300I_CP0_REG_RANDOM:
            logfatal("Writing CP0 register R4300I_CP0_REG_RANDOM as dword!");
        case R4300I_CP0_REG_ENTRYLO0:
            N64CPU.cp0.entry_lo0.raw = value & CP0_ENTRY_LO_WRITE_MASK;
            break;
        case R4300I_CP0_REG_ENTRYLO1:
            N64CPU.cp0.entry_lo1.raw = value & CP0_ENTRY_LO_WRITE_MASK;
            break;
        case R4300I_CP0_REG_CONTEXT:
            N64CPU.cp0.context.raw = (value & 0xFFFFFFFFFF800000) | (N64CPU.cp0.context.raw & 0x7FFFFF);
            break;
        case R4300I_CP0_REG_PAGEMASK:
            logfatal("Writing CP0 register R4300I_CP0_REG_PAGEMASK as dword!");
        case R4300I_CP0_REG_WIRED:
            logfatal("Writing CP0 register R4300I_CP0_REG_WIRED as dword!");
        case R4300I_CP0_REG_7:
            logfatal("Writing CP0 register R4300I_CP0_REG_7 as dword!");
        case R4300I_CP0_REG_BADVADDR: // read only
            break;
        case R4300I_CP0_REG_COUNT:
            logfatal("Writing CP0 register R4300I_CP0_REG_COUNT as dword!");
        case R4300I_CP0_REG_ENTRYHI:
            N64CPU.cp0.entry_hi.raw = value & CP0_ENTRY_HI_WRITE_MASK;
            break;
        case R4300I_CP0_REG_COMPARE:
            logfatal("Writing CP0 register R4300I_CP0_REG_COMPARE as dword!");
        case R4300I_CP0_REG_STATUS:
            N64CP0.status.raw = value;
        case R4300I_CP0_REG_CAUSE: {
            cp0_cause_t newcause;
            newcause.raw = value;
            N64CPU.cp0.cause.ip0 = newcause.ip0;
            N64CPU.cp0.cause.ip1 = newcause.ip1;
            break;
        }
        case R4300I_CP0_REG_EPC:
            N64CPU.cp0.EPC = value;
            break;
        case R4300I_CP0_REG_PRID:
            logfatal("Writing CP0 register R4300I_CP0_REG_PRID as dword!");
        case R4300I_CP0_REG_CONFIG:
            logfatal("Writing CP0 register R4300I_CP0_REG_CONFIG as dword!");
        case R4300I_CP0_REG_LLADDR:
            N64CPU.cp0.lladdr = value;
            break;
        case R4300I_CP0_REG_WATCHLO:
            logfatal("Writing CP0 register R4300I_CP0_REG_WATCHLO as dword!");
        case R4300I_CP0_REG_WATCHHI:
            logfatal("Writing CP0 register R4300I_CP0_REG_WATCHHI as dword!");
        case R4300I_CP0_REG_XCONTEXT:
            N64CPU.cp0.x_context.raw = (value & 0xFFFFFFFE00000000) | (N64CPU.cp0.x_context.raw & 0x00000001FFFFFFFF);
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
            N64CP0.error_epc = value;
            break;
        case R4300I_CP0_REG_31:
            logfatal("Writing CP0 register R4300I_CP0_REG_31 as dword!");
        default:
            logfatal("Unsupported CP0 $%s (%d) read", cp0_register_names[r], r);
    }
}

INLINE u64 get_cp0_register_dword(u8 r) {
    switch (r) {
        case R4300I_CP0_REG_INDEX:
            logfatal("Reading CP0 register R4300I_CP0_REG_INDEX as dword!");
        case R4300I_CP0_REG_RANDOM:
            logfatal("Reading CP0 register R4300I_CP0_REG_RANDOM as dword!");
        case R4300I_CP0_REG_ENTRYLO0:
            return N64CP0.entry_lo0.raw;
        case R4300I_CP0_REG_ENTRYLO1:
            return N64CP0.entry_lo1.raw;
        case R4300I_CP0_REG_CONTEXT:
            return N64CPU.cp0.context.raw;
        case R4300I_CP0_REG_PAGEMASK:
            logfatal("Reading CP0 register R4300I_CP0_REG_PAGEMASK as dword!");
        case R4300I_CP0_REG_WIRED:
            logfatal("Reading CP0 register R4300I_CP0_REG_WIRED as dword!");
        case R4300I_CP0_REG_7:
            logfatal("Reading CP0 register R4300I_CP0_REG_7 as dword!");
        case R4300I_CP0_REG_BADVADDR:
            return N64CP0.bad_vaddr;
        case R4300I_CP0_REG_COUNT:
            logfatal("Reading CP0 register R4300I_CP0_REG_COUNT as dword!");
        case R4300I_CP0_REG_ENTRYHI:
            return N64CPU.cp0.entry_hi.raw;
        case R4300I_CP0_REG_COMPARE:
            logfatal("Reading CP0 register R4300I_CP0_REG_COMPARE as dword!");
        case R4300I_CP0_REG_STATUS:
            return N64CPU.cp0.status.raw;
        case R4300I_CP0_REG_CAUSE:
            logfatal("Reading CP0 register R4300I_CP0_REG_CAUSE as dword!");
        case R4300I_CP0_REG_EPC:
            return N64CPU.cp0.EPC;
        case R4300I_CP0_REG_PRID:
            return N64CPU.cp0.PRId;
        case R4300I_CP0_REG_CONFIG:
            logfatal("Reading CP0 register R4300I_CP0_REG_CONFIG as dword!");
        case R4300I_CP0_REG_LLADDR:
            return N64CP0.lladdr;
        case R4300I_CP0_REG_WATCHLO:
            logfatal("Reading CP0 register R4300I_CP0_REG_WATCHLO as dword!");
        case R4300I_CP0_REG_WATCHHI:
            logfatal("Reading CP0 register R4300I_CP0_REG_WATCHHI as dword!");
        case R4300I_CP0_REG_XCONTEXT:
            return N64CP0.x_context.raw & 0xFFFFFFFFFFFFFFF0;
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
            return N64CP0.error_epc;
        case R4300I_CP0_REG_31:
            logfatal("Reading CP0 register R4300I_CP0_REG_31 as dword!");
        default:
            logfatal("Unsupported CP0 $%s (%d) read", cp0_register_names[r], r);
    }
}

INLINE void set_fpu_register_dword_fr(u8 r, u64 value) {
    if (!N64CPU.cp0.status.fr) {
        // When this bit is not set, accessing odd registers is not allowed.
        r &= ~1;
    }

    N64CPU.f[r].raw = value;
}

INLINE void set_fpu_register_dword(u8 r, u64 value) {
    N64CPU.f[r].raw = value;
}

INLINE u64 get_fpu_register_dword_fr(u8 r) {
    if (!N64CPU.cp0.status.fr) {
        // When this bit is not set, accessing odd registers is not allowed.
        r &= ~1;
    }

    return N64CPU.f[r].raw;
}

INLINE u64 get_fpu_register_dword(u8 r) {
    return N64CPU.f[r].raw;
}

INLINE void set_fpu_register_word_fr(u8 r, u32 value) {
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

INLINE void set_fpu_register_word(u8 r, u32 value) {
    N64CPU.f[r].raw = value;
}

INLINE u32 get_fpu_register_word_fr(u8 r) {
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

INLINE u32 get_fpu_register_word(u8 r) {
    return N64CPU.f[r].lo;
}

INLINE void set_fpu_register_double(u8 r, double value) {
    _Static_assert(sizeof(double) == sizeof(u64), "double and dword need to both be 64 bits for this to work.");

    u64 rawvalue;
    memcpy(&rawvalue, &value, sizeof(double));
    set_fpu_register_dword(r, rawvalue);
}

INLINE double get_fpu_register_double(u8 r) {
    _Static_assert(sizeof(double) == sizeof(u64), "double and dword need to both be 64 bits for this to work.");
    double doublevalue;
    u64 rawvalue = get_fpu_register_dword(r);
    memcpy(&doublevalue, &rawvalue, sizeof(double));
    return doublevalue;
}

INLINE void set_fpu_register_float(u8 r, float value) {
    _Static_assert(sizeof(float) == sizeof(u32), "float and word need to both be 32 bits for this to work.");

    u32 rawvalue;
    memcpy(&rawvalue, &value, sizeof(float));
    set_fpu_register_word(r, rawvalue);
}

INLINE float get_fpu_register_float_raw(u8 fs) {
    _Static_assert(sizeof(float) == sizeof(u32), "float and word need to both be 32 bits for this to work.");
    u32 rawvalue = get_fpu_register_word(fs);
    float floatvalue;
    memcpy(&floatvalue, &rawvalue, sizeof(float));
    return floatvalue;
}

INLINE float get_fpu_register_float_ft(u8 ft) {
    return get_fpu_register_float_raw(ft);
}

INLINE float get_fpu_register_float_fs(u8 fs) {
    if (!N64CP0.status.fr) {
        fs &= ~1;
    }
    return get_fpu_register_float_raw(fs);
}

INLINE void link_r4300i(int reg) {
    u64 pc = N64CPU.pc + 4;
    set_register(reg, pc); // Skips the instruction in the delay slot on return
}

INLINE void branch_abs(u64 address) {
    N64CPU.next_pc = address;
    N64CPU.branch = true;
}

INLINE void branch_offset(s16 offset) {
    s32 soffset = offset;
    soffset *= 4;
    // This is taking advantage of the fact that we add 4 to the PC after each instruction.
    // Due to the compiler expecting pipelining, the address we get here will be 4 _too early_

    branch_abs(N64CPU.pc + soffset);
}

INLINE void conditional_branch_likely(u32 offset, bool condition) {
    if (condition) {
        branch_offset(offset);
        N64CPU.branch_likely_taken = true; // For dynarec
        N64CPU.branch = true;
    } else {
        N64CPU.branch_likely_taken = false; // For dynarec
        // Skip instruction in delay slot
        set_pc_dword_r4300i(N64CPU.pc + 4);
    }
}

INLINE void conditional_branch(u32 offset, bool condition) {
    N64CPU.branch = true;
    if (condition) {
        branch_offset(offset);
    }
}

#endif //N64_R4300I_REGISTER_ACCESS_H
