#include "mips32.h"
#include "../common/log.h"
#include "sign_extension.h"

void check_sword_add_overflow(sword addend1, sword addend2, sword result) {
    if (addend1 > 0 && addend2 > 0) {
        if (result < 0) {
            logfatal("Integer overflow exception")
        }
    } else if (addend1 < 0 && addend2 < 0) {
        if (result > 0) {
            logfatal("Integer overflow exception")
        }
    }
}

void check_sdword_add_overflow(sdword addend1, sdword addend2, sdword result) {
    if (addend1 > 0 && addend2 > 0) {
        if (result < 0) {
            logfatal("Integer overflow exception")
        }
    } else if (addend1 < 0 && addend2 < 0) {
        if (result > 0) {
            logfatal("Integer overflow exception")
        }
    }
}

MIPS32_INSTR(add) {
    logfatal("ADD unimplemented")
}

MIPS32_INSTR(addi) {
    dword reg_addend = cpu->gpr[instruction.i.rs];

    if (cpu->width_mode == M32) {
        sword addend1 = sign_extend_word(instruction.i.immediate, 16, 32);
        sword addend2 = reg_addend;
        sword result = addend1 + addend2;

        check_sword_add_overflow(addend1, addend2, result);

        set_register(cpu, instruction.i.rt, result);
        logtrace("Setting r%d to r%d (0x%016lX) + %d (0x%08X) = 0x%016lX",
                 instruction.i.rt, instruction.i.rs, reg_addend, addend1, addend1, cpu->gpr[instruction.i.rt])
    } else if (cpu->width_mode == M64) {
        sdword addend1 = sign_extend_dword(instruction.i.immediate, 16, 64);
        sdword addend2 = reg_addend;
        sdword result = addend1 + addend2;

        check_sdword_add_overflow(addend1, addend2, result);

        set_register(cpu, instruction.i.rt, result);
        logtrace("Setting r%d to r%d (0x%016lX) + %ld (0x%016lX) = 0x%016lX",
                 instruction.i.rt, instruction.i.rs, reg_addend, addend1, addend1, cpu->gpr[instruction.i.rt])
    }
}

MIPS32_INSTR(addiu) {
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

MIPS32_INSTR(andi) {
    if (cpu->width_mode == M32) {
        word immediate = instruction.i.immediate;
        word result = immediate & cpu->gpr[instruction.i.rs];
        set_register(cpu, instruction.i.rt, result);
    } else {
        dword immediate = instruction.i.immediate;
        dword result = immediate & cpu->gpr[instruction.i.rs];
        set_register(cpu, instruction.i.rt, result);
    }
}

void branch(r4300i_t* cpu, word offset) {
    sword soffset = sign_extend_word(offset, 16, 32);
    soffset <<= 2;
    // This is taking advantage of the fact that we add 4 to the PC after each instruction.
    // Due to the compiler expecting pipelining, the address we get here will be 4 _too early_
    cpu->branch_pc = cpu->pc + soffset;

    // Execute one instruction before taking the branch
    cpu->branch = true;
    cpu->branch_delay = 1;
}

void conditional_branch(r4300i_t* cpu, word offset, bool condition) {
    unimplemented(cpu->width_mode == M64, "Branch in 64bit mode")
    if (condition) {
        branch(cpu, offset);
    }
}

MIPS32_INSTR(beq) {
    conditional_branch(cpu, instruction.i.immediate, cpu->gpr[instruction.i.rs] == cpu->gpr[instruction.i.rt]);
}

void beql(r4300i_t* cpu, mips32_instruction_t instruction) {
    unimplemented(cpu->width_mode == M64, "BEQL in 64bit mode")
    if (cpu->gpr[instruction.i.rs] == cpu->gpr[instruction.i.rt]) {
        branch(cpu, instruction.i.immediate);
    } else {
        cpu->pc += 4; // Skip instruction in delay slot
    }
}

MIPS32_INSTR(bne) {
    conditional_branch(cpu, instruction.i.immediate, cpu->gpr[instruction.i.rs] != cpu->gpr[instruction.i.rt]);
}

MIPS32_INSTR(jal) {
    unimplemented(cpu->width_mode == M64, "JAL in 64 bit mode")
    set_register(cpu, R4300I_REG_LR, cpu->pc + 4); // Skips the next instruction for some reason

    word target = instruction.j.target;
    target <<= 2;
    target |= ((cpu->pc - 4) & 0xF0000000); // PC is 4 ahead

    cpu->pc = target;
}

MIPS32_INSTR(slti) {
    unimplemented(cpu->width_mode == M64, "SLTI in 64 bit mode")
    sword immediate = sign_extend_word(instruction.i.immediate, 16, 32);
    if (cpu->gpr[instruction.i.rs] < immediate) {
        cpu->gpr[instruction.i.rt] = 1;
    } else {
        cpu->gpr[instruction.i.rt] = 0;
    }
}

MIPS32_INSTR(mtc0) {
    // TODO: throw a "coprocessor unusuable exception" if CP0 disabled - see manual
    cpu->cp0.r[instruction.r.rd] = cpu->gpr[instruction.r.rt];
    set_cp0_register(cpu, instruction.r.rd, cpu->gpr[instruction.r.rt]);
    logtrace("Setting CP0 r%d to the value of CPU r%d, which is 0x%016lX", instruction.r.rd, instruction.r.rt, cpu->gpr[instruction.r.rt]);
}

MIPS32_INSTR(lui) {
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


MIPS32_INSTR(lw) {
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

MIPS32_INSTR(sw) {
    shalf offset = instruction.i.immediate;
    if (cpu->width_mode == M32) {
        word address = cpu->gpr[instruction.i.rs];
        address += offset;
        cpu->write_word(address, cpu->gpr[instruction.i.rt]);
    } else {
        logfatal("Store word in 64 bit mode")
    }
}

MIPS32_INSTR(ori) {
    set_register(cpu, instruction.i.rt, instruction.i.immediate | cpu->gpr[instruction.i.rs]);
}

MIPS32_INSTR(xori) {
    set_register(cpu, instruction.i.rt, instruction.i.immediate ^ cpu->gpr[instruction.i.rs]);
}