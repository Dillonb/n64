#include "r4300i.h"
#include "../common/log.h"
#include "disassemble.h"
#include "mips_instructions.h"
#include "tlb_instructions.h"

const char* register_names[] = {
        "zero", "at", "v0", "v1", "a0", "a1", "a2", "a3", "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
        "s0",   "s1", "s2", "s3", "s4", "s5", "s6", "s7", "t8", "t9", "k0", "k1", "gp", "sp", "s8", "ra"
};

const char* cp0_register_names[] = {
        "Index", "Random", "EntryLo0", "EntryLo1", "Context", "PageMask", "Wired", "7", "BadVAddr", "Count", "EntryHi",
        "Compare", "Status", "Cause", "EPC", "PRId", "Config", "LLAddr", "WatchLo", "WatchHi", "XContext", "21", "22",
        "23", "24", "25", "Parity Error", "Cache Error", "TagLo", "TagHi"
};

// Exceptions
#define EXCEPTION_INTERRUPT            0
#define EXCEPTION_COPROCESSOR_UNUSABLE 11

void exception(r4300i_t* cpu, word pc, word code, word coprocessor_error) {
    loginfo("Exception thrown! Code: %d Coprocessor: %d", code, coprocessor_error)
    if (cpu->branch) {
        unimplemented(cpu->cp0.status.exl, "handling branch delay when exl == true")
        cpu->cp0.cause.branch_delay = true;
        cpu->branch = false;
        cpu->branch_delay = 0;
        cpu->branch_pc = 0;
        logwarn("Exception thrown in a branch delay slot! make sure this is being handled correctly. "
                 "EPC is supposed to be set to the address of the branch preceding the slot.")
        pc -= 4;
    } else {
        cpu->cp0.cause.branch_delay = false;
    }

    if (!cpu->cp0.status.exl) {
        cpu->cp0.EPC = pc;
        cpu->cp0.status.exl = true;
    }

    cpu->cp0.cause.exception_code = code;
    cpu->cp0.cause.coprocessor_error = coprocessor_error;

    if (cpu->cp0.status.bev) {
        switch (code) {
            case EXCEPTION_COPROCESSOR_UNUSABLE:
                logfatal("Cop unusable, the PC below is wrong. See page 181 in the manual.")
                cpu->pc = 0x80000180;
                break;
            default:
                logfatal("Unknown exception %d with BEV! See page 181 in the manual.", code)
        }
    } else {
        switch (code) {
            case EXCEPTION_INTERRUPT:
                cpu->pc = 0x80000180;
                break;
            case EXCEPTION_COPROCESSOR_UNUSABLE:
                cpu->pc = 0x80000180;
                break;
            default:
                logfatal("Unknown exception %d without BEV! See page 181 in the manual.", code)
        }
    }
}

mips_instruction_type_t r4300i_cp0_decode(r4300i_t* cpu, word pc, mips_instruction_t instr) {
    if (instr.last11 == 0) {
        switch (instr.r.rs) {
            case COP_MF:
                return MIPS_CP_MFC0;
            case COP_MT: // Last 11 bits are 0
                return MIPS_CP_MTC0;
            default: {
                char buf[50];
                disassemble(pc, instr.raw, buf, 50);
                logfatal("other/unknown MIPS CP0 0x%08X with rs: %d%d%d%d%d [%s]", instr.raw,
                         instr.rs0, instr.rs1, instr.rs2, instr.rs3, instr.rs4, buf)
            }
        }
    } else {
        switch (instr.fr.funct) {
            case COP_FUNCT_TLBWI_MULT:
                return MIPS_TLBWI;
            case COP_FUNCT_TLBP:
                printf("TLBP at pc: 0x%08X\n", pc);
                return MIPS_TLBP;
            case COP_FUNCT_TLBR_SUB:
                logfatal("tlbr")
                return MIPS_TLBR;
            case COP_FUNCT_ERET:
                return MIPS_ERET;
            default: {
                char buf[50];
                disassemble(pc, instr.raw, buf, 50);
                logfatal("other/unknown MIPS CP0 0x%08X with FUNCT: %d%d%d%d%d%d [%s]", instr.raw,
                         instr.funct0, instr.funct1, instr.funct2, instr.funct3, instr.funct4, instr.funct5, buf)
            }
        }
    }
}

