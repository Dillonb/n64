#ifndef N64_R4300I_H
#define N64_R4300I_H
#include <stdbool.h>

#include "../common/util.h"
#include "../common/log.h"

#define R4300I_REG_LR 31

#define R4300I_CP0_REG_INDEX    0
#define R4300I_CP0_REG_ENTRYLO0 2
#define R4300I_CP0_REG_ENTRYLO1 3
#define R4300I_CP0_REG_PAGEMASK 5
#define R4300I_CP0_REG_BADVADDR 8
#define R4300I_CP0_REG_COUNT    9
#define R4300I_CP0_REG_ENTRYHI  10
#define R4300I_CP0_REG_COMPARE  11
#define R4300I_CP0_REG_STATUS   12
#define R4300I_CP0_REG_CAUSE    13
#define R4300I_CP0_REG_EPC      14
#define R4300I_CP0_REG_TAGLO    28
#define R4300I_CP0_REG_TAGHI    29

#define CP0_STATUS_WRITE_MASK 0xFF57FFFF

typedef union cp0_status {
    word raw;
    struct {
        bool ie:1;
        bool exl:1;
        bool erl:1;
        byte ksu:2;
        bool ux:1;
        bool sx:1;
        bool kx:1;
        byte im:8;
        unsigned ds:9;
        bool re:1;
        bool fr:1;
        bool rp:1;
        bool cu0:1;
        bool cu1:1;
        bool cu2:1;
        bool cu3:1;
    };
    struct {
        unsigned:16;
        bool de:1;
        bool ce:1;
        bool ch:1;
        bool:1;
        bool sr:1;
        bool ts:1;
        bool bev:1;
        bool:1;
        bool its:1;
        unsigned:7;
    };
} cp0_status_t;

typedef union cp0_cause {
    struct {
        byte:8;
        byte interrupt_pending:8;
        unsigned:16;
    };
    struct {
        byte:2;
        byte exception_code:5;
        bool:1;
        bool ip0:1;
        bool ip1:1;
        bool ip2:1;
        bool ip3:1;
        bool ip4:1;
        bool ip5:1;
        bool ip6:1;
        bool ip7:1;
        unsigned:12;
        byte coprocessor_error:2;
        bool:1;
        bool branch_delay:1;
    };
    word raw;
} cp0_cause_t;

typedef union cp0_entry_lo {
    word raw;
    struct {
        bool g:1;
        bool v:1;
        bool d:1;
        unsigned c:3;
        unsigned pfn:20;
        unsigned:6;
    };
} cp0_entry_lo_t;

typedef union cp0_page_mask {
    word raw;
    struct {
        unsigned:13;
        unsigned mask:12;
        unsigned:7;
    };
} cp0_page_mask_t;

typedef union cp0_entry_hi {
    struct {
        unsigned asid:8;
        unsigned:5;
        unsigned vpn2:19;
    };
    word raw;
} cp0_entry_hi_t;

typedef struct tlb_entry {
    union {
        word raw;
    } entry_lo0;

    union {
        word raw;
    } entry_lo1;

    union {
        word raw;
        struct {
            unsigned asid:8;
            unsigned:4;
            bool g:1;
            unsigned vpn2:19;
        };
    } entry_hi;

    union {
        word raw;
    } page_mask;

} tlb_entry_t;

typedef struct cp0 {
    word index;
    word random;
    cp0_entry_lo_t entry_lo0;
    cp0_entry_lo_t entry_lo1;
    word context;
    cp0_page_mask_t page_mask;
    word wired;
    word r7;
    word bad_vaddr;
    word count;
    cp0_entry_hi_t entry_hi;
    word compare;
    cp0_status_t status;
    cp0_cause_t cause;
    word EPC;
    word PRId;
    word config;
    word lladdr;
    word watch_lo;
    word watch_hi;
    word x_context;
    word r21;
    word r22;
    word r23;
    word r24;
    word r25;
    word parity_error;
    word cache_error;
    word tag_lo;
    word tag_hi;
    word error_epc;
    word r31;

    tlb_entry_t tlb[32];
} cp0_t;

