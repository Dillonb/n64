#include "r4300i.h"
#include <log.h>
#include <system/n64system.h>
#include <mem/n64bus.h>
#include "disassemble.h"
#include "mips_instructions.h"
#include "fpu_instructions.h"
#include "tlb_instructions.h"

const char* register_names[] = {
        "zero", // 0
        "at",   // 1
        "v0",   // 2
        "v1",   // 3
        "a0",   // 4
        "a1",   // 5
        "a2",   // 6
        "a3",   // 7
        "t0",   // 8
        "t1",   // 9
        "t2",   // 10
        "t3",   // 11
        "t4",   // 12
        "t5",   // 13
        "t6",   // 14
        "t7",   // 15
        "s0",   // 16
        "s1",   // 17
        "s2",   // 18
        "s3",   // 19
        "s4",   // 20
        "s5",   // 21
        "s6",   // 22
        "s7",   // 23
        "t8",   // 24
        "t9",   // 25
        "k0",   // 26
        "k1",   // 27
        "gp",   // 28
        "sp",   // 29
        "s8",   // 30
        "ra"    // 31
};

const char* cp0_register_names[] = {
        "Index", "Random", "EntryLo0", "EntryLo1", "Context", "PageMask", "Wired", "7", "BadVAddr", "Count", "EntryHi",
        "Compare", "Status", "Cause", "EPC", "PRId", "Config", "LLAddr", "WatchLo", "WatchHi", "XContext", "21", "22",
        "23", "24", "25", "Parity Error", "Cache Error", "TagLo", "TagHi", "error_epc", "r31"
};

r4300i_t n64cpu;

void r4300i_handle_exception(dword pc, word code, sword coprocessor_error) {
    bool old_exl = N64CP0.status.exl;
    loginfo("Exception thrown! Code: %d Coprocessor: %d", code, coprocessor_error);
    // In a branch delay slot, set EPC to the branch PRECEDING the slot.
    // This is so the exception handler can re-execute the branch on return.
    if (N64CPU.branch) {
        unimplemented(N64CPU.cp0.status.exl, "handling branch delay when exl == true");
        N64CPU.cp0.cause.branch_delay = true;
        N64CPU.branch = false;
        pc -= 4;
    } else {
        N64CPU.cp0.cause.branch_delay = false;
    }

    if (!N64CPU.cp0.status.exl) {
        N64CPU.cp0.EPC = pc;
        N64CPU.cp0.status.exl = true;
    }

    N64CPU.cp0.cause.exception_code = code;
    if (coprocessor_error > 0) {
        N64CPU.cp0.cause.coprocessor_error = coprocessor_error;
    } else {
        N64CPU.cp0.cause.coprocessor_error = 0;
    }

    if (N64CPU.cp0.status.bev) {
        switch (code) {
            case EXCEPTION_COPROCESSOR_UNUSABLE:
                logfatal("Cop unusable, the PC below is wrong. See page 181 in the manual.");
                set_pc_word_r4300i(0x80000180);
                break;
            default:
                logfatal("Unknown exception %d with BEV! See page 181 in the manual.", code);
        }
    } else {
        switch (code) {
            case EXCEPTION_INTERRUPT:
            case EXCEPTION_COPROCESSOR_UNUSABLE:
            case EXCEPTION_TRAP:
            case EXCEPTION_BREAKPOINT:
            case EXCEPTION_SYSCALL:
            case EXCEPTION_ADDRESS_ERROR_LOAD:
            case EXCEPTION_ARITHMETIC_OVERFLOW:
                set_pc_word_r4300i(0x80000180);
                break;
            case EXCEPTION_TLB_MISS_LOAD:
            case EXCEPTION_TLB_MISS_STORE:
                if (old_exl) {
                    set_pc_word_r4300i(0x80000180);
                } else if (N64CP0.is_64bit_addressing){
                    set_pc_word_r4300i(0x80000080);
                } else {
                    set_pc_word_r4300i(0x80000000);
                }
                break;
            default:
                logfatal("Unknown exception %d without BEV! See page 181 in the manual.", code);
        }
    }
    cp0_status_updated();
    N64CPU.exception = true;
}

