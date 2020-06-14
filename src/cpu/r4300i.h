#ifndef N64_R4300I_H
#define N64_R4300I_H
#include <stdbool.h>

#include "../common/util.h"
#include "../common/log.h"

#define R4300I_REG_LR 31

typedef enum width_mode {
    M32,
    M64
} width_mode_t;

typedef struct cp0 {
    dword r[32];
} cp0_t;

typedef struct r4300i {
    width_mode_t width_mode;
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

    /*
    dword (*read_dword)(word);
    void (*write_dword)(word, dword);
     */
} r4300i_t;

typedef union mips32_instruction {
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

} mips32_instruction_t;

typedef enum mips32_instruction_type {
    LUI,
    ADDI,
    ADDIU,
    ANDI,
    LBU,
    LW,
    BLEZL,
    BNE,
    BNEL,
    BEQ,
    BEQL,
    NOP,
    SB,
    SW,
    ORI,
    JAL,
    SLTI,
    XORI,

    // Coprocessor
    CP_MTC0,

    // Special
    SPC_SRL,
    SPC_JR,
    SPC_MFHI,
    SPC_MFLO,
    SPC_MULTU,
    SPC_ADD,
    SPC_ADDU,
    SPC_AND,
    SPC_SUBU,
    SPC_OR,
    SPC_SLT,

    // REGIMM
    RI_BGEZL
} mips32_instruction_type_t;

void r4300i_step(r4300i_t* cpu);

INLINE void set_register(r4300i_t* cpu, byte r, dword value) {
    if (cpu->width_mode == M32) {
        value &= 0xFFFFFFFF;
        logtrace("Setting r%d to [0x%08lX]", r, value)
    } else {
        logtrace("Setting r%d to [0x%016lX]", r, value)
    }
    if (r != 0) {
        if (r < 64) {
            cpu->gpr[r] = value;
        } else {
            logfatal("Write to invalid register: %d", r)
        }
    }
}

INLINE dword get_register(r4300i_t* cpu, word r) {
    dword mask = cpu->width_mode == M32 ? 0xFFFFFFFF : 0xFFFFFFFFFFFFFFFF;
    if (r < 64) {
        dword value = cpu->gpr[r] & mask;
        logtrace("Reading r%d: 0x%08lX", r, value)
        return value;
    } else {
        logfatal("Attempted to read invalid register: %d", r)
    }
}

INLINE void set_cp0_register(r4300i_t* cpu, int r, dword value) {
    // TODO do these need to be forced to 32 bits as well?
    cpu->cp0.r[r] = value;
}

#endif //N64_R4300I_H
