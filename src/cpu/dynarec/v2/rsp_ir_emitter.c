#include "rsp_ir_emitter.h"
#include <disassemble.h>
#include <dynarec/v2/ir_context.h>
#include <dynarec/v2/ir_emitter.h>
#include <r4300i.h>
#include <rsp.h>
#include <rsp_instructions.h>
#include <rsp_vector_instructions.h>

ir_instruction_t* ir_get_rsp_memory_access_address(mips_instruction_t instruction) {
    ir_instruction_t* base = ir_emit_load_guest_gpr(instruction.i.rs);
    ir_instruction_t* i_offset = ir_emit_set_constant_s16(instruction.i.immediate, NO_GUEST_REG);
    return ir_emit_add(base, i_offset, NO_GUEST_REG);
}

void ir_emit_rsp_link(u8 guest_reg, u16 address) {
    u64 link_addr = address + 8; // Skip delay slot
    ir_emit_set_constant_64(link_addr, guest_reg);
}

IR_RSP_EMITTER(lui) {
    u32 constant = (u32)instruction.i.immediate << 16;
    ir_emit_set_constant_u32(constant, instruction.i.rt);
}

IR_RSP_EMITTER(addi) {
    ir_instruction_t* addend1 = ir_emit_load_guest_gpr(instruction.i.rs);
    ir_instruction_t* addend2 = ir_emit_set_constant_s16(instruction.i.immediate, NO_GUEST_REG);
    ir_emit_add(addend1, addend2, instruction.i.rt);
}

IR_RSP_EMITTER(andi) {
    ir_instruction_t* operand1 = ir_emit_load_guest_gpr(instruction.i.rs);
    ir_instruction_t* operand2 = ir_emit_set_constant_u16(instruction.i.immediate, NO_GUEST_REG);
    ir_emit_and(operand1, operand2, instruction.i.rt);
}

IR_RSP_EMITTER(lbu) {
    ir_emit_load(VALUE_TYPE_U8, ir_get_rsp_memory_access_address(instruction), instruction.i.rt);
}

IR_RSP_EMITTER(lhu) {
    ir_emit_load(VALUE_TYPE_U16, ir_get_rsp_memory_access_address(instruction), instruction.i.rt);
}

IR_RSP_EMITTER(lh) {
    ir_emit_load(VALUE_TYPE_S16, ir_get_rsp_memory_access_address(instruction), instruction.i.rt);
}

IR_RSP_EMITTER(lw) {
    ir_emit_load(VALUE_TYPE_S32, ir_get_rsp_memory_access_address(instruction), instruction.i.rt);
}

IR_RSP_EMITTER(lwu) {
    logfatal("Unimplemented IR emitter: lwu");
}

IR_RSP_EMITTER(beq) {
    ir_instruction_t* rs = ir_emit_load_guest_gpr(instruction.i.rs);
    ir_instruction_t* rt = ir_emit_load_guest_gpr(instruction.i.rt);
    ir_instruction_t* cond = ir_emit_check_condition(CONDITION_EQUAL, rs, rt, NO_GUEST_REG);
    ir_emit_conditional_branch(cond, instruction.i.immediate, address);
}

IR_RSP_EMITTER(bgtz) {
    ir_instruction_t* rs = ir_emit_load_guest_gpr(instruction.i.rs);
    ir_instruction_t* cond = ir_emit_check_condition(CONDITION_GREATER_THAN_SIGNED, rs, ir_emit_set_constant_u16(0, NO_GUEST_REG), NO_GUEST_REG);
    ir_emit_conditional_branch(cond, instruction.i.immediate, address);
}
IR_RSP_EMITTER(blez) {
    ir_instruction_t* rs = ir_emit_load_guest_gpr(instruction.i.rs);
    ir_instruction_t* cond = ir_emit_check_condition(CONDITION_LESS_OR_EQUAL_TO_SIGNED, rs, ir_emit_set_constant_u16(0, NO_GUEST_REG), NO_GUEST_REG);
    ir_emit_conditional_branch(cond, instruction.i.immediate, address);
}

IR_RSP_EMITTER(bne) {
    ir_instruction_t* rs = ir_emit_load_guest_gpr(instruction.i.rs);
    ir_instruction_t* rt = ir_emit_load_guest_gpr(instruction.i.rt);
    ir_instruction_t* cond = ir_emit_check_condition(CONDITION_NOT_EQUAL, rs, rt, NO_GUEST_REG);
    ir_emit_conditional_branch(cond, instruction.i.immediate, address);
}

IR_RSP_EMITTER(sb) {
    ir_instruction_t* value = ir_emit_load_guest_gpr(instruction.i.rt);
    ir_emit_store(VALUE_TYPE_U8, ir_get_rsp_memory_access_address(instruction), value);
}

IR_RSP_EMITTER(sh) {
    ir_instruction_t* value = ir_emit_load_guest_gpr(instruction.i.rt);
    ir_emit_store(VALUE_TYPE_U16, ir_get_rsp_memory_access_address(instruction), value);
}

IR_RSP_EMITTER(sw) {
    ir_instruction_t* value = ir_emit_load_guest_gpr(instruction.i.rt);
    ir_emit_store(VALUE_TYPE_U32, ir_get_rsp_memory_access_address(instruction), value);
}

IR_RSP_EMITTER(ori) {
    ir_instruction_t* i_operand = ir_emit_load_guest_gpr(instruction.i.rs);
    ir_instruction_t* i_operand2 = ir_emit_set_constant_u16(instruction.i.immediate, NO_GUEST_REG);
    ir_emit_or(i_operand, i_operand2, instruction.i.rt);
}

IR_RSP_EMITTER(j) {
    u16 target = (instruction.j.target << 2) & 0xFFF;
    ir_emit_set_block_exit_pc(ir_emit_set_constant_u16(target, NO_GUEST_REG));
}

IR_RSP_EMITTER(jal) {
    CALL_IR_RSP_EMITTER_NOBREAK(j);
    ir_emit_rsp_link(MIPS_REG_RA, address);
}

IR_RSP_EMITTER(slti) {
    logfatal("Unimplemented IR emitter: slti");
}

IR_RSP_EMITTER(sltiu) {
    logfatal("Unimplemented IR emitter: sltiu");
}