INLINE mipsinstr_handler_t r4300i_cp0_decode(dword pc, mips_instruction_t instr) {
    if (instr.last11 == 0) {
        switch (instr.r.rs) {
            case COP_MF:
                return mips_mfc0;
            case COP_MT: // Last 11 bits are 0
                return mips_mtc0;
            case COP_DMT:
                return mips_dmtc0;
            case COP_DMF:
                return mips_dmfc0;
            default: {
                char buf[50];
                disassemble(pc, instr.raw, buf, 50);
                logfatal("other/unknown MIPS CP0 0x%08X with rs: %d%d%d%d%d [%s]", instr.raw,
                         instr.rs0, instr.rs1, instr.rs2, instr.rs3, instr.rs4, buf);
            }
        }
    } else {
        switch (instr.fr.funct) {
            case COP_FUNCT_TLBWI_MULT:
                return mips_tlbwi;
            case COP_FUNCT_TLBP:
                return mips_tlbp;
            case COP_FUNCT_TLBR_SUB:
                return mips_tlbr;
            case COP_FUNCT_ERET:
                return mips_eret;
            case COP_FUNCT_WAIT:
                return mips_nop;
            case COP_FUNCT_TLBWR_MOV:
                return mips_tlbwr;
            default: {
                char buf[50];
                disassemble(pc, instr.raw, buf, 50);
                logfatal("other/unknown MIPS CP0 0x%08X with FUNCT: %d%d%d%d%d%d [%s]", instr.raw,
                         instr.funct0, instr.funct1, instr.funct2, instr.funct3, instr.funct4, instr.funct5, buf);
            }
        }
    }
}

