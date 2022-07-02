#include "rsp_instructions.h"
#include <log.h>
#include <n64_rsp_bus.h>

#define RSP_REG_LR 31

INLINE void rsp_branch_abs(word address) {
    N64RSP.next_pc = address;
}

INLINE void rsp_branch_offset(shalf offset) {
    rsp_branch_abs(N64RSP.pc + offset);
}

INLINE void rsp_conditional_branch(word offset, bool condition) {
    if (condition) {
        rsp_branch_offset(offset);
    }
}

INLINE void rsp_link(int reg) {
    set_rsp_register(reg, (N64RSP.pc << 2) + 4); // Skips the instruction in the delay slot on return
}

RSP_INSTR(rsp_nop) {}

RSP_INSTR(rsp_ori) {
    set_rsp_register(instruction.i.rt, get_rsp_register(instruction.i.rs) | instruction.i.immediate);
}

RSP_INSTR(rsp_xori) {
        set_rsp_register(instruction.i.rt, instruction.i.immediate ^ get_rsp_register(instruction.i.rs));
}

RSP_INSTR(rsp_lui) {
    word immediate = instruction.i.immediate << 16;
    set_rsp_register(instruction.i.rt, immediate);
}

RSP_INSTR(rsp_addi) {
    sword reg_addend = get_rsp_register(instruction.i.rs);
    shalf imm_addend = instruction.i.immediate;
    sword result = imm_addend + reg_addend;
    set_rsp_register(instruction.i.rt, result);
}

RSP_INSTR(rsp_spc_sll) {
    word value = get_rsp_register(instruction.r.rt);
    word result = value << instruction.r.sa;
    set_rsp_register(instruction.r.rd, result);
}

RSP_INSTR(rsp_spc_srl) {
        word value = get_rsp_register(instruction.r.rt);
        word result = value >> instruction.r.sa;
        set_rsp_register(instruction.r.rd, result);
}

RSP_INSTR(rsp_spc_sra) {
    sword value = get_rsp_register(instruction.r.rt);
    sword result = value >> instruction.r.sa;
    set_rsp_register(instruction.r.rd, result);
}

RSP_INSTR(rsp_spc_srav) {
    sword value = get_rsp_register(instruction.r.rt);
    sword result = value >> (get_rsp_register(instruction.r.rs) & 0b11111);
    set_rsp_register(instruction.r.rd, result);
}

RSP_INSTR(rsp_spc_srlv) {
    word value = get_rsp_register(instruction.r.rt);
    sword result = value >> (get_rsp_register(instruction.r.rs) & 0b11111);
    set_rsp_register(instruction.r.rd, result);
}

RSP_INSTR(rsp_spc_sllv) {
        word value = get_rsp_register(instruction.r.rt);
        sword result = value << (get_rsp_register(instruction.r.rs) & 0b11111);
        set_rsp_register(instruction.r.rd, result);
}

RSP_INSTR(rsp_spc_sub) {
    sword operand1 = get_rsp_register(instruction.r.rs);
    sword operand2 = get_rsp_register(instruction.r.rt);

    sword result = operand1 - operand2;
    set_rsp_register(instruction.r.rd, result);
}

RSP_INSTR(rsp_spc_or) {
    set_rsp_register(instruction.r.rd, get_rsp_register(instruction.r.rs) | get_rsp_register(instruction.r.rt));
}

RSP_INSTR(rsp_spc_xor) {
    set_rsp_register(instruction.r.rd, get_rsp_register(instruction.r.rs) ^ get_rsp_register(instruction.r.rt));
}

RSP_INSTR(rsp_spc_nor) {
    set_rsp_register(instruction.r.rd, ~(get_rsp_register(instruction.r.rs) | get_rsp_register(instruction.r.rt)));
}

RSP_INSTR(rsp_spc_slt) {
    sword op1 = get_rsp_register(instruction.r.rs);
    sword op2 = get_rsp_register(instruction.r.rt);

    // RS - RT
    sword result = op1 - op2;
    // if RS is LESS than RT
    // aka, if result is negative

    if (result < 0) {
        set_rsp_register(instruction.r.rd, 1);
    } else {
        set_rsp_register(instruction.r.rd, 0);
    }
}

RSP_INSTR(rsp_spc_sltu) {
    word op1 = get_rsp_register(instruction.r.rs);
    word op2 = get_rsp_register(instruction.r.rt);

    if (op1 < op2) {
        set_rsp_register(instruction.r.rd, 1);
    } else {
        set_rsp_register(instruction.r.rd, 0);
    }
}

RSP_INSTR(rsp_spc_add) {
    word addend1 = get_rsp_register(instruction.r.rs);
    word addend2 = get_rsp_register(instruction.r.rt);

    word result = addend1 + addend2;

    set_rsp_register(instruction.r.rd, result);
}

RSP_INSTR(rsp_spc_and) {
    set_rsp_register(instruction.r.rd, get_rsp_register(instruction.r.rs) & get_rsp_register(instruction.r.rt));
}

RSP_INSTR(rsp_spc_break) {
    N64RSP.status.halt = true;
    N64RSP.steps = 0;
    N64RSP.status.broke = true;

    if (N64RSP.status.intr_on_break) {
        interrupt_raise(INTERRUPT_SP);
    }
}

