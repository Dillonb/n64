#include <util.h>
#include <cpu/mips_instructions.h>

#include <string.h>

#define SHOULD_LOG_PASSED_TESTS false
#include "unit.h"

#define imm11_entry(_input, _immediate, _output) {.input = _input, .immediate = _immediate, .output = _output}
#define r21_entry(_r1, _r2, _output) {.r1 = _r1, .r2 = _r2, .output = _output}
#define branch_entry(_r1, _r2, _taken) {.r1 = _r1, .r2 = _r2, .taken = _taken}
#define sa_entry(_input, _sa, _output) {.input = _input, .sa = _sa, .output = _output}
#define lohi_entry(_r1, _r2, _lo, _hi) {.r1 = _r1, .r2 = _r2, .lo = _lo, .hi = _hi}

void run_testcase(const char *filename, mipsinstr_handler_t instr) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        logfatal("Unable to find testcase %s", filename);
    }

    char *line = NULL;
    size_t len = 0;

    while (getline(&line, &len, fp) != -1) {
        char* name = strtok(line, " ");
        char* mode = strtok(NULL, " ");

        if (strcmp(mode, "imm11") == 0) {
            case_instr_1_1_imm testcase;

            char *tok = strtok(NULL, " ");
            testcase.input = strtoul(tok, NULL, 16);

            tok = strtok(NULL, " ");
            testcase.immediate = strtoul(tok, NULL, 16);

            tok = strtok(NULL, " ");
            testcase.output = strtoul(tok, NULL, 16);

            test_instr_1_1_imm(testcase, instr, name);
        } else if (strcmp(mode, "branch") == 0) {
            case_branch_instr testcase;

            char *tok = strtok(NULL, " ");
            testcase.r1 = strtoul(tok, NULL, 16);

            tok = strtok(NULL, " ");
            testcase.r2 = strtoul(tok, NULL, 16);

            tok = strtok(NULL, " ");
            testcase.taken = strcmp(tok, "true") == 0 || strcmp(tok, "true\n") == 0;

            test_instr_branch(testcase, instr, name);
        } else if (strcmp(mode, "r21") == 0) {
            case_instr_2_1 testcase;

            char *tok = strtok(NULL, " ");
            testcase.r1 = strtoul(tok, NULL, 16);

            tok = strtok(NULL, " ");
            testcase.r2 = strtoul(tok, NULL, 16);

            tok = strtok(NULL, " ");
            testcase.output = strtoul(tok, NULL, 16);

            test_instr_2_1(testcase, instr, name);
        } else if (strcmp(mode, "sa") == 0) {
            case_sa_instr testcase;

            char *tok = strtok(NULL, " ");
            testcase.input = strtoul(tok, NULL, 16);

            tok = strtok(NULL, " ");
            testcase.sa = strtoul(tok, NULL, 16);

            tok = strtok(NULL, " ");
            testcase.output = strtoul(tok, NULL, 16);

            test_instr_sa(testcase, instr, name);
        } else if (strcmp(mode, "lohi") == 0) {
            case_lohi_instr testcase;

            char *tok = strtok(NULL, " ");
            testcase.r1 = strtoul(tok, NULL, 16);

            tok = strtok(NULL, " ");
            testcase.r2 = strtoul(tok, NULL, 16);

            tok = strtok(NULL, " ");
            testcase.lo = strtoul(tok, NULL, 16);

            tok = strtok(NULL, " ");
            testcase.hi = strtoul(tok, NULL, 16);

            test_instr_lohi(testcase, instr, name);
        } else {
            logfatal("Unknown mode: %s in file: %s", mode, filename);
        }
    }

    if (tests_failed) {
        logdie("Tests failed: %d", tests_failed);
    } else {
        printf("%s: passed!\n", filename);
    }
}

int main(int argc, char** argv) {
    n64cpu_ptr = malloc(sizeof(r4300i_t));
    run_testcase("addi.testcase",   &mips_addi);
    run_testcase("bgezal.testcase", &mips_ri_bgezal);
    run_testcase("bltz.testcase",   &mips_ri_bltz);
    run_testcase("nor.testcase",    &mips_spc_nor);
    run_testcase("sltu.testcase",   &mips_spc_sltu);
    run_testcase("addiu.testcase",  &mips_addiu);
    run_testcase("bgezl.testcase",  &mips_ri_bgezl);
    run_testcase("bnel.testcase",   &mips_bnel);
    run_testcase("ori.testcase",    &mips_ori);
    run_testcase("sra.testcase",    &mips_spc_sra);
    run_testcase("add.testcase",    &mips_spc_add);
    run_testcase("bgez.testcase",   &mips_ri_bgez);
    run_testcase("bne.testcase",    &mips_bne);
    run_testcase("or.testcase",     &mips_spc_or);
    run_testcase("srav.testcase",   &mips_spc_srav);
    run_testcase("addu.testcase",   &mips_spc_addu);
    run_testcase("bgtzl.testcase",  &mips_bgtzl);
    run_testcase("div.testcase",    &mips_spc_div);
    run_testcase("sll.testcase",    &mips_spc_sll);
    run_testcase("srl.testcase",    &mips_spc_srl);
    run_testcase("andi.testcase",   &mips_andi);
    run_testcase("bgtz.testcase",   &mips_bgtz);
    run_testcase("divu.testcase",   &mips_spc_divu);
    run_testcase("sllv.testcase",   &mips_spc_sllv);
    run_testcase("srlv.testcase",   &mips_spc_srlv);
    run_testcase("and.testcase",    &mips_spc_and);
    run_testcase("blezl.testcase",  &mips_blezl);
    run_testcase("dsll.testcase",   &mips_spc_dsll);
    run_testcase("slti.testcase",   &mips_slti);
    run_testcase("subu.testcase",   &mips_spc_subu);
    run_testcase("beql.testcase",   &mips_beql);
    run_testcase("blez.testcase",   &mips_blez);
    run_testcase("mult.testcase",   &mips_spc_mult);
    run_testcase("sltiu.testcase",  &mips_sltiu);
    run_testcase("xori.testcase",   &mips_xori);
    run_testcase("beq.testcase",    &mips_beq);
    run_testcase("bltzl.testcase",  &mips_ri_bltzl);
    run_testcase("multu.testcase",  &mips_spc_multu);
    run_testcase("slt.testcase",    &mips_spc_slt);
    run_testcase("xor.testcase",    &mips_spc_xor);

    free(n64cpu_ptr);
    n64cpu_ptr = NULL;

    if (tests_failed) {
        logdie("Tests failed: %d", tests_failed);
    }
}