typedef union fcr0 {
    word raw;
} fcr0_t;

typedef union fcr31 {
    word raw;

    struct {
        byte rounding_mode:2;
        bool flag_inexact_operation:1;
        bool flag_underflow:1;
        bool flag_overflow:1;
        bool flag_division_by_zero:1;
        bool flag_invalid_operation:1;
        bool enable_inexact_operation:1;
        bool enable_underflow:1;
        bool enable_overflow:1;
        bool enable_division_by_zero:1;
        bool enable_invalid_operation:1;
        bool cause_inexact_operation:1;
        bool cause_underflow:1;
        bool cause_overflow:1;
        bool cause_division_by_zero:1;
        bool cause_invalid_operation:1;
        bool cause_unimplemented_operation:1;
        unsigned:5;
        bool compare:1;
        bool fs:1;
        unsigned:7;
    };
} fcr31_t;

typedef struct r4300i {
    dword gpr[32];
    word pc;
    dword mult_hi;
    dword mult_lo;

    fcr0_t  fcr0;
    fcr31_t fcr31;

    byte f[256];

    cp0_t cp0;

    // Branch delay
    bool branch;
    int branch_delay;
    word branch_pc;

    byte (*read_byte)(word);
    void (*write_byte)(word, byte);

    half (*read_half)(word);
    void (*write_half)(word, half);

    word (*read_word)(word);
    void (*write_word)(word, word);

    dword (*read_dword)(word);
    void (*write_dword)(word, dword);
} r4300i_t;

typedef union mips_instruction {
    word raw;

    struct {
        unsigned:26;
        bool op5:1;
        bool op4:1;
        bool op3:1;
        bool op2:1;
        bool op1:1;
        bool op0:1;
    };

    struct {
        unsigned:26;
        unsigned op:6;
    };

    struct {
        unsigned immediate:16;
        unsigned rt:5;
        unsigned rs:5;
        unsigned op:6;
    } i;

    struct {
        unsigned offset:16;
        unsigned ft:5;
        unsigned base:5;
        unsigned op:6;
    } fi;

    struct {
        unsigned target:26;
        unsigned op:6;
    } j;

    struct {
        unsigned funct:6;
        unsigned sa:5;
        unsigned rd:5;
        unsigned rt:5;
        unsigned rs:5;
        unsigned op:6;
    } r;

    struct {
        unsigned funct:6;
        unsigned fd:5;
        unsigned fs:5;
        unsigned ft:5;
        unsigned fmt:5;
        unsigned op:6;
    } fr;

    struct {
        unsigned funct5:1;
        unsigned funct4:1;
        unsigned funct3:1;
        unsigned funct2:1;
        unsigned funct1:1;
        unsigned funct0:1;
        unsigned:26;
    };

    struct {
        unsigned:16;
        unsigned rt4:1;
        unsigned rt3:1;
        unsigned rt2:1;
        unsigned rt1:1;
        unsigned rt0:1;
        unsigned:11;
    };

    struct {
        unsigned:21;
        unsigned rs4:1;
        unsigned rs3:1;
        unsigned rs2:1;
        unsigned rs1:1;
        unsigned rs0:1;
        unsigned:6;
    };

    struct {
        unsigned last11:11;
        unsigned:21;
    };

} mips_instruction_t;

