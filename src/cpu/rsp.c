#include "rsp.h"
#include "mips_instructions.h"
#include "rsp_instructions.h"
#include "rsp_vector_instructions.h"
#include "disassemble.h"

#define exec_instr(key, fn) case key: fn(rsp, instruction); break;

bool rsp_acquire_semaphore(n64_system_t* system) {
    if (system->rsp.semaphore_held) {
        return false; // Semaphore is already held
    } else {
        system->rsp.semaphore_held = true;
        return true; // Acquired semaphore.
    }
}

mips_instruction_type_t rsp_cp0_decode(rsp_t* rsp, word pc, mips_instruction_t instr) {
    if (instr.last11 == 0) {
        switch (instr.r.rs) {
            case COP_MT: return MIPS_CP_MTC0;
            case COP_MF: return MIPS_CP_MFC0;
            default: {
                char buf[50];
                disassemble(pc, instr.raw, buf, 50);
                logfatal("other/unknown MIPS RSP CP0 0x%08X with rs: %d%d%d%d%d [%s]", instr.raw,
                         instr.rs0, instr.rs1, instr.rs2, instr.rs3, instr.rs4, buf)
            }
        }
    } else {
        switch (instr.fr.funct) {
            default: {
                char buf[50];
                disassemble(pc, instr.raw, buf, 50);
                logfatal("other/unknown MIPS RSP CP0 0x%08X with FUNCT: %d%d%d%d%d%d [%s]", instr.raw,
                         instr.funct0, instr.funct1, instr.funct2, instr.funct3, instr.funct4, instr.funct5, buf)
            }
        }
    }
}

mips_instruction_type_t rsp_special_decode(rsp_t* rsp, word pc, mips_instruction_t instr) {
    switch (instr.r.funct) {
        case FUNCT_SLL:    return MIPS_SPC_SLL;
        case FUNCT_SRL:    return MIPS_SPC_SRL;
        //case FUNCT_SRA:    return MIPS_SPC_SRA;
        //case FUNCT_SRAV:   return MIPS_SPC_SRAV;
        //case FUNCT_SLLV:   return MIPS_SPC_SLLV;
        //case FUNCT_SRLV:   return MIPS_SPC_SRLV;
        case FUNCT_JR:     return MIPS_SPC_JR;
        //case FUNCT_JALR:   return MIPS_SPC_JALR;
        //case FUNCT_MULT:   return MIPS_SPC_MULT;
        //case FUNCT_MULTU:  return MIPS_SPC_MULTU;
        //case FUNCT_DIV:    return MIPS_SPC_DIV;
        //case FUNCT_DIVU:   return MIPS_SPC_DIVU;
        case FUNCT_ADD:    return MIPS_SPC_ADD;
        //case FUNCT_ADDU:   return MIPS_SPC_ADDU;
        //case FUNCT_AND:    return MIPS_SPC_AND;
        //case FUNCT_NOR:    return MIPS_SPC_NOR;
        //case FUNCT_SUB:    return MIPS_SPC_SUB;
        //case FUNCT_SUBU:   return MIPS_SPC_SUBU;
        //case FUNCT_OR:     return MIPS_SPC_OR;
        //case FUNCT_XOR:    return MIPS_SPC_XOR;
        //case FUNCT_SLT:    return MIPS_SPC_SLT;
        //case FUNCT_SLTU:   return MIPS_SPC_SLTU;
        default: {
            char buf[50];
            disassemble(pc, instr.raw, buf, 50);
            logfatal("other/unknown MIPS RSP Special 0x%08X with FUNCT: %d%d%d%d%d%d [%s]", instr.raw,
                     instr.funct0, instr.funct1, instr.funct2, instr.funct3, instr.funct4, instr.funct5, buf)
        }
    }
}

mips_instruction_type_t rsp_lwc2_decode(rsp_t* rsp, word pc, mips_instruction_t instr) {
    switch (instr.v.funct) {
        case LWC2_LBV: return RSP_LWC2_LBV;
        case LWC2_LDV: return RSP_LWC2_LDV;
        case LWC2_LFV: return RSP_LWC2_LFV;
        case LWC2_LHV: return RSP_LWC2_LHV;
        case LWC2_LLV: return RSP_LWC2_LLV;
        case LWC2_LPV: return RSP_LWC2_LPV;
        case LWC2_LQV: return RSP_LWC2_LQV;
        case LWC2_LRV: return RSP_LWC2_LRV;
        case LWC2_LSV: return RSP_LWC2_LSV;
        case LWC2_LTV: return RSP_LWC2_LTV;
        case LWC2_LUV: return RSP_LWC2_LUV;
        default:
            logfatal("other/unknown MIPS RSP LWC2 with funct: 0x%02X", instr.v.funct)
    }
}