INLINE mipsinstr_handler_t r4300i_cp1_decode(dword pc, mips_instruction_t instr) {
    // This function uses a series of two switch statements.
    // If the instruction doesn't use the RS field for the opcode, then control will fall through to the next
    // switch, and check the FUNCT. It may be worth profiling and seeing if it's faster to check FUNCT first at some point
    switch (instr.r.rs) {
        case COP_CF:
            return mips_cfc1;
        case COP_MF:
            return mips_mfc1;
        case COP_DMF:
            return mips_dmfc1;
        case COP_MT:
            return mips_mtc1;
        case COP_DMT:
            return mips_dmtc1;
        case COP_CT:
            return mips_ctc1;
        case COP_BC:
            switch (instr.r.rt) {
                case COP_BC_BCT:
                    return mips_cp_bc1t;
                case COP_BC_BCF:
                    return mips_cp_bc1f;
                case COP_BC_BCTL:
                    return mips_cp_bc1tl;
                case COP_BC_BCFL:
                    return mips_cp_bc1fl;
                default: {
                    char buf[50];
                    disassemble(pc, instr.raw, buf, 50);
                    logfatal("other/unknown MIPS BC 0x%08X [%s]", instr.raw, buf);
                }
            }
    }
    switch (instr.fr.funct) {
        case COP_FUNCT_ADD:
            switch (instr.fr.fmt) {
                case FP_FMT_DOUBLE:
                    return mips_cp_add_d;
                case FP_FMT_SINGLE:
                    return mips_cp_add_s;
                default:
                    logfatal("Undefined!");
            }
        case COP_FUNCT_TLBR_SUB: {
            switch (instr.fr.fmt) {
                case FP_FMT_DOUBLE:
                    return mips_cp_sub_d;
                case FP_FMT_SINGLE:
                    return mips_cp_sub_s;
                default:
                    logfatal("Undefined!");
            }
        }
        case COP_FUNCT_TLBWI_MULT:
            switch (instr.fr.fmt) {
                case FP_FMT_DOUBLE:
                    return mips_cp_mul_d;
                case FP_FMT_SINGLE:
                    return mips_cp_mul_s;
                default:
                    logfatal("Undefined!");
            }
        case COP_FUNCT_DIV:
            switch (instr.fr.fmt) {
                case FP_FMT_DOUBLE:
                    return mips_cp_div_d;
                case FP_FMT_SINGLE:
                    return mips_cp_div_s;
                default:
                    logfatal("Undefined!");
            }
        case COP_FUNCT_TRUNC_L:
            switch (instr.fr.fmt) {
                case FP_FMT_DOUBLE:
                    return mips_cp_trunc_l_d;
                case FP_FMT_SINGLE:
                    return mips_cp_trunc_l_s;
                default:
                    logfatal("Undefined!");
            }
        case COP_FUNCT_ROUND_L:
            switch (instr.fr.fmt) {
                case FP_FMT_DOUBLE:
                    return mips_cp_round_l_d;
                case FP_FMT_SINGLE:
                    return mips_cp_round_l_s;
                default:
                    logfatal("Undefined!");
            }
        case COP_FUNCT_TRUNC_W:
            switch (instr.fr.fmt) {
                case FP_FMT_DOUBLE:
                    return mips_cp_trunc_w_d;
                case FP_FMT_SINGLE:
                    return mips_cp_trunc_w_s;
                default:
                    logfatal("Undefined!");
            }
        case COP_FUNCT_FLOOR_W:
            switch (instr.fr.fmt) {
                case FP_FMT_DOUBLE:
                    return mips_cp_floor_w_d;
                case FP_FMT_SINGLE:
                    return mips_cp_floor_w_s;
                default:
                    logfatal("Undefined!");
            }
        case COP_FUNCT_ROUND_W:
            switch (instr.fr.fmt) {
                case FP_FMT_DOUBLE:
                    return mips_cp_round_w_d;
                case FP_FMT_SINGLE:
                    return mips_cp_round_w_s;
                default:
                    logfatal("Undefined!");
            }
        case COP_FUNCT_CVT_D:
            switch (instr.fr.fmt) {
                case FP_FMT_SINGLE:
                    return mips_cp_cvt_d_s;
                case FP_FMT_W:
                    return mips_cp_cvt_d_w;
                case FP_FMT_L:
                    return mips_cp_cvt_d_l;
                default:
                    logfatal("Undefined!");
            }
        case COP_FUNCT_CVT_L:
            switch (instr.fr.fmt) {
                case FP_FMT_DOUBLE:
                    return mips_cp_cvt_l_d;
                case FP_FMT_SINGLE:
                    return mips_cp_cvt_l_s;
                default:
                    logfatal("Undefined!");
            }
        case COP_FUNCT_CVT_S:
            switch (instr.fr.fmt) {
                case FP_FMT_DOUBLE:
                    return mips_cp_cvt_s_d;
                case FP_FMT_W:
                    return mips_cp_cvt_s_w;
                case FP_FMT_L:
                    return mips_cp_cvt_s_l;
                default:
                    logfatal("Undefined!");
            }
        case COP_FUNCT_CVT_W:
            switch (instr.fr.fmt) {
                case FP_FMT_DOUBLE:
                    return mips_cp_cvt_w_d;
                case FP_FMT_SINGLE:
                    return mips_cp_cvt_w_s;
                default:
                    logfatal("Undefined!");
            }
        case COP_FUNCT_SQRT:
            switch (instr.fr.fmt) {
                case FP_FMT_DOUBLE:
                    return mips_cp_sqrt_d;
                case FP_FMT_SINGLE:
                    return mips_cp_sqrt_s;
                default:
                    logfatal("Undefined!");
            }

        case COP_FUNCT_ABS:
            switch (instr.fr.fmt) {
                case FP_FMT_DOUBLE:
                    return mips_cp_abs_d;
                case FP_FMT_SINGLE:
                    return mips_cp_abs_s;
                default:
                    logfatal("Undefined!");
            }


        case COP_FUNCT_TLBWR_MOV:
            switch (instr.fr.fmt) {
                case FP_FMT_DOUBLE:
                    return mips_cp_mov_d;
                case FP_FMT_SINGLE:
                    return mips_cp_mov_s;
                default:
                    logfatal("Undefined!");
            }
        case COP_FUNCT_NEG:
            switch (instr.fr.fmt) {
                case FP_FMT_DOUBLE:
                    return mips_cp_neg_d;
                case FP_FMT_SINGLE:
                    return mips_cp_neg_s;
                default:
                    logfatal("Undefined!");
            }
        case COP_FUNCT_C_F:
            switch (instr.fr.fmt) {
                case FP_FMT_DOUBLE:
                    return mips_cp_c_f_d;
                case FP_FMT_SINGLE:
                    return mips_cp_c_f_s;
                default:
                    logfatal("Undefined!");
            }
        case COP_FUNCT_C_UN:
            switch (instr.fr.fmt) {
                case FP_FMT_DOUBLE:
                    return mips_cp_c_un_d;
                case FP_FMT_SINGLE:
                    return mips_cp_c_un_s;
                default:
                    logfatal("Undefined!");
            }
        case COP_FUNCT_C_EQ:
            switch (instr.fr.fmt) {
                case FP_FMT_DOUBLE:
                    return mips_cp_c_eq_d;
                case FP_FMT_SINGLE:
                    return mips_cp_c_eq_s;
                default:
                    logfatal("Undefined!");
            }
        case COP_FUNCT_C_UEQ:
            switch (instr.fr.fmt) {
                case FP_FMT_DOUBLE:
                    return mips_cp_c_ueq_d;
                case FP_FMT_SINGLE:
                    return mips_cp_c_ueq_s;
                default:
                    logfatal("Undefined!");
            }
        case COP_FUNCT_C_OLT:
            switch (instr.fr.fmt) {
                case FP_FMT_DOUBLE:
                    return mips_cp_c_olt_d;
                case FP_FMT_SINGLE:
                    return mips_cp_c_olt_s;
                default:
                    logfatal("Undefined!");
            }
        case COP_FUNCT_C_ULT:
            switch (instr.fr.fmt) {
                case FP_FMT_DOUBLE:
                    return mips_cp_c_ult_d;
                case FP_FMT_SINGLE:
                    return mips_cp_c_ult_s;
                default:
                    logfatal("Undefined!");
            }
        case COP_FUNCT_C_OLE:
            switch (instr.fr.fmt) {
                case FP_FMT_DOUBLE:
                    return mips_cp_c_ole_d;
                case FP_FMT_SINGLE:
                    return mips_cp_c_ole_s;
                default:
                    logfatal("Undefined!");
            }
        case COP_FUNCT_C_ULE:
            switch (instr.fr.fmt) {
                case FP_FMT_DOUBLE:
                    return mips_cp_c_ule_d;
                case FP_FMT_SINGLE:
                    return mips_cp_c_ule_s;
                default:
                    logfatal("Undefined!");
            }
        case COP_FUNCT_C_SF:
            switch (instr.fr.fmt) {
                case FP_FMT_DOUBLE:
                    return mips_cp_c_sf_d;
                case FP_FMT_SINGLE:
                    return mips_cp_c_sf_s;
                default:
                    logfatal("Undefined!");
            }
        case COP_FUNCT_C_NGLE:
            switch (instr.fr.fmt) {
                case FP_FMT_DOUBLE:
                    return mips_cp_c_ngle_d;
                case FP_FMT_SINGLE:
                    return mips_cp_c_ngle_s;
                default:
                    logfatal("Undefined!");
            }
        case COP_FUNCT_C_SEQ:
            switch (instr.fr.fmt) {
                case FP_FMT_DOUBLE:
                    return mips_cp_c_seq_d;
                case FP_FMT_SINGLE:
                    return mips_cp_c_seq_s;
                default:
                    logfatal("Undefined!");
            }
        case COP_FUNCT_C_NGL:
            switch (instr.fr.fmt) {
                case FP_FMT_DOUBLE:
                    return mips_cp_c_ngl_d;
                case FP_FMT_SINGLE:
                    return mips_cp_c_ngl_s;
                default:
                    logfatal("Undefined!");
            }
        case COP_FUNCT_C_LT:
            switch (instr.fr.fmt) {
                case FP_FMT_DOUBLE:
                    return mips_cp_c_lt_d;
                case FP_FMT_SINGLE:
                    return mips_cp_c_lt_s;
                default:
                    logfatal("Undefined!");
            }
        case COP_FUNCT_C_NGE:
            switch (instr.fr.fmt) {
                case FP_FMT_DOUBLE:
                    return mips_cp_c_nge_d;
                case FP_FMT_SINGLE:
                    return mips_cp_c_nge_s;
                default:
                    logfatal("Undefined!");
            }
        case COP_FUNCT_C_LE:
            switch (instr.fr.fmt) {
                case FP_FMT_DOUBLE:
                    return mips_cp_c_le_d;
                case FP_FMT_SINGLE:
                    return mips_cp_c_le_s;
                default:
                    logfatal("Undefined!");
            }
        case COP_FUNCT_C_NGT:
            switch (instr.fr.fmt) {
                case FP_FMT_DOUBLE:
                    return mips_cp_c_ngt_d;
                case FP_FMT_SINGLE:
                    return mips_cp_c_ngt_s;
                default:
                    logfatal("Undefined!");
            }
    }

    char buf[50];
    disassemble(pc, instr.raw, buf, 50);
    logfatal("other/unknown MIPS CP1 0x%08X with rs: %d%d%d%d%d and FUNCT: %d%d%d%d%d%d [%s]", instr.raw,
             instr.rs0, instr.rs1, instr.rs2, instr.rs3, instr.rs4,
             instr.funct0, instr.funct1, instr.funct2, instr.funct3, instr.funct4, instr.funct5, buf);
}

