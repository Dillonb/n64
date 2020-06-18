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

INLINE void link(r4300i_t* cpu) {
    set_register(cpu, R4300I_REG_LR, cpu->pc + 4); // Skips the instruction in the delay slot on return
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
    if (condition) {
        branch_offset(cpu, offset);
    } else {
        cpu->pc += 4; // Skip instruction in delay slot
    }
}

void conditional_branch(r4300i_t* cpu, word offset, bool condition) {
    if (condition) {
        branch_offset(cpu, offset);
    }
}



MIPS32_INSTR(mips32_addi) {
    sword reg_addend = get_register(cpu, instruction.i.rs);

    sword imm_addend = sign_extend_dword(instruction.i.immediate, 16, 32);

    sword result = imm_addend + reg_addend;

    check_sword_add_overflow(imm_addend, reg_addend, result);

    set_register(cpu, instruction.i.rt, result);
    logtrace("Setting r%d to r%d (0x%08X) + %d (0x%08X) = 0x%08lX",
             instruction.i.rt, instruction.i.rs, reg_addend, imm_addend, imm_addend, get_register(cpu, instruction.i.rt))
}

MIPS32_INSTR(mips32_addiu) {
    word reg_addend = get_register(cpu, instruction.i.rs);
    word addend = sign_extend_word(instruction.i.immediate, 16, 32);

    word result = reg_addend + addend;
    dword dresult = sign_extend_dword(result, 32, 64);

    set_register(cpu, instruction.i.rt, dresult);
    logtrace("Setting r%d to r%d (0x%08X) + %d (0x%08X) = 0x%016lX",
             instruction.i.rt, instruction.i.rs, reg_addend, addend, addend, dresult)
}

MIPS32_INSTR(mips32_andi) {
    dword immediate = instruction.i.immediate;
    dword result = immediate & get_register(cpu, instruction.i.rs);
    set_register(cpu, instruction.i.rt, result);
}

MIPS32_INSTR(mips32_beq) {
    conditional_branch(cpu, instruction.i.immediate, get_register(cpu, instruction.i.rs) == get_register(cpu, instruction.i.rt));
}

MIPS32_INSTR(mips32_beql) {
    conditional_branch_likely(cpu, instruction.i.immediate, get_register(cpu, instruction.i.rs) == get_register(cpu, instruction.i.rt));
}

MIPS32_INSTR(mips32_bgtz) {
    sdword reg = get_register(cpu, instruction.i.rs);
    conditional_branch(cpu, instruction.i.immediate,  reg > 0);
}

MIPS32_INSTR(mips32_blezl) {
    sdword reg = get_register(cpu, instruction.i.rs);
    conditional_branch_likely(cpu, instruction.i.immediate, reg <= 0);
}

MIPS32_INSTR(mips32_bne) {
    logtrace("Branch if: 0x%08lX != 0x%08lX", get_register(cpu, instruction.i.rs), get_register(cpu, instruction.i.rt))
    conditional_branch(cpu, instruction.i.immediate, get_register(cpu, instruction.i.rs) != get_register(cpu, instruction.i.rt));
}

MIPS32_INSTR(mips32_bnel) {
    logtrace("Branch if: 0x%08lX != 0x%08lX", get_register(cpu, instruction.i.rs), get_register(cpu, instruction.i.rt))
    conditional_branch_likely(cpu, instruction.i.immediate, get_register(cpu, instruction.i.rs) != get_register(cpu, instruction.i.rt));
}


MIPS32_INSTR(mips32_cache) {
    return; // No need to emulate the cache. Might be fun to do someday for accuracy.
}

MIPS32_INSTR(mips32_j) {
    word target = instruction.j.target;
    target <<= 2;
    target |= ((cpu->pc - 4) & 0xF0000000); // PC is 4 ahead

    branch_abs(cpu, target);
}

