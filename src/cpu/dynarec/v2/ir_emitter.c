#include "ir_emitter.h"
#include "ir_context.h"

#include <util.h>
#include <dynarec/dynarec.h>
#include <disassemble.h>

#define IR_UNIMPLEMENTED(opc) logfatal("Unimplemented IR translation for instruction " #opc)

ir_instruction_t* get_memory_access_address(mips_instruction_t instruction, bus_access_t bus_access) {
    ir_instruction_t* base = ir_emit_load_guest_reg(instruction.i.rs);
    ir_instruction_t* i_offset = ir_emit_set_constant_s16(instruction.i.immediate, NO_GUEST_REG);
    ir_instruction_t* virtual = ir_emit_add(base, i_offset, NO_GUEST_REG);
    return ir_emit_tlb_lookup(virtual, NO_GUEST_REG, bus_access);
}

void ir_emit_conditional_branch(ir_instruction_t* condition, s16 offset, u64 virtual_address) {
    ir_instruction_t* pc_if_false = ir_emit_set_constant_64(virtual_address + 8, NO_GUEST_REG); // Account for instruction in delay slot
    ir_instruction_t* pc_if_true = ir_emit_set_constant_64(virtual_address + 4 + (s64)offset * 4, NO_GUEST_REG);
    ir_emit_conditional_set_block_exit_pc(condition, pc_if_true, pc_if_false);
}

void ir_emit_conditional_branch_likely(ir_instruction_t* condition, s16 offset, u64 virtual_address, int index) {
    // Identical - ir_emit_conditional_branch already skips the delay slot when calculating the exit PC.
    ir_emit_conditional_branch(condition, offset, virtual_address);
    // The only difference is likely branches conditionally exit the block early when not taken.
    ir_emit_conditional_block_exit(ir_emit_not(condition, NO_GUEST_REG), index);
}

void ir_emit_abs_branch(ir_instruction_t* address) {
    ir_emit_set_block_exit_pc(address);
}

void ir_emit_link(u8 guest_reg, u64 virtual_address) {
    u64 link_addr = virtual_address + 4; // Skip delay slot
    ir_emit_set_constant_64(link_addr, guest_reg);
}

IR_EMITTER(lui) {
    s64 ext = (s16)instruction.i.immediate;
    ext *= 65536;
    ir_emit_set_constant_64(ext, instruction.i.rt);
}

IR_EMITTER(ori) {
    ir_instruction_t* i_operand = ir_emit_load_guest_reg(instruction.i.rs);
    ir_instruction_t* i_operand2 = ir_emit_set_constant_u16(instruction.i.immediate, NO_GUEST_REG);

    ir_emit_or(i_operand, i_operand2, instruction.i.rt);
}

IR_EMITTER(andi) {
    ir_instruction_t* i_operand = ir_emit_load_guest_reg(instruction.i.rs);
    ir_instruction_t* i_operand2 = ir_emit_set_constant_u16(instruction.i.immediate, NO_GUEST_REG);

    ir_emit_and(i_operand, i_operand2, instruction.i.rt);
}

IR_EMITTER(sll) {
    ir_instruction_t* operand = ir_emit_load_guest_reg(instruction.r.rt);
    ir_instruction_t* shift_amount = ir_emit_set_constant_u16(instruction.r.sa, NO_GUEST_REG);
    ir_instruction_t* shift_result = ir_emit_shift(operand, shift_amount, VALUE_TYPE_S32, SHIFT_DIRECTION_LEFT, NO_GUEST_REG);
    ir_emit_mask_and_cast(shift_result, VALUE_TYPE_S32, instruction.r.rd);
}

IR_EMITTER(srl) {
    ir_instruction_t* operand = ir_emit_load_guest_reg(instruction.r.rt);
    ir_instruction_t* shift_amount = ir_emit_set_constant_u16(instruction.r.sa, NO_GUEST_REG);
    ir_instruction_t* shift_result = ir_emit_shift(operand, shift_amount, VALUE_TYPE_U32, SHIFT_DIRECTION_RIGHT, NO_GUEST_REG);
    ir_emit_mask_and_cast(shift_result, VALUE_TYPE_S32, instruction.r.rd);
}

IR_EMITTER(sllv) {
    ir_instruction_t* value = ir_emit_load_guest_reg(instruction.r.rt);
    ir_instruction_t* shift_amount = ir_emit_load_guest_reg(instruction.r.rs);
    shift_amount = ir_emit_and(shift_amount, ir_emit_set_constant_u16(0b11111, NO_GUEST_REG), NO_GUEST_REG);
    ir_instruction_t* result = ir_emit_shift(value, shift_amount, VALUE_TYPE_U32, SHIFT_DIRECTION_LEFT, NO_GUEST_REG);
    ir_emit_mask_and_cast(result, VALUE_TYPE_S32, instruction.r.rd);
}

