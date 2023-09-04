#include "rsp_ir_emitter.h"
#include <disassemble.h>
#include <dynarec/v2/ir_context.h>
#include <dynarec/v2/ir_emitter.h>
#include <r4300i.h>
#include <rsp.h>

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
    logfatal("Unimplemented IR emitter: lbu");
}

IR_RSP_EMITTER(lhu) {
    logfatal("Unimplemented IR emitter: lhu");
}

IR_RSP_EMITTER(lh) {
    logfatal("Unimplemented IR emitter: lh");
}

IR_RSP_EMITTER(lw) {
    ir_emit_load(VALUE_TYPE_U32, ir_get_rsp_memory_access_address(instruction), instruction.i.rt);
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
    logfatal("Unimplemented IR emitter: bgtz");
}
IR_RSP_EMITTER(blez) {
    logfatal("Unimplemented IR emitter: blez");
}

IR_RSP_EMITTER(bne) {
    ir_instruction_t* rs = ir_emit_load_guest_gpr(instruction.i.rs);
    ir_instruction_t* rt = ir_emit_load_guest_gpr(instruction.i.rt);
    ir_instruction_t* cond = ir_emit_check_condition(CONDITION_NOT_EQUAL, rs, rt, NO_GUEST_REG);
    ir_emit_conditional_branch(cond, instruction.i.immediate, address);
}

IR_RSP_EMITTER(sb) {
    logfatal("Unimplemented IR emitter: sb");
}

IR_RSP_EMITTER(sh) {
    logfatal("Unimplemented IR emitter: sh");
}

IR_RSP_EMITTER(sw) {
    logfatal("Unimplemented IR emitter: sw");
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

ir_instruction_t* ir_emit_get_rsp_status_bit(int bit, u8 guest_reg) {
    ir_instruction_t* rsp_status = ir_emit_get_ptr(VALUE_TYPE_U32, &N64RSP.status.raw, NO_GUEST_REG);
    ir_instruction_t* shifted = ir_emit_shift(rsp_status, ir_emit_set_constant_u16(bit, NO_GUEST_REG), VALUE_TYPE_U32, SHIFT_DIRECTION_RIGHT, NO_GUEST_REG);
    return ir_emit_and(shifted, ir_emit_set_constant_u16(1, NO_GUEST_REG), guest_reg);
}

IR_RSP_EMITTER(mfc0) {
    switch (instruction.r.rd) {
        case RSP_CP0_DMA_CACHE:
            logfatal("RSP JIT unimplemented MFC0 RSP_CP0_DMA_CACHE");
        case RSP_CP0_DMA_DRAM:
            logfatal("RSP JIT unimplemented MFC0 RSP_CP0_DMA_DRAM");
        case RSP_CP0_DMA_READ_LENGTH:
            logfatal("RSP JIT unimplemented MFC0 RSP_CP0_DMA_READ_LENGTH");
        case RSP_CP0_DMA_WRITE_LENGTH:
            logfatal("RSP JIT unimplemented MFC0 RSP_CP0_DMA_WRITE_LENGTH");
        case RSP_CP0_SP_STATUS:
            ir_emit_get_ptr(VALUE_TYPE_U32, &N64RSP.status.raw, instruction.r.rt);
            break;
        case RSP_CP0_DMA_FULL:
            ir_emit_get_rsp_status_bit(3, instruction.r.rt);
            break;
        case RSP_CP0_DMA_BUSY:
            ir_emit_get_rsp_status_bit(2, instruction.r.rt);
            break;
        case RSP_CP0_DMA_RESERVED:
            logfatal("RSP JIT unimplemented MFC0 RSP_CP0_DMA_RESERVED");
        case RSP_CP0_CMD_START:
            logfatal("RSP JIT unimplemented MFC0 RSP_CP0_CMD_START");
        case RSP_CP0_CMD_END:
            logfatal("RSP JIT unimplemented MFC0 RSP_CP0_CMD_END");
        case RSP_CP0_CMD_CURRENT:
            logfatal("RSP JIT unimplemented MFC0 RSP_CP0_CMD_CURRENT");
        case RSP_CP0_CMD_STATUS:
            logfatal("RSP JIT unimplemented MFC0 RSP_CP0_CMD_STATUS");
        case RSP_CP0_CMD_CLOCK:
            logfatal("RSP JIT unimplemented MFC0 RSP_CP0_CMD_CLOCK");
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
    logfatal("Unimplemented RSP emitter: sll");
}

IR_RSP_EMITTER(srl) {
    logfatal("Unimplemented RSP emitter: srl");
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
    logfatal("Unimplemented RSP emitter: add");
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
    logfatal("Unimplemented RSP emitter: break");
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
            case OPC_CP1:      IR_RSP_UNIMPLEMENTED(OPC_CP1);     //return rsp_cp1_decode(pc, instr);
            case OPC_CP2:      IR_RSP_UNIMPLEMENTED(OPC_CP2); //return rsp_cp2_decode(pc, instruction);
            case OPC_SPCL:     CALL_IR_RSP_EMITTER(spcl); //return rsp_special_decode(pc, instruction);
            case OPC_REGIMM:   IR_RSP_UNIMPLEMENTED(OPC_REGIMM); //return rsp_regimm_decode(pc, instruction);
            case RSP_OPC_LWC2: IR_RSP_UNIMPLEMENTED(RSP_OPC_LWC2); //return rsp_lwc2_decode(pc, instruction);
            case RSP_OPC_SWC2: IR_RSP_UNIMPLEMENTED(RSP_OPC_SWC2); //return rsp_swc2_decode(pc, instruction);

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