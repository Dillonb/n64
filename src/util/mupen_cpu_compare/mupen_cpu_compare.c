#include "mupen_mocks.h"
#include "mips_instructions.def"

#include "mupen_cpu_compare.h"
#include "../../cpu/mips_instructions.h"

#define INSTR_ENTRY(key, _name, _mine, _mupen) case key: { \
    instruction_compare_t compare;                  \
    compare.valid = true;                           \
    compare.mine = _mine;                           \
    compare.mupen = _mupen;                         \
    compare.name = _name;                           \
    return compare;                                 \
}

instruction_compare_t unknown_comparison = {.valid = false, .name = "UNKNOWN", .mine = NULL, .mupen = NULL};

instruction_compare_t get_comparison(mips_instruction_type_t type) {
    switch (type) {
        /* TODO: not sure how I'm going to compare loads/stores yet.
        INSTR_ENTRY(MIPS_LB, mips_lb, LB)
        INSTR_ENTRY(MIPS_LBU, mips_lbu, LBU)
        INSTR_ENTRY(MIPS_LH, mips_lh, LH)
        INSTR_ENTRY(MIPS_LHU, mips_lhu, LHU)
        INSTR_ENTRY(MIPS_LW, mips_lw, LW)
        INSTR_ENTRY(MIPS_LWU, mips_lwu, LWU)
        INSTR_ENTRY(MIPS_LWL, mips_lwl, LWL)
        INSTR_ENTRY(MIPS_LWR, mips_lwr, LWR)
        INSTR_ENTRY(MIPS_LD, mips_ld, LD)
        INSTR_ENTRY(MIPS_LDL, mips_ldl, LDL)
        INSTR_ENTRY(MIPS_LDR, mips_ldr, LDR)
        INSTR_ENTRY(MIPS_SB, mips_sb, SB)
        INSTR_ENTRY(MIPS_SH, mips_sh, SH)
        INSTR_ENTRY(MIPS_SW, mips_sw, SW)
        INSTR_ENTRY(MIPS_SWL, mips_swl, SWL)
        INSTR_ENTRY(MIPS_SWR, mips_swr, SWR)
        INSTR_ENTRY(MIPS_SD, mips_sd, SD)
        INSTR_ENTRY(MIPS_SDL, mips_sdl, SDL)
        INSTR_ENTRY(MIPS_SDR, mips_sdr, SDR)
         */
        INSTR_ENTRY(MIPS_SPC_ADD, "MIPS_SPC_ADD", mips_spc_add, ADD)
        INSTR_ENTRY(MIPS_SPC_ADDU, "MIPS_SPC_ADDU", mips_spc_addu, ADDU)
        INSTR_ENTRY(MIPS_ADDI, "MIPS_ADDI", mips_addi, ADDI)
        INSTR_ENTRY(MIPS_ADDIU, "MIPS_ADDIU", mips_addiu, ADDIU)
        INSTR_ENTRY(MIPS_SPC_DADD, "MIPS_SPC_DADD", mips_spc_dadd, DADD)
        INSTR_ENTRY(MIPS_DADDI, "MIPS_DADDI", mips_daddi, DADDI)
        INSTR_ENTRY(MIPS_SPC_SUB, "MIPS_SPC_SUB", mips_spc_sub, SUB)
        INSTR_ENTRY(MIPS_SPC_SUBU, "MIPS_SPC_SUBU", mips_spc_subu, SUBU)
        INSTR_ENTRY(MIPS_SPC_SLT, "MIPS_SPC_SLT", mips_spc_slt, SLT)
        INSTR_ENTRY(MIPS_SPC_SLTU, "MIPS_SPC_SLTU", mips_spc_sltu, SLTU)
        INSTR_ENTRY(MIPS_SLTI, "MIPS_SLTI", mips_slti, SLTI)
        INSTR_ENTRY(MIPS_SLTIU, "MIPS_SLTIU", mips_sltiu, SLTIU)
        INSTR_ENTRY(MIPS_SPC_AND, "MIPS_SPC_AND", mips_spc_and, AND)
        INSTR_ENTRY(MIPS_ANDI, "MIPS_ANDI", mips_andi, ANDI)
        INSTR_ENTRY(MIPS_SPC_OR, "MIPS_SPC_OR", mips_spc_or, OR)
        INSTR_ENTRY(MIPS_ORI, "MIPS_ORI", mips_ori, ORI)
        INSTR_ENTRY(MIPS_SPC_XOR, "MIPS_SPC_XOR", mips_spc_xor, XOR)
        INSTR_ENTRY(MIPS_XORI, "MIPS_XORI", mips_xori, XORI)
        INSTR_ENTRY(MIPS_SPC_NOR, "MIPS_SPC_NOR", mips_spc_nor, NOR)
        //INSTR_ENTRY(MIPS_LUI, mips_lui, LUI)
        INSTR_ENTRY(MIPS_SPC_SLL, "MIPS_SPC_SLL", mips_spc_sll, SLL)
        INSTR_ENTRY(MIPS_SPC_SLLV, "MIPS_SPC_SLLV", mips_spc_sllv, SLLV)
        INSTR_ENTRY(MIPS_SPC_DSLL, "MIPS_SPC_DSLL", mips_spc_dsll, DSLL)
        INSTR_ENTRY(MIPS_SPC_DSLLV, "MIPS_SPC_DSLLV", mips_spc_dsllv, DSLLV)
        INSTR_ENTRY(MIPS_SPC_DSLL32, "MIPS_SPC_DSLL32", mips_spc_dsll32, DSLL32)
        INSTR_ENTRY(MIPS_SPC_SRL, "MIPS_SPC_SRL", mips_spc_srl, SRL)
        INSTR_ENTRY(MIPS_SPC_SRLV, "MIPS_SPC_SRLV", mips_spc_srlv, SRLV)
        INSTR_ENTRY(MIPS_SPC_SRA, "MIPS_SPC_SRA", mips_spc_sra, SRA)
        INSTR_ENTRY(MIPS_SPC_SRAV, "MIPS_SPC_SRAV", mips_spc_srav, SRAV)
        INSTR_ENTRY(MIPS_SPC_DSRA32, "MIPS_SPC_DSRA32", mips_spc_dsra32, DSRA32)
        INSTR_ENTRY(MIPS_SPC_MULT, "MIPS_SPC_MULT", mips_spc_mult, MULT)
        INSTR_ENTRY(MIPS_SPC_MULTU, "MIPS_SPC_MULTU", mips_spc_multu, MULTU)
        INSTR_ENTRY(MIPS_SPC_DMULTU, "MIPS_SPC_DMULTU", mips_spc_dmultu, DMULTU)
        INSTR_ENTRY(MIPS_SPC_DIV, "MIPS_SPC_DIV", mips_spc_div, DIV)
        INSTR_ENTRY(MIPS_SPC_DIVU, "MIPS_SPC_DIVU", mips_spc_divu, DIVU)
        INSTR_ENTRY(MIPS_SPC_DDIVU, "MIPS_SPC_DDIVU", mips_spc_ddivu, DDIVU)
        INSTR_ENTRY(MIPS_SPC_MFHI, "MIPS_SPC_MFHI", mips_spc_mfhi, MFHI)
        INSTR_ENTRY(MIPS_SPC_MTHI, "MIPS_SPC_MTHI", mips_spc_mthi, MTHI)
        INSTR_ENTRY(MIPS_SPC_MFLO, "MIPS_SPC_MFLO", mips_spc_mflo, MFLO)
        INSTR_ENTRY(MIPS_SPC_MTLO, "MIPS_SPC_MTLO", mips_spc_mtlo, MTLO)
        default:
            return unknown_comparison;
    }
}
