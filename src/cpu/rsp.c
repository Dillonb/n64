#include "rsp.h"
#include "mips_instructions.h"
#include "rsp_instructions.h"
#include "rsp_vector_instructions.h"
#include "disassemble.h"

#include <mem/mem_util.h>

#define exec_instr(key, fn) case key: fn(rsp, cache.instruction); break;

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
                         instr.rs0, instr.rs1, instr.rs2, instr.rs3, instr.rs4, buf);
            }
        }
    } else {
        switch (instr.fr.funct) {
            default: {
                char buf[50];
                disassemble(pc, instr.raw, buf, 50);
                logfatal("other/unknown MIPS RSP CP0 0x%08X with FUNCT: %d%d%d%d%d%d [%s]", instr.raw,
                         instr.funct0, instr.funct1, instr.funct2, instr.funct3, instr.funct4, instr.funct5, buf);
            }
        }
    }
}

mips_instruction_type_t rsp_cp2_decode(rsp_t* rsp, word pc, mips_instruction_t instr) {
    if (instr.cp2_vec.is_vec) {
        switch (instr.cp2_vec.funct) {
            case FUNCT_RSP_VEC_VABS:  return RSP_VEC_VABS;
            case FUNCT_RSP_VEC_VADD:  return RSP_VEC_VADD;
            case FUNCT_RSP_VEC_VADDC: return RSP_VEC_VADDC;
            case FUNCT_RSP_VEC_VAND:  return RSP_VEC_VAND;
            case FUNCT_RSP_VEC_VCH:   return RSP_VEC_VCH;
            case FUNCT_RSP_VEC_VCL:   return RSP_VEC_VCL;
            case FUNCT_RSP_VEC_VCR:   return RSP_VEC_VCR;
            case FUNCT_RSP_VEC_VEQ:   return RSP_VEC_VEQ;
            case FUNCT_RSP_VEC_VGE:   return RSP_VEC_VGE;
            case FUNCT_RSP_VEC_VLT:   return RSP_VEC_VLT;
            case FUNCT_RSP_VEC_VMACF: return RSP_VEC_VMACF;
            case FUNCT_RSP_VEC_VMACQ: return RSP_VEC_VMACQ;
            case FUNCT_RSP_VEC_VMACU: return RSP_VEC_VMACU;
            case FUNCT_RSP_VEC_VMADH: return RSP_VEC_VMADH;
            case FUNCT_RSP_VEC_VMADL: return RSP_VEC_VMADL;
            case FUNCT_RSP_VEC_VMADM: return RSP_VEC_VMADM;
            case FUNCT_RSP_VEC_VMADN: return RSP_VEC_VMADN;
            case FUNCT_RSP_VEC_VMOV:  return RSP_VEC_VMOV;
            case FUNCT_RSP_VEC_VMRG:  return RSP_VEC_VMRG;
            case FUNCT_RSP_VEC_VMUDH: return RSP_VEC_VMUDH;
            case FUNCT_RSP_VEC_VMUDL: return RSP_VEC_VMUDL;
            case FUNCT_RSP_VEC_VMUDM: return RSP_VEC_VMUDM;
            case FUNCT_RSP_VEC_VMUDN: return RSP_VEC_VMUDN;
            case FUNCT_RSP_VEC_VMULF: return RSP_VEC_VMULF;
            case FUNCT_RSP_VEC_VMULQ: return RSP_VEC_VMULQ;
            case FUNCT_RSP_VEC_VMULU: return RSP_VEC_VMULU;
            case FUNCT_RSP_VEC_VNAND: return RSP_VEC_VNAND;
            case FUNCT_RSP_VEC_VNE:   return RSP_VEC_VNE;
            case FUNCT_RSP_VEC_VNOP:  return RSP_VEC_VNOP;
            case FUNCT_RSP_VEC_VNOR:  return RSP_VEC_VNOR;
            case FUNCT_RSP_VEC_VNXOR: return RSP_VEC_VNXOR;
            case FUNCT_RSP_VEC_VOR :  return RSP_VEC_VOR;
            case FUNCT_RSP_VEC_VRCP:  return RSP_VEC_VRCP;
            case FUNCT_RSP_VEC_VRCPH: return RSP_VEC_VRCPH;
            case FUNCT_RSP_VEC_VRCPL: return RSP_VEC_VRCPL;
            case FUNCT_RSP_VEC_VRNDN: return RSP_VEC_VRNDN;
            case FUNCT_RSP_VEC_VRNDP: return RSP_VEC_VRNDP;
            case FUNCT_RSP_VEC_VRSQ:  return RSP_VEC_VRSQ;
            case FUNCT_RSP_VEC_VRSQH: return RSP_VEC_VRSQH;
            case FUNCT_RSP_VEC_VRSQL: return RSP_VEC_VRSQL;
            case FUNCT_RSP_VEC_VSAR:  return RSP_VEC_VSAR;
            case FUNCT_RSP_VEC_VSUB:  return RSP_VEC_VSUB;
            case FUNCT_RSP_VEC_VSUBC: return RSP_VEC_VSUBC;
            case FUNCT_RSP_VEC_VXOR:  return RSP_VEC_VXOR;
            default: {
                char buf[50];
                disassemble(pc, instr.raw, buf, 50);
                logfatal("Invalid RSP CP2 VEC [0x%08X]=0x%08X | Capstone thinks it's %s", pc, instr.raw, buf);
            }
        }
    } else {
        switch (instr.cp2_regmove.funct) {
            case COP_CF: return RSP_CFC2;
            case COP_CT: return RSP_CTC2;
            case COP_MF: return RSP_MFC2;
            case COP_MT: return RSP_MTC2;
            default: {
                char buf[50];
                disassemble(pc, instr.raw, buf, 50);
                logfatal("Invalid RSP CP2 regmove instruction! [0x%08x]=0x%08x | Capstone thinks it's %s", pc, instr.raw, buf);
            }
        }
    }
}