typedef enum mips_instruction_type {
    MIPS_LD,
    MIPS_LUI,
    MIPS_ADDI,
    MIPS_DADDI,
    MIPS_ADDIU,
    MIPS_ANDI,
    MIPS_LBU,
    MIPS_LHU,
    MIPS_LH,
    MIPS_LW,
    MIPS_LWU,
    MIPS_BLEZ,
    MIPS_BLEZL,
    MIPS_BNE,
    MIPS_BNEL,
    MIPS_CACHE,
    MIPS_BEQ,
    MIPS_BEQL,
    MIPS_BGTZ,
    MIPS_BGTZL,
    MIPS_NOP,
    MIPS_SB,
    MIPS_SH,
    MIPS_SW,
    MIPS_SD,
    MIPS_ORI,
    MIPS_J,
    MIPS_JAL,
    MIPS_SLTI,
    MIPS_SLTIU,
    MIPS_XORI,
    MIPS_LB,
    MIPS_LDC1,
    MIPS_SDC1,
    MIPS_LWC1,
    MIPS_SWC1,
    MIPS_LWL,
    MIPS_LWR,
    MIPS_SWL,
    MIPS_SWR,
    MIPS_LDL,
    MIPS_LDR,
    MIPS_SDL,
    MIPS_SDR,

    // Coprocessor
    MIPS_CP_MFC0,
    MIPS_CP_MTC0,
    MIPS_CP_MFC1,
    MIPS_CP_MTC1,

    MIPS_ERET,
    MIPS_TLBWI,
    MIPS_TLBP,
    MIPS_TLBR,

    MIPS_CP_CTC1,
    MIPS_CP_CFC1,

    MIPS_CP_BC1F,
    MIPS_CP_BC1T,
    MIPS_CP_BC1FL,
    MIPS_CP_BC1TL,

    MIPS_CP_ADD_D,
    MIPS_CP_ADD_S,
    MIPS_CP_SUB_D,
    MIPS_CP_SUB_S,
    MIPS_CP_MUL_D,
    MIPS_CP_MUL_S,
    MIPS_CP_DIV_D,
    MIPS_CP_DIV_S,
    MIPS_CP_TRUNC_L_D,
    MIPS_CP_TRUNC_L_S,
    MIPS_CP_TRUNC_W_D,
    MIPS_CP_TRUNC_W_S,

    MIPS_CP_CVT_D_S,
    MIPS_CP_CVT_D_W,
    MIPS_CP_CVT_D_L,
    MIPS_CP_CVT_L_S,
    MIPS_CP_CVT_L_D,
    MIPS_CP_CVT_S_D,
    MIPS_CP_CVT_S_W,
    MIPS_CP_CVT_S_L,
    MIPS_CP_CVT_W_S,
    MIPS_CP_CVT_W_D,
    MIPS_CP_C_F_S,
    MIPS_CP_C_UN_S,
    MIPS_CP_C_EQ_S,
    MIPS_CP_C_UEQ_S,
    MIPS_CP_C_OLT_S,
    MIPS_CP_C_ULT_S,
    MIPS_CP_C_OLE_S,
    MIPS_CP_C_ULE_S,
    MIPS_CP_C_SF_S,
    MIPS_CP_C_NGLE_S,
    MIPS_CP_C_SEQ_S,
    MIPS_CP_C_NGL_S,
    MIPS_CP_C_LT_S,
    MIPS_CP_C_NGE_S,
    MIPS_CP_C_LE_S,
    MIPS_CP_C_NGT_S,
    MIPS_CP_C_F_D,
    MIPS_CP_C_UN_D,
    MIPS_CP_C_EQ_D,
    MIPS_CP_C_UEQ_D,
    MIPS_CP_C_OLT_D,
    MIPS_CP_C_ULT_D,
    MIPS_CP_C_OLE_D,
    MIPS_CP_C_ULE_D,
    MIPS_CP_C_SF_D,
    MIPS_CP_C_NGLE_D,
    MIPS_CP_C_SEQ_D,
    MIPS_CP_C_NGL_D,
    MIPS_CP_C_LT_D,
    MIPS_CP_C_NGE_D,
    MIPS_CP_C_LE_D,
    MIPS_CP_C_NGT_D,
    MIPS_CP_MOV_D,
    MIPS_CP_MOV_S,

    // Special
    MIPS_SPC_SLL,
    MIPS_SPC_SRL,
    MIPS_SPC_SRA,
    MIPS_SPC_SRAV,
    MIPS_SPC_SLLV,
    MIPS_SPC_SRLV,
    MIPS_SPC_JR,
    MIPS_SPC_JALR,
    MIPS_SPC_MFHI,
    MIPS_SPC_MTHI,
    MIPS_SPC_MFLO,
    MIPS_SPC_MTLO,
    MIPS_SPC_DSLLV,
    MIPS_SPC_MULT,
    MIPS_SPC_MULTU,
    MIPS_SPC_DIV,
    MIPS_SPC_DIVU,
    MIPS_SPC_DMULTU,
    MIPS_SPC_DDIVU,
    MIPS_SPC_ADD,
    MIPS_SPC_ADDU,
    MIPS_SPC_AND,
    MIPS_SPC_NOR,
    MIPS_SPC_SUB,
    MIPS_SPC_SUBU,
    MIPS_SPC_OR,
    MIPS_SPC_XOR,
    MIPS_SPC_SLT,
    MIPS_SPC_SLTU,
    MIPS_SPC_DADD,
    MIPS_SPC_DSLL,
    MIPS_SPC_DSLL32,
    MIPS_SPC_DSRA32,

    // REGIMM
    MIPS_RI_BLTZ,
    MIPS_RI_BLTZL,
    MIPS_RI_BGEZ,
    MIPS_RI_BGEZL,
    MIPS_RI_BGEZAL
} mips_instruction_type_t;