IR_RSP_EMITTER(xori) {
    logfatal("Unimplemented IR emitter: xori");
}

IR_RSP_EMITTER(lb) {
    logfatal("Unimplemented IR emitter: lb");
}

IR_RSP_EMITTER(mtc0) {
    ir_instruction_t* rt = ir_emit_load_guest_gpr(instruction.r.rt);
    ir_emit_call_2((uintptr_t)&set_rsp_cp0_register, ir_emit_set_constant_u16(instruction.r.rd, NO_GUEST_REG), rt);
}

ir_instruction_t* ir_emit_get_ptr_bit(void* ptr, int bit, u8 guest_reg) {
    ir_instruction_t* rsp_status = ir_emit_get_ptr(VALUE_TYPE_U32, ptr, NO_GUEST_REG);
    ir_instruction_t* shifted = ir_emit_shift(rsp_status, ir_emit_set_constant_u16(bit, NO_GUEST_REG), VALUE_TYPE_U32, SHIFT_DIRECTION_RIGHT, NO_GUEST_REG);
    return ir_emit_and(shifted, ir_emit_set_constant_u16(1, NO_GUEST_REG), guest_reg);
}

IR_RSP_EMITTER(mfc0) {
    switch (instruction.r.rd) {
        case RSP_CP0_DMA_CACHE:
            ir_emit_get_ptr(VALUE_TYPE_S32, &N64RSP.io.mem_addr.raw, instruction.r.rt);
            break;
        case RSP_CP0_DMA_DRAM:
            ir_emit_get_ptr(VALUE_TYPE_S32, &N64RSP.io.dram_addr.raw, instruction.r.rt);
            break;
        case RSP_CP0_DMA_READ_LENGTH:
        case RSP_CP0_DMA_WRITE_LENGTH:
            ir_emit_get_ptr(VALUE_TYPE_S32, &N64RSP.io.dma.raw, instruction.r.rt);
        case RSP_CP0_SP_STATUS:
            ir_emit_get_ptr(VALUE_TYPE_S32, &N64RSP.status.raw, instruction.r.rt);
            break;
        case RSP_CP0_DMA_FULL:
            ir_emit_get_ptr_bit(&N64RSP.status.raw, 3, instruction.r.rt);
            break;
        case RSP_CP0_DMA_BUSY:
            ir_emit_get_ptr_bit(&N64RSP.status.raw, 2, instruction.r.rt);
            break;
        case RSP_CP0_DMA_RESERVED:
            ir_emit_call_0((uintptr_t)&rsp_acquire_semaphore, instruction.r.rt);
            break;
        case RSP_CP0_CMD_START:
            logfatal("RSP JIT unimplemented MFC0 RSP_CP0_CMD_START");
        case RSP_CP0_CMD_END:
            ir_emit_get_ptr(VALUE_TYPE_S32, &n64sys.dpc.end, instruction.r.rt);
            break;
        case RSP_CP0_CMD_CURRENT:
            ir_emit_get_ptr(VALUE_TYPE_S32, &n64sys.dpc.current, instruction.r.rt);
            break;
        case RSP_CP0_CMD_STATUS:
            ir_emit_get_ptr(VALUE_TYPE_S32, &n64sys.dpc.status.raw, instruction.r.rt);
        case RSP_CP0_CMD_CLOCK:
            logwarn("Read from RDP clock: returning 0.");
            ir_emit_set_constant_u16(0, instruction.r.rt);
            break;
        case RSP_CP0_CMD_BUSY:
            logfatal("Read from unknown RSP CP0 register RSP_CP0_CMD_BUSY");
        case RSP_CP0_CMD_PIPE_BUSY:
            logfatal("Read from unknown RSP CP0 register RSP_CP0_CMD_PIPE_BUSY");
        case RSP_CP0_CMD_TMEM_BUSY:
            logfatal("Read from unknown RSP CP0 register RSP_CP0_CMD_TMEM_BUSY");
        default:
            logfatal("Unsupported RSP CP0 $c%d read", instruction.r.rd);
    }
}

IR_RSP_EMITTER(cp0) {
    if (instruction.is_coprocessor_funct) {
        switch (instruction.fr.funct) {
            default: {
                char buf[50];
                disassemble(address, instruction.raw, buf, 50);
                logfatal("other/unknown MIPS RSP CP0 0x%08X with FUNCT: %d%d%d%d%d%d [%s]", instruction.raw,
                         instruction.funct0, instruction.funct1, instruction.funct2, instruction.funct3, instruction.funct4, instruction.funct5, buf);
            }
        }
    } else {
        switch (instruction.r.rs) {
            case COP_MT: CALL_IR_RSP_EMITTER(mtc0);
            case COP_MF: CALL_IR_RSP_EMITTER(mfc0);
            default: {
                char buf[50];
                disassemble(address, instruction.raw, buf, 50);
                logfatal("other/unknown MIPS RSP CP0 0x%08X with rs: %d%d%d%d%d [%s]", instruction.raw,
                         instruction.rs0, instruction.rs1, instruction.rs2, instruction.rs3, instruction.rs4, buf);
            }
        }
    }
}

IR_RSP_EMITTER(sll) {
    ir_instruction_t* operand = ir_emit_load_guest_gpr(instruction.r.rt);
    ir_instruction_t* shift_amount = ir_emit_set_constant_u16(instruction.r.sa, NO_GUEST_REG);
    ir_instruction_t* shift_result = ir_emit_shift(operand, shift_amount, VALUE_TYPE_S32, SHIFT_DIRECTION_LEFT, NO_GUEST_REG);
    ir_emit_mask_and_cast(shift_result, VALUE_TYPE_S32, instruction.r.rd);
}

IR_RSP_EMITTER(srl) {
    ir_instruction_t* operand = ir_emit_load_guest_gpr(instruction.r.rt);
    ir_instruction_t* shift_amount = ir_emit_set_constant_u16(instruction.r.sa, NO_GUEST_REG);
    ir_instruction_t* shift_result = ir_emit_shift(operand, shift_amount, VALUE_TYPE_U32, SHIFT_DIRECTION_RIGHT, NO_GUEST_REG);
    ir_emit_mask_and_cast(shift_result, VALUE_TYPE_S32, instruction.r.rd);
}