IR_EMITTER(srlv) {
    ir_instruction_t* value = ir_emit_load_guest_reg(instruction.r.rt);
    ir_instruction_t* shift_amount = ir_emit_load_guest_reg(instruction.r.rs);
    shift_amount = ir_emit_and(shift_amount, ir_emit_set_constant_u16(0b11111, NO_GUEST_REG), NO_GUEST_REG);
    ir_instruction_t* result = ir_emit_shift(value, shift_amount, VALUE_TYPE_U32, SHIFT_DIRECTION_RIGHT, NO_GUEST_REG);
    ir_emit_mask_and_cast(result, VALUE_TYPE_S32, instruction.r.rd);
}

IR_EMITTER(sh) {
    ir_instruction_t* address = get_memory_access_address(instruction, BUS_STORE);
    ir_instruction_t* value = ir_emit_load_guest_reg(instruction.i.rt);
    ir_emit_store(VALUE_TYPE_U16, address, value);
}

IR_EMITTER(sw) {
    ir_instruction_t* address = get_memory_access_address(instruction, BUS_STORE);
    ir_instruction_t* value = ir_emit_load_guest_reg(instruction.i.rt);
    ir_emit_store(VALUE_TYPE_U32, address, value);
}

IR_EMITTER(sd) {
    ir_instruction_t* address = get_memory_access_address(instruction, BUS_STORE);
    ir_instruction_t* value = ir_emit_load_guest_reg(instruction.i.rt);
    ir_emit_store(VALUE_TYPE_U64, address, value);
}

IR_EMITTER(lw) {
    ir_emit_load(VALUE_TYPE_S32, get_memory_access_address(instruction, BUS_LOAD), instruction.i.rt);
}

IR_EMITTER(lbu) {
    ir_emit_load(VALUE_TYPE_U8, get_memory_access_address(instruction, BUS_LOAD), instruction.i.rt);
}

IR_EMITTER(lb) {
    ir_emit_load(VALUE_TYPE_S8, get_memory_access_address(instruction, BUS_LOAD), instruction.i.rt);
}

IR_EMITTER(lhu) {
    ir_emit_load(VALUE_TYPE_U16, get_memory_access_address(instruction, BUS_LOAD), instruction.i.rt);
}

IR_EMITTER(lh) {
    ir_emit_load(VALUE_TYPE_S16, get_memory_access_address(instruction, BUS_LOAD), instruction.i.rt);
}

IR_EMITTER(ld) {
    ir_emit_load(VALUE_TYPE_U64, get_memory_access_address(instruction, BUS_LOAD), instruction.i.rt);
}

IR_EMITTER(bne) {
    ir_instruction_t* rs = ir_emit_load_guest_reg(instruction.i.rs);
    ir_instruction_t* rt = ir_emit_load_guest_reg(instruction.i.rt);
    ir_instruction_t* cond = ir_emit_check_condition(CONDITION_NOT_EQUAL, rs, rt, NO_GUEST_REG);
    ir_emit_conditional_branch(cond, instruction.i.immediate, virtual_address);
}

IR_EMITTER(bnel) {
    ir_instruction_t* rs = ir_emit_load_guest_reg(instruction.i.rs);
    ir_instruction_t* rt = ir_emit_load_guest_reg(instruction.i.rt);
    ir_instruction_t* cond = ir_emit_check_condition(CONDITION_NOT_EQUAL, rs, rt, NO_GUEST_REG);
    ir_emit_conditional_branch_likely(cond, instruction.i.immediate, virtual_address, index);
}

IR_EMITTER(beq) {
    ir_instruction_t* rs = ir_emit_load_guest_reg(instruction.i.rs);
    ir_instruction_t* rt = ir_emit_load_guest_reg(instruction.i.rt);
    ir_instruction_t* cond = ir_emit_check_condition(CONDITION_EQUAL, rs, rt, NO_GUEST_REG);
    ir_emit_conditional_branch(cond, instruction.i.immediate, virtual_address);
}

IR_EMITTER(bgtz) {
    ir_instruction_t* rs = ir_emit_load_guest_reg(instruction.i.rs);
    ir_instruction_t* zero = ir_emit_load_guest_reg(0);
    ir_instruction_t* cond = ir_emit_check_condition(CONDITION_GREATER_THAN_SIGNED, rs, zero, NO_GUEST_REG);
    ir_emit_conditional_branch(cond, instruction.i.immediate, virtual_address);
}

