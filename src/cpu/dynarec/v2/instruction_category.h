#ifndef N64_INSTRUCTION_INFO_H
#define N64_INSTRUCTION_INFO_H

/* TODO: do I need to have this info upfront or can each instruction emit exception checking ir?
typedef struct dynarec_instruction_info {
    dynarec_instruction_category_t category;
    bool exception_possible;
} dynarec_instruction_info_t;

#define THROWS true
#define NOTHROW false
#define INFO(cat, exception) {.category = (cat), .exception_possible = (exception) }
 */

INLINE dynarec_instruction_category_t cp0_instruction_category(mips_instruction_t instr) {
    if (instr.is_coprocessor_funct) {
        switch (instr.fr.funct) {
            case COP_FUNCT_TLBWI_MULT: return TLB_WRITE;
            case COP_FUNCT_TLBWR_MOV: return TLB_WRITE;
            case COP_FUNCT_TLBP: return NORMAL;
            case COP_FUNCT_TLBR_SUB: return NORMAL;
            case COP_FUNCT_ERET: return BLOCK_ENDER;
            case COP_FUNCT_WAIT: return NORMAL;
            default: {
                char buf[50];
                disassemble(0, instr.raw, buf, 50);
                logfatal("other/unknown MIPS CP0 0x%08X with FUNCT: %d%d%d%d%d%d [%s]", instr.raw,
                         instr.funct0, instr.funct1, instr.funct2, instr.funct3, instr.funct4, instr.funct5, buf);
            }
        }
    } else {
        switch (instr.r.rs) {
            case COP_MF: return NORMAL;
            case COP_DMF: return NORMAL;
            case COP_MT: return NORMAL;
            case COP_DMT: return NORMAL;
            default: {
                char buf[50];
                disassemble(0, instr.raw, buf, 50);
                logfatal("other/unknown MIPS CP0 0x%08X with rs: %d%d%d%d%d [%s]", instr.raw,
                         instr.rs0, instr.rs1, instr.rs2, instr.rs3, instr.rs4, buf);
            }
        }
    }
}

INLINE dynarec_instruction_category_t special_instruction_category(mips_instruction_t instr) {
    switch (instr.r.funct) {
        case FUNCT_SLL: return NORMAL;
        case FUNCT_SRL: return NORMAL;
        case FUNCT_SRA: return NORMAL;
        case FUNCT_SRAV: return NORMAL;
        case FUNCT_SLLV: return NORMAL;
        case FUNCT_SRLV: return NORMAL;
        case FUNCT_JR: return BRANCH;
        case FUNCT_JALR: return BRANCH;
        case FUNCT_SYSCALL: return BLOCK_ENDER;
        case FUNCT_MFHI: return NORMAL;
        case FUNCT_MTHI: return NORMAL;
        case FUNCT_MFLO: return NORMAL;
        case FUNCT_MTLO: return NORMAL;
        case FUNCT_DSLLV: return NORMAL;
        case FUNCT_DSRLV: return NORMAL;
        case FUNCT_DSRAV: return NORMAL;
        case FUNCT_MULT: return NORMAL;
        case FUNCT_MULTU: return NORMAL;
        case FUNCT_DIV: return NORMAL;
        case FUNCT_DIVU: return NORMAL;
        case FUNCT_DMULT: return NORMAL;
        case FUNCT_DMULTU: return NORMAL;
        case FUNCT_DDIV: return NORMAL;
        case FUNCT_DDIVU: return NORMAL;
        case FUNCT_ADD: return NORMAL;
        case FUNCT_ADDU: return NORMAL;
        case FUNCT_AND: return NORMAL;
        case FUNCT_NOR: return NORMAL;
        case FUNCT_SUB: return NORMAL;
        case FUNCT_SUBU: return NORMAL;
        case FUNCT_OR: return NORMAL;
        case FUNCT_XOR: return NORMAL;
        case FUNCT_SLT: return NORMAL;
        case FUNCT_SLTU: return NORMAL;
        case FUNCT_DADD: return NORMAL;
        case FUNCT_DADDU: return NORMAL;
        case FUNCT_DSUB: return NORMAL;
        case FUNCT_DSUBU: return NORMAL;
        case FUNCT_TEQ: return NORMAL;
        case FUNCT_DSLL: return NORMAL;
        case FUNCT_DSRL: return NORMAL;
        case FUNCT_DSRA: return NORMAL;
        case FUNCT_DSLL32: return NORMAL;
        case FUNCT_DSRL32: return NORMAL;
        case FUNCT_DSRA32: return NORMAL;
        case FUNCT_BREAK: return BLOCK_ENDER;
        case FUNCT_SYNC: return NORMAL;
        case FUNCT_TGE: return NORMAL;
        case FUNCT_TGEU: return NORMAL;
        case FUNCT_TLT: return NORMAL;
        case FUNCT_TLTU: return NORMAL;
        case FUNCT_TNE: return NORMAL;
        default: {
            char buf[50];
            disassemble(0, instr.raw, buf, 50);
            logfatal("other/unknown MIPS Special 0x%08X with FUNCT: %d%d%d%d%d%d [%s]", instr.raw,
                     instr.funct0, instr.funct1, instr.funct2, instr.funct3, instr.funct4, instr.funct5, buf);
        }
    }
}