IR_RSP_EMITTER(sra) {
    logfatal("Unimplemented RSP emitter: sra");
}

IR_RSP_EMITTER(srav) {
    logfatal("Unimplemented RSP emitter: srav");
}

IR_RSP_EMITTER(sllv) {
    logfatal("Unimplemented RSP emitter: sllv");
}

IR_RSP_EMITTER(srlv) {
    logfatal("Unimplemented RSP emitter: srlv");
}

IR_RSP_EMITTER(jr) {
    ir_emit_set_block_exit_pc(ir_emit_load_guest_gpr(instruction.i.rs));
}

IR_RSP_EMITTER(jalr) {
    logfatal("Unimplemented RSP emitter: jalr");
}

IR_RSP_EMITTER(add) {
    ir_instruction_t* operand1 = ir_emit_load_guest_gpr(instruction.r.rs);
    ir_instruction_t* operand2 = ir_emit_load_guest_gpr(instruction.r.rt);
    ir_emit_add(operand1, operand2, instruction.r.rd);
}

IR_RSP_EMITTER(and) {
    ir_instruction_t* operand1 = ir_emit_load_guest_gpr(instruction.r.rs);
    ir_instruction_t* operand2 = ir_emit_load_guest_gpr(instruction.r.rt);
    ir_emit_and(operand1, operand2, instruction.r.rd);
}

IR_RSP_EMITTER(sub) {
    logfatal("Unimplemented RSP emitter: sub");
}

IR_RSP_EMITTER(or) {
    logfatal("Unimplemented RSP emitter: or");
}

IR_RSP_EMITTER(xor) {
    logfatal("Unimplemented RSP emitter: xor");
}

IR_RSP_EMITTER(nor) {
    logfatal("Unimplemented RSP emitter: nor");
}

IR_RSP_EMITTER(slt) {
    logfatal("Unimplemented RSP emitter: slt");
}

IR_RSP_EMITTER(sltu) {
    logfatal("Unimplemented RSP emitter: sltu");
}

IR_RSP_EMITTER(break) {
    ir_emit_call_0((uintptr_t)&rsp_do_break, NO_GUEST_REG);
}

IR_RSP_EMITTER(spcl) {
    switch (instruction.r.funct) {
        case FUNCT_SLL:    CALL_IR_RSP_EMITTER(sll);
        case FUNCT_SRL:    CALL_IR_RSP_EMITTER(srl);
        case FUNCT_SRA:    CALL_IR_RSP_EMITTER(sra);
        case FUNCT_SRAV:   CALL_IR_RSP_EMITTER(srav);
        case FUNCT_SLLV:   CALL_IR_RSP_EMITTER(sllv);
        case FUNCT_SRLV:   CALL_IR_RSP_EMITTER(srlv);
        case FUNCT_JR:     CALL_IR_RSP_EMITTER(jr);
        case FUNCT_JALR:   CALL_IR_RSP_EMITTER(jalr);
        //case FUNCT_MULT:   return rsp_spc_mult;
        //case FUNCT_MULTU:  return rsp_spc_multu;
        //case FUNCT_DIV:    return rsp_spc_div;
        //case FUNCT_DIVU:   return rsp_spc_divu;
        case FUNCT_ADD:    CALL_IR_RSP_EMITTER(add);
        case FUNCT_ADDU:   CALL_IR_RSP_EMITTER(add);
        case FUNCT_AND:    CALL_IR_RSP_EMITTER(and);
        case FUNCT_SUB:    CALL_IR_RSP_EMITTER(sub);
        case FUNCT_SUBU:   CALL_IR_RSP_EMITTER(sub);
        case FUNCT_OR:     CALL_IR_RSP_EMITTER(or);
        case FUNCT_XOR:    CALL_IR_RSP_EMITTER(xor);
        case FUNCT_NOR:    CALL_IR_RSP_EMITTER(nor);
        case FUNCT_SLT:    CALL_IR_RSP_EMITTER(slt);
        case FUNCT_SLTU:   CALL_IR_RSP_EMITTER(sltu);

        case FUNCT_BREAK: CALL_IR_RSP_EMITTER(break);
        default: {
            char buf[50];
            disassemble(address, instruction.raw, buf, 50);
            logfatal("other/unknown MIPS RSP Special 0x%08X with FUNCT: %d%d%d%d%d%d [%s]", instruction.raw,
                     instruction.funct0, instruction.funct1, instruction.funct2, instruction.funct3, instruction.funct4, instruction.funct5, buf);
        }
    }
}

IR_RSP_EMITTER(regimm) {
    logfatal("RSP regimm emitter unimplemented");
}

ir_instruction_t* get_loadstore_addr(mips_instruction_t instruction, int shift_amount) {
    ir_instruction_t* base = ir_emit_load_guest_gpr(instruction.v.base);
    ir_instruction_t* offset = ir_emit_set_constant_s32(sign_extend_7bit_offset(instruction.v.offset, shift_amount), NO_GUEST_REG);
    return ir_emit_add(base, offset, NO_GUEST_REG);
}

IR_RSP_EMITTER(lwc2_lbv) {
    ir_instruction_t* addr = get_loadstore_addr(instruction, SHIFT_AMOUNT_LBV_SBV);
    ir_instruction_t* old_value = ir_emit_load_guest_vpr(IR_VPR(instruction.v.vt));
    ir_emit_rsp_lwc2(addr, old_value, IR_RSP_LWC2_LBV, instruction.v.element, IR_VPR(instruction.v.vt));
}

IR_RSP_EMITTER(lwc2_ldv) {
    ir_instruction_t* addr = get_loadstore_addr(instruction, SHIFT_AMOUNT_LDV_SDV);
    ir_instruction_t* old_value = ir_emit_load_guest_vpr(IR_VPR(instruction.v.vt));
    ir_emit_rsp_lwc2(addr, old_value, IR_RSP_LWC2_LDV, instruction.v.element, IR_VPR(instruction.v.vt));
}

IR_RSP_EMITTER(lwc2_lfv) {
    ir_instruction_t* addr = get_loadstore_addr(instruction, SHIFT_AMOUNT_LFV_SFV);
    ir_instruction_t* old_value = ir_emit_load_guest_vpr(IR_VPR(instruction.v.vt));
    ir_emit_rsp_lwc2(addr, old_value, IR_RSP_LWC2_LFV, instruction.v.element, IR_VPR(instruction.v.vt));
}