mips_instruction_type_t rsp_swc2_decode(rsp_t* rsp, word pc, mips_instruction_t instr) {
    switch (instr.v.funct) {
        case LWC2_LBV: return RSP_SWC2_SBV;
        case LWC2_LDV: return RSP_SWC2_SDV;
        case LWC2_LFV: return RSP_SWC2_SFV;
        case LWC2_LHV: return RSP_SWC2_SHV;
        case LWC2_LLV: return RSP_SWC2_SLV;
        case LWC2_LPV: return RSP_SWC2_SPV;
        case LWC2_LQV: return RSP_SWC2_SQV;
        case LWC2_LRV: return RSP_SWC2_SRV;
        case LWC2_LSV: return RSP_SWC2_SSV;
        case LWC2_LTV: return RSP_SWC2_STV;
        case LWC2_LUV: return RSP_SWC2_SUV;
        default:
            logfatal("other/unknown MIPS RSP LWC2 with funct: 0x%02X", instr.v.funct)
    }
}

mips_instruction_type_t rsp_instruction_decode(rsp_t* rsp, word pc, mips_instruction_t instr) {
        char buf[50];
        if (n64_log_verbosity >= LOG_VERBOSITY_DEBUG) {
            disassemble(pc, instr.raw, buf, 50);
            logdebug("RSP [0x%08X]=0x%08X %s", pc, instr.raw, buf)
        }
        if (instr.raw == 0) {
            return MIPS_NOP;
        }
        switch (instr.op) {
            //case OPC_LUI:   return MIPS_LUI;
            //case OPC_ADDIU: return MIPS_ADDIU;
            case OPC_ADDI:  return MIPS_ADDI;
            case OPC_ANDI:  return MIPS_ANDI;
            //case OPC_LBU:   return MIPS_LBU;
            case OPC_LHU:   return MIPS_LHU;
            case OPC_LH:    return MIPS_LH;
            case OPC_LW:    return MIPS_LW;
            //case OPC_LWU:   return MIPS_LWU;
            case OPC_BEQ:   return MIPS_BEQ;
            //case OPC_BEQL:  return MIPS_BEQL;
            case OPC_BGTZ:  return MIPS_BGTZ;
            case OPC_BLEZ:  return MIPS_BLEZ;
            case OPC_BNE:   return MIPS_BNE;
            //case OPC_BNEL:  return MIPS_BNEL;
            //case OPC_CACHE: return MIPS_CACHE;
            //case OPC_SB:    return MIPS_SB;
            case OPC_SH:    return MIPS_SH;
            case OPC_SW:    return MIPS_SW;
            case OPC_ORI:   return MIPS_ORI;
            case OPC_J:     return MIPS_J;
            case OPC_JAL:   return MIPS_JAL;
            //case OPC_SLTI:  return MIPS_SLTI;
            //case OPC_SLTIU: return MIPS_SLTIU;
            //case OPC_XORI:  return MIPS_XORI;
            //case OPC_LB:    return MIPS_LB;
            //case OPC_LWL:   return MIPS_LWL;
            //case OPC_LWR:   return MIPS_LWR;
            //case OPC_SWL:   return MIPS_SWL;
            //case OPC_SWR:   return MIPS_SWR;

            case OPC_CP0:    return rsp_cp0_decode(rsp, pc, instr);
            case OPC_CP1:    logfatal("Decoding RSP CP1 instruction!")     //return rsp_cp1_decode(rsp, pc, instr);
            case OPC_SPCL:   return rsp_special_decode(rsp, pc, instr);
            case OPC_REGIMM: logfatal("Decoding RSP REGIMM instruction!")  //return rsp_regimm_decode(rsp, pc, instr);
            case RSP_OPC_LWC2: return rsp_lwc2_decode(rsp, pc, instr);
            case RSP_OPC_SWC2: return rsp_swc2_decode(rsp, pc, instr);

            default:
                if (n64_log_verbosity < LOG_VERBOSITY_DEBUG) {
                    disassemble(pc, instr.raw, buf, 50);
                }
                logfatal("[RSP] Failed to decode instruction 0x%08X opcode %d%d%d%d%d%d [%s]",
                         instr.raw, instr.op0, instr.op1, instr.op2, instr.op3, instr.op4, instr.op5, buf)
        }
}

