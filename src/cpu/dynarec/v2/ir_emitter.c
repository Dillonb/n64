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

void ir_emit_abs_branch(ir_instruction_t* address) {
    ir_emit_set_block_exit_pc(address);
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

IR_EMITTER(sw) {
    ir_instruction_t* address = get_memory_access_address(instruction, BUS_STORE);
    ir_instruction_t* value = ir_emit_load_guest_reg(instruction.i.rt);
    ir_emit_store(VALUE_TYPE_U32, address, value);
}

IR_EMITTER(lw) {
    ir_emit_load(VALUE_TYPE_S32, get_memory_access_address(instruction, BUS_LOAD), instruction.i.rt);
}

IR_EMITTER(lhu) {
    ir_emit_load(VALUE_TYPE_U16, get_memory_access_address(instruction, BUS_LOAD), instruction.i.rt);
}

IR_EMITTER(ld) {
    ir_emit_load(VALUE_TYPE_64, get_memory_access_address(instruction, BUS_LOAD), instruction.i.rt);
}

IR_EMITTER(bne) {
    ir_instruction_t* rs = ir_emit_load_guest_reg(instruction.i.rs);
    ir_instruction_t* rt = ir_emit_load_guest_reg(instruction.i.rt);
    ir_instruction_t* cond = ir_emit_check_condition(CONDITION_NOT_EQUAL, rs, rt);
    ir_emit_conditional_branch(cond, instruction.i.immediate, virtual_address);
}

IR_EMITTER(jr) {
    ir_emit_abs_branch(ir_emit_load_guest_reg(instruction.i.rs));
}

IR_EMITTER(mtc0) {
    ir_instruction_t* value = ir_context.guest_gpr_to_value[instruction.r.rt];
    switch (instruction.r.rd) {
        case R4300I_CP0_REG_INDEX: logfatal("emit MTC0 R4300I_CP0_REG_INDEX");
        case R4300I_CP0_REG_RANDOM: logfatal("emit MTC0 R4300I_CP0_REG_RANDOM");
        case R4300I_CP0_REG_COUNT: logfatal("emit MTC0 R4300I_CP0_REG_COUNT");
        case R4300I_CP0_REG_CAUSE: logfatal("emit MTC0 R4300I_CP0_REG_CAUSE");
        case R4300I_CP0_REG_TAGLO: logfatal("emit MTC0 R4300I_CP0_REG_TAGLO");
        case R4300I_CP0_REG_TAGHI: logfatal("emit MTC0 R4300I_CP0_REG_TAGHI");
        case R4300I_CP0_REG_COMPARE: logfatal("emit MTC0 R4300I_CP0_REG_COMPARE");
        case R4300I_CP0_REG_STATUS: {
            logfatal("emit MTC0 R4300I_CP0_REG_STATUS");
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
    logfatal("Emit MTC0");
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
        case FUNCT_SLL: IR_UNIMPLEMENTED(FUNCT_SLL);
        case FUNCT_SRL: IR_UNIMPLEMENTED(FUNCT_SRL);
        case FUNCT_SRA: IR_UNIMPLEMENTED(FUNCT_SRA);
        case FUNCT_SRAV: IR_UNIMPLEMENTED(FUNCT_SRAV);
        case FUNCT_SLLV: IR_UNIMPLEMENTED(FUNCT_SLLV);
        case FUNCT_SRLV: IR_UNIMPLEMENTED(FUNCT_SRLV);
        case FUNCT_JR: CALL_IR_EMITTER(jr);
        case FUNCT_JALR: IR_UNIMPLEMENTED(FUNCT_JALR);
        case FUNCT_SYSCALL: IR_UNIMPLEMENTED(FUNCT_SYSCALL);
        case FUNCT_MFHI: IR_UNIMPLEMENTED(FUNCT_MFHI);
        case FUNCT_MTHI: IR_UNIMPLEMENTED(FUNCT_MTHI);
        case FUNCT_MFLO: IR_UNIMPLEMENTED(FUNCT_MFLO);
        case FUNCT_MTLO: IR_UNIMPLEMENTED(FUNCT_MTLO);
        case FUNCT_DSLLV: IR_UNIMPLEMENTED(FUNCT_DSLLV);
        case FUNCT_DSRLV: IR_UNIMPLEMENTED(FUNCT_DSRLV);
        case FUNCT_DSRAV: IR_UNIMPLEMENTED(FUNCT_DSRAV);
        case FUNCT_MULT: IR_UNIMPLEMENTED(FUNCT_MULT);
        case FUNCT_MULTU: IR_UNIMPLEMENTED(FUNCT_MULTU);
        case FUNCT_DIV: IR_UNIMPLEMENTED(FUNCT_DIV);
        case FUNCT_DIVU: IR_UNIMPLEMENTED(FUNCT_DIVU);
        case FUNCT_DMULT: IR_UNIMPLEMENTED(FUNCT_DMULT);
        case FUNCT_DMULTU: IR_UNIMPLEMENTED(FUNCT_DMULTU);
        case FUNCT_DDIV: IR_UNIMPLEMENTED(FUNCT_DDIV);
        case FUNCT_DDIVU: IR_UNIMPLEMENTED(FUNCT_DDIVU);
        case FUNCT_ADD: IR_UNIMPLEMENTED(FUNCT_ADD);
        case FUNCT_ADDU: IR_UNIMPLEMENTED(FUNCT_ADDU);
        case FUNCT_AND: IR_UNIMPLEMENTED(FUNCT_AND);
        case FUNCT_NOR: IR_UNIMPLEMENTED(FUNCT_NOR);
        case FUNCT_SUB: IR_UNIMPLEMENTED(FUNCT_SUB);
        case FUNCT_SUBU: IR_UNIMPLEMENTED(FUNCT_SUBU);
        case FUNCT_OR: IR_UNIMPLEMENTED(FUNCT_OR);
        case FUNCT_XOR: IR_UNIMPLEMENTED(FUNCT_XOR);
        case FUNCT_SLT: IR_UNIMPLEMENTED(FUNCT_SLT);
        case FUNCT_SLTU: IR_UNIMPLEMENTED(FUNCT_SLTU);
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
        case RT_BGEZAL: IR_UNIMPLEMENTED(RT_BGEZAL);
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
        case OPC_ADDIU: IR_UNIMPLEMENTED(OPC_ADDIU);
        case OPC_ADDI: IR_UNIMPLEMENTED(OPC_ADDI);
        case OPC_DADDI: IR_UNIMPLEMENTED(OPC_DADDI);
        case OPC_ANDI: CALL_IR_EMITTER(andi);
        case OPC_LBU: IR_UNIMPLEMENTED(OPC_LBU);
        case OPC_LHU: CALL_IR_EMITTER(lhu);
        case OPC_LH: IR_UNIMPLEMENTED(OPC_LH);
        case OPC_LW: CALL_IR_EMITTER(lw);
        case OPC_LWU: IR_UNIMPLEMENTED(OPC_LWU);
        case OPC_BEQ: IR_UNIMPLEMENTED(OPC_BEQ);
        case OPC_BEQL: IR_UNIMPLEMENTED(OPC_BEQL);
        case OPC_BGTZ: IR_UNIMPLEMENTED(OPC_BGTZ);
        case OPC_BGTZL: IR_UNIMPLEMENTED(OPC_BGTZL);
        case OPC_BLEZ: IR_UNIMPLEMENTED(OPC_BLEZ);
        case OPC_BLEZL: IR_UNIMPLEMENTED(OPC_BLEZL);
        case OPC_BNE: CALL_IR_EMITTER(bne);
        case OPC_BNEL: IR_UNIMPLEMENTED(OPC_BNEL);
        case OPC_CACHE: IR_UNIMPLEMENTED(OPC_CACHE);
        case OPC_SB: IR_UNIMPLEMENTED(OPC_SB);
        case OPC_SH: IR_UNIMPLEMENTED(OPC_SH);
        case OPC_SW:CALL_IR_EMITTER(sw);
        case OPC_SD: IR_UNIMPLEMENTED(OPC_SD);
        case OPC_ORI: CALL_IR_EMITTER(ori);
        case OPC_J: IR_UNIMPLEMENTED(OPC_J);
        case OPC_JAL: IR_UNIMPLEMENTED(OPC_JAL);
        case OPC_SLTI: IR_UNIMPLEMENTED(OPC_SLTI);
        case OPC_SLTIU: IR_UNIMPLEMENTED(OPC_SLTIU);
        case OPC_XORI: IR_UNIMPLEMENTED(OPC_XORI);
        case OPC_DADDIU: IR_UNIMPLEMENTED(OPC_DADDIU);
        case OPC_LB: IR_UNIMPLEMENTED(OPC_LB);
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