IR_RSP_EMITTER(lwc2_lhv) {
    ir_instruction_t* addr = get_loadstore_addr(instruction, SHIFT_AMOUNT_LHV_SHV);
    ir_instruction_t* old_value = ir_emit_load_guest_vpr(IR_VPR(instruction.v.vt));
    ir_emit_rsp_lwc2(addr, old_value, IR_RSP_LWC2_LHV, instruction.v.element, IR_VPR(instruction.v.vt));
}

IR_RSP_EMITTER(lwc2_llv) {
    ir_instruction_t* addr = get_loadstore_addr(instruction, SHIFT_AMOUNT_LLV_SLV);
    ir_instruction_t* old_value = ir_emit_load_guest_vpr(IR_VPR(instruction.v.vt));
    ir_emit_rsp_lwc2(addr, old_value, IR_RSP_LWC2_LLV, instruction.v.element, IR_VPR(instruction.v.vt));
}

IR_RSP_EMITTER(lwc2_lpv) {
    ir_instruction_t* addr = get_loadstore_addr(instruction, SHIFT_AMOUNT_LPV_SPV);
    ir_instruction_t* old_value = ir_emit_load_guest_vpr(IR_VPR(instruction.v.vt));
    ir_emit_rsp_lwc2(addr, old_value, IR_RSP_LWC2_LPV, instruction.v.element, IR_VPR(instruction.v.vt));
}

IR_RSP_EMITTER(lwc2_lqv) {
    ir_instruction_t* addr = get_loadstore_addr(instruction, SHIFT_AMOUNT_LQV_SQV);
    ir_instruction_t* old_value = ir_emit_load_guest_vpr(IR_VPR(instruction.v.vt));
    ir_emit_rsp_lwc2(addr, old_value, IR_RSP_LWC2_LQV, instruction.v.element, IR_VPR(instruction.v.vt));
}

IR_RSP_EMITTER(lwc2_lrv) {
    ir_instruction_t* addr = get_loadstore_addr(instruction, SHIFT_AMOUNT_LRV_SRV);
    ir_instruction_t* old_value = ir_emit_load_guest_vpr(IR_VPR(instruction.v.vt));
    ir_emit_rsp_lwc2(addr, old_value, IR_RSP_LWC2_LRV, instruction.v.element, IR_VPR(instruction.v.vt));
}

IR_RSP_EMITTER(lwc2_lsv) {
    ir_instruction_t* addr = get_loadstore_addr(instruction, SHIFT_AMOUNT_LSV_SSV);
    ir_instruction_t* old_value = ir_emit_load_guest_vpr(IR_VPR(instruction.v.vt));
    ir_emit_rsp_lwc2(addr, old_value, IR_RSP_LWC2_LSV, instruction.v.element, IR_VPR(instruction.v.vt));
}

IR_RSP_EMITTER(lwc2_ltv) {
    ir_instruction_t* addr = get_loadstore_addr(instruction, SHIFT_AMOUNT_LTV_STV);
    ir_instruction_t* old_value = ir_emit_load_guest_vpr(IR_VPR(instruction.v.vt));
    ir_emit_rsp_lwc2(addr, old_value, IR_RSP_LWC2_LTV, instruction.v.element, IR_VPR(instruction.v.vt));
}

IR_RSP_EMITTER(lwc2_luv) {
    ir_instruction_t* addr = get_loadstore_addr(instruction, SHIFT_AMOUNT_LUV_SUV);
    ir_instruction_t* old_value = ir_emit_load_guest_vpr(IR_VPR(instruction.v.vt));
    ir_emit_rsp_lwc2(addr, old_value, IR_RSP_LWC2_LUV, instruction.v.element, IR_VPR(instruction.v.vt));
}

IR_RSP_EMITTER(lwc2) {
    switch (instruction.v.funct) {
        case LWC2_LBV: CALL_IR_RSP_EMITTER(lwc2_lbv);
        case LWC2_LDV: CALL_IR_RSP_EMITTER(lwc2_ldv);
        case LWC2_LFV: CALL_IR_RSP_EMITTER(lwc2_lfv);
        case LWC2_LHV: CALL_IR_RSP_EMITTER(lwc2_lhv);
        case LWC2_LLV: CALL_IR_RSP_EMITTER(lwc2_llv);
        case LWC2_LPV: CALL_IR_RSP_EMITTER(lwc2_lpv);
        case LWC2_LQV: CALL_IR_RSP_EMITTER(lwc2_lqv);
        case LWC2_LRV: CALL_IR_RSP_EMITTER(lwc2_lrv);
        case LWC2_LSV: CALL_IR_RSP_EMITTER(lwc2_lsv);
        case LWC2_LTV: CALL_IR_RSP_EMITTER(lwc2_ltv);
        case LWC2_LUV: CALL_IR_RSP_EMITTER(lwc2_luv);
        case SWC2_SWV: return; // Does not exist on the RSP
        default:
            logfatal("other/unknown MIPS RSP LWC2 with funct: 0x%02X", instruction.v.funct);
    }
}

IR_RSP_EMITTER(swc2_sbv) {
    ir_instruction_t* addr = get_loadstore_addr(instruction, SHIFT_AMOUNT_LBV_SBV);
    ir_instruction_t* value = ir_emit_load_guest_vpr(IR_VPR(instruction.v.vt));
    ir_emit_rsp_swc2(addr, value, IR_RSP_SWC2_SBV, instruction.v.element);
}

IR_RSP_EMITTER(swc2_sdv) {
    ir_instruction_t* addr = get_loadstore_addr(instruction, SHIFT_AMOUNT_LDV_SDV);
    ir_instruction_t* value = ir_emit_load_guest_vpr(IR_VPR(instruction.v.vt));
    ir_emit_rsp_swc2(addr, value, IR_RSP_SWC2_SDV, instruction.v.element);
}

