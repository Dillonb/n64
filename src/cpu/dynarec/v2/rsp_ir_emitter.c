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

IR_RSP_EMITTER(ori) {
    ir_instruction_t* i_operand = ir_emit_load_guest_gpr(instruction.i.rs);
    ir_instruction_t* i_operand2 = ir_emit_set_constant_u16(instruction.i.immediate, NO_GUEST_REG);
    ir_emit_or(i_operand, i_operand2, instruction.i.rt);
}

IR_RSP_EMITTER(j) {
    u16 target = (instruction.j.target << 2) & 0xFFF;
    ir_emit_set_block_exit_pc(ir_emit_set_constant_u16(target, NO_GUEST_REG));
}

IR_RSP_EMITTER(addi) {
    ir_instruction_t* addend1 = ir_emit_load_guest_gpr(instruction.i.rs);
    ir_instruction_t* addend2 = ir_emit_set_constant_s16(instruction.i.immediate, NO_GUEST_REG);
    ir_emit_add(addend1, addend2, instruction.i.rt);
}

IR_RSP_EMITTER(lw) {
    ir_emit_load(VALUE_TYPE_U32, ir_get_rsp_memory_access_address(instruction), instruction.i.rt);
}

IR_RSP_EMITTER(andi) {
    ir_instruction_t* operand1 = ir_emit_load_guest_gpr(instruction.i.rs);
    ir_instruction_t* operand2 = ir_emit_set_constant_u16(instruction.i.immediate, NO_GUEST_REG);
    ir_emit_and(operand1, operand2, instruction.i.rt);
}

IR_RSP_EMITTER(beq) {
    ir_instruction_t* rs = ir_emit_load_guest_gpr(instruction.i.rs);
    ir_instruction_t* rt = ir_emit_load_guest_gpr(instruction.i.rt);
    ir_instruction_t* cond = ir_emit_check_condition(CONDITION_EQUAL, rs, rt, NO_GUEST_REG);
    ir_emit_conditional_branch(cond, instruction.i.immediate, address);
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
            case OPC_LUI:   IR_RSP_UNIMPLEMENTED(OPC_LUI);
            case OPC_ADDIU: IR_RSP_UNIMPLEMENTED(OPC_ADDIU);
            case OPC_ADDI:  CALL_IR_RSP_EMITTER(addi);
            case OPC_ANDI:  CALL_IR_RSP_EMITTER(andi);
            case OPC_LBU:   IR_RSP_UNIMPLEMENTED(OPC_LBU);
            case OPC_LHU:   IR_RSP_UNIMPLEMENTED(OPC_LHU);
            case OPC_LH:    IR_RSP_UNIMPLEMENTED(OPC_LH);
            case OPC_LW:    CALL_IR_RSP_EMITTER(lw);
            case OPC_LWU:   IR_RSP_UNIMPLEMENTED(OPC_LWU);
            case OPC_BEQ:   CALL_IR_RSP_EMITTER(beq);
            //case OPC_BEQL:  return rsp_beql;
            case OPC_BGTZ:  IR_RSP_UNIMPLEMENTED(OPC_BGTZ);
            case OPC_BLEZ:  IR_RSP_UNIMPLEMENTED(OPC_BLEZ);
            case OPC_BNE:   IR_RSP_UNIMPLEMENTED(OPC_BNE);
            //case OPC_BNEL:  return rsp_bnel;
            //case OPC_CACHE: return rsp_cache;
            case OPC_SB:    IR_RSP_UNIMPLEMENTED(OPC_SB);
            case OPC_SH:    IR_RSP_UNIMPLEMENTED(OPC_SH);
            case OPC_SW:    IR_RSP_UNIMPLEMENTED(OPC_SW);
            case OPC_ORI:   CALL_IR_RSP_EMITTER(ori);
            case OPC_J:     CALL_IR_RSP_EMITTER(j);
            case OPC_JAL:   IR_RSP_UNIMPLEMENTED(OPC_JAL);
            case OPC_SLTI:  IR_RSP_UNIMPLEMENTED(OPC_SLTI);
            case OPC_SLTIU: IR_RSP_UNIMPLEMENTED(OPC_SLTIU);
            case OPC_XORI:  IR_RSP_UNIMPLEMENTED(OPC_XORI);
            case OPC_LB:    IR_RSP_UNIMPLEMENTED(OPC_LB);
            //case OPC_LWL:   return rsp_lwl;
            //case OPC_LWR:   return rsp_lwr;
            //case OPC_SWL:   return rsp_swl;
            //case OPC_SWR:   return rsp_swr;

            case OPC_CP0:      IR_RSP_UNIMPLEMENTED(OPC_CP0); //return rsp_cp0_decode(pc, instruction);
            case OPC_CP1:      IR_RSP_UNIMPLEMENTED(OPC_CP1);     //return rsp_cp1_decode(pc, instr);
            case OPC_CP2:      IR_RSP_UNIMPLEMENTED(OPC_CP2); //return rsp_cp2_decode(pc, instruction);
            case OPC_SPCL:     IR_RSP_UNIMPLEMENTED(OPC_SPCL); //return rsp_special_decode(pc, instruction);
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