MIPS32_INSTR(mips32_jal) {
    link(cpu);

    word target = instruction.j.target;
    target <<= 2;
    target |= ((cpu->pc - 4) & 0xF0000000); // PC is 4 ahead

    branch_abs(cpu, target);
}

MIPS32_INSTR(mips32_slti) {
    sdword immediate = sign_extend_word(instruction.i.immediate, 16, 64);
    logtrace("Set if %ld < %ld", get_register(cpu, instruction.i.rs), immediate)
    if (get_register(cpu, instruction.i.rs) < immediate) {
        set_register(cpu, instruction.i.rt, 1);
    } else {
        set_register(cpu, instruction.i.rt, 0);
    }
}

MIPS32_INSTR(mips32_mtc0) {
    word value = get_register(cpu, instruction.r.rt);
    set_cp0_register(cpu, instruction.r.rd, value);
}

MIPS32_INSTR(mips32_lui) {
    word immediate = instruction.i.immediate << 16;
    set_register(cpu, instruction.i.rt, sign_extend_dword(immediate, 32, 64));
}

MIPS32_INSTR(mips32_lbu) {
    shalf offset = instruction.i.immediate;
    logtrace("LBU offset: %d", offset)
    word address = get_register(cpu, instruction.i.rs) + offset;
    byte value   = cpu->read_byte(address);

    set_register(cpu, instruction.i.rt, value);
}

MIPS32_INSTR(mips32_lw) {
    shalf offset = instruction.i.immediate;
    word address = get_register(cpu, instruction.i.rs) + offset;
    if ((address & 0b11) > 0) {
        logfatal("TODO: throw an 'address error' exception! Tried to load from unaligned address 0x%08X", address)
    }

    dword value = cpu->read_word(address);
    value = sign_extend_dword(value, 32, 64);

    set_register(cpu, instruction.i.rt, value);
}

MIPS32_INSTR(mips32_sb) {
    shalf offset = instruction.i.immediate;
    word address = get_register(cpu, instruction.i.rs);
    address += offset;
    byte value = get_register(cpu, instruction.i.rt) & 0xFF;
    cpu->write_byte(address, value);
}

MIPS32_INSTR(mips32_sw) {
    shalf offset = instruction.i.immediate;
    word address = get_register(cpu, instruction.i.rs);
    address += offset;
    cpu->write_word(address, get_register(cpu, instruction.i.rt));
}

MIPS32_INSTR(mips32_ori) {
    set_register(cpu, instruction.i.rt, instruction.i.immediate | get_register(cpu, instruction.i.rs));
}

MIPS32_INSTR(mips32_xori) {
    set_register(cpu, instruction.i.rt, instruction.i.immediate ^ get_register(cpu, instruction.i.rs));
}

MIPS32_INSTR(mips32_lb) {
    shalf offset    = instruction.i.immediate;
    word address    = get_register(cpu, instruction.i.rs) + offset;
    byte value      = cpu->read_byte(address);
    word sext_value = sign_extend_word(value, 8, 64);

    set_register(cpu, instruction.i.rt, sext_value);
}

MIPS32_INSTR(mips32_spc_sll) {
    sword result = get_register(cpu, instruction.r.rt) << instruction.r.sa;
    set_register(cpu, instruction.r.rd, (sdword)result);
}

MIPS32_INSTR(mips32_spc_srl) {
    word value = get_register(cpu, instruction.r.rt);
    sword result = value >> instruction.r.sa;
    set_register(cpu, instruction.r.rd, (sdword) result);
}

MIPS32_INSTR(mips32_spc_sllv) {
    word value = get_register(cpu, instruction.r.rt);
    sword result = value << (get_register(cpu, instruction.r.rs) & 0b11111);
    set_register(cpu, instruction.r.rd, (sdword)result);
}

MIPS32_INSTR(mips32_spc_srlv) {
    word value = get_register(cpu, instruction.r.rt);
    sword result = value >> (get_register(cpu, instruction.r.rs) & 0b11111);
    set_register(cpu, instruction.r.rd, (sdword)result);
}