void rsp_step(n64_system_t* system) {
    rsp_t* rsp = &system->rsp;
    dword pc = rsp->pc & 0xFFFFFF;
    mips_instruction_t instruction;
    instruction.raw = rsp->read_word(pc);

    rsp->pc += 4;

    switch (rsp_instruction_decode(rsp, pc, instruction)) {
        case MIPS_NOP: break;
        exec_instr(MIPS_ORI,     rsp_ori)
        exec_instr(MIPS_ADDI,    rsp_addi)
        exec_instr(MIPS_SPC_ADD, rsp_spc_add)
        exec_instr(MIPS_ANDI,    rsp_andi)
        exec_instr(MIPS_SH,      rsp_sh)
        exec_instr(MIPS_SW,      rsp_sw)
        exec_instr(MIPS_LHU,     rsp_lhu)
        exec_instr(MIPS_LH,      rsp_lh)
        exec_instr(MIPS_LW,      rsp_lw)

        exec_instr(MIPS_J,      rsp_j)
        exec_instr(MIPS_JAL,    rsp_jal)

        exec_instr(MIPS_SPC_JR,  rsp_spc_jr)
        exec_instr(MIPS_SPC_SLL, rsp_spc_sll)
        exec_instr(MIPS_SPC_SRL, rsp_spc_srl)

        exec_instr(MIPS_BNE,  rsp_bne)
        exec_instr(MIPS_BEQ,  rsp_beq)
        exec_instr(MIPS_BGTZ, rsp_bgtz)
        exec_instr(MIPS_BLEZ, rsp_blez)

        exec_instr(RSP_LWC2_LBV, rsp_lwc2_lbv)
        exec_instr(RSP_LWC2_LDV, rsp_lwc2_ldv)
        exec_instr(RSP_LWC2_LFV, rsp_lwc2_lfv)
        exec_instr(RSP_LWC2_LHV, rsp_lwc2_lhv)
        exec_instr(RSP_LWC2_LLV, rsp_lwc2_llv)
        exec_instr(RSP_LWC2_LPV, rsp_lwc2_lpv)
        exec_instr(RSP_LWC2_LQV, rsp_lwc2_lqv)
        exec_instr(RSP_LWC2_LRV, rsp_lwc2_lrv)
        exec_instr(RSP_LWC2_LSV, rsp_lwc2_lsv)
        exec_instr(RSP_LWC2_LTV, rsp_lwc2_ltv)
        exec_instr(RSP_LWC2_LUV, rsp_lwc2_luv)

        exec_instr(RSP_SWC2_SBV, rsp_swc2_sbv)
        exec_instr(RSP_SWC2_SDV, rsp_swc2_sdv)
        exec_instr(RSP_SWC2_SFV, rsp_swc2_sfv)
        exec_instr(RSP_SWC2_SHV, rsp_swc2_shv)
        exec_instr(RSP_SWC2_SLV, rsp_swc2_slv)
        exec_instr(RSP_SWC2_SPV, rsp_swc2_spv)
        exec_instr(RSP_SWC2_SQV, rsp_swc2_sqv)
        exec_instr(RSP_SWC2_SRV, rsp_swc2_srv)
        exec_instr(RSP_SWC2_SSV, rsp_swc2_ssv)
        exec_instr(RSP_SWC2_STV, rsp_swc2_stv)
        exec_instr(RSP_SWC2_SUV, rsp_swc2_suv)

        case MIPS_CP_MTC0: rsp_mtc0(system, instruction); break;
        case MIPS_CP_MFC0: rsp_mfc0(system, instruction); break;
        default:
            logfatal("[RSP] Unknown instruction!")
    }

    if (rsp->branch) {
        if (rsp->branch_delay == 0) {
            logtrace("[RSP] [BRANCH DELAY] Branching to 0x%08X", rsp->branch_pc)
            rsp->pc = rsp->branch_pc;
            rsp->branch = false;
        } else {
            logtrace("[RSP] [BRANCH DELAY] Need to execute %d more instruction(s).", rsp->branch_delay)
            rsp->branch_delay--;
        }
    }
}