IR_RSP_EMITTER(swc2_sfv) {
    ir_instruction_t* addr = get_loadstore_addr(instruction, SHIFT_AMOUNT_LFV_SFV);
    ir_instruction_t* value = ir_emit_load_guest_vpr(IR_VPR(instruction.v.vt));
    ir_emit_rsp_swc2(addr, value, IR_RSP_SWC2_SFV, instruction.v.element);
}

IR_RSP_EMITTER(swc2_shv) {
    ir_instruction_t* addr = get_loadstore_addr(instruction, SHIFT_AMOUNT_LHV_SHV);
    ir_instruction_t* value = ir_emit_load_guest_vpr(IR_VPR(instruction.v.vt));
    ir_emit_rsp_swc2(addr, value, IR_RSP_SWC2_SHV, instruction.v.element);
}

IR_RSP_EMITTER(swc2_slv) {
    ir_instruction_t* addr = get_loadstore_addr(instruction, SHIFT_AMOUNT_LLV_SLV);
    ir_instruction_t* value = ir_emit_load_guest_vpr(IR_VPR(instruction.v.vt));
    ir_emit_rsp_swc2(addr, value, IR_RSP_SWC2_SLV, instruction.v.element);
}

IR_RSP_EMITTER(swc2_spv) {
    ir_instruction_t* addr = get_loadstore_addr(instruction, SHIFT_AMOUNT_LPV_SPV);
    ir_instruction_t* value = ir_emit_load_guest_vpr(IR_VPR(instruction.v.vt));
    ir_emit_rsp_swc2(addr, value, IR_RSP_SWC2_SPV, instruction.v.element);
}

IR_RSP_EMITTER(swc2_sqv) {
    ir_instruction_t* addr = get_loadstore_addr(instruction, SHIFT_AMOUNT_LQV_SQV);
    ir_instruction_t* value = ir_emit_load_guest_vpr(IR_VPR(instruction.v.vt));
    ir_emit_rsp_swc2(addr, value, IR_RSP_SWC2_SQV, instruction.v.element);
}

IR_RSP_EMITTER(swc2_srv) {
    ir_instruction_t* addr = get_loadstore_addr(instruction, SHIFT_AMOUNT_LRV_SRV);
    ir_instruction_t* value = ir_emit_load_guest_vpr(IR_VPR(instruction.v.vt));
    ir_emit_rsp_swc2(addr, value, IR_RSP_SWC2_SRV, instruction.v.element);
}

IR_RSP_EMITTER(swc2_ssv) {
    ir_instruction_t* addr = get_loadstore_addr(instruction, SHIFT_AMOUNT_LSV_SSV);
    ir_instruction_t* value = ir_emit_load_guest_vpr(IR_VPR(instruction.v.vt));
    ir_emit_rsp_swc2(addr, value, IR_RSP_SWC2_SSV, instruction.v.element);
}

IR_RSP_EMITTER(swc2_stv) {
    ir_instruction_t* addr = get_loadstore_addr(instruction, SHIFT_AMOUNT_LTV_STV);
    ir_instruction_t* value = ir_emit_load_guest_vpr(IR_VPR(instruction.v.vt));
    ir_emit_rsp_swc2(addr, value, IR_RSP_SWC2_STV, instruction.v.element);
}

IR_RSP_EMITTER(swc2_suv) {
    ir_instruction_t* addr = get_loadstore_addr(instruction, SHIFT_AMOUNT_LUV_SUV);
    ir_instruction_t* value = ir_emit_load_guest_vpr(IR_VPR(instruction.v.vt));
    ir_emit_rsp_swc2(addr, value, IR_RSP_SWC2_SUV, instruction.v.element);
}

IR_RSP_EMITTER(swc2_swv) {
    ir_instruction_t* addr = get_loadstore_addr(instruction, SHIFT_AMOUNT_SWV);
    ir_instruction_t* value = ir_emit_load_guest_vpr(IR_VPR(instruction.v.vt));
    ir_emit_rsp_swc2(addr, value, IR_RSP_SWC2_SWV, instruction.v.element);
}


IR_RSP_EMITTER(swc2) {
    switch (instruction.v.funct) {
        case LWC2_LBV: CALL_IR_RSP_EMITTER(swc2_sbv);
        case LWC2_LDV: CALL_IR_RSP_EMITTER(swc2_sdv);
        case LWC2_LFV: CALL_IR_RSP_EMITTER(swc2_sfv);
        case LWC2_LHV: CALL_IR_RSP_EMITTER(swc2_shv);
        case LWC2_LLV: CALL_IR_RSP_EMITTER(swc2_slv);
        case LWC2_LPV: CALL_IR_RSP_EMITTER(swc2_spv);
        case LWC2_LQV: CALL_IR_RSP_EMITTER(swc2_sqv);
        case LWC2_LRV: CALL_IR_RSP_EMITTER(swc2_srv);
        case LWC2_LSV: CALL_IR_RSP_EMITTER(swc2_ssv);
        case LWC2_LTV: CALL_IR_RSP_EMITTER(swc2_stv);
        case LWC2_LUV: CALL_IR_RSP_EMITTER(swc2_suv);
        case SWC2_SWV: CALL_IR_RSP_EMITTER(swc2_swv);

        default:
            logfatal("other/unknown MIPS RSP SWC2 with funct: 0x%02X", instruction.v.funct);
    }
}

IR_RSP_EMITTER(vec_vabs) {
    logfatal("Unimplemented RSP IR emitter: vec_vabs");
}

IR_RSP_EMITTER(vec_vadd) {
    logfatal("Unimplemented RSP IR emitter: vec_vadd");
}

IR_RSP_EMITTER(vec_vaddc) {
    logfatal("Unimplemented RSP IR emitter: vec_vaddc");
}

IR_RSP_EMITTER(vec_vand) {
    logfatal("Unimplemented RSP IR emitter: vec_vand");
}

IR_RSP_EMITTER(vec_vch) {
    logfatal("Unimplemented RSP IR emitter: vec_vch");
}

IR_RSP_EMITTER(vec_vcl) {
    logfatal("Unimplemented RSP IR emitter: vec_vcl");
}

IR_RSP_EMITTER(vec_vcr) {
    logfatal("Unimplemented RSP IR emitter: vec_vcr");
}

IR_RSP_EMITTER(vec_veq) {
    logfatal("Unimplemented RSP IR emitter: vec_veq");
}