IR_EMITTER(bgezal) {
    ir_instruction_t* rs = ir_emit_load_guest_reg(instruction.i.rs);
    ir_instruction_t* zero = ir_emit_load_guest_reg(0);
    ir_instruction_t* cond = ir_emit_check_condition(CONDITION_GREATER_OR_EQUAL_TO_SIGNED, rs, zero, NO_GUEST_REG);

    ir_emit_conditional_branch(cond, instruction.i.immediate, virtual_address);
    ir_emit_link(MIPS_REG_RA, virtual_address);
}

IR_EMITTER(j) {
    u64 target = instruction.j.target;
    target <<= 2;
    target |= (virtual_address & 0xFFFFFFFFF0000000);

    ir_instruction_t* address = ir_emit_set_constant_64(target, NO_GUEST_REG);
    ir_emit_abs_branch(address);
}

IR_EMITTER(jal) {
    emit_j_ir(instruction, index, virtual_address, physical_address);
    ir_emit_link(MIPS_REG_RA, virtual_address);
}

IR_EMITTER(jr) {
    ir_emit_abs_branch(ir_emit_load_guest_reg(instruction.i.rs));
}

IR_EMITTER(jalr) {
    ir_emit_abs_branch(ir_emit_load_guest_reg(instruction.i.rs));
    ir_emit_link(instruction.r.rd, virtual_address);
}

IR_EMITTER(multu) {
    ir_instruction_t* multiplicand1 = ir_emit_load_guest_reg(instruction.r.rs);
    ir_instruction_t* multiplicand2 = ir_emit_load_guest_reg(instruction.r.rt);
    ir_emit_multiply(multiplicand1, multiplicand2, VALUE_TYPE_U32);
}

IR_EMITTER(mflo) {
    ir_emit_get_mult_result(MULT_RESULT_LO, instruction.r.rd);
}

IR_EMITTER(mfhi) {
    ir_emit_get_mult_result(MULT_RESULT_HI, instruction.r.rd);
}

IR_EMITTER(add) {
    ir_instruction_t* addend1 = ir_emit_load_guest_reg(instruction.r.rs);
    ir_instruction_t* addend2 = ir_emit_load_guest_reg(instruction.r.rt);
    // TODO: check for signed overflow
    ir_instruction_t* result = ir_emit_add(addend1, addend2, NO_GUEST_REG);
    ir_emit_mask_and_cast(result, VALUE_TYPE_S32, instruction.r.rd);
}

IR_EMITTER(addu) {
    ir_instruction_t* addend1 = ir_emit_load_guest_reg(instruction.r.rs);
    ir_instruction_t* addend2 = ir_emit_load_guest_reg(instruction.r.rt);
    ir_instruction_t* result = ir_emit_add(addend1, addend2, NO_GUEST_REG);
    ir_emit_mask_and_cast(result, VALUE_TYPE_S32, instruction.r.rd);
}

IR_EMITTER(and) {
    ir_instruction_t* operand1 = ir_emit_load_guest_reg(instruction.r.rs);
    ir_instruction_t* operand2 = ir_emit_load_guest_reg(instruction.r.rt);
    ir_emit_and(operand1, operand2, instruction.r.rd);
}

IR_EMITTER(subu) {
    ir_instruction_t* minuend    = ir_emit_mask_and_cast(ir_emit_load_guest_reg(instruction.r.rs), VALUE_TYPE_U32, NO_GUEST_REG);
    ir_instruction_t* subtrahend = ir_emit_mask_and_cast(ir_emit_load_guest_reg(instruction.r.rt), VALUE_TYPE_U32, NO_GUEST_REG);
    ir_emit_sub(minuend, subtrahend, VALUE_TYPE_U32, instruction.r.rd);
}

IR_EMITTER(or) {
    ir_instruction_t* operand1 = ir_emit_load_guest_reg(instruction.r.rs);
    ir_instruction_t* operand2 = ir_emit_load_guest_reg(instruction.r.rt);
    ir_emit_or(operand1, operand2, instruction.r.rd);
}

IR_EMITTER(xor) {
    ir_instruction_t* operand1 = ir_emit_load_guest_reg(instruction.r.rs);
    ir_instruction_t* operand2 = ir_emit_load_guest_reg(instruction.r.rt);
    ir_emit_xor(operand1, operand2, instruction.r.rd);
}

