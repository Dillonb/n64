#ifndef N64_R4300I_H
#define N64_R4300I_H
#include <stdbool.h>

#include "../common/util.h"

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

    word (*read_word)(word);
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
} mips32_instruction_t;

typedef enum mips32_instruction_type {
    MTC0,
    LUI,
    ADDIU,
    LW
} mips32_instruction_type_t;

void r4300i_step(r4300i_t* cpu, word instruction);

#endif //N64_R4300I_H