INLINE mipsinstr_handler_t r4300i_special_decode(dword pc, mips_instruction_t instr) {
    switch (instr.r.funct) {
        case FUNCT_SLL:     return mips_spc_sll;
        case FUNCT_SRL:     return mips_spc_srl;
        case FUNCT_SRA:     return mips_spc_sra;
        case FUNCT_SRAV:    return mips_spc_srav;
        case FUNCT_SLLV:    return mips_spc_sllv;
        case FUNCT_SRLV:    return mips_spc_srlv;
        case FUNCT_JR:      return mips_spc_jr;
        case FUNCT_JALR:    return mips_spc_jalr;
        case FUNCT_SYSCALL: return mips_spc_syscall;
        case FUNCT_MFHI:    return mips_spc_mfhi;
        case FUNCT_MTHI:    return mips_spc_mthi;
        case FUNCT_MFLO:    return mips_spc_mflo;
        case FUNCT_MTLO:    return mips_spc_mtlo;
        case FUNCT_DSLLV:   return mips_spc_dsllv;
        case FUNCT_DSRLV:   return mips_spc_dsrlv;
        case FUNCT_DSRAV:   return mips_spc_dsrav;
        case FUNCT_MULT:    return mips_spc_mult;
        case FUNCT_MULTU:   return mips_spc_multu;
        case FUNCT_DIV:     return mips_spc_div;
        case FUNCT_DIVU:    return mips_spc_divu;
        case FUNCT_DMULT:   return mips_spc_dmult;
        case FUNCT_DMULTU:  return mips_spc_dmultu;
        case FUNCT_DDIV:    return mips_spc_ddiv;
        case FUNCT_DDIVU:   return mips_spc_ddivu;
        case FUNCT_ADD:     return mips_spc_add;
        case FUNCT_ADDU:    return mips_spc_addu;
        case FUNCT_AND:     return mips_spc_and;
        case FUNCT_NOR:     return mips_spc_nor;
        case FUNCT_SUB:     return mips_spc_sub;
        case FUNCT_SUBU:    return mips_spc_subu;
        case FUNCT_OR:      return mips_spc_or;
        case FUNCT_XOR:     return mips_spc_xor;
        case FUNCT_SLT:     return mips_spc_slt;
        case FUNCT_SLTU:    return mips_spc_sltu;
        case FUNCT_DADD:    return mips_spc_dadd;
        case FUNCT_DADDU:   return mips_spc_daddu;
        case FUNCT_DSUB:    return mips_spc_dsub;
        case FUNCT_DSUBU:   return mips_spc_dsubu;
        case FUNCT_TGE:     return mips_spc_tge;
        case FUNCT_TGEU:    return mips_spc_tgeu;
        case FUNCT_TLT:     return mips_spc_tlt;
        case FUNCT_TLTU:    return mips_spc_tltu;
        case FUNCT_TEQ:     return mips_spc_teq;
        case FUNCT_TNE:     return mips_spc_tne;
        case FUNCT_DSLL:    return mips_spc_dsll;
        case FUNCT_DSRL:    return mips_spc_dsrl;
        case FUNCT_DSRA:    return mips_spc_dsra;
        case FUNCT_DSLL32:  return mips_spc_dsll32;
        case FUNCT_DSRL32:  return mips_spc_dsrl32;
        case FUNCT_DSRA32:  return mips_spc_dsra32;
        case FUNCT_BREAK:   return mips_spc_break;
        case FUNCT_SYNC:    return mips_nop;
        default: {
            char buf[50];
            disassemble(pc, instr.raw, buf, 50);
            logfatal("other/unknown MIPS Special 0x%08X with FUNCT: %d%d%d%d%d%d [%s]", instr.raw,
                     instr.funct0, instr.funct1, instr.funct2, instr.funct3, instr.funct4, instr.funct5, buf);
        }
    }
}

