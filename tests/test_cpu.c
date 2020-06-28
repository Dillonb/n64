#include "../src/common/util.h"
#include "../src/cpu/mips.h"

#define SHOULD_LOG_PASSED_TESTS false
#include "unit.h"

#define imm11_entry(_input, _immediate, _output) {.input = _input, .immediate = _immediate, .output = _output}
#define imm21_entry(_r1, _r2, _output) {.r1 = _r1, .r2 = _r2, .output = _output}

case_instr_1_1_imm addi_table[] = {
        imm11_entry(0x0000000000000000, 0x0001, 0x0000000000000001),
        imm11_entry(0x0000000000000001, 0x0000, 0x0000000000000001),
        // 16 bit negative number
        imm11_entry(0x0000000000000002, 0xFFFF, 0x0000000000000001),
        // 32 bit negative number
        imm11_entry(0x00000000FFFFFFFF, 0x0002, 0x0000000000000001),
        // 64 bit overflow
        imm11_entry(0xFFFFFFFFFFFFFFFF, 0x0001, 0x0000000000000000),
        // Sign extension
        imm11_entry(0x00000000FFFFFFFE, 0x0001, 0xFFFFFFFFFFFFFFFF),
        // 32 bit overflow
        imm11_entry(0x00000000FFFFFFFF, 0x0001, 0x0000000000000000),
};

case_instr_1_1_imm addiu_table[] = {
        imm11_entry(0x0000000000000000, 0x0001, 0x0000000000000001),
        imm11_entry(0x0000000000000001, 0x0000, 0x0000000000000001),
        // 16 bit negative number
        imm11_entry(0x0000000000000002, 0xFFFF, 0x0000000000000001),
        // 32 bit negative number
        imm11_entry(0x00000000FFFFFFFF, 0x0002, 0x0000000000000001),
        // 64 bit overflow
        imm11_entry(0xFFFFFFFFFFFFFFFF, 0x0001, 0x0000000000000000),
        // Sign extension
        imm11_entry(0x00000000FFFFFFFE, 0x0001, 0xFFFFFFFFFFFFFFFF),
        // 32 bit overflow
        imm11_entry(0x00000000FFFFFFFF, 0x0001, 0x0000000000000000),
};

case_instr_2_1 addu_table[] = {
        imm21_entry(0x0000000000000000, 0x0000000000000001, 0x0000000000000001),
};

int main(int argc, char** argv) {
    test_table_instr_1_1_imm(addi_table, sizeof(addi_table) / sizeof(case_instr_1_1_imm), &mips_addi, "addi");
    test_table_instr_1_1_imm(addiu_table, sizeof(addiu_table) / sizeof(case_instr_1_1_imm), &mips_addiu, "addiu");
    test_table_instr_2_1(addu_table, sizeof(addu_table) / sizeof(case_instr_2_1), &mips_spc_addu, "addu");

    if (tests_failed) {
        logdie("Tests failed!")
    }
}