mips_instruction_type_t r4300i_cp1_decode(r4300i_t* cpu, word pc, mips_instruction_t instr) {
    if (!cpu->cp0.status.cu1) {
        exception(cpu, pc, EXCEPTION_COPROCESSOR_UNUSABLE, 1);
    }
    // This function uses a series of two switch statements.
    // If the instruction doesn't use the RS field for the opcode, then control will fall through to the next
    // switch, and check the FUNCT. It may be worth profiling and seeing if it's faster to check FUNCT first at some point
    switch (instr.r.rs) {
        case COP_CF:
            return MIPS_CP_CFC1;
        case COP_MF:
            return MIPS_CP_MFC1;
        case COP_MT:
            return MIPS_CP_MTC1;
        case COP_CT:
            return MIPS_CP_CTC1;
        case COP_BC:
            switch (instr.r.rt) {
                case COP_BC_BCT:
                    return MIPS_CP_BC1T;
                case COP_BC_BCF:
                    return MIPS_CP_BC1F;
                case COP_BC_BCTL:
                    return MIPS_CP_BC1TL;
                case COP_BC_BCFL:
                    return MIPS_CP_BC1FL;
                default: {
                    char buf[50];
                    disassemble(pc, instr.raw, buf, 50);
                    logfatal("other/unknown MIPS BC 0x%08X [%s]", instr.raw, buf)
                }
            }
    }
    switch (instr.fr.funct) {
        case COP_FUNCT_ADD:
            switch (instr.fr.fmt) {
                case FP_FMT_DOUBLE:
                    return MIPS_CP_ADD_D;
                case FP_FMT_SINGLE:
                    return MIPS_CP_ADD_S;
                default:
                    logfatal("Undefined!")
            }
        case COP_FUNCT_TLBR_SUB: {
            switch (instr.fr.fmt) {
                case FP_FMT_DOUBLE:
                    return MIPS_CP_SUB_D;
                case FP_FMT_SINGLE:
                    return MIPS_CP_SUB_S;
                default:
                    logfatal("Undefined!")
            }
        }
        case COP_FUNCT_TLBWI_MULT:
            switch (instr.fr.fmt) {
                case FP_FMT_DOUBLE:
                    return MIPS_CP_MUL_D;
                case FP_FMT_SINGLE:
                    return MIPS_CP_MUL_S;
                default:
                    logfatal("Undefined!")
            }
        case COP_FUNCT_DIV:
            switch (instr.fr.fmt) {
                case FP_FMT_DOUBLE:
                    return MIPS_CP_DIV_D;
                case FP_FMT_SINGLE:
                    return MIPS_CP_DIV_S;
                default:
                    logfatal("Undefined!")
            }
        case COP_FUNCT_TRUNC_L:
            switch (instr.fr.fmt) {
                case FP_FMT_DOUBLE:
                    return MIPS_CP_TRUNC_L_D;
                case FP_FMT_SINGLE:
                    return MIPS_CP_TRUNC_L_S;
                default:
                    logfatal("Undefined!")
            }
        case COP_FUNCT_TRUNC_W:
            switch (instr.fr.fmt) {
                case FP_FMT_DOUBLE:
                    return MIPS_CP_TRUNC_W_D;
                case FP_FMT_SINGLE:
                    return MIPS_CP_TRUNC_W_S;
                default:
                    logfatal("Undefined!")
            }
        case COP_FUNCT_CVT_D:
            switch (instr.fr.fmt) {
                case FP_FMT_SINGLE:
                    return MIPS_CP_CVT_D_S;
                case FP_FMT_W:
                    return MIPS_CP_CVT_D_W;
                case FP_FMT_L:
                    return MIPS_CP_CVT_D_L;
                default:
                    logfatal("Undefined!")
            }
        case COP_FUNCT_CVT_L:
            switch (instr.fr.fmt) {
                case FP_FMT_DOUBLE:
                    return MIPS_CP_CVT_L_D;
                case FP_FMT_SINGLE:
                    return MIPS_CP_CVT_L_S;
                default:
                    logfatal("Undefined!")
            }
        case COP_FUNCT_CVT_S:
            switch (instr.fr.fmt) {
                case FP_FMT_DOUBLE:
                    return MIPS_CP_CVT_S_D;
                case FP_FMT_W:
                    return MIPS_CP_CVT_S_W;
                case FP_FMT_L:
                    return MIPS_CP_CVT_S_L;
                default:
                    logfatal("Undefined!")
            }
        case COP_FUNCT_CVT_W:
            switch (instr.fr.fmt) {
                case FP_FMT_DOUBLE:
                    return MIPS_CP_CVT_W_D;
                case FP_FMT_SINGLE:
                    return MIPS_CP_CVT_W_S;
                default:
                    logfatal("Undefined!")
            }
        case COP_FUNCT_SQRT:
            switch (instr.fr.fmt) {
                case FP_FMT_DOUBLE:
                    return MIPS_CP_SQRT_D;
                case FP_FMT_SINGLE:
                    return MIPS_CP_SQRT_S;
                default:
                    logfatal("Undefined!")
            }

        case COP_FUNCT_MOV:
            switch (instr.fr.fmt) {
                case FP_FMT_DOUBLE:
                    return MIPS_CP_MOV_D;
                case FP_FMT_SINGLE:
                    return MIPS_CP_MOV_S;
                default:
                    logfatal("Undefined!")
            }
        case COP_FUNCT_NEG:
            switch (instr.fr.fmt) {
                case FP_FMT_DOUBLE:
                    return MIPS_CP_NEG_D;
                case FP_FMT_SINGLE:
                    return MIPS_CP_NEG_S;
                default:
                    logfatal("Undefined!")
            }
        case COP_FUNCT_C_F:
            logfatal("COP_FUNCT_C_F unimplemented")
        case COP_FUNCT_C_UN:
            logfatal("COP_FUNCT_C_UN unimplemented")
        case COP_FUNCT_C_EQ:
            switch (instr.fr.fmt) {
                case FP_FMT_DOUBLE:
                    return MIPS_CP_C_EQ_D;
                case FP_FMT_SINGLE:
                    return MIPS_CP_C_EQ_S;
                default:
                    logfatal("Undefined!")
            }
        case COP_FUNCT_C_UEQ:
            logfatal("COP_FUNCT_C_UEQ unimplemented")
        case COP_FUNCT_C_OLT:
            logfatal("COP_FUNCT_C_OLT unimplemented")
        case COP_FUNCT_C_ULT:
            logfatal("COP_FUNCT_C_ULT unimplemented")
        case COP_FUNCT_C_OLE:
            logfatal("COP_FUNCT_C_OLE unimplemented")
        case COP_FUNCT_C_ULE:
            logfatal("COP_FUNCT_C_ULE unimplemented")
        case COP_FUNCT_C_SF:
            logfatal("COP_FUNCT_C_SF unimplemented")
        case COP_FUNCT_C_NGLE:
            logfatal("COP_FUNCT_C_NGLE unimplemented")
        case COP_FUNCT_C_SEQ:
            logfatal("COP_FUNCT_C_SEQ unimplemented")
        case COP_FUNCT_C_NGL:
            logfatal("COP_FUNCT_C_NGL unimplemented")
        case COP_FUNCT_C_LT:
            switch (instr.fr.fmt) {
                case FP_FMT_DOUBLE:
                    return MIPS_CP_C_LT_D;
                case FP_FMT_SINGLE:
                    return MIPS_CP_C_LT_S;
                default:
                    logfatal("Undefined!")
            }
        case COP_FUNCT_C_NGE:
            logfatal("COP_FUNCT_C_NGE unimplemented")
        case COP_FUNCT_C_LE:
            switch (instr.fr.fmt) {
                case FP_FMT_DOUBLE:
                    return MIPS_CP_C_LE_D;
                case FP_FMT_SINGLE:
                    return MIPS_CP_C_LE_S;
                default:
                    logfatal("Undefined!")
            }
            logfatal("COP_FUNCT_C_LE unimplemented")
        case COP_FUNCT_C_NGT:
            logfatal("COP_FUNCT_C_NGT unimplemented")
    }

    char buf[50];
    disassemble(pc, instr.raw, buf, 50);
    logfatal("other/unknown MIPS CP1 0x%08X with rs: %d%d%d%d%d and FUNCT: %d%d%d%d%d%d [%s]", instr.raw,
             instr.rs0, instr.rs1, instr.rs2, instr.rs3, instr.rs4,
             instr.funct0, instr.funct1, instr.funct2, instr.funct3, instr.funct4, instr.funct5, buf)
}