IR_RSP_EMITTER(vec_vge) {
    logfatal("Unimplemented RSP IR emitter: vec_vge");
}

IR_RSP_EMITTER(vec_vlt) {
    logfatal("Unimplemented RSP IR emitter: vec_vlt");
}

IR_RSP_EMITTER(vec_vmacf) {
    logfatal("Unimplemented RSP IR emitter: vec_vmacf");
}

IR_RSP_EMITTER(vec_vmacq) {
    logfatal("Unimplemented RSP IR emitter: vec_vmacq");
}

IR_RSP_EMITTER(vec_vmacu) {
    logfatal("Unimplemented RSP IR emitter: vec_vmacu");
}

IR_RSP_EMITTER(vec_vmadh) {
    logfatal("Unimplemented RSP IR emitter: vec_vmadh");
}

IR_RSP_EMITTER(vec_vmadl) {
    logfatal("Unimplemented RSP IR emitter: vec_vmadl");
}

IR_RSP_EMITTER(vec_vmadm) {
    logfatal("Unimplemented RSP IR emitter: vec_vmadm");
}

IR_RSP_EMITTER(vec_vmadn) {
    logfatal("Unimplemented RSP IR emitter: vec_vmadn");
}

IR_RSP_EMITTER(vec_vmov) {
    logfatal("Unimplemented RSP IR emitter: vec_vmov");
}

IR_RSP_EMITTER(vec_vmrg) {
    logfatal("Unimplemented RSP IR emitter: vec_vmrg");
}

IR_RSP_EMITTER(vec_vmudh) {
    logfatal("Unimplemented RSP IR emitter: vec_vmudh");
}

IR_RSP_EMITTER(vec_vmudl) {
    logfatal("Unimplemented RSP IR emitter: vec_vmudl");
}

IR_RSP_EMITTER(vec_vmudm) {
    logfatal("Unimplemented RSP IR emitter: vec_vmudm");
}

IR_RSP_EMITTER(vec_vmudn) {
    logfatal("Unimplemented RSP IR emitter: vec_vmudn");
}

IR_RSP_EMITTER(vec_vmulf) {
    logfatal("Unimplemented RSP IR emitter: vec_vmulf");
}

IR_RSP_EMITTER(vec_vmulq) {
    logfatal("Unimplemented RSP IR emitter: vec_vmulq");
}

IR_RSP_EMITTER(vec_vmulu) {
    logfatal("Unimplemented RSP IR emitter: vec_vmulu");
}

IR_RSP_EMITTER(vec_vnand) {
    logfatal("Unimplemented RSP IR emitter: vec_vnand");
}

IR_RSP_EMITTER(vec_vne) {
    logfatal("Unimplemented RSP IR emitter: vec_vne");
}

IR_RSP_EMITTER(vec_vnop) {
    logfatal("Unimplemented RSP IR emitter: vec_vnop");
}

IR_RSP_EMITTER(vec_vnor) {
    logfatal("Unimplemented RSP IR emitter: vec_vnor");
}

IR_RSP_EMITTER(vec_vnxor) {
    logfatal("Unimplemented RSP IR emitter: vec_vnxor");
}

IR_RSP_EMITTER(vec_vor) {
    logfatal("Unimplemented RSP IR emitter: vec_vor");
}

IR_RSP_EMITTER(vec_vrcp) {
    logfatal("Unimplemented RSP IR emitter: vec_vrcp");
}

IR_RSP_EMITTER(vec_vrcph_vrsqh) {
    logfatal("Unimplemented RSP IR emitter: vec_vrcph_vrsqh");
}

IR_RSP_EMITTER(vec_vrcpl) {
    logfatal("Unimplemented RSP IR emitter: vec_vrcpl");
}

IR_RSP_EMITTER(vec_vrndn) {
    logfatal("Unimplemented RSP IR emitter: vec_vrndn");
}

IR_RSP_EMITTER(vec_vrndp) {
    logfatal("Unimplemented RSP IR emitter: vec_vrndp");
}

IR_RSP_EMITTER(vec_vrsq) {
    logfatal("Unimplemented RSP IR emitter: vec_vrsq");
}

IR_RSP_EMITTER(vec_vrsql) {
    logfatal("Unimplemented RSP IR emitter: vec_vrsql");
}

IR_RSP_EMITTER(vec_vsar) {
    logfatal("Unimplemented RSP IR emitter: vec_vsar");
}

IR_RSP_EMITTER(vec_vsub) {
    logfatal("Unimplemented RSP IR emitter: vec_vsub");
}

IR_RSP_EMITTER(vec_vsubc) {
    logfatal("Unimplemented RSP IR emitter: vec_vsubc");
}

IR_RSP_EMITTER(vec_vxor) {
    logfatal("Unimplemented RSP IR emitter: vec_vxor");
}

IR_RSP_EMITTER(vec_vzero) {
    logfatal("Unimplemented RSP IR emitter: vec_vzero");
}

IR_RSP_EMITTER(cfc2) {
    logfatal("Unimplemented RSP IR emitter: cfc2");
}

IR_RSP_EMITTER(ctc2) {
    logfatal("Unimplemented RSP IR emitter: ctc2");
}

IR_RSP_EMITTER(mfc2) {
    logfatal("Unimplemented RSP IR emitter: mfc2");
}

IR_RSP_EMITTER(mtc2) {
    ir_instruction_t* value = ir_emit_load_guest_gpr(instruction.cp2_regmove.rt);
    ir_instruction_t* old_value = ir_emit_load_guest_vpr(IR_VPR(instruction.cp2_regmove.rd));
    ir_emit_ir_vpr_insert(old_value, value, VALUE_TYPE_U16, instruction.cp2_regmove.e, IR_VPR(instruction.cp2_regmove.rd));
}

