#include "../src/common/util.h"
#include "../src/cpu/mips.h"

#define SHOULD_LOG_PASSED_TESTS false
#include "unit.h"

#define imm11_entry(_input, _immediate, _output) {.input = _input, .immediate = _immediate, .output = _output}

case_instr_1_1_imm addi_table[] = {
        imm11_entry(0x0000000000000000, 0x0000000000000001, 0x0000000000000001),
        imm11_entry(0x0000000000000001, 0x0000000000000000, 0x0000000000000001),
        imm11_entry(0xFFFFFFFFFFFFFFFF, 0x0000000000000001, 0x0000000000000000),
};

int main(int argc, char** argv) {
    test_table_instr_1_1_imm(addi_table, sizeof(addi_table) / sizeof(case_instr_1_1_imm), &mips_addi, "addi");

    if (tests_failed) {
        logdie("Tests failed!")
    }
}