IR_EMITTER(addiu) {
    ir_instruction_t* addend1 = ir_emit_load_guest_reg(instruction.i.rs);
    ir_instruction_t* addend2 = ir_emit_set_constant_s16(instruction.i.immediate, NO_GUEST_REG);
    ir_instruction_t* result = ir_emit_add(addend1, addend2, NO_GUEST_REG);
    ir_emit_mask_and_cast(result, VALUE_TYPE_S32, instruction.i.rt);
}

IR_EMITTER(addi) {
    ir_instruction_t* addend1 = ir_emit_load_guest_reg(instruction.i.rs);
    ir_instruction_t* addend2 = ir_emit_set_constant_s16(instruction.i.immediate, NO_GUEST_REG);
    // TODO: check for signed overflow
    ir_instruction_t* result = ir_emit_add(addend1, addend2, NO_GUEST_REG);
    ir_emit_mask_and_cast(result, VALUE_TYPE_S32, instruction.i.rt);
}

IR_EMITTER(slt) {
    ir_instruction_t* op1 = ir_emit_load_guest_reg(instruction.r.rs);
    ir_instruction_t* op2 = ir_emit_load_guest_reg(instruction.r.rt);
    ir_emit_check_condition(CONDITION_LESS_THAN_SIGNED, op1, op2, instruction.r.rd);
}

IR_EMITTER(sltu) {
    ir_instruction_t* op1 = ir_emit_load_guest_reg(instruction.r.rs);
    ir_instruction_t* op2 = ir_emit_load_guest_reg(instruction.r.rt);
    ir_emit_check_condition(CONDITION_LESS_THAN_UNSIGNED, op1, op2, instruction.r.rd);
}