INLINE dynarec_instruction_category_t regimm_instruction_category(mips_instruction_t instr) {
    switch (instr.i.rt) {
        case RT_BLTZ: return BRANCH;
        case RT_BLTZL: return BRANCH_LIKELY;
        case RT_BLTZAL: return BRANCH;
        case RT_BGEZ: return BRANCH;
        case RT_BGEZL: return BRANCH_LIKELY;
        case RT_BGEZAL: return BRANCH;
        case RT_BGEZALL: return BRANCH_LIKELY;
        case RT_TGEI: return NORMAL;
        case RT_TGEIU: return NORMAL;
        case RT_TLTI: return NORMAL;
        case RT_TLTIU: return NORMAL;
        case RT_TEQI: return NORMAL;
        case RT_TNEI: return NORMAL;
        default: {
            char buf[50];
            disassemble(0, instr.raw, buf, 50);
            logfatal("other/unknown MIPS REGIMM 0x%08X with RT: %d%d%d%d%d [%s]", instr.raw,
                     instr.rt0, instr.rt1, instr.rt2, instr.rt3, instr.rt4, buf);
        }
    }
}

INLINE dynarec_instruction_category_t cp1_instruction_category(mips_instruction_t instr) {
    if (instr.is_coprocessor_funct) {
        return NORMAL;
    } else {
        switch (instr.r.rs) {
            case COP_CF:
            case COP_MF:
            case COP_DMF:
            case COP_MT:
            case COP_DMT:
            case COP_CT:
                return NORMAL;
            case COP_BC:
                switch (instr.r.rt) {
                    case COP_BC_BCT:
                    case COP_BC_BCF:
                        return BRANCH;
                    case COP_BC_BCTL:
                    case COP_BC_BCFL:
                        return BRANCH_LIKELY;
                    default: {
                        char buf[50];
                        disassemble(0, instr.raw, buf, 50);
                        logfatal("other/unknown MIPS BC 0x%08X [%s]", instr.raw, buf);
                    }
                }
        }
    }
    logfatal("Did not match any instructions.");
}

dynarec_instruction_category_t instr_category(mips_instruction_t instr) {
    if (unlikely(instr.raw == 0)) {
        return NORMAL;
    }
    switch (instr.op) {
        case OPC_CP0:    return cp0_instruction_category(instr);
        case OPC_SPCL:   return special_instruction_category(instr);
        case OPC_REGIMM: return regimm_instruction_category(instr);
        
        // Main CPU only
        case OPC_CP1:    return cp1_instruction_category(instr);

        // RSP only
        case OPC_CP2:      return NORMAL;
        case RSP_OPC_LWC2: return NORMAL;
        case RSP_OPC_SWC2: return STORE;

        case OPC_LD: return NORMAL;
        case OPC_LUI: return NORMAL;
        case OPC_ADDIU: return NORMAL;
        case OPC_ADDI: return NORMAL;
        case OPC_DADDI: return NORMAL;
        case OPC_ANDI: return NORMAL;
        case OPC_LBU: return NORMAL;
        case OPC_LHU: return NORMAL;
        case OPC_LH: return NORMAL;
        case OPC_LW: return NORMAL;
        case OPC_LWU: return NORMAL;
        case OPC_BEQ: return BRANCH;
        case OPC_BEQL: return BRANCH_LIKELY;
        case OPC_BGTZ: return BRANCH;
        case OPC_BGTZL: return BRANCH_LIKELY;
        case OPC_BLEZ: return BRANCH;
        case OPC_BLEZL: return BRANCH_LIKELY;
        case OPC_BNE: return BRANCH;
        case OPC_BNEL: return BRANCH_LIKELY;
        case OPC_CACHE: return CACHE;
        case OPC_SB: return NORMAL;
        case OPC_SH: return NORMAL;
        case OPC_SW: return NORMAL;
        case OPC_SD: return NORMAL;
        case OPC_ORI: return NORMAL;
        case OPC_J: return BRANCH;
        case OPC_JAL: return BRANCH;
        case OPC_SLTI: return NORMAL;
        case OPC_SLTIU: return NORMAL;
        case OPC_XORI: return NORMAL;
        case OPC_DADDIU: return NORMAL;
        case OPC_LB: return NORMAL;
        case OPC_LDC1: return NORMAL;
        case OPC_SDC1: return NORMAL;
        case OPC_LWC1: return NORMAL;
        case OPC_SWC1: return NORMAL;
        case OPC_LWL: return NORMAL;
        case OPC_LWR: return NORMAL;
        case OPC_SWL: return NORMAL;
        case OPC_SWR: return NORMAL;
        case OPC_LDL: return NORMAL;
        case OPC_LDR: return NORMAL;
        case OPC_SDL: return NORMAL;
        case OPC_SDR: return NORMAL;
        case OPC_LL: return NORMAL;
        case OPC_LLD: return NORMAL;
        case OPC_SC: return NORMAL;
        case OPC_SCD: return NORMAL;
        case OPC_RDHWR: return BLOCK_ENDER; // Invalid instruction used by Linux
        default: {
            char buf[50];
            disassemble(0, instr.raw, buf, 50);
            logfatal("Failed to decode instruction 0x%08X opcode %d%d%d%d%d%d [%s]",
                     instr.raw, instr.op0, instr.op1, instr.op2, instr.op3, instr.op4, instr.op5, buf);
        }
    }
}


#endif // N64_INSTRUCTION_INFO_H