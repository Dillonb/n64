#include "ir_emitter.h"

#include <util.h>
#include <dynarec/dynarec.h>
#include <disassemble.h>

#define IR_UNIMPLEMENTED(opc) logfatal("Unimplemented IR translation for instruction " #opc)

void emit_cp0_instruction_ir(mips_instruction_t instr) {
    if (instr.last11 == 0) {
        switch (instr.r.rs) {
            case COP_MF: IR_UNIMPLEMENTED(COP_MF);
            case COP_DMF: IR_UNIMPLEMENTED(COP_DMF);
            // Last 11 bits are 0
            case COP_MT: IR_UNIMPLEMENTED(COP_MT);
            case COP_DMT: IR_UNIMPLEMENTED(COP_DMT);
            default: {
                char buf[50];
                disassemble(0, instr.raw, buf, 50);
                logfatal("other/unknown MIPS CP0 0x%08X with rs: %d%d%d%d%d [%s]", instr.raw,
                         instr.rs0, instr.rs1, instr.rs2, instr.rs3, instr.rs4, buf);
            }
        }
    } else {
        switch (instr.fr.funct) {
            case COP_FUNCT_TLBWI_MULT: IR_UNIMPLEMENTED(COP_FUNCT_TLBWI_MULT);
            case COP_FUNCT_TLBWR_MOV: IR_UNIMPLEMENTED(COP_FUNCT_TLBWR_MOV);
            case COP_FUNCT_TLBP: IR_UNIMPLEMENTED(COP_FUNCT_TLBP);
            case COP_FUNCT_TLBR_SUB: IR_UNIMPLEMENTED(COP_FUNCT_TLBR_SUB);
            case COP_FUNCT_ERET: IR_UNIMPLEMENTED(COP_FUNCT_ERET);
            case COP_FUNCT_WAIT: IR_UNIMPLEMENTED(COP_FUNCT_WAIT);
            default: {
                char buf[50];
                disassemble(0, instr.raw, buf, 50);
                logfatal("other/unknown MIPS CP0 0x%08X with FUNCT: %d%d%d%d%d%d [%s]", instr.raw,
                         instr.funct0, instr.funct1, instr.funct2, instr.funct3, instr.funct4, instr.funct5, buf);
            }
        }
    }
}

void emit_special_instruction_ir(mips_instruction_t instr) {
    switch (instr.r.funct) {
        case FUNCT_SLL: IR_UNIMPLEMENTED(FUNCT_SLL);
        case FUNCT_SRL: IR_UNIMPLEMENTED(FUNCT_SRL);
        case FUNCT_SRA: IR_UNIMPLEMENTED(FUNCT_SRA);
        case FUNCT_SRAV: IR_UNIMPLEMENTED(FUNCT_SRAV);
        case FUNCT_SLLV: IR_UNIMPLEMENTED(FUNCT_SLLV);
        case FUNCT_SRLV: IR_UNIMPLEMENTED(FUNCT_SRLV);
        case FUNCT_JR: IR_UNIMPLEMENTED(FUNCT_JR);
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
            disassemble(0, instr.raw, buf, 50);
            logfatal("other/unknown MIPS Special 0x%08X with FUNCT: %d%d%d%d%d%d [%s]", instr.raw,
                     instr.funct0, instr.funct1, instr.funct2, instr.funct3, instr.funct4, instr.funct5, buf);
        }
    }
}


void emit_regimm_instruction_ir(mips_instruction_t instr) {
    switch (instr.i.rt) {
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
            disassemble(0, instr.raw, buf, 50);
            logfatal("other/unknown MIPS REGIMM 0x%08X with RT: %d%d%d%d%d [%s]", instr.raw,
                     instr.rt0, instr.rt1, instr.rt2, instr.rt3, instr.rt4, buf);
        }
    }
}