IR_EMITTER(mtc0) {
    ir_instruction_t* value = ir_emit_load_guest_reg(instruction.r.rt);
    switch (instruction.r.rd) {
        // Passthrough
        case R4300I_CP0_REG_TAGLO:
        case R4300I_CP0_REG_TAGHI:
            ir_emit_set_cp0(instruction.r.rd, value);
            break;

        // Other
        case R4300I_CP0_REG_INDEX: logfatal("emit MTC0 R4300I_CP0_REG_INDEX");
        case R4300I_CP0_REG_RANDOM: logfatal("emit MTC0 R4300I_CP0_REG_RANDOM");
        case R4300I_CP0_REG_COUNT: {
            ir_instruction_t* shift_amount = ir_emit_set_constant_u16(1, NO_GUEST_REG);
            ir_instruction_t* value_shifted = ir_emit_shift(value, shift_amount, VALUE_TYPE_U64, SHIFT_DIRECTION_LEFT, NO_GUEST_REG);
            ir_emit_set_cp0(R4300I_CP0_REG_COUNT, value_shifted);
            break;
        }
        case R4300I_CP0_REG_CAUSE: {
            ir_instruction_t* cause_mask = ir_emit_set_constant_u16(0x300, NO_GUEST_REG);
            ir_instruction_t* cause_masked = ir_emit_and(value, cause_mask, NO_GUEST_REG);

            ir_instruction_t* inverse_cause_mask = ir_emit_not(cause_mask, NO_GUEST_REG);
            ir_instruction_t* old_cause = ir_emit_get_cp0(R4300I_CP0_REG_CAUSE, NO_GUEST_REG);
            ir_instruction_t* old_cause_masked = ir_emit_and(old_cause, inverse_cause_mask, NO_GUEST_REG);

            ir_instruction_t* new_cause = ir_emit_or(old_cause_masked, cause_masked, NO_GUEST_REG);
            ir_emit_set_cp0(R4300I_CP0_REG_CAUSE, new_cause);
            break;
        }
        case R4300I_CP0_REG_COMPARE: {
            // Lower compare interrupt
            ir_emit_set_cp0(R4300I_CP0_REG_CAUSE,
                            ir_emit_and(
                                    ir_emit_get_cp0(R4300I_CP0_REG_CAUSE, NO_GUEST_REG),
                                    ir_emit_set_constant_s32(~(1 << 15), NO_GUEST_REG),
                                    NO_GUEST_REG));
            ir_emit_set_cp0(R4300I_CP0_REG_COMPARE, value);
            break;
        }
        case R4300I_CP0_REG_STATUS: {
            ir_instruction_t* status_mask = ir_emit_set_constant_u32(CP0_STATUS_WRITE_MASK, NO_GUEST_REG);
            ir_instruction_t* inverse_status_mask = ir_emit_not(status_mask, NO_GUEST_REG);
            ir_instruction_t* old_status = ir_emit_get_cp0(R4300I_CP0_REG_STATUS, NO_GUEST_REG);
            ir_instruction_t* old_status_masked = ir_emit_and(old_status, inverse_status_mask, NO_GUEST_REG);
            ir_instruction_t* value_masked = ir_emit_and(value, status_mask, NO_GUEST_REG);
            ir_instruction_t* new_status = ir_emit_or(value_masked, old_status_masked, NO_GUEST_REG);
            ir_emit_set_cp0(R4300I_CP0_REG_STATUS, new_status);
            // TODO: handle all the side effects of this write
            break;
        }
        case R4300I_CP0_REG_ENTRYLO0: logfatal("emit MTC0 R4300I_CP0_REG_ENTRYLO0");
        case R4300I_CP0_REG_ENTRYLO1: logfatal("emit MTC0 R4300I_CP0_REG_ENTRYLO1");
        case R4300I_CP0_REG_ENTRYHI: logfatal("emit MTC0 R4300I_CP0_REG_ENTRYHI");
        case R4300I_CP0_REG_PAGEMASK: logfatal("emit MTC0 R4300I_CP0_REG_PAGEMASK");
        case R4300I_CP0_REG_EPC: logfatal("emit MTC0 R4300I_CP0_REG_EPC");
        case R4300I_CP0_REG_CONFIG: logfatal("emit MTC0 R4300I_CP0_REG_CONFIG");
        case R4300I_CP0_REG_WATCHLO: logfatal("emit MTC0 R4300I_CP0_REG_WATCHLO");
        case R4300I_CP0_REG_WATCHHI: logfatal("emit MTC0 R4300I_CP0_REG_WATCHHI");
        case R4300I_CP0_REG_WIRED: logfatal("emit MTC0 R4300I_CP0_REG_WIRED");
        case R4300I_CP0_REG_CONTEXT: logfatal("emit MTC0 R4300I_CP0_REG_CONTEXT");
        case R4300I_CP0_REG_XCONTEXT: logfatal("emit MTC0 R4300I_CP0_REG_XCONTEXT");
        case R4300I_CP0_REG_LLADDR: logfatal("emit MTC0 R4300I_CP0_REG_LLADDR");
        case R4300I_CP0_REG_ERR_EPC: logfatal("emit MTC0 R4300I_CP0_REG_ERR_EPC");
        case R4300I_CP0_REG_PRID: logfatal("emit MTC0 R4300I_CP0_REG_PRID");
        case R4300I_CP0_REG_PARITYER: logfatal("emit MTC0 R4300I_CP0_REG_PARITYER");
        case R4300I_CP0_REG_CACHEER: logfatal("emit MTC0 R4300I_CP0_REG_CACHEER");
        case R4300I_CP0_REG_7: logfatal("emit MTC0 R4300I_CP0_REG_7");
        case R4300I_CP0_REG_21: logfatal("emit MTC0 R4300I_CP0_REG_21");
        case R4300I_CP0_REG_22: logfatal("emit MTC0 R4300I_CP0_REG_22");
        case R4300I_CP0_REG_23: logfatal("emit MTC0 R4300I_CP0_REG_23");
        case R4300I_CP0_REG_24: logfatal("emit MTC0 R4300I_CP0_REG_24");
        case R4300I_CP0_REG_25: logfatal("emit MTC0 R4300I_CP0_REG_25");
        case R4300I_CP0_REG_31: logfatal("emit MTC0 R4300I_CP0_REG_31");
            break;
    }
}

IR_EMITTER(cp0_instruction) {
    if (instruction.last11 == 0) {
        switch (instruction.r.rs) {
            case COP_MF: IR_UNIMPLEMENTED(COP_MF);
            case COP_DMF: IR_UNIMPLEMENTED(COP_DMF);
            // Last 11 bits are 0
            case COP_MT: CALL_IR_EMITTER(mtc0);
            case COP_DMT: IR_UNIMPLEMENTED(COP_DMT);
            default: {
                char buf[50];
                disassemble(0, instruction.raw, buf, 50);
                logfatal("other/unknown MIPS CP0 0x%08X with rs: %d%d%d%d%d [%s]", instruction.raw,
                         instruction.rs0, instruction.rs1, instruction.rs2, instruction.rs3, instruction.rs4, buf);
            }
        }
    } else {
        switch (instruction.fr.funct) {
            case COP_FUNCT_TLBWI_MULT: IR_UNIMPLEMENTED(COP_FUNCT_TLBWI_MULT);
            case COP_FUNCT_TLBWR_MOV: IR_UNIMPLEMENTED(COP_FUNCT_TLBWR_MOV);
            case COP_FUNCT_TLBP: IR_UNIMPLEMENTED(COP_FUNCT_TLBP);
            case COP_FUNCT_TLBR_SUB: IR_UNIMPLEMENTED(COP_FUNCT_TLBR_SUB);
            case COP_FUNCT_ERET: IR_UNIMPLEMENTED(COP_FUNCT_ERET);
            case COP_FUNCT_WAIT: IR_UNIMPLEMENTED(COP_FUNCT_WAIT);
            default: {
                char buf[50];
                disassemble(0, instruction.raw, buf, 50);
                logfatal("other/unknown MIPS CP0 0x%08X with FUNCT: %d%d%d%d%d%d [%s]", instruction.raw,
                         instruction.funct0, instruction.funct1, instruction.funct2, instruction.funct3, instruction.funct4, instruction.funct5, buf);
            }
        }
    }
}

