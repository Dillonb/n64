#include "mips32.h"
#include "../common/log.h"
#include "sign_extension.h"

void mtc0(r4300i_t* cpu, mips32_instruction_t instruction) {
    // TODO: throw a "coprocessor unusuable exception" if CP0 disabled - see manual
    cpu->cp0.r[instruction.r.rd] = cpu->gpr[instruction.r.rt];
    set_cp0_register(cpu, instruction.r.rd, cpu->gpr[instruction.r.rt]);
    logtrace("Setting CP0 r%d to the value of CPU r%d, which is 0x%016lX", instruction.r.rd, instruction.r.rt, cpu->gpr[instruction.r.rt]);
}

void lui(r4300i_t* cpu, mips32_instruction_t instruction) {
    word immediate = instruction.i.immediate << 16;

    switch (cpu->width_mode) {
        case M32:
            set_register(cpu, instruction.i.rt, immediate);
            break;
        case M64:
            set_register(cpu, instruction.i.rt, sign_extend_dword(immediate, 32, 64));
            break;
    }
}

void addiu(r4300i_t* cpu, mips32_instruction_t instruction) {
    dword reg_addend = cpu->gpr[instruction.i.rs];

    if (cpu->width_mode == M32) {
        word addend = sign_extend_word(instruction.i.immediate, 16, 32);
        set_register(cpu, instruction.i.rt, reg_addend + addend);
        logtrace("Setting r%d to r%d (0x%016lX) + %d (0x%08X) = 0x%016lX",
                instruction.i.rt, instruction.i.rs, reg_addend, addend, addend, cpu->gpr[instruction.i.rt])
    } else if (cpu->width_mode == M64) {
        dword addend = sign_extend_dword(instruction.i.immediate, 16, 64);
        set_register(cpu, instruction.i.rt, reg_addend + addend);
        logtrace("Setting r%d to r%d (0x%016lX) + %d (0x%016lX) = 0x%016lX",
                 instruction.i.rt, instruction.i.rs, reg_addend, addend, addend, cpu->gpr[instruction.i.rt])
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

    set_register(cpu, instruction.i.rt, value);
}

void sw(r4300i_t* cpu, mips32_instruction_t instruction) {
    shalf offset = instruction.i.immediate;
    if (cpu->width_mode == M32) {
        word address = cpu->gpr[instruction.i.rs];
        address += offset;
        cpu->write_word(address, cpu->gpr[instruction.i.rt]);
    } else {
        logfatal("Store word in 64 bit mode")
    }
}

void bne(r4300i_t* cpu, mips32_instruction_t instruction) {
    if (cpu->gpr[instruction.r.rs] != cpu->gpr[instruction.r.rt]) {
        logfatal("Need to branch")
    }
}

void ori(r4300i_t* cpu, mips32_instruction_t instruction) {
    set_register(cpu, instruction.i.rt, instruction.i.immediate | cpu->gpr[instruction.i.rs]);
}