mips_instruction_type_t rsp_special_decode(rsp_t* rsp, word pc, mips_instruction_t instr) {
    switch (instr.r.funct) {
        case FUNCT_SLL:    return MIPS_SPC_SLL;
        case FUNCT_SRL:    return MIPS_SPC_SRL;
        case FUNCT_SRA:    return MIPS_SPC_SRA;
        //case FUNCT_SRAV:   return MIPS_SPC_SRAV;
        case FUNCT_SLLV:   return MIPS_SPC_SLLV;
        case FUNCT_SRLV:   return MIPS_SPC_SRLV;
        case FUNCT_JR:     return MIPS_SPC_JR;
        case FUNCT_JALR:   return MIPS_SPC_JALR;
        //case FUNCT_MULT:   return MIPS_SPC_MULT;
        //case FUNCT_MULTU:  return MIPS_SPC_MULTU;
        //case FUNCT_DIV:    return MIPS_SPC_DIV;
        //case FUNCT_DIVU:   return MIPS_SPC_DIVU;
        case FUNCT_ADD:    return MIPS_SPC_ADD;
        case FUNCT_ADDU:   return MIPS_SPC_ADD;
        case FUNCT_AND:    return MIPS_SPC_AND;
        case FUNCT_SUB:    return MIPS_SPC_SUB;
        //case FUNCT_SUBU:   return MIPS_SPC_SUBU;
        case FUNCT_OR:     return MIPS_SPC_OR;
        case FUNCT_XOR:    return MIPS_SPC_XOR;
        case FUNCT_NOR:    return MIPS_SPC_NOR;
        case FUNCT_SLT:    return MIPS_SPC_SLT;
        //case FUNCT_SLTU:   return MIPS_SPC_SLTU;

        case FUNCT_BREAK: return MIPS_SPC_BREAK;
        default: {
            char buf[50];
            disassemble(pc, instr.raw, buf, 50);
            logfatal("other/unknown MIPS RSP Special 0x%08X with FUNCT: %d%d%d%d%d%d [%s]", instr.raw,
                     instr.funct0, instr.funct1, instr.funct2, instr.funct3, instr.funct4, instr.funct5, buf);
        }
    }
}