void r4300i_step(r4300i_t* cpu);

extern const char* register_names[];
extern const char* cp0_register_names[];

INLINE void set_register(r4300i_t* cpu, byte r, dword value) {
    logtrace("Setting $%s (r%d) to [0x%016lX]", register_names[r], r, value)
    if (r != 0) {
        if (r < 64) {
            cpu->gpr[r] = value;
        } else {
            logfatal("Write to invalid register: %d", r)
        }
    }
}

INLINE dword get_register(r4300i_t* cpu, byte r) {
    if (r < 64) {
        dword value = cpu->gpr[r];
        logtrace("Reading $%s (r%d): 0x%08lX", register_names[r], r, value)
        return value;
    } else {
        logfatal("Attempted to read invalid register: %d", r)
    }
}

INLINE void on_change_fr(r4300i_t* cpu, cp0_status_t oldstatus) {
    logfatal("FR changed from %d to %d!", oldstatus.fr, cpu->cp0.status.fr)
}

INLINE void set_cp0_register(r4300i_t* cpu, byte r, word value) {
    switch (r) {
        case R4300I_CP0_REG_INDEX:
            cpu->cp0.index = value;
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
            logwarn("$Compare written with 0x%08X (count is now 0x%08X)", value, cpu->cp0.count);
            cpu->cp0.cause.ip7 = false;
            cpu->cp0.compare = value;
            break;
        case R4300I_CP0_REG_STATUS: {
            cp0_status_t oldstatus = cpu->cp0.status;

            cpu->cp0.status.raw &= value & ~CP0_STATUS_WRITE_MASK;
            cpu->cp0.status.raw |= value & CP0_STATUS_WRITE_MASK;

            if (oldstatus.fr != cpu->cp0.status.fr) {
                on_change_fr(cpu, oldstatus);
            }

            loginfo("    CP0 status: ie:  %d", cpu->cp0.status.ie)
            loginfo("    CP0 status: exl: %d", cpu->cp0.status.exl)
            loginfo("    CP0 status: erl: %d", cpu->cp0.status.erl)
            loginfo("    CP0 status: ksu: %d", cpu->cp0.status.ksu)
            loginfo("    CP0 status: ux:  %d", cpu->cp0.status.ux)
            loginfo("    CP0 status: sx:  %d", cpu->cp0.status.sx)
            loginfo("    CP0 status: kx:  %d", cpu->cp0.status.kx)
            loginfo("    CP0 status: im:  %d", cpu->cp0.status.im)
            loginfo("    CP0 status: ds:  %d", cpu->cp0.status.ds)
            loginfo("    CP0 status: re:  %d", cpu->cp0.status.re)
            loginfo("    CP0 status: fr:  %d", cpu->cp0.status.fr)
            loginfo("    CP0 status: rp:  %d", cpu->cp0.status.rp)
            loginfo("    CP0 status: cu0: %d", cpu->cp0.status.cu0)
            loginfo("    CP0 status: cu1: %d", cpu->cp0.status.cu1)
            loginfo("    CP0 status: cu2: %d", cpu->cp0.status.cu2)
            loginfo("    CP0 status: cu3: %d", cpu->cp0.status.cu3)
            break;
        }
        case R4300I_CP0_REG_ENTRYLO0:
            cpu->cp0.entry_lo0.raw = value;
            break;
        case R4300I_CP0_REG_ENTRYLO1:
            cpu->cp0.entry_lo1.raw = value;
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
        default:
            logfatal("Unsupported CP0 $%s (%d) set: 0x%08X", cp0_register_names[r], r, value)
    }

    loginfo("CP0 $%s = 0x%08X", cp0_register_names[r], value)
}

