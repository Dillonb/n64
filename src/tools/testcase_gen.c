#include <string.h>
#include <log.h>
#include <r4300i.h>
#include <mips_instructions.h>
#include <assert.h>

typedef enum mipsinstr_type_t {
    IMM_RSRTI,
    BRANCH,
    JUMP,
    RSRTRD,
    SHIFT,
    MULT
} mipsinstr_type;

#define MATCH(name, handler, type) do { if (strcmp(name, instruction_name) == 0) {instruction_handler = handler; instruction_type = type; } } while(0)

void gen_imm_rsrti(char* name, mipsinstr_handler_t handler) {
    const int num_64bit_args = 50000;
    const int num_32bit_args = 50000;

    int num_cases = num_32bit_args + num_64bit_args;

    dword regargs[num_32bit_args + num_64bit_args];
    half immargs[num_32bit_args + num_64bit_args];
    dword expected_result[num_32bit_args + num_64bit_args];

    static_assert(RAND_MAX == 2147483647, "this code depends on RAND_MAX being int32_max");

    for (int i = 0; i < num_32bit_args; i++) {
        sword regarg = rand() << 1;
        half immarg = rand();
        regargs[i] = (sdword)regarg;
        immargs[i] = immarg;

    }

    for (int i = 0; i < num_64bit_args; i++) {
        dword regarg = rand();
        regarg <<= 32;
        regarg |= rand();
        half immarg = rand();
        regargs[num_32bit_args + i] = regarg;
        immargs[num_32bit_args + i] = immarg;
    }
    memset(&N64CPU, 0, sizeof(N64CPU));
    for (int i = 0; i < num_cases; i++) {
        mips_instruction_t instruction;
        instruction.i.immediate = immargs[i];

        instruction.i.rt = 1;
        instruction.i.rs = 1;
        N64CPU.gpr[1] = regargs[i];

        handler(instruction);

        expected_result[i] = N64CPU.gpr[1];
    }

    printf("align(4)\n");
    printf("NumCases: \n\tdw $%08X\n\n", num_cases);
    printf("align(8)\n");
    printf("RegArgs:\n");
    for (int i = 0; i < num_cases; i++) {
        printf("\tdd $%016lX\n", regargs[i]);
    }
    printf("align(8)\n");
    printf("Expected:\n");
    for (int i = 0; i < num_cases; i++) {
        printf("\tdd $%016lX\n", expected_result[i]);
    }
    printf("align(2)\n");
    printf("\nImmArgs:\n");
    for (int i = 0; i < num_cases; i++) {
        printf("\tdh $%04X\n", immargs[i]);
    }
}

void gen_shift(char* name, mipsinstr_handler_t handler) {
    const int num_64bit_args = 1500;
    const int num_32bit_args = 1500;

    int num_cases = num_32bit_args + num_64bit_args;
    int num_results = num_cases * 32;

    dword regargs[num_cases];
    dword expected_result[num_results];

    static_assert(RAND_MAX == 2147483647, "this code depends on RAND_MAX being int32_max");

    for (int i = 0; i < num_32bit_args; i++) {
        sword regarg = rand() << 1;
        regargs[i] = (sdword)regarg;
    }

    for (int i = 0; i < num_64bit_args; i++) {
        dword regarg = rand();
        regarg <<= 32;
        regarg |= rand();
        regargs[num_32bit_args + i] = regarg;
    }
    memset(&N64CPU, 0, sizeof(N64CPU));
    for (int i = 0; i < num_cases; i++) {
        for (int sa = 0; sa < 32; sa++) {
            mips_instruction_t instruction;
            instruction.r.sa = sa;

            instruction.r.rt = 1;
            instruction.r.rd = 1;
            N64CPU.gpr[1] = regargs[i];

            handler(instruction);

            expected_result[i * 32 + sa] = N64CPU.gpr[1];
        }
    }

    printf("align(4)\n");
    printf("NumCases: \n\tdw $%08X\n\n", num_cases);
    printf("align(8)\n");
    printf("RegArgs:\n");
    for (int i = 0; i < num_cases; i++) {
        printf("\tdd $%016lX\n", regargs[i]);
    }
    printf("align(8)\n");
    printf("Expected:\n");
    for (int i = 0; i < num_results; i++) {
        printf("\tdd $%016lX\n", expected_result[i]);
    }
}