IR_EMITTER(special_instruction) {
    switch (instruction.r.funct) {
        case FUNCT_SLL: CALL_IR_EMITTER(sll);
        case FUNCT_SRL: CALL_IR_EMITTER(srl);
        case FUNCT_SRA: IR_UNIMPLEMENTED(FUNCT_SRA);
        case FUNCT_SRAV: IR_UNIMPLEMENTED(FUNCT_SRAV);
        case FUNCT_SLLV: CALL_IR_EMITTER(sllv);
        case FUNCT_SRLV: CALL_IR_EMITTER(srlv);
        case FUNCT_JR: CALL_IR_EMITTER(jr);
        case FUNCT_JALR: CALL_IR_EMITTER(jalr);
        case FUNCT_SYSCALL: IR_UNIMPLEMENTED(FUNCT_SYSCALL);
        case FUNCT_MFHI: IR_UNIMPLEMENTED(FUNCT_MFHI);
        case FUNCT_MTHI: IR_UNIMPLEMENTED(FUNCT_MTHI);
        case FUNCT_MFLO: CALL_IR_EMITTER(mflo);
        case FUNCT_MTLO: IR_UNIMPLEMENTED(FUNCT_MTLO);
        case FUNCT_DSLLV: IR_UNIMPLEMENTED(FUNCT_DSLLV);
        case FUNCT_DSRLV: IR_UNIMPLEMENTED(FUNCT_DSRLV);
        case FUNCT_DSRAV: IR_UNIMPLEMENTED(FUNCT_DSRAV);
        case FUNCT_MULT: IR_UNIMPLEMENTED(FUNCT_MULT);
        case FUNCT_MULTU: CALL_IR_EMITTER(multu);
        case FUNCT_DIV: IR_UNIMPLEMENTED(FUNCT_DIV);
        case FUNCT_DIVU: IR_UNIMPLEMENTED(FUNCT_DIVU);
        case FUNCT_DMULT: IR_UNIMPLEMENTED(FUNCT_DMULT);
        case FUNCT_DMULTU: IR_UNIMPLEMENTED(FUNCT_DMULTU);
        case FUNCT_DDIV: IR_UNIMPLEMENTED(FUNCT_DDIV);
        case FUNCT_DDIVU: IR_UNIMPLEMENTED(FUNCT_DDIVU);
        case FUNCT_ADD: CALL_IR_EMITTER(add);
        case FUNCT_ADDU: CALL_IR_EMITTER(addu);
        case FUNCT_AND: CALL_IR_EMITTER(and);
        case FUNCT_NOR: IR_UNIMPLEMENTED(FUNCT_NOR);
        case FUNCT_SUB: IR_UNIMPLEMENTED(FUNCT_SUB);
        case FUNCT_SUBU: CALL_IR_EMITTER(subu);
        case FUNCT_OR: CALL_IR_EMITTER(or);
        case FUNCT_XOR: CALL_IR_EMITTER(xor);
        case FUNCT_SLT: CALL_IR_EMITTER(slt);
        case FUNCT_SLTU: CALL_IR_EMITTER(sltu);
        case FUNCT_DADD: IR_UNIMPLEMENTED(FUNCT_DADD);
        case FUNCT_DADDU: IR_UNIMPLEMENTED(FUNCT_DADDU);
        case FUNCT_DSUB: IR_UNIMPLEMENTED(FUNCT_DSUB);
        case FUNCT_DSUBU: IR_UNIMPLEMENTED(FUNCT_DSUBU);
        case FUNCT_TEQ: IR_UNIMPLEMENTED(FUNCT_TEQ);
        case FUNCT_DSLL: IR_UNIMPLEMENTED(FUNCT_DSLL);
        case FUNCT_DSRL: IR_UNIMPLEMENTED(FUNCT_DSRL);
        case FUNCT_DSRA: IR_UNIMPLEMENTED(FUNCT_DSRA);
        case FUNCT_DSLL32: IR_UNIMPLEMENTED(FUNCT_DSLL32);
        case FUNCT_DSRL32: IR_UNIMPLEMENTED(FUNCT_DSRL32);
        case FUNCT_DSRA32: IR_UNIMPLEMENTED(FUNCT_DSRA32);
        case FUNCT_BREAK: IR_UNIMPLEMENTED(FUNCT_BREAK);
        case FUNCT_SYNC: IR_UNIMPLEMENTED(FUNCT_SYNC);
        case FUNCT_TGE: IR_UNIMPLEMENTED(FUNCT_TGE);
        case FUNCT_TGEU: IR_UNIMPLEMENTED(FUNCT_TGEU);
        case FUNCT_TLT: IR_UNIMPLEMENTED(FUNCT_TLT);
        case FUNCT_TLTU: IR_UNIMPLEMENTED(FUNCT_TLTU);
        case FUNCT_TNE: IR_UNIMPLEMENTED(FUNCT_TNE);
        default: {
            char buf[50];
            disassemble(0, instruction.raw, buf, 50);
            logfatal("other/unknown MIPS Special 0x%08X with FUNCT: %d%d%d%d%d%d [%s]", instruction.raw,
                     instruction.funct0, instruction.funct1, instruction.funct2, instruction.funct3, instruction.funct4, instruction.funct5, buf);
        }
    }
}