mips_instruction_type_t r4300i_special_decode(r4300i_t* cpu, word pc, mips_instruction_t instr) {
    switch (instr.r.funct) {
        case FUNCT_SLL:    return MIPS_SPC_SLL;
        case FUNCT_SRL:    return MIPS_SPC_SRL;
        case FUNCT_SRA:    return MIPS_SPC_SRA;
        case FUNCT_SRAV:   return MIPS_SPC_SRAV;
        case FUNCT_SLLV:   return MIPS_SPC_SLLV;
        case FUNCT_SRLV:   return MIPS_SPC_SRLV;
        case FUNCT_JR:     return MIPS_SPC_JR;
        case FUNCT_JALR:   return MIPS_SPC_JALR;
        case FUNCT_MFHI:   return MIPS_SPC_MFHI;
        case FUNCT_MTHI:   return MIPS_SPC_MTHI;
        case FUNCT_MFLO:   return MIPS_SPC_MFLO;
        case FUNCT_MTLO:   return MIPS_SPC_MTLO;
        case FUNCT_DSLLV:  return MIPS_SPC_DSLLV;
        case FUNCT_MULT:   return MIPS_SPC_MULT;
        case FUNCT_MULTU:  return MIPS_SPC_MULTU;
        case FUNCT_DIV:    return MIPS_SPC_DIV;
        case FUNCT_DIVU:   return MIPS_SPC_DIVU;
        case FUNCT_DMULTU: return MIPS_SPC_DMULTU;
        case FUNCT_DDIVU:  return MIPS_SPC_DDIVU;
        case FUNCT_ADD:    return MIPS_SPC_ADD;
        case FUNCT_ADDU:   return MIPS_SPC_ADDU;
        case FUNCT_AND:    return MIPS_SPC_AND;
        case FUNCT_NOR:    return MIPS_SPC_NOR;
        case FUNCT_SUB:    return MIPS_SPC_SUB;
        case FUNCT_SUBU:   return MIPS_SPC_SUBU;
        case FUNCT_OR:     return MIPS_SPC_OR;
        case FUNCT_XOR:    return MIPS_SPC_XOR;
        case FUNCT_SLT:    return MIPS_SPC_SLT;
        case FUNCT_SLTU:   return MIPS_SPC_SLTU;
        case FUNCT_DADD:   return MIPS_SPC_DADD;
        case FUNCT_DSLL:   return MIPS_SPC_DSLL;
        case FUNCT_DSLL32: return MIPS_SPC_DSLL32;
        case FUNCT_DSRA32: return MIPS_SPC_DSRA32;
        default: {
            char buf[50];
            disassemble(pc, instr.raw, buf, 50);
            logfatal("other/unknown MIPS Special 0x%08X with FUNCT: %d%d%d%d%d%d [%s]", instr.raw,
                     instr.funct0, instr.funct1, instr.funct2, instr.funct3, instr.funct4, instr.funct5, buf)
        }
    }
}