mips_instruction_type_t rsp_regimm_decode(rsp_t* cpu, word pc, mips_instruction_t instr) {
    switch (instr.i.rt) {
        case RT_BLTZ:   return MIPS_RI_BLTZ;
        case RT_BGEZ:   return MIPS_RI_BGEZ;
        //case RT_BGEZAL: return MIPS_RI_BGEZAL;
        default: {
            char buf[50];
            disassemble(pc, instr.raw, buf, 50);
            logfatal("other/unknown RSP REGIMM 0x%08X with RT: %d%d%d%d%d [%s]", instr.raw,
                     instr.rt0, instr.rt1, instr.rt2, instr.rt3, instr.rt4, buf);
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
            logfatal("other/unknown MIPS RSP LWC2 with funct: 0x%02X", instr.v.funct);
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
            logfatal("other/unknown MIPS RSP SWC2 with funct: 0x%02X", instr.v.funct);
    }
}

mips_instruction_type_t rsp_instruction_decode(rsp_t* rsp, word pc, mips_instruction_t instr) {
        char buf[50];
        if (n64_log_verbosity >= LOG_VERBOSITY_DEBUG) {
            disassemble(pc, instr.raw, buf, 50);
            logdebug("RSP [0x%08X]=0x%08X %s", pc, instr.raw, buf);
        }
        if (instr.raw == 0) {
            return MIPS_NOP;
        }
        switch (instr.op) {
            case OPC_LUI:   return MIPS_LUI;
            case OPC_ADDIU: return MIPS_ADDI;
            case OPC_ADDI:  return MIPS_ADDI;
            case OPC_ANDI:  return MIPS_ANDI;
            case OPC_LBU:   return MIPS_LBU;
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
            case OPC_SB:    return MIPS_SB;
            case OPC_SH:    return MIPS_SH;
            case OPC_SW:    return MIPS_SW;
            case OPC_ORI:   return MIPS_ORI;
            case OPC_J:     return MIPS_J;
            case OPC_JAL:   return MIPS_JAL;
            //case OPC_SLTI:  return MIPS_SLTI;
            //case OPC_SLTIU: return MIPS_SLTIU;
            case OPC_XORI:  return MIPS_XORI;
            case OPC_LB:    return MIPS_LB;
            //case OPC_LWL:   return MIPS_LWL;
            //case OPC_LWR:   return MIPS_LWR;
            //case OPC_SWL:   return MIPS_SWL;
            //case OPC_SWR:   return MIPS_SWR;

            case OPC_CP0:      return rsp_cp0_decode(rsp, pc, instr);
            case OPC_CP1:      logfatal("Decoding RSP CP1 instruction!");     //return rsp_cp1_decode(rsp, pc, instr);
            case OPC_CP2:      return rsp_cp2_decode(rsp, pc, instr);
            case OPC_SPCL:     return rsp_special_decode(rsp, pc, instr);
            case OPC_REGIMM:   return rsp_regimm_decode(rsp, pc, instr);
            case RSP_OPC_LWC2: return rsp_lwc2_decode(rsp, pc, instr);
            case RSP_OPC_SWC2: return rsp_swc2_decode(rsp, pc, instr);

            default:
                if (n64_log_verbosity < LOG_VERBOSITY_DEBUG) {
                    disassemble(pc, instr.raw, buf, 50);
                }
                logfatal("[RSP] Failed to decode instruction 0x%08X opcode %d%d%d%d%d%d [%s]",
                         instr.raw, instr.op0, instr.op1, instr.op2, instr.op3, instr.op4, instr.op5, buf);
        }
}

void rsp_step(n64_system_t* system) {
    rsp_t* rsp = &system->rsp;
    word pc = rsp->pc & 0xFFF;
    if (pc % 4 != 0) {
        logfatal("RSP PC at misaligned address!");
    }
    rsp_icache_entry_t cache = system->rsp.icache[pc / 4];

    if (cache.type == MIPS_UNKNOWN) {
        // RSP can only read from IMEM.
        cache.instruction.raw = word_from_byte_array((byte*) &system->mem.sp_imem, pc);
        cache.type = rsp_instruction_decode(rsp, pc, cache.instruction);
        system->rsp.icache[pc / 4] = cache;

    }

    rsp->pc += 4;

    switch (cache.type) {
        case MIPS_NOP: break;
        exec_instr(MIPS_ORI,     rsp_ori)
        exec_instr(MIPS_XORI,    rsp_xori)
        exec_instr(MIPS_LUI,     rsp_lui)
        exec_instr(MIPS_ADDI,    rsp_addi)
        exec_instr(MIPS_SPC_ADD, rsp_spc_add)
        exec_instr(MIPS_SPC_AND, rsp_spc_and)
        exec_instr(MIPS_ANDI,    rsp_andi)
        exec_instr(MIPS_LB,      rsp_lb)
        exec_instr(MIPS_LBU,     rsp_lbu)
        exec_instr(MIPS_SB,      rsp_sb)
        exec_instr(MIPS_SH,      rsp_sh)
        exec_instr(MIPS_SW,      rsp_sw)
        exec_instr(MIPS_LHU,     rsp_lhu)
        exec_instr(MIPS_LH,      rsp_lh)
        exec_instr(MIPS_LW,      rsp_lw)

        exec_instr(MIPS_J,      rsp_j)
        exec_instr(MIPS_JAL,    rsp_jal)

        exec_instr(MIPS_SPC_JR,   rsp_spc_jr)
        exec_instr(MIPS_SPC_JALR, rsp_spc_jalr)
        exec_instr(MIPS_SPC_SLL,  rsp_spc_sll)
        exec_instr(MIPS_SPC_SRL,  rsp_spc_srl)
        exec_instr(MIPS_SPC_SRA,  rsp_spc_sra)
        exec_instr(MIPS_SPC_SLLV, rsp_spc_sllv)
        exec_instr(MIPS_SPC_SRLV, rsp_spc_srlv)
        exec_instr(MIPS_SPC_SUB,  rsp_spc_sub)
        exec_instr(MIPS_SPC_OR,   rsp_spc_or)
        exec_instr(MIPS_SPC_XOR,  rsp_spc_xor)
        exec_instr(MIPS_SPC_NOR,  rsp_spc_nor)
        exec_instr(MIPS_SPC_SLT,  rsp_spc_slt)

        case MIPS_SPC_BREAK: rsp_spc_break(system, cache.instruction);

        exec_instr(MIPS_BNE,  rsp_bne)
        exec_instr(MIPS_BEQ,  rsp_beq)
        exec_instr(MIPS_BGTZ, rsp_bgtz)
        exec_instr(MIPS_BLEZ, rsp_blez)

        exec_instr(MIPS_RI_BLTZ,   rsp_ri_bltz)
        exec_instr(MIPS_RI_BGEZ,   rsp_ri_bgez)
        exec_instr(MIPS_RI_BGEZAL, rsp_ri_bgezal)

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
        exec_instr(RSP_CFC2, rsp_cfc2)
        exec_instr(RSP_CTC2, rsp_ctc2)
        exec_instr(RSP_MFC2, rsp_mfc2)
        exec_instr(RSP_MTC2, rsp_mtc2)

        exec_instr(RSP_VEC_VABS,  rsp_vec_vabs)
        exec_instr(RSP_VEC_VADD,  rsp_vec_vadd)
        exec_instr(RSP_VEC_VADDC, rsp_vec_vaddc)
        exec_instr(RSP_VEC_VAND,  rsp_vec_vand)
        exec_instr(RSP_VEC_VCH,   rsp_vec_vch)
        exec_instr(RSP_VEC_VCL,   rsp_vec_vcl)
        exec_instr(RSP_VEC_VCR,   rsp_vec_vcr)
        exec_instr(RSP_VEC_VEQ,   rsp_vec_veq)
        exec_instr(RSP_VEC_VGE,   rsp_vec_vge)
        exec_instr(RSP_VEC_VLT,   rsp_vec_vlt)
        exec_instr(RSP_VEC_VMACF, rsp_vec_vmacf)
        exec_instr(RSP_VEC_VMACQ, rsp_vec_vmacq)
        exec_instr(RSP_VEC_VMACU, rsp_vec_vmacu)
        exec_instr(RSP_VEC_VMADH, rsp_vec_vmadh)
        exec_instr(RSP_VEC_VMADL, rsp_vec_vmadl)
        exec_instr(RSP_VEC_VMADM, rsp_vec_vmadm)
        exec_instr(RSP_VEC_VMADN, rsp_vec_vmadn)
        exec_instr(RSP_VEC_VMOV,  rsp_vec_vmov)
        exec_instr(RSP_VEC_VMRG,  rsp_vec_vmrg)
        exec_instr(RSP_VEC_VMUDH, rsp_vec_vmudh)
        exec_instr(RSP_VEC_VMUDL, rsp_vec_vmudl)
        exec_instr(RSP_VEC_VMUDM, rsp_vec_vmudm)
        exec_instr(RSP_VEC_VMUDN, rsp_vec_vmudn)
        exec_instr(RSP_VEC_VMULF, rsp_vec_vmulf)
        exec_instr(RSP_VEC_VMULQ, rsp_vec_vmulq)
        exec_instr(RSP_VEC_VMULU, rsp_vec_vmulu)
        exec_instr(RSP_VEC_VNAND, rsp_vec_vnand)
        exec_instr(RSP_VEC_VNE,   rsp_vec_vne)
        exec_instr(RSP_VEC_VNOP,  rsp_vec_vnop)
        exec_instr(RSP_VEC_VNOR,  rsp_vec_vnor)
        exec_instr(RSP_VEC_VNXOR, rsp_vec_vnxor)
        exec_instr(RSP_VEC_VOR,   rsp_vec_vor)
        exec_instr(RSP_VEC_VRCP,  rsp_vec_vrcp)
        exec_instr(RSP_VEC_VRCPH, rsp_vec_vrcph_vrsqh)
        exec_instr(RSP_VEC_VRCPL, rsp_vec_vrcpl)
        exec_instr(RSP_VEC_VRNDN, rsp_vec_vrndn)
        exec_instr(RSP_VEC_VRNDP, rsp_vec_vrndp)
        exec_instr(RSP_VEC_VRSQ,  rsp_vec_vrsq)
        exec_instr(RSP_VEC_VRSQH, rsp_vec_vrcph_vrsqh)
        exec_instr(RSP_VEC_VRSQL, rsp_vec_vrsql)
        exec_instr(RSP_VEC_VSAR,  rsp_vec_vsar)
        exec_instr(RSP_VEC_VSUB,  rsp_vec_vsub)
        exec_instr(RSP_VEC_VSUBC, rsp_vec_vsubc)
        exec_instr(RSP_VEC_VXOR,  rsp_vec_vxor)

        case MIPS_CP_MTC0: rsp_mtc0(system, cache.instruction); break;
        case MIPS_CP_MFC0: rsp_mfc0(system, cache.instruction); break;
        default:
            logfatal("[RSP] Unknown instruction!");
    }

    if (rsp->branch) {
        if (rsp->branch_delay == 0) {
            logtrace("[RSP] [BRANCH DELAY] Branching to 0x%08X", rsp->branch_pc);
            rsp->pc = rsp->branch_pc;
            rsp->branch = false;
        } else {
            logtrace("[RSP] [BRANCH DELAY] Need to execute %d more instruction(s).", rsp->branch_delay);
            rsp->branch_delay--;
        }
    }
}
