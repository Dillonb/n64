#include "mips32.h"
#include "../common/log.h"
#include "sign_extension.h"

void add(r4300i_t* cpu, mips32_instruction_t instruction) {
    logfatal("ADD unimplemented")
}

void addi(r4300i_t* cpu, mips32_instruction_t instruction) {
    dword reg_addend = cpu->gpr[instruction.i.rs];

    if (cpu->width_mode == M32) {
        sword addend1 = sign_extend_word(instruction.i.immediate, 16, 32);
        sword addend2 = reg_addend;
        sword result = addend1 + addend2;

        if (addend1 > 0 && addend2 > 0) {
            if (result < 0) {
                logfatal("Integer overflow exception")
            }
        } else if (addend1 < 0 && addend2 < 0) {
            if (result > 0) {
                logfatal("Integer overflow exception")
            }
        }


        set_register(cpu, instruction.i.rt, result);
        logtrace("Setting r%d to r%d (0x%016lX) + %d (0x%08X) = 0x%016lX",
                 instruction.i.rt, instruction.i.rs, reg_addend, addend1, addend1, cpu->gpr[instruction.i.rt])
    } else if (cpu->width_mode == M64) {
        sdword addend1 = sign_extend_dword(instruction.i.immediate, 16, 64);
        sdword addend2 = reg_addend;
        sdword result = addend1 + addend2;

        if (addend1 > 0 && addend2 > 0) {
            if (result < 0) {
                logfatal("Integer overflow exception")
            }
        } else if (addend1 < 0 && addend2 < 0) {
            if (result > 0) {
                logfatal("Integer overflow exception")
            }
        }

        set_register(cpu, instruction.i.rt, result);
        logtrace("Setting r%d to r%d (0x%016lX) + %ld (0x%016lX) = 0x%016lX",
                 instruction.i.rt, instruction.i.rs, reg_addend, addend1, addend1, cpu->gpr[instruction.i.rt])
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
        logtrace("Setting r%d to r%d (0x%016lX) + %ld (0x%016lX) = 0x%016lX",
                 instruction.i.rt, instruction.i.rs, reg_addend, addend, addend, cpu->gpr[instruction.i.rt])
    }
}

void conditional_branch(r4300i_t* cpu, word offset, bool condition) {
    unimplemented(cpu->width_mode == M64, "BNE in 64bit mode")
    if (condition) {
        sword soffset = sign_extend_word(offset, 16, 32);
        soffset <<= 2;
        // This is taking advantage of the fact that we add 4 to the PC after each instruction.
        // Due to the compiler expecting pipelining, the address we get here will be 4 _too early_
        cpu->pc += soffset;
    }
}

void beq(r4300i_t* cpu, mips32_instruction_t instruction) {
    conditional_branch(cpu, instruction.i.immediate, cpu->gpr[instruction.i.rs] == cpu->gpr[instruction.i.rt]);
}

void bne(r4300i_t* cpu, mips32_instruction_t instruction) {
    conditional_branch(cpu, instruction.i.immediate, cpu->gpr[instruction.i.rs] != cpu->gpr[instruction.i.rt]);
}

void jal(r4300i_t* cpu, mips32_instruction_t instruction) {
    unimplemented(cpu->width_mode == M64, "JAL in 64 bit mode")
    set_register(cpu, R4300I_REG_LR, cpu->pc + 4); // Skips the next instruction for some reason

    word target = instruction.j.target;
    target <<= 2;
    target |= ((cpu->pc - 4) & 0xF0000000); // PC is 4 ahead

    cpu->pc = target;
}

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

void ori(r4300i_t* cpu, mips32_instruction_t instruction) {
    set_register(cpu, instruction.i.rt, instruction.i.immediate | cpu->gpr[instruction.i.rs]);
}
