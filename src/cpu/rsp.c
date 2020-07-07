#include "rsp.h"
#include "../common/log.h"
#include "mips_instructions.h"
#include "rsp_instructions.h"
#include "disassemble.h"

#define exec_instr(key, fn) case key: fn(rsp, instruction); break;

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
        //case FUNCT_SLL:    return MIPS_SPC_SLL;
        //case FUNCT_SRL:    return MIPS_SPC_SRL;
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
        //case FUNCT_ADD:    return MIPS_SPC_ADD;
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
            //case OPC_LHU:   return MIPS_LHU;
            //case OPC_LH:    return MIPS_LH;
            case OPC_LW:    return MIPS_LW;
            //case OPC_LWU:   return MIPS_LWU;
            case OPC_BEQ:   return MIPS_BEQ;
            //case OPC_BEQL:  return MIPS_BEQL;
            //case OPC_BGTZ:  return MIPS_BGTZ;
            //case OPC_BLEZ:  return MIPS_BLEZ;
            case OPC_BNE:   return MIPS_BNE;
            //case OPC_BNEL:  return MIPS_BNEL;
            //case OPC_CACHE: return MIPS_CACHE;
            //case OPC_SB:    return MIPS_SB;
            //case OPC_SH:    return MIPS_SH;
            //case OPC_SW:    return MIPS_SW;
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
        exec_instr(MIPS_ORI,  rsp_ori)
        exec_instr(MIPS_ADDI, rsp_addi)
        exec_instr(MIPS_ANDI, rsp_andi)
        exec_instr(MIPS_LW,   rsp_lw)

        exec_instr(MIPS_J,      rsp_j)
        exec_instr(MIPS_JAL,    rsp_jal)
        exec_instr(MIPS_SPC_JR, rsp_spc_jr)

        exec_instr(MIPS_BNE, rsp_bne)
        exec_instr(MIPS_BEQ, rsp_beq)

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
