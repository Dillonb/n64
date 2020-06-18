#ifndef N64_R4300I_H
#define N64_R4300I_H
#include <stdbool.h>

#include "../common/util.h"
#include "../common/log.h"

#define R4300I_REG_LR 31

#define R4300I_CP0_REG_COUNT   9
#define R4300I_CP0_REG_COMPARE 11
#define R4300I_CP0_REG_CAUSE   13
#define R4300I_CP0_REG_TAGLO   28
#define R4300I_CP0_REG_TAGHI   29

typedef struct cp0 {
    // Internal tool for stepping $Count
    bool count_stepper;
    word r[32];
} cp0_t;

typedef struct r4300i {
    dword gpr[32];
    dword pc;
    dword mult_hi;
    dword mult_lo;
    bool llb;
    cp0_t cp0;

    // Branch delay
    bool branch;
    int branch_delay;
    word branch_pc;

    byte (*read_byte)(word);
    void (*write_byte)(word, byte);

    /*
    half (*read_half)(word);
    void (*write_half)(word, half);
     */

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

} mips_instruction_t;

typedef enum mips_instruction_type {
    MIPS_LD,
    MIPS_LUI,
    MIPS_ADDI,
    MIPS_DADDI,
    MIPS_ADDIU,
    MIPS_ANDI,
    MIPS_LBU,
    MIPS_LW,
    MIPS_BLEZL,
    MIPS_BNE,
    MIPS_BNEL,
    MIPS_CACHE,
    MIPS_BEQ,
    MIPS_BEQL,
    MIPS_BGTZ,
    MIPS_NOP,
    MIPS_SB,
    MIPS_SD,
    MIPS_SW,
    MIPS_ORI,
    MIPS_J,
    MIPS_JAL,
    MIPS_SLTI,
    MIPS_XORI,
    MIPS_LB,

    // Coprocessor
    MIPS_CP_MTC0,

    // Special
    MIPS_SPC_SLL,
    MIPS_SPC_SRL,
    MIPS_SPC_SLLV,
    MIPS_SPC_SRLV,
    MIPS_SPC_JR,
    MIPS_SPC_MFHI,
    MIPS_SPC_MFLO,
    MIPS_SPC_MULTU,
    MIPS_SPC_ADD,
    MIPS_SPC_ADDU,
    MIPS_SPC_AND,
    MIPS_SPC_SUBU,
    MIPS_SPC_OR,
    MIPS_SPC_XOR,
    MIPS_SPC_SLT,
    MIPS_SPC_SLTU,
    MIPS_SPC_DADD,

    // REGIMM
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

INLINE void set_cp0_register(r4300i_t* cpu, byte r, word value) {
    logwarn("TODO: throw a \"coprocessor unusuable exception\" if CP0 disabled")
    switch (r) {
        // No special handling needed for these registers
        case R4300I_CP0_REG_COUNT:
        case R4300I_CP0_REG_CAUSE:
        case R4300I_CP0_REG_TAGLO: // Used for the cache, which is unimplemented.
        case R4300I_CP0_REG_TAGHI: // Used for the cache, which is unimplemented.
            break;
        case R4300I_CP0_REG_COMPARE:
            logwarn("$Compare written with 0x%08X (count is now 0x%08X) - TODO: clear interrupt in $Cause", value, cpu->cp0.r[R4300I_CP0_REG_COUNT]);
            break;
        default:
            logfatal("Unsupported CP0 $%s (%d) set: 0x%08X", cp0_register_names[r], r, value)
    }

    logwarn("CP0 $%s = 0x%08X", cp0_register_names[r], value)

    cpu->cp0.r[r] = value;
}

#endif //N64_R4300I_H