INLINE mipsinstr_handler_t r4300i_regimm_decode(dword pc, mips_instruction_t instr) {
    switch (instr.i.rt) {
        case RT_BLTZ:    return mips_ri_bltz;
        case RT_BLTZL:   return mips_ri_bltzl;
        case RT_BGEZ:    return mips_ri_bgez;
        case RT_BGEZL:   return mips_ri_bgezl;
        case RT_BLTZAL:  return mips_ri_bltzal;
        case RT_BGEZAL:  return mips_ri_bgezal;
        case RT_BGEZALL: return mips_ri_bgezall;
        case RT_TGEI:    return mips_ri_tgei;
        case RT_TGEIU:   return mips_ri_tgeiu;
        case RT_TLTI:    return mips_ri_tlti;
        case RT_TLTIU:   return mips_ri_tltiu;
        case RT_TEQI:    return mips_ri_teqi;
        case RT_TNEI:    return mips_ri_tnei;
        default: {
            char buf[50];
            disassemble(pc, instr.raw, buf, 50);
            logfatal("other/unknown MIPS REGIMM 0x%08X with RT: %d%d%d%d%d [%s]", instr.raw,
                     instr.rt0, instr.rt1, instr.rt2, instr.rt3, instr.rt4, buf);
        }
    }
}

