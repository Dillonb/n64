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

void addiu(r4300i_t* cpu, mips32_instruction_t instruction) {
    if (cpu->width_mode == M32) {
        word addend = sign_extend_word(instruction.i.immediate, 16, 32);
        cpu->gpr[instruction.i.rt] = cpu->gpr[instruction.i.rs] + addend;
    } else if (cpu->width_mode == M64) {
        dword addend = sign_extend_dword(instruction.i.immediate, 16, 64);
        cpu->gpr[instruction.i.rt] = cpu->gpr[instruction.i.rs] + addend;
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