IR_EMITTER(regimm_instruction) {
    switch (instruction.i.rt) {
        case RT_BLTZ: IR_UNIMPLEMENTED(RT_BLTZ);
        case RT_BLTZL: IR_UNIMPLEMENTED(RT_BLTZL);
        case RT_BLTZAL: IR_UNIMPLEMENTED(RT_BLTZAL);
        case RT_BGEZ: IR_UNIMPLEMENTED(RT_BGEZ);
        case RT_BGEZL: IR_UNIMPLEMENTED(RT_BGEZL);
        case RT_BGEZAL: CALL_IR_EMITTER(bgezal);
        case RT_BGEZALL: IR_UNIMPLEMENTED(RT_BGEZALL);
        case RT_TGEI: IR_UNIMPLEMENTED(RT_TGEI);
        case RT_TGEIU: IR_UNIMPLEMENTED(RT_TGEIU);
        case RT_TLTI: IR_UNIMPLEMENTED(RT_TLTI);
        case RT_TLTIU: IR_UNIMPLEMENTED(RT_TLTIU);
        case RT_TEQI: IR_UNIMPLEMENTED(RT_TEQI);
        case RT_TNEI: IR_UNIMPLEMENTED(RT_TNEI);
        default: {
            char buf[50];
            disassemble(0, instruction.raw, buf, 50);
            logfatal("other/unknown MIPS REGIMM 0x%08X with RT: %d%d%d%d%d [%s]", instruction.raw,
                     instruction.rt0, instruction.rt1, instruction.rt2, instruction.rt3, instruction.rt4, buf);
        }
    }
}

IR_EMITTER(cp1_instruction) {
    // This function uses a series of two switch statements.
    // If the instruction doesn't use the RS field for the opcode, then control will fall through to the next
    // switch, and check the FUNCT. It may be worth profiling and seeing if it's faster to check FUNCT first at some point
    switch (instruction.r.rs) {
        case COP_CF: IR_UNIMPLEMENTED(COP_CF);
        case COP_MF: IR_UNIMPLEMENTED(COP_MF);
        case COP_DMF: IR_UNIMPLEMENTED(COP_DMF);
        case COP_MT: IR_UNIMPLEMENTED(COP_MT);
        case COP_DMT: IR_UNIMPLEMENTED(COP_DMT);
        case COP_CT: IR_UNIMPLEMENTED(COP_CT);
        case COP_BC:
            switch (instruction.r.rt) {
                case COP_BC_BCT: IR_UNIMPLEMENTED(COP_BC_BCT);
                case COP_BC_BCF: IR_UNIMPLEMENTED(COP_BC_BCF);
                case COP_BC_BCTL: IR_UNIMPLEMENTED(COP_BC_BCTL);
                case COP_BC_BCFL: IR_UNIMPLEMENTED(COP_BC_BCFL);
                default: {
                    char buf[50];
                    disassemble(0, instruction.raw, buf, 50);
                    logfatal("other/unknown MIPS BC 0x%08X [%s]", instruction.raw, buf);
                }
            }
    }
    IR_UNIMPLEMENTED(SomeFPUInstruction);
}