mipsinstr_handler_t r4300i_instruction_decode(dword pc, mips_instruction_t instr) {
#ifdef LOG_ENABLED
    char buf[50];
    if (n64_log_verbosity >= LOG_VERBOSITY_DEBUG) {
        disassemble(pc, instr.raw, buf, 50);
        logdebug("[0x%016lX]=0x%08X %s", pc, instr.raw, buf);
    }
#endif
    if (unlikely(instr.raw == 0)) {
        return mips_nop;
    }
    switch (instr.op) {
        case OPC_CP0:    return r4300i_cp0_decode(pc, instr);
        case OPC_CP1:    return r4300i_cp1_decode(pc, instr);
        case OPC_SPCL:   return r4300i_special_decode(pc, instr);
        case OPC_REGIMM: return r4300i_regimm_decode(pc, instr);

        case OPC_LD:     return mips_ld;
        case OPC_LUI:    return mips_lui;
        case OPC_ADDIU:  return mips_addiu;
        case OPC_ADDI:   return mips_addi;
        case OPC_DADDI:  return mips_daddi;
        case OPC_ANDI:   return mips_andi;
        case OPC_LBU:    return mips_lbu;
        case OPC_LHU:    return mips_lhu;
        case OPC_LH:     return mips_lh;
        case OPC_LW:     return mips_lw;
        case OPC_LWU:    return mips_lwu;
        case OPC_BEQ:    return mips_beq;
        case OPC_BEQL:   return mips_beql;
        case OPC_BGTZ:   return mips_bgtz;
        case OPC_BGTZL:  return mips_bgtzl;
        case OPC_BLEZ:   return mips_blez;
        case OPC_BLEZL:  return mips_blezl;
        case OPC_BNE:    return mips_bne;
        case OPC_BNEL:   return mips_bnel;
        case OPC_CACHE:  return mips_cache;
        case OPC_SB:     return mips_sb;
        case OPC_SH:     return mips_sh;
        case OPC_SW:     return mips_sw;
        case OPC_SD:     return mips_sd;
        case OPC_ORI:    return mips_ori;
        case OPC_J:      return mips_j;
        case OPC_JAL:    return mips_jal;
        case OPC_SLTI:   return mips_slti;
        case OPC_SLTIU:  return mips_sltiu;
        case OPC_XORI:   return mips_xori;
        case OPC_DADDIU: return mips_daddiu;
        case OPC_LB:     return mips_lb;
        case OPC_LDC1:   return mips_ldc1;
        case OPC_SDC1:   return mips_sdc1;
        case OPC_LWC1:   return mips_lwc1;
        case OPC_SWC1:   return mips_swc1;
        case OPC_LWL:    return mips_lwl;
        case OPC_LWR:    return mips_lwr;
        case OPC_SWL:    return mips_swl;
        case OPC_SWR:    return mips_swr;
        case OPC_LDL:    return mips_ldl;
        case OPC_LDR:    return mips_ldr;
        case OPC_SDL:    return mips_sdl;
        case OPC_SDR:    return mips_sdr;
        case OPC_LL:     return mips_ll;
        case OPC_LLD:    return mips_lld;
        case OPC_SC:     return mips_sc;
        case OPC_SCD:    return mips_scd;
        default:
#ifdef LOG_ENABLED
            if (n64_log_verbosity < LOG_VERBOSITY_DEBUG) {
                disassemble(pc, instr.raw, buf, 50);
            }
            logfatal("Failed to decode instruction 0x%08X opcode %d%d%d%d%d%d [%s]",
                     instr.raw, instr.op0, instr.op1, instr.op2, instr.op3, instr.op4, instr.op5, buf);
#else
            logfatal("Failed to decode instruction 0x%08X opcode %d%d%d%d%d%d]",
                     instr.raw, instr.op0, instr.op1, instr.op2, instr.op3, instr.op4, instr.op5);
#endif
    }
}