void emit_cp1_instruction_ir(mips_instruction_t instr) {
    // This function uses a series of two switch statements.
    // If the instruction doesn't use the RS field for the opcode, then control will fall through to the next
    // switch, and check the FUNCT. It may be worth profiling and seeing if it's faster to check FUNCT first at some point
    switch (instr.r.rs) {
        case COP_CF: IR_UNIMPLEMENTED(COP_CF);
        case COP_MF: IR_UNIMPLEMENTED(COP_MF);
        case COP_DMF: IR_UNIMPLEMENTED(COP_DMF);
        case COP_MT: IR_UNIMPLEMENTED(COP_MT);
        case COP_DMT: IR_UNIMPLEMENTED(COP_DMT);
        case COP_CT: IR_UNIMPLEMENTED(COP_CT);
        case COP_BC:
            switch (instr.r.rt) {
                case COP_BC_BCT: IR_UNIMPLEMENTED(COP_BC_BCT);
                case COP_BC_BCF: IR_UNIMPLEMENTED(COP_BC_BCF);
                case COP_BC_BCTL: IR_UNIMPLEMENTED(COP_BC_BCTL);
                case COP_BC_BCFL: IR_UNIMPLEMENTED(COP_BC_BCFL);
                default: {
                    char buf[50];
                    disassemble(0, instr.raw, buf, 50);
                    logfatal("other/unknown MIPS BC 0x%08X [%s]", instr.raw, buf);
                }
            }
    }
    IR_UNIMPLEMENTED(SomeFPUInstruction);
}

void emit_instruction_ir(mips_instruction_t instr) {
    if (unlikely(instr.raw == 0)) {
        return; // do nothing for NOP
    }
    switch (instr.op) {
        case OPC_CP0:    emit_cp0_instruction_ir(instr); break;
        case OPC_CP1:    emit_cp1_instruction_ir(instr); break;
        case OPC_SPCL:   emit_special_instruction_ir(instr); break;
        case OPC_REGIMM: emit_regimm_instruction_ir(instr); break;

        case OPC_LD: IR_UNIMPLEMENTED(OPC_LD);
        case OPC_LUI: IR_UNIMPLEMENTED(OPC_LUI);
        case OPC_ADDIU: IR_UNIMPLEMENTED(OPC_ADDIU);
        case OPC_ADDI: IR_UNIMPLEMENTED(OPC_ADDI);
        case OPC_DADDI: IR_UNIMPLEMENTED(OPC_DADDI);
        case OPC_ANDI: IR_UNIMPLEMENTED(OPC_ANDI);
        case OPC_LBU: IR_UNIMPLEMENTED(OPC_LBU);
        case OPC_LHU: IR_UNIMPLEMENTED(OPC_LHU);
        case OPC_LH: IR_UNIMPLEMENTED(OPC_LH);
        case OPC_LW: IR_UNIMPLEMENTED(OPC_LW);
        case OPC_LWU: IR_UNIMPLEMENTED(OPC_LWU);
        case OPC_BEQ: IR_UNIMPLEMENTED(OPC_BEQ);
        case OPC_BEQL: IR_UNIMPLEMENTED(OPC_BEQL);
        case OPC_BGTZ: IR_UNIMPLEMENTED(OPC_BGTZ);
        case OPC_BGTZL: IR_UNIMPLEMENTED(OPC_BGTZL);
        case OPC_BLEZ: IR_UNIMPLEMENTED(OPC_BLEZ);
        case OPC_BLEZL: IR_UNIMPLEMENTED(OPC_BLEZL);
        case OPC_BNE: IR_UNIMPLEMENTED(OPC_BNE);
        case OPC_BNEL: IR_UNIMPLEMENTED(OPC_BNEL);
        case OPC_CACHE: IR_UNIMPLEMENTED(OPC_CACHE);
        case OPC_SB: IR_UNIMPLEMENTED(OPC_SB);
        case OPC_SH: IR_UNIMPLEMENTED(OPC_SH);
        case OPC_SW: IR_UNIMPLEMENTED(OPC_SW);
        case OPC_SD: IR_UNIMPLEMENTED(OPC_SD);
        case OPC_ORI: IR_UNIMPLEMENTED(OPC_ORI);
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
            disassemble(0, instr.raw, buf, 50);
            logfatal("Failed to decode instruction 0x%08X opcode %d%d%d%d%d%d [%s]",
                     instr.raw, instr.op0, instr.op1, instr.op2, instr.op3, instr.op4, instr.op5, buf);
        }
    }
}