mips_instruction_type_t r4300i_regimm_decode(r4300i_t* cpu, word pc, mips_instruction_t instr) {
    switch (instr.i.rt) {
        case RT_BLTZ:   return MIPS_RI_BLTZ;
        case RT_BLTZL:  return MIPS_RI_BLTZL;
        case RT_BGEZ:   return MIPS_RI_BGEZ;
        case RT_BGEZL:  return MIPS_RI_BGEZL;
        case RT_BGEZAL: return MIPS_RI_BGEZAL;
        default: {
            char buf[50];
            disassemble(pc, instr.raw, buf, 50);
            logfatal("other/unknown MIPS REGIMM 0x%08X with RT: %d%d%d%d%d [%s]", instr.raw,
                     instr.rt0, instr.rt1, instr.rt2, instr.rt3, instr.rt4, buf)
        }
    }
}

mips_instruction_type_t r4300i_instruction_decode(r4300i_t* cpu, word pc, mips_instruction_t instr) {
    char buf[50];
    if (n64_log_verbosity >= LOG_VERBOSITY_DEBUG) {
        disassemble(pc, instr.raw, buf, 50);
        logdebug("[0x%08X]=0x%08X %s", pc, instr.raw, buf)
    }
    if (instr.raw == 0) {
        return MIPS_NOP;
    }
    switch (instr.op) {
        case OPC_CP0:    return r4300i_cp0_decode(cpu, pc, instr);
        case OPC_CP1:    return r4300i_cp1_decode(cpu, pc, instr);
        case OPC_SPCL:   return r4300i_special_decode(cpu, pc, instr);
        case OPC_REGIMM: return r4300i_regimm_decode(cpu, pc, instr);

        case OPC_LD:    return MIPS_LD;
        case OPC_LUI:   return MIPS_LUI;
        case OPC_ADDIU: return MIPS_ADDIU;
        case OPC_ADDI:  return MIPS_ADDI;
        case OPC_DADDI: return MIPS_DADDI;
        case OPC_ANDI:  return MIPS_ANDI;
        case OPC_LBU:   return MIPS_LBU;
        case OPC_LHU:   return MIPS_LHU;
        case OPC_LH:    return MIPS_LH;
        case OPC_LW:    return MIPS_LW;
        case OPC_LWU:   return MIPS_LWU;
        case OPC_BEQ:   return MIPS_BEQ;
        case OPC_BEQL:  return MIPS_BEQL;
        case OPC_BGTZ:  return MIPS_BGTZ;
        case OPC_BGTZL: return MIPS_BGTZL;
        case OPC_BLEZ:  return MIPS_BLEZ;
        case OPC_BLEZL: return MIPS_BLEZL;
        case OPC_BNE:   return MIPS_BNE;
        case OPC_BNEL:  return MIPS_BNEL;
        case OPC_CACHE: return MIPS_CACHE;
        case OPC_SB:    return MIPS_SB;
        case OPC_SH:    return MIPS_SH;
        case OPC_SW:    return MIPS_SW;
        case OPC_SD:    return MIPS_SD;
        case OPC_ORI:   return MIPS_ORI;
        case OPC_J:     return MIPS_J;
        case OPC_JAL:   return MIPS_JAL;
        case OPC_SLTI:  return MIPS_SLTI;
        case OPC_SLTIU: return MIPS_SLTIU;
        case OPC_XORI:  return MIPS_XORI;
        case OPC_LB:    return MIPS_LB;
        case OPC_LDC1:  return MIPS_LDC1;
        case OPC_SDC1:  return MIPS_SDC1;
        case OPC_LWC1:  return MIPS_LWC1;
        case OPC_SWC1:  return MIPS_SWC1;
        case OPC_LWL:   return MIPS_LWL;
        case OPC_LWR:   return MIPS_LWR;
        case OPC_SWL:   return MIPS_SWL;
        case OPC_SWR:   return MIPS_SWR;
        case OPC_LDL:   return MIPS_LDL;
        case OPC_LDR:   return MIPS_LDR;
        case OPC_SDL:   return MIPS_SDL;
        case OPC_SDR:   return MIPS_SDR;
        default:
            if (n64_log_verbosity < LOG_VERBOSITY_DEBUG) {
                disassemble(pc, instr.raw, buf, 50);
            }
            logfatal("Failed to decode instruction 0x%08X opcode %d%d%d%d%d%d [%s]",
                     instr.raw, instr.op0, instr.op1, instr.op2, instr.op3, instr.op4, instr.op5, buf)
    }
}