IR_RSP_EMITTER(cp2) {
    if (instruction.cp2_vec.is_vec) {
        switch (instruction.cp2_vec.funct) {
            case FUNCT_RSP_VEC_VABS:  CALL_IR_RSP_EMITTER(vec_vabs);
            case FUNCT_RSP_VEC_VADD:  CALL_IR_RSP_EMITTER(vec_vadd);
            case FUNCT_RSP_VEC_VADDC: CALL_IR_RSP_EMITTER(vec_vaddc);
            case FUNCT_RSP_VEC_VAND:  CALL_IR_RSP_EMITTER(vec_vand);
            case FUNCT_RSP_VEC_VCH:   CALL_IR_RSP_EMITTER(vec_vch);
            case FUNCT_RSP_VEC_VCL:   CALL_IR_RSP_EMITTER(vec_vcl);
            case FUNCT_RSP_VEC_VCR:   CALL_IR_RSP_EMITTER(vec_vcr);
            case FUNCT_RSP_VEC_VEQ:   CALL_IR_RSP_EMITTER(vec_veq);
            case FUNCT_RSP_VEC_VGE:   CALL_IR_RSP_EMITTER(vec_vge);
            case FUNCT_RSP_VEC_VLT:   CALL_IR_RSP_EMITTER(vec_vlt);
            case FUNCT_RSP_VEC_VMACF: CALL_IR_RSP_EMITTER(vec_vmacf);
            case FUNCT_RSP_VEC_VMACQ: CALL_IR_RSP_EMITTER(vec_vmacq);
            case FUNCT_RSP_VEC_VMACU: CALL_IR_RSP_EMITTER(vec_vmacu);
            case FUNCT_RSP_VEC_VMADH: CALL_IR_RSP_EMITTER(vec_vmadh);
            case FUNCT_RSP_VEC_VMADL: CALL_IR_RSP_EMITTER(vec_vmadl);
            case FUNCT_RSP_VEC_VMADM: CALL_IR_RSP_EMITTER(vec_vmadm);
            case FUNCT_RSP_VEC_VMADN: CALL_IR_RSP_EMITTER(vec_vmadn);
            case FUNCT_RSP_VEC_VMOV:  CALL_IR_RSP_EMITTER(vec_vmov);
            case FUNCT_RSP_VEC_VMRG:  CALL_IR_RSP_EMITTER(vec_vmrg);
            case FUNCT_RSP_VEC_VMUDH: CALL_IR_RSP_EMITTER(vec_vmudh);
            case FUNCT_RSP_VEC_VMUDL: CALL_IR_RSP_EMITTER(vec_vmudl);
            case FUNCT_RSP_VEC_VMUDM: CALL_IR_RSP_EMITTER(vec_vmudm);
            case FUNCT_RSP_VEC_VMUDN: CALL_IR_RSP_EMITTER(vec_vmudn);
            case FUNCT_RSP_VEC_VMULF: CALL_IR_RSP_EMITTER(vec_vmulf);
            case FUNCT_RSP_VEC_VMULQ: CALL_IR_RSP_EMITTER(vec_vmulq);
            case FUNCT_RSP_VEC_VMULU: CALL_IR_RSP_EMITTER(vec_vmulu);
            case FUNCT_RSP_VEC_VNAND: CALL_IR_RSP_EMITTER(vec_vnand);
            case FUNCT_RSP_VEC_VNE:   CALL_IR_RSP_EMITTER(vec_vne);
            case FUNCT_RSP_VEC_VNOP:  CALL_IR_RSP_EMITTER(vec_vnop);
            case FUNCT_RSP_VEC_VNOR:  CALL_IR_RSP_EMITTER(vec_vnor);
            case FUNCT_RSP_VEC_VNXOR: CALL_IR_RSP_EMITTER(vec_vnxor);
            case FUNCT_RSP_VEC_VOR :  CALL_IR_RSP_EMITTER(vec_vor);
            case FUNCT_RSP_VEC_VRCP:  CALL_IR_RSP_EMITTER(vec_vrcp);
            case FUNCT_RSP_VEC_VRCPH: CALL_IR_RSP_EMITTER(vec_vrcph_vrsqh);
            case FUNCT_RSP_VEC_VRCPL: CALL_IR_RSP_EMITTER(vec_vrcpl);
            case FUNCT_RSP_VEC_VRNDN: CALL_IR_RSP_EMITTER(vec_vrndn);
            case FUNCT_RSP_VEC_VRNDP: CALL_IR_RSP_EMITTER(vec_vrndp);
            case FUNCT_RSP_VEC_VRSQ:  CALL_IR_RSP_EMITTER(vec_vrsq);
            case FUNCT_RSP_VEC_VRSQH: CALL_IR_RSP_EMITTER(vec_vrcph_vrsqh);
            case FUNCT_RSP_VEC_VRSQL: CALL_IR_RSP_EMITTER(vec_vrsql);
            case FUNCT_RSP_VEC_VSAR:  CALL_IR_RSP_EMITTER(vec_vsar);
            case FUNCT_RSP_VEC_VSUB:  CALL_IR_RSP_EMITTER(vec_vsub);
            case FUNCT_RSP_VEC_VSUBC: CALL_IR_RSP_EMITTER(vec_vsubc);
            case FUNCT_RSP_VEC_VXOR:  CALL_IR_RSP_EMITTER(vec_vxor);
            case FUNCT_RSP_VEC_VSUT:  CALL_IR_RSP_EMITTER(vec_vzero); // undocumented
            case FUNCT_RSP_VEC_VADDB: CALL_IR_RSP_EMITTER(vec_vzero); // undocumented
            case FUNCT_RSP_VEC_VSUBB: CALL_IR_RSP_EMITTER(vec_vzero); // undocumented
            case FUNCT_RSP_VEC_VACCB: CALL_IR_RSP_EMITTER(vec_vzero); // undocumented
            case FUNCT_RSP_VEC_VSUCB: CALL_IR_RSP_EMITTER(vec_vzero); // undocumented
            case FUNCT_RSP_VEC_VSAD:  CALL_IR_RSP_EMITTER(vec_vzero); // undocumented
            case FUNCT_RSP_VEC_VSAC:  CALL_IR_RSP_EMITTER(vec_vzero); // undocumented
            case FUNCT_RSP_VEC_VSUM:  CALL_IR_RSP_EMITTER(vec_vzero); // undocumented
            case FUNCT_RSP_VEC_0x1E:  CALL_IR_RSP_EMITTER(vec_vzero); // undocumented
            case FUNCT_RSP_VEC_0x1F:  CALL_IR_RSP_EMITTER(vec_vzero); // undocumented
            case FUNCT_RSP_VEC_0x2E:  CALL_IR_RSP_EMITTER(vec_vzero); // undocumented
            case FUNCT_RSP_VEC_0x2F:  CALL_IR_RSP_EMITTER(vec_vzero); // undocumented
            case FUNCT_RSP_VEC_VEXTT: CALL_IR_RSP_EMITTER(vec_vzero); // undocumented
            case FUNCT_RSP_VEC_VEXTQ: CALL_IR_RSP_EMITTER(vec_vzero); // undocumented
            case FUNCT_RSP_VEC_VEXTN: CALL_IR_RSP_EMITTER(vec_vzero); // undocumented
            case FUNCT_RSP_VEC_0x3B:  CALL_IR_RSP_EMITTER(vec_vzero); // undocumented
            case FUNCT_RSP_VEC_VINST: CALL_IR_RSP_EMITTER(vec_vzero); // undocumented
            case FUNCT_RSP_VEC_VINSQ: CALL_IR_RSP_EMITTER(vec_vzero); // undocumented
            case FUNCT_RSP_VEC_VINSN: CALL_IR_RSP_EMITTER(vec_vzero); // undocumented
            case FUNCT_RSP_VEC_VNULL: return; // undocumented (NOP)
            default: {
                char buf[50];
                disassemble(address, instruction.raw, buf, 50);
                logfatal("Invalid RSP CP2 VEC with FUNCT 0x%02X [0x%08X]=0x%08X | Capstone thinks it's %s", instruction.cp2_vec.funct, address, instruction.raw, buf);
            }
        }
    } else {
        switch (instruction.cp2_regmove.funct) {
            case COP_CF: CALL_IR_RSP_EMITTER(cfc2);
            case COP_CT: CALL_IR_RSP_EMITTER(ctc2);
            case COP_MF: CALL_IR_RSP_EMITTER(mfc2);
            case COP_MT: CALL_IR_RSP_EMITTER(mtc2);
            default: {
                char buf[50];
                disassemble(address, instruction.raw, buf, 50);
                logfatal("Invalid RSP CP2 regmove instruction! [0x%08x]=0x%08x | Capstone thinks it's %s", address, instruction.raw, buf);
            }
        }
    }
}

