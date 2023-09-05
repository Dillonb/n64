#include "rsp_instructions.h"
#include <log.h>
#include <n64_rsp_bus.h>

#define RSP_REG_LR 31

INLINE void rsp_branch_abs(u32 address) {
    N64RSP.next_pc = address;
}

INLINE void rsp_branch_offset(s16 offset) {
    rsp_branch_abs(N64RSP.pc + offset);
}

INLINE void rsp_conditional_branch(u32 offset, bool condition) {
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
    u32 immediate = instruction.i.immediate << 16;
    set_rsp_register(instruction.i.rt, immediate);
}

RSP_INSTR(rsp_addi) {
    s32 reg_addend = get_rsp_register(instruction.i.rs);
    s16 imm_addend = instruction.i.immediate;
    s32 result = imm_addend + reg_addend;
    set_rsp_register(instruction.i.rt, result);
}

RSP_INSTR(rsp_spc_sll) {
    u32 value = get_rsp_register(instruction.r.rt);
    u32 result = value << instruction.r.sa;
    set_rsp_register(instruction.r.rd, result);
}

RSP_INSTR(rsp_spc_srl) {
        u32 value = get_rsp_register(instruction.r.rt);
        u32 result = value >> instruction.r.sa;
        set_rsp_register(instruction.r.rd, result);
}

RSP_INSTR(rsp_spc_sra) {
    s32 value = get_rsp_register(instruction.r.rt);
    s32 result = value >> instruction.r.sa;
    set_rsp_register(instruction.r.rd, result);
}

RSP_INSTR(rsp_spc_srav) {
    s32 value = get_rsp_register(instruction.r.rt);
    s32 result = value >> (get_rsp_register(instruction.r.rs) & 0b11111);
    set_rsp_register(instruction.r.rd, result);
}

RSP_INSTR(rsp_spc_srlv) {
    u32 value = get_rsp_register(instruction.r.rt);
    s32 result = value >> (get_rsp_register(instruction.r.rs) & 0b11111);
    set_rsp_register(instruction.r.rd, result);
}

RSP_INSTR(rsp_spc_sllv) {
        u32 value = get_rsp_register(instruction.r.rt);
        s32 result = value << (get_rsp_register(instruction.r.rs) & 0b11111);
        set_rsp_register(instruction.r.rd, result);
}

RSP_INSTR(rsp_spc_sub) {
    s32 operand1 = get_rsp_register(instruction.r.rs);
    s32 operand2 = get_rsp_register(instruction.r.rt);

    s32 result = operand1 - operand2;
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
    s32 op1 = get_rsp_register(instruction.r.rs);
    s32 op2 = get_rsp_register(instruction.r.rt);

    // RS - RT
    s32 result = op1 - op2;
    // if RS is LESS than RT
    // aka, if result is negative

    if (result < 0) {
        set_rsp_register(instruction.r.rd, 1);
    } else {
        set_rsp_register(instruction.r.rd, 0);
    }
}

RSP_INSTR(rsp_spc_sltu) {
    u32 op1 = get_rsp_register(instruction.r.rs);
    u32 op2 = get_rsp_register(instruction.r.rt);

    if (op1 < op2) {
        set_rsp_register(instruction.r.rd, 1);
    } else {
        set_rsp_register(instruction.r.rd, 0);
    }
}

RSP_INSTR(rsp_spc_add) {
    u32 addend1 = get_rsp_register(instruction.r.rs);
    u32 addend2 = get_rsp_register(instruction.r.rt);

    u32 result = addend1 + addend2;

    set_rsp_register(instruction.r.rd, result);
}

RSP_INSTR(rsp_spc_and) {
    set_rsp_register(instruction.r.rd, get_rsp_register(instruction.r.rs) & get_rsp_register(instruction.r.rt));
}

void rsp_do_break() {
    N64RSP.status.halt = true;
    N64RSP.steps = 0;
    N64RSP.status.broke = true;

    if (N64RSP.status.intr_on_break) {
        interrupt_raise(INTERRUPT_SP);
    }
}

RSP_INSTR(rsp_spc_break) {
    rsp_do_break();
}

RSP_INSTR(rsp_lb) {
    s16 offset = instruction.i.immediate;
    u32 address = get_rsp_register(instruction.i.rs) + offset;

    s8 value = n64_rsp_read_byte(address);
    set_rsp_register(instruction.i.rt, (s32)value);
}