int main(int argc, char** argv) {
    if (argc != 2) {
        logdie("Usage: %s <instruction>", argv[0]);
    }

    char* instruction_name = argv[1];
    mipsinstr_handler_t instruction_handler = NULL;
    mipsinstr_type instruction_type;

    // MATCH("addi", mips_addi, IMM_RSRTI_EXCEPTION); // TODO exception thrown
    // MATCH("daddi", mips_daddi, IMM_RSRTI_EXCEPTION); // TODO exception thrown

    MATCH("andi", mips_andi, IMM_RSRTI);
    MATCH("addiu", mips_addiu, IMM_RSRTI);
    MATCH("slti", mips_slti, IMM_RSRTI);
    MATCH("sltiu", mips_sltiu, IMM_RSRTI);
    MATCH("ori", mips_ori, IMM_RSRTI);
    MATCH("xori", mips_xori, IMM_RSRTI);
    MATCH("daddiu", mips_daddiu, IMM_RSRTI);


    MATCH("beq", mips_beq, BRANCH);
    MATCH("beql", mips_beql, BRANCH);
    MATCH("bgtz", mips_bgtz, BRANCH);
    MATCH("bgtzl", mips_bgtzl, BRANCH);
    MATCH("blez", mips_blez, BRANCH);
    MATCH("blezl", mips_blezl, BRANCH);
    MATCH("bne", mips_bne, BRANCH);
    MATCH("bnel", mips_bnel, BRANCH);
    MATCH("bltz", mips_ri_bltz, BRANCH);
    MATCH("bltzl", mips_ri_bltzl, BRANCH);
    MATCH("bgez", mips_ri_bgez, BRANCH);
    MATCH("bgezl", mips_ri_bgezl, BRANCH);
    MATCH("bgezal", mips_ri_bgezal, BRANCH);

    MATCH("j", mips_j, JUMP);
    MATCH("jal", mips_jal, JUMP);
    MATCH("jr", mips_spc_jr, JUMP);
    MATCH("jalr", mips_spc_jalr, JUMP);

    MATCH("sll", mips_spc_sll, SHIFT);
    MATCH("srl", mips_spc_srl, SHIFT);
    MATCH("sra", mips_spc_sra, SHIFT);
    MATCH("dsll", mips_spc_dsll, SHIFT);
    MATCH("dsll32", mips_spc_dsll32, SHIFT);
    MATCH("dsra32", mips_spc_dsra32, SHIFT);


    MATCH("srav", mips_spc_srav, RSRTRD);
    MATCH("sllv", mips_spc_sllv, RSRTRD);
    MATCH("srlv", mips_spc_srlv, RSRTRD);
    MATCH("dsllv", mips_spc_dsllv, RSRTRD);
    MATCH("add", mips_spc_add, RSRTRD);
    MATCH("addu", mips_spc_addu, RSRTRD);
    MATCH("and", mips_spc_and, RSRTRD);
    MATCH("nor", mips_spc_nor, RSRTRD);
    MATCH("sub", mips_spc_sub, RSRTRD);
    MATCH("subu", mips_spc_subu, RSRTRD);
    MATCH("or", mips_spc_or, RSRTRD);
    MATCH("xor", mips_spc_xor, RSRTRD);
    MATCH("slt", mips_spc_slt, RSRTRD);
    MATCH("sltu", mips_spc_sltu, RSRTRD);
    MATCH("dadd", mips_spc_dadd, RSRTRD);

    MATCH("mult", mips_spc_mult, MULT);
    MATCH("multu", mips_spc_multu, MULT);
    MATCH("div", mips_spc_div, MULT);
    MATCH("divu", mips_spc_divu, MULT);
    MATCH("dmultu", mips_spc_dmultu, MULT);
    MATCH("ddiv", mips_spc_ddiv, MULT);
    MATCH("ddivu", mips_spc_ddivu, MULT);

    // TODO: CP1
    // MATCH("cp_bc1f", mips_cp_bc1f, UNKNOWN);
    // MATCH("cp_bc1fl", mips_cp_bc1fl, UNKNOWN);
    // MATCH("cp_bc1t", mips_cp_bc1t, UNKNOWN);
    // MATCH("cp_bc1tl", mips_cp_bc1tl, UNKNOWN);
    // MATCH("cp_mul_d", mips_cp_mul_d, UNKNOWN);
    // MATCH("cp_mul_s", mips_cp_mul_s, UNKNOWN);
    // MATCH("cp_div_d", mips_cp_div_d, UNKNOWN);
    // MATCH("cp_div_s", mips_cp_div_s, UNKNOWN);
    // MATCH("cp_add_d", mips_cp_add_d, UNKNOWN);
    // MATCH("cp_add_s", mips_cp_add_s, UNKNOWN);
    // MATCH("cp_sub_d", mips_cp_sub_d, UNKNOWN);
    // MATCH("cp_sub_s", mips_cp_sub_s, UNKNOWN);
    // MATCH("cp_trunc_l_d", mips_cp_trunc_l_d, UNKNOWN);
    // MATCH("cp_trunc_l_s", mips_cp_trunc_l_s, UNKNOWN);
    // MATCH("cp_trunc_w_d", mips_cp_trunc_w_d, UNKNOWN);
    // MATCH("cp_trunc_w_s", mips_cp_trunc_w_s, UNKNOWN);
    // MATCH("cp_cvt_d_s", mips_cp_cvt_d_s, UNKNOWN);
    // MATCH("cp_cvt_d_w", mips_cp_cvt_d_w, UNKNOWN);
    // MATCH("cp_cvt_d_l", mips_cp_cvt_d_l, UNKNOWN);
    // MATCH("cp_cvt_l_s", mips_cp_cvt_l_s, UNKNOWN);
    // MATCH("cp_cvt_l_d", mips_cp_cvt_l_d, UNKNOWN);
    // MATCH("cp_cvt_s_d", mips_cp_cvt_s_d, UNKNOWN);
    // MATCH("cp_cvt_s_w", mips_cp_cvt_s_w, UNKNOWN);
    // MATCH("cp_cvt_s_l", mips_cp_cvt_s_l, UNKNOWN);
    // MATCH("cp_cvt_w_s", mips_cp_cvt_w_s, UNKNOWN);
    // MATCH("cp_cvt_w_d", mips_cp_cvt_w_d, UNKNOWN);
    // MATCH("cp_sqrt_s", mips_cp_sqrt_s, UNKNOWN);
    // MATCH("cp_sqrt_d", mips_cp_sqrt_d, UNKNOWN);
    // MATCH("cp_c_f_s", mips_cp_c_f_s, UNKNOWN);
    // MATCH("cp_c_un_s", mips_cp_c_un_s, UNKNOWN);
    // MATCH("cp_c_eq_s", mips_cp_c_eq_s, UNKNOWN);
    // MATCH("cp_c_ueq_s", mips_cp_c_ueq_s, UNKNOWN);
    // MATCH("cp_c_olt_s", mips_cp_c_olt_s, UNKNOWN);
    // MATCH("cp_c_ult_s", mips_cp_c_ult_s, UNKNOWN);
    // MATCH("cp_c_ole_s", mips_cp_c_ole_s, UNKNOWN);
    // MATCH("cp_c_ule_s", mips_cp_c_ule_s, UNKNOWN);
    // MATCH("cp_c_sf_s", mips_cp_c_sf_s, UNKNOWN);
    // MATCH("cp_c_ngle_s", mips_cp_c_ngle_s, UNKNOWN);
    // MATCH("cp_c_seq_s", mips_cp_c_seq_s, UNKNOWN);
    // MATCH("cp_c_ngl_s", mips_cp_c_ngl_s, UNKNOWN);
    // MATCH("cp_c_lt_s", mips_cp_c_lt_s, UNKNOWN);
    // MATCH("cp_c_nge_s", mips_cp_c_nge_s, UNKNOWN);
    // MATCH("cp_c_le_s", mips_cp_c_le_s, UNKNOWN);
    // MATCH("cp_c_ngt_s", mips_cp_c_ngt_s, UNKNOWN);
    // MATCH("cp_c_f_d", mips_cp_c_f_d, UNKNOWN);
    // MATCH("cp_c_un_d", mips_cp_c_un_d, UNKNOWN);
    // MATCH("cp_c_eq_d", mips_cp_c_eq_d, UNKNOWN);
    // MATCH("cp_c_ueq_d", mips_cp_c_ueq_d, UNKNOWN);
    // MATCH("cp_c_olt_d", mips_cp_c_olt_d, UNKNOWN);
    // MATCH("cp_c_ult_d", mips_cp_c_ult_d, UNKNOWN);
    // MATCH("cp_c_ole_d", mips_cp_c_ole_d, UNKNOWN);
    // MATCH("cp_c_ule_d", mips_cp_c_ule_d, UNKNOWN);
    // MATCH("cp_c_sf_d", mips_cp_c_sf_d, UNKNOWN);
    // MATCH("cp_c_ngle_d", mips_cp_c_ngle_d, UNKNOWN);
    // MATCH("cp_c_seq_d", mips_cp_c_seq_d, UNKNOWN);
    // MATCH("cp_c_ngl_d", mips_cp_c_ngl_d, UNKNOWN);
    // MATCH("cp_c_lt_d", mips_cp_c_lt_d, UNKNOWN);
    // MATCH("cp_c_nge_d", mips_cp_c_nge_d, UNKNOWN);
    // MATCH("cp_c_le_d", mips_cp_c_le_d, UNKNOWN);
    // MATCH("cp_c_ngt_d", mips_cp_c_ngt_d, UNKNOWN);
    // MATCH("cp_mov_s", mips_cp_mov_s, UNKNOWN);
    // MATCH("cp_mov_d", mips_cp_mov_d, UNKNOWN);
    // MATCH("cp_neg_s", mips_cp_neg_s, UNKNOWN);
    // MATCH("cp_neg_d", mips_cp_neg_d, UNKNOWN);

    // TODO: load/store
    // MATCH("ld", mips_ld, UNKNOWN);
    // MATCH("lui", mips_lui, UNKNOWN);
    // MATCH("lbu", mips_lbu, UNKNOWN);
    // MATCH("lhu", mips_lhu, UNKNOWN);
    // MATCH("lh", mips_lh, UNKNOWN);
    // MATCH("lw", mips_lw, UNKNOWN);
    // MATCH("lwu", mips_lwu, UNKNOWN);
    // MATCH("sb", mips_sb, UNKNOWN);
    // MATCH("sh", mips_sh, UNKNOWN);
    // MATCH("sw", mips_sw, UNKNOWN);
    // MATCH("sd", mips_sd, UNKNOWN);
    // MATCH("lb", mips_lb, UNKNOWN);
    // MATCH("lwl", mips_lwl, UNKNOWN);
    // MATCH("lwr", mips_lwr, UNKNOWN);
    // MATCH("swl", mips_swl, UNKNOWN);
    // MATCH("swr", mips_swr, UNKNOWN);
    // MATCH("ldl", mips_ldl, UNKNOWN);
    // MATCH("ldr", mips_ldr, UNKNOWN);
    // MATCH("sdl", mips_sdl, UNKNOWN);
    // MATCH("sdr", mips_sdr, UNKNOWN);
    // MATCH("ldc1", mips_ldc1, UNKNOWN);
    // MATCH("sdc1", mips_sdc1, UNKNOWN);
    // MATCH("lwc1", mips_lwc1, UNKNOWN);
    // MATCH("swc1", mips_swc1, UNKNOWN);

    if (instruction_handler == NULL) {
        logdie("unknown/unsupported instruction: %s", instruction_name);
    }

    switch (instruction_type) {
        case IMM_RSRTI:
            gen_imm_rsrti(instruction_name, instruction_handler);
            break;
        case BRANCH:
            logfatal("Unimplemented type: BRANCH");
            break;
        case JUMP:
            logfatal("Unimplemented type: JUMP");
            break;
        case RSRTRD:
            logfatal("Unimplemented type: RSRTRD");
            break;
        case SHIFT:
            gen_shift(instruction_name, instruction_handler);
            break;
        case MULT:
            logfatal("Unimplemented type: MULT");
            break;
    }
}