MIPS32_INSTR(mips32_spc_jr) {
    branch_abs(cpu, get_register(cpu, instruction.r.rs));
}

MIPS32_INSTR(mips32_spc_mfhi) {
    logfatal("mfhi")
}

MIPS32_INSTR(mips32_spc_mflo) {
    set_register(cpu, instruction.r.rd, cpu->mult_lo);
}

MIPS32_INSTR(mips32_spc_multu) {
    word multiplicand_1 = get_register(cpu, instruction.r.rs);
    word multiplicand_2 = get_register(cpu, instruction.r.rt);

    dword result = multiplicand_1 * multiplicand_2;

    word result_lower = result         & 0xFFFFFFFF;
    word result_upper = (result >> 32) & 0xFFFFFFFF;

    cpu->mult_lo = result_lower;
    cpu->mult_hi = result_upper;
}

MIPS32_INSTR(mips32_spc_add) {

    sword addend1 = get_register(cpu, instruction.r.rs);
    sword addend2 = get_register(cpu, instruction.r.rt);

    sword sresult = addend1 + addend2;
    check_sword_add_overflow(addend1, addend2, sresult);

    dword result = sign_extend_dword(sresult, 32, 64);

    set_register(cpu, instruction.r.rd, result);
}

MIPS32_INSTR(mips32_spc_addu) {
    word result = get_register(cpu, instruction.r.rs) + get_register(cpu, instruction.r.rt);
    dword sex_result = sign_extend_dword(result, 32, 64);
    set_register(cpu, instruction.r.rd, sex_result);
}

MIPS32_INSTR(mips32_spc_and) {
    dword result = get_register(cpu, instruction.r.rs) & get_register(cpu, instruction.r.rt);
    set_register(cpu, instruction.r.rd, result);
}

MIPS32_INSTR(mips32_spc_subu) {
    word operand1 = get_register(cpu, instruction.r.rs);
    word operand2 = get_register(cpu, instruction.r.rt);

    word result = operand1 - operand2;
    set_register(cpu, instruction.r.rd, result);
}

MIPS32_INSTR(mips32_spc_or) {
    set_register(cpu, instruction.r.rd, get_register(cpu, instruction.r.rs) | get_register(cpu, instruction.r.rt));
}

MIPS32_INSTR(mips32_spc_xor) {
    set_register(cpu, instruction.r.rd, get_register(cpu, instruction.r.rs) ^ get_register(cpu, instruction.r.rt));
}

MIPS32_INSTR(mips32_spc_slt) {
    sdword op1 = get_register(cpu, instruction.r.rs);
    sdword op2 = get_register(cpu, instruction.r.rt);

    // RS - RT
    sdword result = op1 - op2;
    // if RS is LESS than RT
    // aka, if result is negative

    logtrace("Set if %ld < %ld", op1, op2)
    if (result < 0) {
        set_register(cpu, instruction.r.rd, 1);
    } else {
        set_register(cpu, instruction.r.rd, 0);
    }
}

MIPS32_INSTR(mips32_spc_sltu) {
    dword op1 = get_register(cpu, instruction.r.rs);
    dword op2 = get_register(cpu, instruction.r.rt);

    logtrace("Set if %lu < %lu", op1, op2)
    if (op1 < op2) {
        set_register(cpu, instruction.r.rd, 1);
    } else {
        set_register(cpu, instruction.r.rd, 0);
    }
}

MIPS32_INSTR(mips32_ri_bgezl) {
    sdword reg = get_register(cpu, instruction.i.rs);
    conditional_branch_likely(cpu, instruction.i.immediate, reg >= 0);
}

MIPS32_INSTR(mips32_ri_bgezal) {
    link(cpu);
    sdword reg = get_register(cpu, instruction.i.rs);
    conditional_branch(cpu, instruction.i.immediate, reg >= 0);
}