void cp0_step(cp0_t* cp0) {
    if (cp0->count < cp0->compare && cp0->count + 2 >= cp0->compare) {
        cp0->cause.ip7 = true;
        logwarn("Compare interrupt!")
    }
    if (cp0->random <= cp0->wired) {
        cp0->random = 31;
    } else {
        cp0->random--;
    }
    cp0->count += 2;
}

#define exec_instr(key, fn) case key: fn(cpu, instruction); break;

void r4300i_step(r4300i_t* cpu) {
    cp0_step(&cpu->cp0);
    dword pc = cpu->pc;
    mips_instruction_t instruction;
    instruction.raw = cpu->read_word(pc);

    byte interrupts = cpu->cp0.cause.interrupt_pending & cpu->cp0.status.im;
    if (interrupts > 0) {
        if(cpu->cp0.status.ie && !cpu->cp0.status.exl && !cpu->cp0.status.erl) {
            cpu->cp0.cause.interrupt_pending = interrupts;
            exception(cpu, cpu->pc, 0, interrupts);
            return;
        }
    }


    cpu->pc += 4;

    switch (r4300i_instruction_decode(cpu, pc, instruction)) {
        case MIPS_NOP: break;

        exec_instr(MIPS_LUI,   mips_lui)
        exec_instr(MIPS_LD,    mips_ld)
        exec_instr(MIPS_ADDI,  mips_addi)
        exec_instr(MIPS_ADDIU, mips_addiu)
        exec_instr(MIPS_DADDI, mips_daddi)
        exec_instr(MIPS_ANDI,  mips_andi)
        exec_instr(MIPS_LBU,   mips_lbu)
        exec_instr(MIPS_LHU,   mips_lhu)
        exec_instr(MIPS_LH,    mips_lh)
        exec_instr(MIPS_LW,    mips_lw)
        exec_instr(MIPS_LWU,   mips_lwu)
        exec_instr(MIPS_BEQ,   mips_beq)
        exec_instr(MIPS_BLEZ,  mips_blez)
        exec_instr(MIPS_BLEZL, mips_blezl)
        exec_instr(MIPS_BNE,   mips_bne)
        exec_instr(MIPS_BNEL,  mips_bnel)
        exec_instr(MIPS_CACHE, mips_cache)
        exec_instr(MIPS_SB,    mips_sb)
        exec_instr(MIPS_SH,    mips_sh)
        exec_instr(MIPS_SW,    mips_sw)
        exec_instr(MIPS_SD,    mips_sd)
        exec_instr(MIPS_ORI,   mips_ori)
        exec_instr(MIPS_J,     mips_j)
        exec_instr(MIPS_JAL,   mips_jal)
        exec_instr(MIPS_SLTI,  mips_slti)
        exec_instr(MIPS_SLTIU, mips_sltiu)
        exec_instr(MIPS_BEQL,  mips_beql)
        exec_instr(MIPS_BGTZ,  mips_bgtz)
        exec_instr(MIPS_BGTZL, mips_bgtzl)
        exec_instr(MIPS_XORI,  mips_xori)
        exec_instr(MIPS_LB,    mips_lb)
        exec_instr(MIPS_LDC1,  mips_ldc1)
        exec_instr(MIPS_SDC1,  mips_sdc1)
        exec_instr(MIPS_LWC1,  mips_lwc1)
        exec_instr(MIPS_SWC1,  mips_swc1)
        exec_instr(MIPS_LWL,   mips_lwl)
        exec_instr(MIPS_LWR,   mips_lwr)
        exec_instr(MIPS_SWL,   mips_swl)
        exec_instr(MIPS_SWR,   mips_swr)
        exec_instr(MIPS_LDL,   mips_ldl)
        exec_instr(MIPS_LDR,   mips_ldr)
        exec_instr(MIPS_SDL,   mips_sdl)
        exec_instr(MIPS_SDR,   mips_sdr)

        // Coprocessor
        exec_instr(MIPS_CP_MFC0, mips_mfc0)
        exec_instr(MIPS_CP_MTC0, mips_mtc0)
        exec_instr(MIPS_CP_MFC1, mips_mfc1)
        exec_instr(MIPS_CP_MTC1, mips_mtc1)

        exec_instr(MIPS_ERET,  mips_eret)
        exec_instr(MIPS_TLBWI, mips_tlbwi)
        exec_instr(MIPS_TLBP,  mips_tlbp)
        exec_instr(MIPS_TLBR,  mips_tlbr)

        exec_instr(MIPS_CP_CFC1, mips_cfc1)
        exec_instr(MIPS_CP_CTC1, mips_ctc1)

        exec_instr(MIPS_CP_BC1F,  mips_cp_bc1f)
        exec_instr(MIPS_CP_BC1T,  mips_cp_bc1t)
        exec_instr(MIPS_CP_BC1FL, mips_cp_bc1fl)
        exec_instr(MIPS_CP_BC1TL, mips_cp_bc1tl)

        exec_instr(MIPS_CP_ADD_D, mips_cp_add_d)
        exec_instr(MIPS_CP_ADD_S, mips_cp_add_s)
        exec_instr(MIPS_CP_SUB_D, mips_cp_sub_d)
        exec_instr(MIPS_CP_SUB_S, mips_cp_sub_s)
        exec_instr(MIPS_CP_MUL_D, mips_cp_mul_d)
        exec_instr(MIPS_CP_MUL_S, mips_cp_mul_s)
        exec_instr(MIPS_CP_DIV_D, mips_cp_div_d)
        exec_instr(MIPS_CP_DIV_S, mips_cp_div_s)

        exec_instr(MIPS_CP_TRUNC_L_D, mips_cp_trunc_l_d)
        exec_instr(MIPS_CP_TRUNC_L_S, mips_cp_trunc_l_s)
        exec_instr(MIPS_CP_TRUNC_W_D, mips_cp_trunc_w_d)
        exec_instr(MIPS_CP_TRUNC_W_S, mips_cp_trunc_w_s)

        exec_instr(MIPS_CP_CVT_D_S, mips_cp_cvt_d_s)
        exec_instr(MIPS_CP_CVT_D_W, mips_cp_cvt_d_w)
        exec_instr(MIPS_CP_CVT_D_L, mips_cp_cvt_d_l)

        exec_instr(MIPS_CP_CVT_L_S, mips_cp_cvt_l_s)
        exec_instr(MIPS_CP_CVT_L_D, mips_cp_cvt_l_d)

        exec_instr(MIPS_CP_CVT_S_D, mips_cp_cvt_s_d)
        exec_instr(MIPS_CP_CVT_S_W, mips_cp_cvt_s_w)
        exec_instr(MIPS_CP_CVT_S_L, mips_cp_cvt_s_l)

        exec_instr(MIPS_CP_CVT_W_S, mips_cp_cvt_w_s)
        exec_instr(MIPS_CP_CVT_W_D, mips_cp_cvt_w_d)

        exec_instr(MIPS_CP_SQRT_S, mips_cp_sqrt_s)
        exec_instr(MIPS_CP_SQRT_D, mips_cp_sqrt_d)

        exec_instr(MIPS_CP_C_F_S,    mips_cp_c_f_s)
        exec_instr(MIPS_CP_C_UN_S,   mips_cp_c_un_s)
        exec_instr(MIPS_CP_C_EQ_S,   mips_cp_c_eq_s)
        exec_instr(MIPS_CP_C_UEQ_S,  mips_cp_c_ueq_s)
        exec_instr(MIPS_CP_C_OLT_S,  mips_cp_c_olt_s)
        exec_instr(MIPS_CP_C_ULT_S,  mips_cp_c_ult_s)
        exec_instr(MIPS_CP_C_OLE_S,  mips_cp_c_ole_s)
        exec_instr(MIPS_CP_C_ULE_S,  mips_cp_c_ule_s)
        exec_instr(MIPS_CP_C_SF_S,   mips_cp_c_sf_s)
        exec_instr(MIPS_CP_C_NGLE_S, mips_cp_c_ngle_s)
        exec_instr(MIPS_CP_C_SEQ_S,  mips_cp_c_seq_s)
        exec_instr(MIPS_CP_C_NGL_S,  mips_cp_c_ngl_s)
        exec_instr(MIPS_CP_C_LT_S,   mips_cp_c_lt_s)
        exec_instr(MIPS_CP_C_NGE_S,  mips_cp_c_nge_s)
        exec_instr(MIPS_CP_C_LE_S,   mips_cp_c_le_s)
        exec_instr(MIPS_CP_C_NGT_S,  mips_cp_c_ngt_s)
        exec_instr(MIPS_CP_C_F_D,    mips_cp_c_f_d)
        exec_instr(MIPS_CP_C_UN_D,   mips_cp_c_un_d)
        exec_instr(MIPS_CP_C_EQ_D,   mips_cp_c_eq_d)
        exec_instr(MIPS_CP_C_UEQ_D,  mips_cp_c_ueq_d)
        exec_instr(MIPS_CP_C_OLT_D,  mips_cp_c_olt_d)
        exec_instr(MIPS_CP_C_ULT_D,  mips_cp_c_ult_d)
        exec_instr(MIPS_CP_C_OLE_D,  mips_cp_c_ole_d)
        exec_instr(MIPS_CP_C_ULE_D,  mips_cp_c_ule_d)
        exec_instr(MIPS_CP_C_SF_D,   mips_cp_c_sf_d)
        exec_instr(MIPS_CP_C_NGLE_D, mips_cp_c_ngle_d)
        exec_instr(MIPS_CP_C_SEQ_D,  mips_cp_c_seq_d)
        exec_instr(MIPS_CP_C_NGL_D,  mips_cp_c_ngl_d)
        exec_instr(MIPS_CP_C_LT_D,   mips_cp_c_lt_d)
        exec_instr(MIPS_CP_C_NGE_D,  mips_cp_c_nge_d)
        exec_instr(MIPS_CP_C_LE_D,   mips_cp_c_le_d)
        exec_instr(MIPS_CP_C_NGT_D,  mips_cp_c_ngt_d)
        exec_instr(MIPS_CP_MOV_S,    mips_cp_mov_s)
        exec_instr(MIPS_CP_MOV_D,    mips_cp_mov_d)
        exec_instr(MIPS_CP_NEG_S,    mips_cp_neg_s)
        exec_instr(MIPS_CP_NEG_D,    mips_cp_neg_d)

        // Special
        exec_instr(MIPS_SPC_SLL,    mips_spc_sll)
        exec_instr(MIPS_SPC_SRL,    mips_spc_srl)
        exec_instr(MIPS_SPC_SRA,    mips_spc_sra)
        exec_instr(MIPS_SPC_SRAV,   mips_spc_srav)
        exec_instr(MIPS_SPC_SLLV,   mips_spc_sllv)
        exec_instr(MIPS_SPC_SRLV,   mips_spc_srlv)
        exec_instr(MIPS_SPC_JR,     mips_spc_jr)
        exec_instr(MIPS_SPC_JALR,   mips_spc_jalr)
        exec_instr(MIPS_SPC_MFHI,   mips_spc_mfhi)
        exec_instr(MIPS_SPC_MTHI,   mips_spc_mthi)
        exec_instr(MIPS_SPC_MFLO,   mips_spc_mflo)
        exec_instr(MIPS_SPC_MTLO,   mips_spc_mtlo)
        exec_instr(MIPS_SPC_DSLLV,  mips_spc_dsllv)
        exec_instr(MIPS_SPC_MULT,   mips_spc_mult)
        exec_instr(MIPS_SPC_MULTU,  mips_spc_multu)
        exec_instr(MIPS_SPC_DIV,    mips_spc_div)
        exec_instr(MIPS_SPC_DIVU,   mips_spc_divu)
        exec_instr(MIPS_SPC_DMULTU, mips_spc_dmultu)
        exec_instr(MIPS_SPC_DDIVU,  mips_spc_ddivu)
        exec_instr(MIPS_SPC_ADD,    mips_spc_add)
        exec_instr(MIPS_SPC_ADDU,   mips_spc_addu)
        exec_instr(MIPS_SPC_AND,    mips_spc_and)
        exec_instr(MIPS_SPC_NOR,    mips_spc_nor)
        exec_instr(MIPS_SPC_SUB,    mips_spc_sub)
        exec_instr(MIPS_SPC_SUBU,   mips_spc_subu)
        exec_instr(MIPS_SPC_OR,     mips_spc_or)
        exec_instr(MIPS_SPC_XOR,    mips_spc_xor)
        exec_instr(MIPS_SPC_SLT,    mips_spc_slt)
        exec_instr(MIPS_SPC_SLTU,   mips_spc_sltu)
        exec_instr(MIPS_SPC_DADD,   mips_spc_dadd)
        exec_instr(MIPS_SPC_DSLL,   mips_spc_dsll)
        exec_instr(MIPS_SPC_DSLL32, mips_spc_dsll32)
        exec_instr(MIPS_SPC_DSRA32, mips_spc_dsra32)

        // REGIMM
        exec_instr(MIPS_RI_BLTZ,   mips_ri_bltz)
        exec_instr(MIPS_RI_BLTZL,  mips_ri_bltzl)
        exec_instr(MIPS_RI_BGEZ,   mips_ri_bgez)
        exec_instr(MIPS_RI_BGEZL,  mips_ri_bgezl)
        exec_instr(MIPS_RI_BGEZAL, mips_ri_bgezal)
        default: logfatal("Unknown instruction type!")
    }

    if (cpu->branch) {
        if (cpu->branch_delay == 0) {
            logtrace("[BRANCH DELAY] Branching to 0x%08X", cpu->branch_pc)
            cpu->pc = cpu->branch_pc;
            cpu->branch = false;
        } else {
            logtrace("[BRANCH DELAY] Need to execute %d more instruction(s).", cpu->branch_delay)
            cpu->branch_delay--;
        }
    }

}
