#include "rsp.h"
#include "mips_instructions.h"
#include "rsp_instructions.h"
#include "rsp_vector_instructions.h"
#include "disassemble.h"

#include <mem/mem_util.h>

bool rsp_acquire_semaphore(n64_system_t* system) {
    if (system->rsp.semaphore_held) {
        return false; // Semaphore is already held
    } else {
        system->rsp.semaphore_held = true;
        return true; // Acquired semaphore.
    }
}

INLINE rspinstr_handler_t rsp_cp0_decode(rsp_t* rsp, word pc, mips_instruction_t instr) {
    if (instr.last11 == 0) {
        switch (instr.r.rs) {
            case COP_MT: return rsp_mtc0;
            case COP_MF: return rsp_mfc0;
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

#define DECODE_RSP_VECTOR_INSTR_VTE(NAME, E) switch((E)) { \
case 0: return NAME##_0;                                    \
case 1: return NAME##_1;                                    \
case 2: return NAME##_2;                                    \
case 3: return NAME##_3;                                    \
case 4: return NAME##_4;                                    \
case 5: return NAME##_5;                                    \
case 6: return NAME##_6;                                    \
case 7: return NAME##_7;                                    \
case 8: return NAME##_8;                                    \
case 9: return NAME##_9;                                    \
case 10: return NAME##_10;                                    \
case 11: return NAME##_11;                                    \
case 12: return NAME##_12;                                    \
case 13: return NAME##_13;                                    \
case 14: return NAME##_14;                                    \
case 15: return NAME##_15;                                    \
}

INLINE rspinstr_handler_t rsp_cp2_decode(rsp_t* rsp, word pc, mips_instruction_t instr) {
    if (instr.cp2_vec.is_vec) {
        switch (instr.cp2_vec.funct) {
            case FUNCT_RSP_VEC_VABS:  DECODE_RSP_VECTOR_INSTR_VTE(rsp_vec_vabs, instr.cp2_vec.e);
            case FUNCT_RSP_VEC_VADD:  DECODE_RSP_VECTOR_INSTR_VTE(rsp_vec_vadd, instr.cp2_vec.e);
            case FUNCT_RSP_VEC_VADDC: DECODE_RSP_VECTOR_INSTR_VTE(rsp_vec_vaddc, instr.cp2_vec.e);
            case FUNCT_RSP_VEC_VAND:  DECODE_RSP_VECTOR_INSTR_VTE(rsp_vec_vand, instr.cp2_vec.e);
            case FUNCT_RSP_VEC_VCH:   DECODE_RSP_VECTOR_INSTR_VTE(rsp_vec_vch, instr.cp2_vec.e);
            case FUNCT_RSP_VEC_VCL:   DECODE_RSP_VECTOR_INSTR_VTE(rsp_vec_vcl, instr.cp2_vec.e);
            case FUNCT_RSP_VEC_VCR:   DECODE_RSP_VECTOR_INSTR_VTE(rsp_vec_vcr, instr.cp2_vec.e);
            case FUNCT_RSP_VEC_VEQ:   DECODE_RSP_VECTOR_INSTR_VTE(rsp_vec_veq, instr.cp2_vec.e);
            case FUNCT_RSP_VEC_VGE:   DECODE_RSP_VECTOR_INSTR_VTE(rsp_vec_vge, instr.cp2_vec.e);
            case FUNCT_RSP_VEC_VLT:   DECODE_RSP_VECTOR_INSTR_VTE(rsp_vec_vlt, instr.cp2_vec.e);
            case FUNCT_RSP_VEC_VMACF: DECODE_RSP_VECTOR_INSTR_VTE(rsp_vec_vmacf, instr.cp2_vec.e);
            case FUNCT_RSP_VEC_VMACQ: DECODE_RSP_VECTOR_INSTR_VTE(rsp_vec_vmacq, instr.cp2_vec.e);
            case FUNCT_RSP_VEC_VMACU: DECODE_RSP_VECTOR_INSTR_VTE(rsp_vec_vmacu, instr.cp2_vec.e);
            case FUNCT_RSP_VEC_VMADH: DECODE_RSP_VECTOR_INSTR_VTE(rsp_vec_vmadh, instr.cp2_vec.e);
            case FUNCT_RSP_VEC_VMADL: DECODE_RSP_VECTOR_INSTR_VTE(rsp_vec_vmadl, instr.cp2_vec.e);
            case FUNCT_RSP_VEC_VMADM: DECODE_RSP_VECTOR_INSTR_VTE(rsp_vec_vmadm, instr.cp2_vec.e);
            case FUNCT_RSP_VEC_VMADN: DECODE_RSP_VECTOR_INSTR_VTE(rsp_vec_vmadn, instr.cp2_vec.e);
            case FUNCT_RSP_VEC_VMOV:  DECODE_RSP_VECTOR_INSTR_VTE(rsp_vec_vmov, instr.cp2_vec.e);
            case FUNCT_RSP_VEC_VMRG:  DECODE_RSP_VECTOR_INSTR_VTE(rsp_vec_vmrg, instr.cp2_vec.e);
            case FUNCT_RSP_VEC_VMUDH: DECODE_RSP_VECTOR_INSTR_VTE(rsp_vec_vmudh, instr.cp2_vec.e);
            case FUNCT_RSP_VEC_VMUDL: DECODE_RSP_VECTOR_INSTR_VTE(rsp_vec_vmudl, instr.cp2_vec.e);
            case FUNCT_RSP_VEC_VMUDM: DECODE_RSP_VECTOR_INSTR_VTE(rsp_vec_vmudm, instr.cp2_vec.e);
            case FUNCT_RSP_VEC_VMUDN: DECODE_RSP_VECTOR_INSTR_VTE(rsp_vec_vmudn, instr.cp2_vec.e);
            case FUNCT_RSP_VEC_VMULF: DECODE_RSP_VECTOR_INSTR_VTE(rsp_vec_vmulf, instr.cp2_vec.e);
            case FUNCT_RSP_VEC_VMULQ: return rsp_vec_vmulq;
            case FUNCT_RSP_VEC_VMULU: return rsp_vec_vmulu;
            case FUNCT_RSP_VEC_VNAND: return rsp_vec_vnand;
            case FUNCT_RSP_VEC_VNE:   DECODE_RSP_VECTOR_INSTR_VTE(rsp_vec_vne, instr.cp2_vec.e);
            case FUNCT_RSP_VEC_VNOP:  return rsp_vec_vnop;
            case FUNCT_RSP_VEC_VNOR:  return rsp_vec_vnor;
            case FUNCT_RSP_VEC_VNXOR: DECODE_RSP_VECTOR_INSTR_VTE(rsp_vec_vnxor, instr.cp2_vec.e);
            case FUNCT_RSP_VEC_VOR :  DECODE_RSP_VECTOR_INSTR_VTE(rsp_vec_vor, instr.cp2_vec.e);
            case FUNCT_RSP_VEC_VRCP:  return rsp_vec_vrcp;
            case FUNCT_RSP_VEC_VRCPH: return rsp_vec_vrcph_vrsqh;
            case FUNCT_RSP_VEC_VRCPL: return rsp_vec_vrcpl;
            case FUNCT_RSP_VEC_VRNDN: return rsp_vec_vrndn;
            case FUNCT_RSP_VEC_VRNDP: return rsp_vec_vrndp;
            case FUNCT_RSP_VEC_VRSQ:  return rsp_vec_vrsq;
            case FUNCT_RSP_VEC_VRSQH: return rsp_vec_vrcph_vrsqh;
            case FUNCT_RSP_VEC_VRSQL: return rsp_vec_vrsql;
            case FUNCT_RSP_VEC_VSAR:  return rsp_vec_vsar;
            case FUNCT_RSP_VEC_VSUB:  DECODE_RSP_VECTOR_INSTR_VTE(rsp_vec_vsub, instr.cp2_vec.e);
            case FUNCT_RSP_VEC_VSUBC: DECODE_RSP_VECTOR_INSTR_VTE(rsp_vec_vsubc, instr.cp2_vec.e);
            case FUNCT_RSP_VEC_VXOR:  DECODE_RSP_VECTOR_INSTR_VTE(rsp_vec_vxor, instr.cp2_vec.e);
            default: {
                char buf[50];
                disassemble(pc, instr.raw, buf, 50);
                logfatal("Invalid RSP CP2 VEC [0x%08X]=0x%08X | Capstone thinks it's %s", pc, instr.raw, buf);
            }
        }
    } else {
        switch (instr.cp2_regmove.funct) {
            case COP_CF: return rsp_cfc2;
            case COP_CT: return rsp_ctc2;
            case COP_MF: return rsp_mfc2;
            case COP_MT: return rsp_mtc2;
            default: {
                char buf[50];
                disassemble(pc, instr.raw, buf, 50);
                logfatal("Invalid RSP CP2 regmove instruction! [0x%08x]=0x%08x | Capstone thinks it's %s", pc, instr.raw, buf);
            }
        }
    }
}

INLINE rspinstr_handler_t rsp_special_decode(rsp_t* rsp, word pc, mips_instruction_t instr) {
    switch (instr.r.funct) {
        case FUNCT_SLL:    return rsp_spc_sll;
        case FUNCT_SRL:    return rsp_spc_srl;
        case FUNCT_SRA:    return rsp_spc_sra;
        //case FUNCT_SRAV:   return rsp_spc_srav;
        case FUNCT_SLLV:   return rsp_spc_sllv;
        case FUNCT_SRLV:   return rsp_spc_srlv;
        case FUNCT_JR:     return rsp_spc_jr;
        case FUNCT_JALR:   return rsp_spc_jalr;
        //case FUNCT_MULT:   return rsp_spc_mult;
        //case FUNCT_MULTU:  return rsp_spc_multu;
        //case FUNCT_DIV:    return rsp_spc_div;
        //case FUNCT_DIVU:   return rsp_spc_divu;
        case FUNCT_ADD:    return rsp_spc_add;
        case FUNCT_ADDU:   return rsp_spc_add;
        case FUNCT_AND:    return rsp_spc_and;
        case FUNCT_SUB:    return rsp_spc_sub;
        //case FUNCT_SUBU:   return rsp_spc_subu;
        case FUNCT_OR:     return rsp_spc_or;
        case FUNCT_XOR:    return rsp_spc_xor;
        case FUNCT_NOR:    return rsp_spc_nor;
        case FUNCT_SLT:    return rsp_spc_slt;
        //case FUNCT_SLTU:   return rsp_spc_sltu;

        case FUNCT_BREAK: return rsp_spc_break;
        default: {
            char buf[50];
            disassemble(pc, instr.raw, buf, 50);
            logfatal("other/unknown MIPS RSP Special 0x%08X with FUNCT: %d%d%d%d%d%d [%s]", instr.raw,
                     instr.funct0, instr.funct1, instr.funct2, instr.funct3, instr.funct4, instr.funct5, buf);
        }
    }
}

INLINE rspinstr_handler_t rsp_regimm_decode(rsp_t* cpu, word pc, mips_instruction_t instr) {
    switch (instr.i.rt) {
        case RT_BLTZ:   return rsp_ri_bltz;
        case RT_BGEZ:   return rsp_ri_bgez;
        //case RT_BGEZAL: return rsp_ri_bgezAL;
        default: {
            char buf[50];
            disassemble(pc, instr.raw, buf, 50);
            logfatal("other/unknown RSP REGIMM 0x%08X with RT: %d%d%d%d%d [%s]", instr.raw,
                     instr.rt0, instr.rt1, instr.rt2, instr.rt3, instr.rt4, buf);
        }
    }
}

INLINE rspinstr_handler_t rsp_lwc2_decode(rsp_t* rsp, word pc, mips_instruction_t instr) {
    switch (instr.v.funct) {
        case LWC2_LBV: return rsp_lwc2_lbv;
        case LWC2_LDV: return rsp_lwc2_ldv;
        case LWC2_LFV: return rsp_lwc2_lfv;
        case LWC2_LHV: return rsp_lwc2_lhv;
        case LWC2_LLV: return rsp_lwc2_llv;
        case LWC2_LPV: return rsp_lwc2_lpv;
        case LWC2_LQV: return rsp_lwc2_lqv;
        case LWC2_LRV: return rsp_lwc2_lrv;
        case LWC2_LSV: return rsp_lwc2_lsv;
        case LWC2_LTV: return rsp_lwc2_ltv;
        case LWC2_LUV: return rsp_lwc2_luv;
        default:
            logfatal("other/unknown MIPS RSP LWC2 with funct: 0x%02X", instr.v.funct);
    }
}

INLINE rspinstr_handler_t rsp_swc2_decode(rsp_t* rsp, word pc, mips_instruction_t instr) {
    switch (instr.v.funct) {
        case LWC2_LBV: return rsp_swc2_sbv;
        case LWC2_LDV: return rsp_swc2_sdv;
        case LWC2_LFV: return rsp_swc2_sfv;
        case LWC2_LHV: return rsp_swc2_shv;
        case LWC2_LLV: return rsp_swc2_slv;
        case LWC2_LPV: return rsp_swc2_spv;
        case LWC2_LQV: return rsp_swc2_sqv;
        case LWC2_LRV: return rsp_swc2_srv;
        case LWC2_LSV: return rsp_swc2_ssv;
        case LWC2_LTV: return rsp_swc2_stv;
        case LWC2_LUV: return rsp_swc2_suv;
        default:
            logfatal("other/unknown MIPS RSP SWC2 with funct: 0x%02X", instr.v.funct);
    }
}

INLINE rspinstr_handler_t rsp_instruction_decode(rsp_t* rsp, word pc, mips_instruction_t instr) {
#ifdef LOG_ENABLED
        char buf[50];
        if (n64_log_verbosity >= LOG_VERBOSITY_DEBUG) {
            disassemble(pc, instr.raw, buf, 50);
            logdebug("RSP [0x%08X]=0x%08X %s", pc, instr.raw, buf);
        }
#endif
        if (instr.raw == 0) {
            return rsp_nop;
        }
        switch (instr.op) {
            case OPC_LUI:   return rsp_lui;
            case OPC_ADDIU: return rsp_addi;
            case OPC_ADDI:  return rsp_addi;
            case OPC_ANDI:  return rsp_andi;
            case OPC_LBU:   return rsp_lbu;
            case OPC_LHU:   return rsp_lhu;
            case OPC_LH:    return rsp_lh;
            case OPC_LW:    return rsp_lw;
            //case OPC_LWU:   return rsp_lwu;
            case OPC_BEQ:   return rsp_beq;
            //case OPC_BEQL:  return rsp_beql;
            case OPC_BGTZ:  return rsp_bgtz;
            case OPC_BLEZ:  return rsp_blez;
            case OPC_BNE:   return rsp_bne;
            //case OPC_BNEL:  return rsp_bnel;
            //case OPC_CACHE: return rsp_cache;
            case OPC_SB:    return rsp_sb;
            case OPC_SH:    return rsp_sh;
            case OPC_SW:    return rsp_sw;
            case OPC_ORI:   return rsp_ori;
            case OPC_J:     return rsp_j;
            case OPC_JAL:   return rsp_jal;
            //case OPC_SLTI:  return rsp_slti;
            //case OPC_SLTIU: return rsp_sltiu;
            case OPC_XORI:  return rsp_xori;
            case OPC_LB:    return rsp_lb;
            //case OPC_LWL:   return rsp_lwl;
            //case OPC_LWR:   return rsp_lwr;
            //case OPC_SWL:   return rsp_swl;
            //case OPC_SWR:   return rsp_swr;

            case OPC_CP0:      return rsp_cp0_decode(rsp, pc, instr);
            case OPC_CP1:      logfatal("Decoding RSP CP1 instruction!");     //return rsp_cp1_decode(rsp, pc, instr);
            case OPC_CP2:      return rsp_cp2_decode(rsp, pc, instr);
            case OPC_SPCL:     return rsp_special_decode(rsp, pc, instr);
            case OPC_REGIMM:   return rsp_regimm_decode(rsp, pc, instr);
            case RSP_OPC_LWC2: return rsp_lwc2_decode(rsp, pc, instr);
            case RSP_OPC_SWC2: return rsp_swc2_decode(rsp, pc, instr);

            default:
#ifdef LOG_ENABLED
                if (n64_log_verbosity < LOG_VERBOSITY_DEBUG) {
                    disassemble(pc, instr.raw, buf, 50);
                }
                logfatal("[RSP] Failed to decode instruction 0x%08X opcode %d%d%d%d%d%d [%s]",
                         instr.raw, instr.op0, instr.op1, instr.op2, instr.op3, instr.op4, instr.op5, buf);
#else
                logfatal("[RSP] Failed to decode instruction 0x%08X opcode %d%d%d%d%d%d [UNKNOWN]",
                         instr.raw, instr.op0, instr.op1, instr.op2, instr.op3, instr.op4, instr.op5);
#endif
        }
}

INLINE void _rsp_step(n64_system_t* system) {
    rsp_t* rsp = &system->rsp;
    half pc = rsp->pc & 0x3FF;
    rsp_icache_entry_t* cache = &system->rsp.icache[pc];

    // Need to decode the instruction?
    if (cache->handler == NULL) {
        half fullsize_pc = pc << 2;
        // RSP can only read from IMEM.
        cache->instruction.raw = word_from_byte_array((byte*) &system->mem.sp_imem, fullsize_pc);
        cache->handler = rsp_instruction_decode(rsp, fullsize_pc, cache->instruction);
    }

    rsp->pc = rsp->next_pc;
    rsp->next_pc++;

    cache->handler(rsp, cache->instruction);
}

void rsp_step(n64_system_t* system) {
    _rsp_step(system);
}

int rsp_run(n64_system_t* system, int steps) {
    // 2 RSP steps per CPU step
    while (steps >= 3) {
        for (int i = 0; i < 2; i++) {
            if (system->rsp.status.halt) {
                return 0;
            } else {
                _rsp_step(system);
            }
        }
        steps -= 3;
    }
    return steps;
}