IR_EMITTER(instruction) {
    if (unlikely(instruction.raw == 0)) {
        return; // do nothing for NOP
    }
    switch (instruction.op) {
        case OPC_CP0:    CALL_IR_EMITTER(cp0_instruction);
        case OPC_CP1:    CALL_IR_EMITTER(cp1_instruction);
        case OPC_SPCL:   CALL_IR_EMITTER(special_instruction);
        case OPC_REGIMM: CALL_IR_EMITTER(regimm_instruction);

        case OPC_LD: CALL_IR_EMITTER(ld);
        case OPC_LUI: CALL_IR_EMITTER(lui);
        case OPC_ADDIU: CALL_IR_EMITTER(addiu);
        case OPC_ADDI: CALL_IR_EMITTER(addi);
        case OPC_DADDI: IR_UNIMPLEMENTED(OPC_DADDI);
        case OPC_ANDI: CALL_IR_EMITTER(andi);
        case OPC_LBU: CALL_IR_EMITTER(lbu);
        case OPC_LHU: CALL_IR_EMITTER(lhu);
        case OPC_LH: CALL_IR_EMITTER(lh);
        case OPC_LW: CALL_IR_EMITTER(lw);
        case OPC_LWU: IR_UNIMPLEMENTED(OPC_LWU);
        case OPC_BEQ: CALL_IR_EMITTER(beq);
        case OPC_BEQL: IR_UNIMPLEMENTED(OPC_BEQL);
        case OPC_BGTZ: CALL_IR_EMITTER(bgtz);
        case OPC_BGTZL: IR_UNIMPLEMENTED(OPC_BGTZL);
        case OPC_BLEZ: IR_UNIMPLEMENTED(OPC_BLEZ);
        case OPC_BLEZL: IR_UNIMPLEMENTED(OPC_BLEZL);
        case OPC_BNE: CALL_IR_EMITTER(bne);
        case OPC_BNEL: CALL_IR_EMITTER(bnel);
        case OPC_CACHE: return; // treat CACHE as a NOP for now
        case OPC_SB: IR_UNIMPLEMENTED(OPC_SB);
        case OPC_SH: CALL_IR_EMITTER(sh);
        case OPC_SW: CALL_IR_EMITTER(sw);
        case OPC_SD: CALL_IR_EMITTER(sd);
        case OPC_ORI: CALL_IR_EMITTER(ori);
        case OPC_J: CALL_IR_EMITTER(j);
        case OPC_JAL: CALL_IR_EMITTER(jal);
        case OPC_SLTI: IR_UNIMPLEMENTED(OPC_SLTI);
        case OPC_SLTIU: IR_UNIMPLEMENTED(OPC_SLTIU);
        case OPC_XORI: IR_UNIMPLEMENTED(OPC_XORI);
        case OPC_DADDIU: IR_UNIMPLEMENTED(OPC_DADDIU);
        case OPC_LB: CALL_IR_EMITTER(lb);
        case OPC_LDC1: IR_UNIMPLEMENTED(OPC_LDC1);
        case OPC_SDC1: IR_UNIMPLEMENTED(OPC_SDC1);
        case OPC_LWC1: IR_UNIMPLEMENTED(OPC_LWC1);
        case OPC_SWC1: IR_UNIMPLEMENTED(OPC_SWC1);
        case OPC_LWL: IR_UNIMPLEMENTED(OPC_LWL);
        case OPC_LWR: IR_UNIMPLEMENTED(OPC_LWR);
        case OPC_SWL: IR_UNIMPLEMENTED(OPC_SWL);
        case OPC_SWR: IR_UNIMPLEMENTED(OPC_SWR);
        case OPC_LDL: IR_UNIMPLEMENTED(OPC_LDL);
        case OPC_LDR: IR_UNIMPLEMENTED(OPC_LDR);
        case OPC_SDL: IR_UNIMPLEMENTED(OPC_SDL);
        case OPC_SDR: IR_UNIMPLEMENTED(OPC_SDR);
        case OPC_LL: IR_UNIMPLEMENTED(OPC_LL);
        case OPC_LLD: IR_UNIMPLEMENTED(OPC_LLD);
        case OPC_SC: IR_UNIMPLEMENTED(OPC_SC);
        case OPC_SCD: IR_UNIMPLEMENTED(OPC_SCD);
        case OPC_RDHWR: IR_UNIMPLEMENTED(OPC_RDHWR); // Invalid instruction used by Linux
        default: {
            char buf[50];
            disassemble(0, instruction.raw, buf, 50);
            logfatal("Failed to decode instruction 0x%08X opcode %d%d%d%d%d%d [%s]",
                     instruction.raw, instruction.op0, instruction.op1, instruction.op2, instruction.op3, instruction.op4, instruction.op5, buf);
        }
    }
}