RSP_INSTR(rsp_lbu) {
    s16 offset = instruction.i.immediate;
    u32 address = get_rsp_register(instruction.i.rs) + offset;

    u8 value = n64_rsp_read_byte(address);
    set_rsp_register(instruction.i.rt, value);
}

RSP_INSTR(rsp_andi) {
        u32 immediate = instruction.i.immediate;
        u32 result = immediate & get_rsp_register(instruction.i.rs);
        set_rsp_register(instruction.i.rt, result);
}

RSP_INSTR(rsp_sb) {
    s16 offset = instruction.i.immediate;
    u32 address = get_rsp_register(instruction.i.rs) + offset;

    u8 value = get_rsp_register(instruction.i.rt);
    n64_rsp_write_byte(address, value);
}

RSP_INSTR(rsp_sh) {
    s16 offset = instruction.i.immediate;
    u32 address = get_rsp_register(instruction.i.rs) + offset;

    u16 value = get_rsp_register(instruction.i.rt);
    n64_rsp_write_half(address, value);
}

RSP_INSTR(rsp_sw) {
    s16 offset = instruction.i.immediate;
    u32 address = get_rsp_register(instruction.i.rs) + offset;

    u32 value = get_rsp_register(instruction.i.rt);
    n64_rsp_write_word(address, value);
}

RSP_INSTR(rsp_lhu) {
    s16 offset = instruction.i.immediate;
    u32 address = get_rsp_register(instruction.i.rs) + offset;

    u16 value = n64_rsp_read_half(address);
    set_rsp_register(instruction.i.rt, value);
}

RSP_INSTR(rsp_lh) {
    s16 offset = instruction.i.immediate;
    u32 address = get_rsp_register(instruction.i.rs) + offset;

    s16 value = n64_rsp_read_half(address);
    set_rsp_register(instruction.i.rt, (s32)value);
}

RSP_INSTR(rsp_lw) {
    s16 offset = instruction.i.immediate;
    u32 address = get_rsp_register(instruction.i.rs) + offset;

    s32 value = n64_rsp_read_word(address);
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
        s16 immediate = instruction.i.immediate;
        s32 reg = get_rsp_register(instruction.i.rs);
        if (reg < immediate) {
            set_rsp_register(instruction.i.rt, 1);
        } else {
            set_rsp_register(instruction.i.rt, 0);
        }
}

RSP_INSTR(rsp_sltiu) {
    s16 immediate = instruction.i.immediate;
    u32 reg = get_rsp_register(instruction.i.rs);
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
    s32 value = get_rsp_cp0_register(instruction.r.rd);
    set_rsp_register(instruction.r.rt, value);
}

RSP_INSTR(rsp_mtc0) {
    u32 value = get_rsp_register(instruction.r.rt);
    set_rsp_cp0_register(instruction.r.rd, value);
}

RSP_INSTR(rsp_bne) {
    rsp_conditional_branch(instruction.i.immediate, get_rsp_register(instruction.i.rs) != get_rsp_register(instruction.i.rt));
}

RSP_INSTR(rsp_beq) {
    rsp_conditional_branch(instruction.i.immediate, get_rsp_register(instruction.i.rs) == get_rsp_register(instruction.i.rt));
}

RSP_INSTR(rsp_bgtz) {
    s32 reg = get_rsp_register(instruction.i.rs);
    rsp_conditional_branch(instruction.i.immediate, reg > 0);
}

RSP_INSTR(rsp_blez) {
    s32 reg = get_rsp_register(instruction.i.rs);
    rsp_conditional_branch(instruction.i.immediate, reg <= 0);
}

RSP_INSTR(rsp_ri_bltz) {
    s32 reg = get_rsp_register(instruction.i.rs);
    rsp_conditional_branch(instruction.i.immediate, reg < 0);
}

RSP_INSTR(rsp_ri_bltzal) {
    s32 reg = get_rsp_register(instruction.i.rs);
    rsp_conditional_branch(instruction.i.immediate, reg < 0);
    rsp_link(RSP_REG_LR);
}

RSP_INSTR(rsp_ri_bgez) {
    s32 reg = get_rsp_register(instruction.i.rs);
    rsp_conditional_branch(instruction.i.immediate, reg >= 0);
}

RSP_INSTR(rsp_ri_bgezal) {
    s32 reg = get_rsp_register(instruction.i.rs);
    rsp_conditional_branch(instruction.i.immediate, reg >= 0);
    rsp_link(RSP_REG_LR);
}