IR_RSP_EMITTER(instruction) {
    if (unlikely(instruction.raw == 0)) {
        return; // do nothing for NOP
    }

#ifdef LOG_ENABLED
        static char buf[50];
        if (n64_log_verbosity >= LOG_VERBOSITY_DEBUG) {
            disassemble(address, instruction.raw, buf, 50);
            logdebug("RSP [0x%08X]=0x%08X %s", address, instruction.raw, buf);
        }
#endif
        switch (instruction.op) {
            case OPC_LUI:   CALL_IR_RSP_EMITTER(lui);
            case OPC_ADDIU: CALL_IR_RSP_EMITTER(addi);
            case OPC_ADDI:  CALL_IR_RSP_EMITTER(addi);
            case OPC_ANDI:  CALL_IR_RSP_EMITTER(andi);
            case OPC_LBU:   CALL_IR_RSP_EMITTER(lbu);
            case OPC_LHU:   CALL_IR_RSP_EMITTER(lhu);
            case OPC_LH:    CALL_IR_RSP_EMITTER(lh);
            case OPC_LW:    CALL_IR_RSP_EMITTER(lw);
            case OPC_LWU:   CALL_IR_RSP_EMITTER(lwu);
            case OPC_BEQ:   CALL_IR_RSP_EMITTER(beq);
            //case OPC_BEQL:  return rsp_beql;
            case OPC_BGTZ:  CALL_IR_RSP_EMITTER(bgtz);
            case OPC_BLEZ:  CALL_IR_RSP_EMITTER(blez);
            case OPC_BNE:   CALL_IR_RSP_EMITTER(bne);
            //case OPC_BNEL:  return rsp_bnel;
            //case OPC_CACHE: return rsp_cache;
            case OPC_SB:    CALL_IR_RSP_EMITTER(sb);
            case OPC_SH:    CALL_IR_RSP_EMITTER(sh);
            case OPC_SW:    CALL_IR_RSP_EMITTER(sw);
            case OPC_ORI:   CALL_IR_RSP_EMITTER(ori);
            case OPC_J:     CALL_IR_RSP_EMITTER(j);
            case OPC_JAL:   CALL_IR_RSP_EMITTER(jal);
            case OPC_SLTI:  CALL_IR_RSP_EMITTER(slti);
            case OPC_SLTIU: CALL_IR_RSP_EMITTER(sltiu);
            case OPC_XORI:  CALL_IR_RSP_EMITTER(xori);
            case OPC_LB:    CALL_IR_RSP_EMITTER(lb);
            //case OPC_LWL:   return rsp_lwl;
            //case OPC_LWR:   return rsp_lwr;
            //case OPC_SWL:   return rsp_swl;
            //case OPC_SWR:   return rsp_swr;

            case OPC_CP0:      CALL_IR_RSP_EMITTER(cp0);
            case OPC_CP1:      logfatal("Tried to execute a COP1 instruction on the RSP!");
            case OPC_CP2:      CALL_IR_RSP_EMITTER(cp2);
            case OPC_SPCL:     CALL_IR_RSP_EMITTER(spcl);
            case OPC_REGIMM:   CALL_IR_RSP_EMITTER(regimm);
            case RSP_OPC_LWC2: CALL_IR_RSP_EMITTER(lwc2);
            case RSP_OPC_SWC2: CALL_IR_RSP_EMITTER(swc2);

            default:
#ifdef LOG_ENABLED
                if (n64_log_verbosity < LOG_VERBOSITY_DEBUG) {
                    disassemble(address, instruction.raw, buf, 50);
                }
                logfatal("[RSP] Failed to decode instruction 0x%08X opcode %d%d%d%d%d%d [%s]",
                         instruction.raw, instruction.op0, instruction.op1, instruction.op2, instruction.op3, instruction.op4, instruction.op5, buf);
#else
                logfatal("[RSP] Failed to decode instruction 0x%08X opcode %d%d%d%d%d%d [UNKNOWN]",
                         instruction.raw, instruction.op0, instruction.op1, instruction.op2, instruction.op3, instruction.op4, instruction.op5);
#endif
        }
}