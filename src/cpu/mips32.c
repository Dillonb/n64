#include "mips32.h"
#include "../common/log.h"
#include "sign_extension.h"

void mtc0(r4300i_t* cpu, mips32_instruction_t instruction) {
    // TODO: throw a "coprocessor unusuable exception" if CP0 disabled - see manual
    cpu->cp0.r[instruction.r.rd] = cpu->gpr[instruction.r.rt];
    logtrace("Setting CP0 r%d to the value of CPU r%d, which is 0x%016lX", instruction.r.rd, instruction.r.rt, cpu->gpr[instruction.r.rt]);
}

void lui(r4300i_t* cpu, mips32_instruction_t instruction) {
    word immediate = instruction.i.immediate << 16;

    switch (cpu->width_mode) {
        case M32:
            cpu->gpr[instruction.i.rt] = immediate;
            break;
        case M64:
            cpu->gpr[instruction.i.rt] = sign_extend_dword(immediate, 32, 64);
            break;
    }
}

// TODO: replace all register reads/writes with calls to wrapper functions
// TODO Make sure all 32bit mode accesses mask the register with 0xFFFFFFFF
// To prevent errors such as this:
// [TRACE] Setting r29 to r29 (0x00000000A4001FF0) + 0xFFFFFFE8 = 0x00000001A4001FD8
//
void addiu(r4300i_t* cpu, mips32_instruction_t instruction) {
    dword reg_addend = cpu->gpr[instruction.i.rs];

    if (cpu->width_mode == M32) {
        word addend = sign_extend_word(instruction.i.immediate, 16, 32);
        cpu->gpr[instruction.i.rt] = reg_addend + addend;
        logtrace("Setting r%d to r%d (0x%016lX) + 0x%08X = 0x%016lX",
                instruction.i.rt, instruction.i.rs, reg_addend, addend, cpu->gpr[instruction.i.rt])
    } else if (cpu->width_mode == M64) {
        dword addend = sign_extend_dword(instruction.i.immediate, 16, 64);
        cpu->gpr[instruction.i.rt] = reg_addend + addend;
        logtrace("Setting r%d to r%d (0x%016lX) + 0x%016lX = 0x%016lX",
                 instruction.i.rt, instruction.i.rs, reg_addend, addend, cpu->gpr[instruction.i.rt])
    }
}

void lw(r4300i_t* cpu, mips32_instruction_t instruction) {
    shalf offset = instruction.i.immediate;
    word address = cpu->gpr[instruction.i.rs] + offset;
    if ((address & 0b11) > 0) {
        logfatal("TODO: throw an 'address error' exception!")
    }

    dword value = cpu->read_word(address);
    if (cpu->width_mode == M64) {
        value = sign_extend_dword(value, 32, 64);
    }

    cpu->gpr[instruction.i.rt] = value;
}

void bne(r4300i_t* cpu, mips32_instruction_t instruction) {
    if (cpu->gpr[instruction.r.rs] != cpu->gpr[instruction.r.rt]) {
        logfatal("Need to branch")
    }
}
