#include "mips32.h"
#include "../common/log.h"
#include "sign_extension.h"

#define NO64 unimplemented(cpu->width_mode == M64, "64 bit mode unimplemented for this instruction!")

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

MIPS32_INSTR(addi) {
    dword reg_addend = get_register(cpu, instruction.i.rs);

    if (cpu->width_mode == M32) {
        sword addend1 = sign_extend_word(instruction.i.immediate, 16, 32);
        sword addend2 = reg_addend;
        sword result = addend1 + addend2;

        check_sword_add_overflow(addend1, addend2, result);

        set_register(cpu, instruction.i.rt, result);
        logtrace("Setting r%d to r%d (0x%016lX) + %d (0x%08X) = 0x%016lX",
                 instruction.i.rt, instruction.i.rs, reg_addend, addend1, addend1, get_register(cpu, instruction.i.rt ))
    } else if (cpu->width_mode == M64) {
        sdword addend1 = sign_extend_dword(instruction.i.immediate, 16, 64);
        sdword addend2 = reg_addend;
        sdword result = addend1 + addend2;

        check_sdword_add_overflow(addend1, addend2, result);

        set_register(cpu, instruction.i.rt, result);
        logtrace("Setting r%d to r%d (0x%016lX) + %ld (0x%016lX) = 0x%016lX",
                 instruction.i.rt, instruction.i.rs, reg_addend, addend1, addend1, get_register(cpu, instruction.i.rt))
    }
}

MIPS32_INSTR(addiu) {
    dword reg_addend = get_register(cpu, instruction.i.rs);

    if (cpu->width_mode == M32) {
        word addend = sign_extend_word(instruction.i.immediate, 16, 32);
        set_register(cpu, instruction.i.rt, reg_addend + addend);
        logtrace("Setting r%d to r%d (0x%016lX) + %d (0x%08X) = 0x%016lX",
                 instruction.i.rt, instruction.i.rs, reg_addend, addend, addend, get_register(cpu, instruction.i.rt))
    } else if (cpu->width_mode == M64) {
        dword addend = sign_extend_dword(instruction.i.immediate, 16, 64);
        set_register(cpu, instruction.i.rt, reg_addend + addend);
        logtrace("Setting r%d to r%d (0x%016lX) + %ld (0x%016lX) = 0x%016lX",
                 instruction.i.rt, instruction.i.rs, reg_addend, addend, addend, get_register(cpu, instruction.i.rt))
    }
}

MIPS32_INSTR(andi) {
    if (cpu->width_mode == M32) {
        word immediate = instruction.i.immediate;
        word result = immediate & get_register(cpu, instruction.i.rs);
        set_register(cpu, instruction.i.rt, result);
    } else {
        dword immediate = instruction.i.immediate;
        dword result = immediate & get_register(cpu, instruction.i.rs);
        set_register(cpu, instruction.i.rt, result);
    }
}

void branch_abs(r4300i_t* cpu, word address) {
    cpu->branch_pc = address;

    // Execute one instruction before taking the branch_offset
    cpu->branch = true;
    cpu->branch_delay = 1;

    logtrace("Setting up a branch_offset (delayed by 1 instruction) to 0x%08X", cpu->branch_pc)
}

void branch_offset(r4300i_t* cpu, word offset) {
    sword soffset = sign_extend_word(offset, 16, 32);
    soffset <<= 2;
    // This is taking advantage of the fact that we add 4 to the PC after each instruction.
    // Due to the compiler expecting pipelining, the address we get here will be 4 _too early_

    branch_abs(cpu, cpu->pc + soffset);
}

void conditional_branch_likely(r4300i_t* cpu, word offset, bool condition) {
    NO64
    if (condition) {
        branch_offset(cpu, offset);
    } else {
        cpu->pc += 4; // Skip instruction in delay slot
    }
}

void conditional_branch(r4300i_t* cpu, word offset, bool condition) {
    unimplemented(cpu->width_mode == M64, "Branch in 64bit mode")
    if (condition) {
        branch_offset(cpu, offset);
    }
}