void on_tlb_exception(dword address) {
    dword vpn2 = address >> 13 & 0x7FFFF;
    dword xvpn2 = address >> 13 & 0x7FFFFFF;
    N64CP0.bad_vaddr = address;
    N64CP0.context.badvpn2 = vpn2;
    N64CP0.x_context.badvpn2 = xvpn2;
    N64CP0.x_context.r = (address >> 62) & 3;
    N64CP0.entry_hi.vpn2 = vpn2;
}

void r4300i_step() {
    N64CPU.cp0.count += CYCLES_PER_INSTR;
    N64CPU.cp0.count &= 0x1FFFFFFFF;
    if (unlikely(N64CPU.cp0.count == (dword)N64CPU.cp0.compare << 1)) {
        N64CPU.cp0.cause.ip7 = true;
        loginfo("Compare interrupt! count = 0x%09lX compare << 1 = 0x%09lX", N64CP0.count, (dword)N64CP0.compare << 1);
        r4300i_interrupt_update();
    }

    /* Commented out for now since the game never actually reads cp0.random
    if (N64CPU.cp0.random <= N64CPU.cp0.wired) {
        N64CPU.cp0.random = 31;
    } else {
        N64CPU.cp0.random--;
    }
     */

    dword pc = N64CPU.pc;
    word physical_pc;
    if (!resolve_virtual_address(pc, &physical_pc)) {
        // tlb exception
        logalways("TLB exception on loading an instruction!");
        on_tlb_exception(pc);
        r4300i_handle_exception(pc, EXCEPTION_TLB_MISS_LOAD, -1);
        return;
    }
    mips_instruction_t instruction;
    instruction.raw = n64_read_physical_word(physical_pc);

    if (unlikely(N64CPU.interrupts > 0)) {
        if(N64CPU.cp0.status.ie && !N64CPU.cp0.status.exl && !N64CPU.cp0.status.erl) {
            r4300i_handle_exception(pc, EXCEPTION_INTERRUPT, -1);
            return;
        }
    }


    N64CPU.prev_pc = N64CPU.pc;
    N64CPU.pc = N64CPU.next_pc;
    N64CPU.next_pc += 4;
    N64CPU.branch = false;

    r4300i_instruction_decode(pc, instruction)(instruction);
    N64CPU.exception = false; // only used in dynarec
}

void r4300i_interrupt_update() {
    N64CPU.interrupts = N64CPU.cp0.cause.interrupt_pending & N64CPU.cp0.status.im;
}

bool instruction_stable(mips_instruction_t instr) {
    if (instr.raw == 0) {
        return true; // NOP
    }

    switch (instr.op) {
        // All branches are stable
        case OPC_REGIMM: // REGIMM opcodes are only branches
        case OPC_J:
        case OPC_JAL:
        case OPC_BEQ:
        case OPC_BEQL:
        case OPC_BGTZ:
        case OPC_BGTZL:
        case OPC_BLEZ:
        case OPC_BLEZL:
        case OPC_BNE:
        case OPC_BNEL:
        // Will always generate the same result given the same args
        case OPC_ANDI:
        case OPC_ORI:
        case OPC_LUI:
            return true;
        // Loads are stable if they load from RAM
        case OPC_LB:
        case OPC_LBU:
        case OPC_LH:
        case OPC_LHU:
        case OPC_LW:
        case OPC_LWU:
        case OPC_LWL:
        case OPC_LWR:
        case OPC_LD:
        case OPC_LDL:
        case OPC_LDR:
            return false; // TODO need const propagation to properly detect if this is a RAM access
        // Stores are stable if they store to RAM
        case OPC_SB:
        case OPC_SH:
        case OPC_SW:
        case OPC_SWL:
        case OPC_SWR:
        case OPC_SD:
        case OPC_SDL:
        case OPC_SDR:
            return false; // TODO need const propagation to properly detect if this is a RAM access
        default:
            return false;
    }
}