RSP_INSTR(rsp_lb) {
    shalf offset = instruction.i.immediate;
    word address = get_rsp_register(instruction.i.rs) + offset;

    sbyte value = n64_rsp_read_byte(address);
    set_rsp_register(instruction.i.rt, (sword)value);
}

RSP_INSTR(rsp_lbu) {
    shalf offset = instruction.i.immediate;
    word address = get_rsp_register(instruction.i.rs) + offset;

    byte value = n64_rsp_read_byte(address);
    set_rsp_register(instruction.i.rt, value);
}

RSP_INSTR(rsp_andi) {
        word immediate = instruction.i.immediate;
        word result = immediate & get_rsp_register(instruction.i.rs);
        set_rsp_register(instruction.i.rt, result);
}

RSP_INSTR(rsp_sb) {
    shalf offset = instruction.i.immediate;
    word address = get_rsp_register(instruction.i.rs) + offset;

    byte value = get_rsp_register(instruction.i.rt);
    n64_rsp_write_byte(address, value);
}

RSP_INSTR(rsp_sh) {
    shalf offset = instruction.i.immediate;
    word address = get_rsp_register(instruction.i.rs) + offset;

    half value = get_rsp_register(instruction.i.rt);
    n64_rsp_write_half(address, value);
}

RSP_INSTR(rsp_sw) {
    shalf offset = instruction.i.immediate;
    word address = get_rsp_register(instruction.i.rs) + offset;

    word value = get_rsp_register(instruction.i.rt);
    n64_rsp_write_word(address, value);
}

RSP_INSTR(rsp_lhu) {
    shalf offset = instruction.i.immediate;
    word address = get_rsp_register(instruction.i.rs) + offset;

    half value = n64_rsp_read_half(address);
    set_rsp_register(instruction.i.rt, value);
}

RSP_INSTR(rsp_lh) {
    shalf offset = instruction.i.immediate;
    word address = get_rsp_register(instruction.i.rs) + offset;

    shalf value = n64_rsp_read_half(address);
    set_rsp_register(instruction.i.rt, (sword)value);
}

RSP_INSTR(rsp_lw) {
    shalf offset = instruction.i.immediate;
    word address = get_rsp_register(instruction.i.rs) + offset;

    sword value = n64_rsp_read_word(address);
    set_rsp_register(instruction.i.rt, value);
}

RSP_INSTR(rsp_j) {
    rsp_branch_abs(instruction.j.target);
}

RSP_INSTR(rsp_jal) {
    rsp_link(RSP_REG_LR);
    rsp_branch_abs(instruction.j.target);
}

RSP_INSTR(rsp_slti) {
        shalf immediate = instruction.i.immediate;
        sword reg = get_rsp_register(instruction.i.rs);
        if (reg < immediate) {
            set_rsp_register(instruction.i.rt, 1);
        } else {
            set_rsp_register(instruction.i.rt, 0);
        }
}

RSP_INSTR(rsp_sltiu) {
    shalf immediate = instruction.i.immediate;
    word reg = get_rsp_register(instruction.i.rs);
    if (reg < immediate) {
        set_rsp_register(instruction.i.rt, 1);
    } else {
        set_rsp_register(instruction.i.rt, 0);
    }
}

RSP_INSTR(rsp_spc_jr) {
    rsp_branch_abs(get_rsp_register(instruction.r.rs) >> 2);
}

RSP_INSTR(rsp_spc_jalr) {
    rsp_branch_abs(get_rsp_register(instruction.r.rs) >> 2);
    rsp_link(instruction.r.rd);
}

RSP_INSTR(rsp_mfc0) {
    sword value = get_rsp_cp0_register(instruction.r.rd);
    set_rsp_register(instruction.r.rt, value);
}

RSP_INSTR(rsp_mtc0) {
    word value = get_rsp_register(instruction.r.rt);
    set_rsp_cp0_register(instruction.r.rd, value);
}

RSP_INSTR(rsp_bne) {
    rsp_conditional_branch(instruction.i.immediate, get_rsp_register(instruction.i.rs) != get_rsp_register(instruction.i.rt));
}

RSP_INSTR(rsp_beq) {
    rsp_conditional_branch(instruction.i.immediate, get_rsp_register(instruction.i.rs) == get_rsp_register(instruction.i.rt));
}

RSP_INSTR(rsp_bgtz) {
    sword reg = get_rsp_register(instruction.i.rs);
    rsp_conditional_branch(instruction.i.immediate, reg > 0);
}

RSP_INSTR(rsp_blez) {
    sword reg = get_rsp_register(instruction.i.rs);
    rsp_conditional_branch(instruction.i.immediate, reg <= 0);
}

RSP_INSTR(rsp_ri_bltz) {
    sword reg = get_rsp_register(instruction.i.rs);
    rsp_conditional_branch(instruction.i.immediate, reg < 0);
}

RSP_INSTR(rsp_ri_bltzal) {
    sword reg = get_rsp_register(instruction.i.rs);
    rsp_conditional_branch(instruction.i.immediate, reg < 0);
    rsp_link(RSP_REG_LR);
}

RSP_INSTR(rsp_ri_bgez) {
    sword reg = get_rsp_register(instruction.i.rs);
    rsp_conditional_branch(instruction.i.immediate, reg >= 0);
}

RSP_INSTR(rsp_ri_bgezal) {
    sword reg = get_rsp_register(instruction.i.rs);
    rsp_conditional_branch(instruction.i.immediate, reg >= 0);
    rsp_link(RSP_REG_LR);
}