INLINE word get_cp0_register(r4300i_t* cpu, byte r) {
    switch (r) {
        case R4300I_CP0_REG_ENTRYLO0:
            return cpu->cp0.entry_lo0.raw;
        case R4300I_CP0_REG_BADVADDR:
            return cpu->cp0.bad_vaddr;
        case R4300I_CP0_REG_STATUS:
            return cpu->cp0.status.raw;
        case R4300I_CP0_REG_ENTRYHI:
            return cpu->cp0.entry_hi.raw;
        case R4300I_CP0_REG_CAUSE:
            return cpu->cp0.cause.raw;
        case R4300I_CP0_REG_EPC:
            return cpu->cp0.EPC;
        case R4300I_CP0_REG_COUNT:
            return cpu->cp0.count;
        case R4300I_CP0_REG_COMPARE:
            return cpu->cp0.compare;
        case R4300I_CP0_REG_INDEX:
            return cpu->cp0.index & 0x8000003F;
        case R4300I_CP0_REG_PAGEMASK:
            return cpu->cp0.page_mask.raw;
        default:
            logfatal("Unsupported CP0 $%s (%d) read", cp0_register_names[r], r)
    }
}

INLINE void set_fpu_register_dword(r4300i_t* cpu, byte r, dword value) {
    dword* darr = (dword*)cpu->f;
    darr[r] = value;
}

INLINE void set_fpu_register_double(r4300i_t* cpu, byte r, double value) {
    double* darr = (double*)cpu->f;
    darr[r] = value;
}

INLINE dword get_fpu_register_dword(r4300i_t* cpu, byte r) {
    dword* darr = (dword*)cpu->f;
    return darr[r];
}

INLINE double get_fpu_register_double(r4300i_t* cpu, byte r) {
    double* darr = (double*)cpu->f;
    return darr[r];
}

INLINE void set_fpu_register_word(r4300i_t* cpu, byte r, word value) {
    dword* darr = (dword*)cpu->f;
    if (!cpu->cp0.status.fr) {
        if ((r & 1) == 0) {
            darr[r] &= 0xFFFFFFFF00000000;
            darr[r] |= value;
        } else {
            darr[r - 1] &= 0x00000000FFFFFFFF;
            darr[r - 1] |= (dword)value << 32;
        }
    } else {
        logfatal("Unimplemented!")
    }
}

INLINE word get_fpu_register_word(r4300i_t* cpu, byte r) {
    dword* darr = (dword*)cpu->f;
    if (!cpu->cp0.status.fr) {
        if ((r & 1) == 0) {
            return darr[r] & 0xFFFFFFFF;
        } else {
            return darr[r - 1] >> 32;
        }
    } else {
        logfatal("Unimplemented!")
    }
}

INLINE void set_fpu_register_float(r4300i_t* cpu, byte r, float value) {
    union {
        float f;
        word w;
    } conv;

    conv.f = value;
    set_fpu_register_word(cpu, r, conv.w);
}

INLINE float get_fpu_register_float(r4300i_t* cpu, byte r) {
    union {
        float f;
        word w;
    } conv;

    conv.w = get_fpu_register_word(cpu, r);
    return conv.f;
}

#endif //N64_R4300I_H