MIPS32_INSTR(beq) {
    conditional_branch(cpu, instruction.i.immediate, get_register(cpu, instruction.i.rs) == get_register(cpu, instruction.i.rt));
}

void beql(r4300i_t* cpu, mips32_instruction_t instruction) {
    unimplemented(cpu->width_mode == M64, "BEQL in 64bit mode")
    conditional_branch_likely(cpu, instruction.i.immediate, get_register(cpu, instruction.i.rs) == get_register(cpu, instruction.i.rt));
}

MIPS32_INSTR(ble) {
    conditional_branch(cpu, instruction.i.immediate, get_register(cpu, instruction.i.rs) != get_register(cpu, instruction.i.rt));
}

MIPS32_INSTR(blezl) {
    sword reg = get_register(cpu, instruction.i.rs);
    conditional_branch_likely(cpu, instruction.i.immediate, reg <= 0);
}

MIPS32_INSTR(bne) {
    logtrace("Branch if: 0x%08lX != 0x%08lX", get_register(cpu, instruction.i.rs), get_register(cpu, instruction.i.rt))
    conditional_branch(cpu, instruction.i.immediate, get_register(cpu, instruction.i.rs) != get_register(cpu, instruction.i.rt));
}

MIPS32_INSTR(bnel) {
    logtrace("Branch if: 0x%08lX != 0x%08lX", get_register(cpu, instruction.i.rs), get_register(cpu, instruction.i.rt))
    conditional_branch_likely(cpu, instruction.i.immediate, get_register(cpu, instruction.i.rs) != get_register(cpu, instruction.i.rt));
}


MIPS32_INSTR(cache) {
    return; // No need to emulate the cache. Might be fun to do someday for accuracy.
}

MIPS32_INSTR(jal) {
    unimplemented(cpu->width_mode == M64, "JAL in 64 bit mode")
    set_register(cpu, R4300I_REG_LR, cpu->pc + 4); // Skips the instruction in the delay slot on return

    word target = instruction.j.target;
    target <<= 2;
    target |= ((cpu->pc - 4) & 0xF0000000); // PC is 4 ahead

    branch_abs(cpu, target);
}

MIPS32_INSTR(slti) {
    unimplemented(cpu->width_mode == M64, "SLTI in 64 bit mode")
    sword immediate = sign_extend_word(instruction.i.immediate, 16, 32);
    logtrace("Set if %ld < %d", get_register(cpu, instruction.i.rs), immediate)
    if (get_register(cpu, instruction.i.rs) < immediate) {
        set_register(cpu, instruction.i.rt, 1);
    } else {
        set_register(cpu, instruction.i.rt, 0);
    }
}

