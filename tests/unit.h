#ifndef N64_UNIT_H
#define N64_UNIT_H

static bool tests_failed = false;

#define failed(message,...) if (1) { \
    fprintf(stderr, COLOR_RED "[FAILED] ");\
    fprintf(stderr, message "\n" COLOR_END, ##__VA_ARGS__);\
    tests_failed = true;}

#define passed(message,...) if (1) { \
    fprintf(stderr, COLOR_GREEN "[PASSED] ");\
    fprintf(stderr, message "\n" COLOR_END, ##__VA_ARGS__);}

typedef struct {
    dword input;
    half immediate;
    dword output;
} case_instr_1_1_imm;

void test_instr_1_1_imm(case_instr_1_1_imm test_case, void (*instr)(r4300i_t *, mips_instruction_t), const char* instr_name) {
    for (int rinput = 1; rinput < 32; rinput++) {
        for (int routput = 1; routput < 32; routput++) {
            if (rinput == routput) continue;
            r4300i_t cpu;

            mips_instruction_t i;
            i.i.immediate = test_case.immediate;
            i.i.rs = rinput;
            i.i.rt = routput;

            set_register(&cpu, rinput, test_case.input);

            instr(&cpu, i);

            dword actual = get_register(&cpu, routput);

            if (actual != test_case.output) {
                failed("%s: r%d, (r%d)%ld, %d | Expected: %ld but got %ld", instr_name, routput, rinput, test_case.input, test_case.immediate, test_case.output, actual)
            } else if (SHOULD_LOG_PASSED_TESTS) {
                passed("%s: r%d, (r%d)%ld, %d | Expected: %ld and got %ld", instr_name, routput, rinput, test_case.input, test_case.immediate, test_case.output, actual)
            }
        }
    }
}

void test_table_instr_1_1_imm(case_instr_1_1_imm* table, int len, void (*instr)(r4300i_t *, mips_instruction_t), const char* instr_name) {
    for (int i = 0; i < len; i++) {
        test_instr_1_1_imm(table[i], instr, instr_name);
    }
}

#endif //N64_UNIT_H
