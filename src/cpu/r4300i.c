#include "r4300i.h"
#include "../common/log.h"
#include "disassemble.h"

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

#define MIPS32_CP 0b010000

void r4300i_step(r4300i_t* cpu, word instruction) {
    mips32_instruction_t parsed;
    parsed.raw = instruction;
    char buf[50];
    disassemble32(cpu->pc, instruction, buf, 50);
    switch (parsed.op) {
        case MIPS32_CP:
            logfatal("MIPS32 coprocessor")
        default:
            logfatal("Failed to decode instruction 0x%08X opcode %d%d%d%d%d%d [%s]",
                     instruction, parsed.op0, parsed.op1, parsed.op2, parsed.op3, parsed.op4, parsed.op5, buf)

    }
}