MIPS32_INSTR(mtc0) {
    // TODO: throw a "coprocessor unusuable exception" if CP0 disabled - see manual
    set_cp0_register(cpu, instruction.r.rd, get_register(cpu, instruction.r.rt));
    logtrace("Setting CP0 r%d to the value of CPU r%d, which is 0x%016lX", instruction.r.rd, instruction.r.rt, get_register(cpu, instruction.r.rt));
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

MIPS32_INSTR(lbu) {
    shalf offset = instruction.i.immediate;
    word address = get_register(cpu, instruction.i.rs) + offset;
    byte value   = cpu->read_byte(address);

    set_register(cpu, instruction.i.rt, value);
}

MIPS32_INSTR(lw) {
    shalf offset = instruction.i.immediate;
    word address = get_register(cpu, instruction.i.rs) + offset;
    if ((address & 0b11) > 0) {
        logfatal("TODO: throw an 'address error' exception!")
    }

    dword value = cpu->read_word(address);
    if (cpu->width_mode == M64) {
        value = sign_extend_dword(value, 32, 64);
    }

    set_register(cpu, instruction.i.rt, value);
}

MIPS32_INSTR(sb) {
    NO64
    shalf offset = instruction.i.immediate;
    word address = get_register(cpu, instruction.i.rs);
    address += offset;
    byte value = get_register(cpu, instruction.i.rt) & 0xFF;
    cpu->write_byte(address, value);
}

MIPS32_INSTR(sw) {
    shalf offset = instruction.i.immediate;
    if (cpu->width_mode == M32) {
        word address = get_register(cpu, instruction.i.rs);
        address += offset;
        cpu->write_word(address, get_register(cpu, instruction.i.rt));
    } else {
        logfatal("Store word in 64 bit mode")
    }
}

MIPS32_INSTR(ori) {
    set_register(cpu, instruction.i.rt, instruction.i.immediate | get_register(cpu, instruction.i.rs));
}

MIPS32_INSTR(xori) {
    set_register(cpu, instruction.i.rt, instruction.i.immediate ^ get_register(cpu, instruction.i.rs));
}

MIPS32_INSTR(spc_srl) {
    NO64
    set_register(cpu, instruction.r.rd, get_register(cpu, instruction.r.rt) >> instruction.r.sa);
}

MIPS32_INSTR(spc_jr) {
    branch_abs(cpu, get_register(cpu, instruction.r.rs));
}

MIPS32_INSTR(spc_mfhi) {
    logfatal("mfhi")
}

MIPS32_INSTR(spc_mflo) {
    set_register(cpu, instruction.r.rd, cpu->mult_lo);
}

MIPS32_INSTR(spc_multu) {
    NO64

    word multiplicand_1 = get_register(cpu, instruction.r.rs);
    word multiplicand_2 = get_register(cpu, instruction.r.rt);

    dword result = multiplicand_1 * multiplicand_2;

    word result_lower = result         & 0xFFFFFFFF;
    word result_upper = (result >> 32) & 0xFFFFFFFF;

    cpu->mult_lo = result_lower;
    cpu->mult_hi = result_upper;
}

MIPS32_INSTR(spc_add) {
    NO64

    sword addend1 = get_register(cpu, instruction.r.rs);
    sword addend2 = get_register(cpu, instruction.r.rt);

    sword result = addend1 + addend2;
    check_sword_add_overflow(addend1, addend2, result);

    set_register(cpu, instruction.r.rd, result);
}

MIPS32_INSTR(spc_addu) {
    NO64

    word result = get_register(cpu, instruction.r.rs) + get_register(cpu, instruction.r.rt);
    set_register(cpu, instruction.r.rd, result);
}

MIPS32_INSTR(spc_and) {
    NO64

    word result = get_register(cpu, instruction.r.rs) & get_register(cpu, instruction.r.rt);
    set_register(cpu, instruction.r.rd, result);
}

MIPS32_INSTR(spc_subu) {
    NO64

    word result = get_register(cpu, instruction.r.rs) - get_register(cpu, instruction.r.rt);
    set_register(cpu, instruction.r.rd, result);
}

MIPS32_INSTR(spc_or) {
    set_register(cpu, instruction.r.rd, get_register(cpu, instruction.r.rs) | get_register(cpu, instruction.r.rt));
}

MIPS32_INSTR(spc_slt) {
    NO64

    sword op1 = get_register(cpu, instruction.r.rs);
    sword op2 = get_register(cpu, instruction.r.rt);

    // RS - RT
    sword result = op1 - op2;
    // if RS is LESS than RT
    // aka, if result is negative

    logtrace("Set if %d < %d", op1, op2)
    if (result < 0) {
        set_register(cpu, instruction.r.rd, 1);
    } else {
        set_register(cpu, instruction.r.rd, 0);
    }
}

MIPS32_INSTR(spc_sltu) {
    NO64

    word op1 = get_register(cpu, instruction.r.rs);
    word op2 = get_register(cpu, instruction.r.rt);

    logtrace("Set if %u < %u", op1, op2)
    if (op1 < op2) {
        set_register(cpu, instruction.r.rd, 1);
    } else {
        set_register(cpu, instruction.r.rd, 0);
    }
}

MIPS32_INSTR(ri_bgezl) {
    sword reg = get_register(cpu, instruction.i.rs);
    conditional_branch_likely(cpu, instruction.i.immediate, reg >= 0);
}