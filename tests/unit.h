#ifndef N64_UNIT_H
#define N64_UNIT_H
#include <cpu/r4300i_register_access.h>
#include <inttypes.h>

static int tests_failed = 0;

#define failed(message,...) if (1) { \
    printf(COLOR_RED "[FAILED] ");\
    printf(message "\n" COLOR_END, ##__VA_ARGS__);\
    tests_failed++;}

#define passed(message,...) if (1) { \
    printf(COLOR_GREEN "[PASSED] ");\
    printf(message "\n" COLOR_END, ##__VA_ARGS__);}

typedef struct {
    u64 input;
    u16 immediate;
    u64 output;
} case_instr_1_1_imm;

typedef struct {
    u64 r1;
    u64 r2;
    u64 output;
} case_instr_2_1;

typedef struct {
    u64 r1;
    u64 r2;
    bool taken;
} case_branch_instr;

typedef struct {
    u64 input;
    u8 sa;
    u64 output;
} case_sa_instr;

typedef struct {
    u64 r1;
    u64 r2;
    u64 lo;
    u64 hi;
} case_lohi_instr;


void test_instr_1_1_imm(case_instr_1_1_imm test_case, mipsinstr_handler_t instr, const char* instr_name) {
    memset(&N64CPU, 0, sizeof(N64CPU));
    int rinput = 1;
    int routput = 2;

    mips_instruction_t i;
    i.i.immediate = test_case.immediate;
    i.i.rs = rinput;
    i.i.rt = routput;

    set_register(rinput, test_case.input);

    instr(i);

    u64 actual = get_register(routput);

    if (actual != test_case.output) {
        failed("%s: r%d, (r%d)%" PRId64 ", %d | Expected: %" PRId64 " but got %" PRId64, instr_name, routput, rinput, test_case.input, test_case.immediate, test_case.output, actual)
    } else if (SHOULD_LOG_PASSED_TESTS) {
        passed("%s: r%d, (r%d)%" PRId64 ", %d | Expected: %" PRId64 " and got %" PRId64, instr_name, routput, rinput, test_case.input, test_case.immediate, test_case.output, actual)
    }
}

void test_instr_2_1(case_instr_2_1 test_case, mipsinstr_handler_t instr, const char* instr_name) {
    memset(&N64CPU, 0, sizeof(N64CPU));
    memset(&N64CPU, 0, sizeof(N64CPU));
    int rinput1 = 1;
    int rinput2 = 2;
    int routput = 3;

    mips_instruction_t i;
    i.r.rs = rinput1;
    i.r.rt = rinput2;
    i.r.rd = routput;

    set_register(rinput1, test_case.r1);
    set_register(rinput2, test_case.r2);

    instr(i);

    u64 actual = get_register(routput);

    if (actual != test_case.output) {
        failed("%s: r%d, (r%d)%" PRId64 ", (r%d)%" PRId64 " | Expected: %" PRId64 " but got %" PRId64, instr_name, routput, rinput1, test_case.r1, rinput2, test_case.r2, test_case.output, actual)
    } else if (SHOULD_LOG_PASSED_TESTS) {
        passed("%s: r%d, (r%d)%" PRId64 ", (r%d)%" PRId64 " | Expected: %" PRId64 " and got %" PRId64, instr_name, routput, rinput1, test_case.r1, rinput2, test_case.r2, test_case.output, actual)
    }
}

#define taken_macro(t) ((t) ? "taken" : "not taken")
void test_instr_branch(case_branch_instr test_case, mipsinstr_handler_t instr, const char* instr_name) {
    memset(&N64CPU, 0, sizeof(N64CPU));
    int r1 = 1;
    int r2 = 2;

    N64CPU.branch = false;

    mips_instruction_t i;
    i.i.rs = r1;
    i.i.rt = r2;
    i.i.immediate = 1; // skip one instruction

    set_register(r1, test_case.r1);
    set_register(r2, test_case.r2);

    instr(i);

    bool taken = N64CPU.next_pc == 4;

    if (taken != test_case.taken) {
        failed("%s: (r%d)0x%016" PRIX64 ", (r%d)0x%016" PRIX64 " | Expected: %s but got %s", instr_name, r1, test_case.r1, r2, test_case.r2, taken_macro(test_case.taken), taken_macro(taken))
    } else if (SHOULD_LOG_PASSED_TESTS) {
        passed("%s: (r%d)0x%016" PRIX64 ", (r%d)0x%016" PRIX64 " | Expected: %s and got %s", instr_name, r1, test_case.r1, r2, test_case.r2, taken_macro(test_case.taken), taken_macro(taken))
    }
}

void test_instr_sa(case_sa_instr test_case, mipsinstr_handler_t instr, const char* instr_name) {
    memset(&N64CPU, 0, sizeof(N64CPU));
    int rinput = 1;
    int routput = 2;

    mips_instruction_t i;
    i.r.rt = rinput;
    i.r.sa = test_case.sa;
    i.r.rd = routput;

    set_register(rinput, test_case.input);

    instr(i);

    u64 actual = get_register(routput);
    u64 expected = test_case.output;

    if (expected != actual) {
        failed("%s: (r%d)0x%016" PRIX64 ", 0x%02X | Expected: 0x%016" PRIX64 " but got 0x%016" PRIX64, instr_name, rinput, test_case.input, test_case.sa, expected, actual)
    } else if (SHOULD_LOG_PASSED_TESTS) {
        passed("%s: (r%d)0x%016" PRIX64 ", 0x%02X | Expected: 0x%016" PRIX64 " and got 0x%016" PRIX64, instr_name, rinput, test_case.input, test_case.sa, expected, actual)
    }
}

void test_instr_lohi(case_lohi_instr test_case, mipsinstr_handler_t instr, const char* instr_name) {
    memset(&N64CPU, 0, sizeof(N64CPU));
    int r1 = 1;
    int r2 = 2;

    mips_instruction_t i;
    i.r.rs = r1;
    i.r.rt = r2;

    set_register(r1, test_case.r1);
    set_register(r2, test_case.r2);

    instr(i);

    u64 expected_lo = test_case.lo;
    u64 expected_hi = test_case.hi;

    u64 actual_lo = N64CPU.mult_lo;
    u64 actual_hi = N64CPU.mult_hi;

    if (expected_lo != actual_lo) {
        failed("%s: (r%d)0x%016" PRIX64 ", (r%d)0x%016" PRIX64 " | LO Expected: 0x%016" PRIX64 " but got 0x%016" PRIX64, instr_name, r1, test_case.r1, r2, test_case.r2, expected_lo, actual_lo)
    } else if (expected_hi != actual_hi) {
        failed("%s: (r%d)0x%016" PRIX64 ", (r%d)0x%016" PRIX64 " | HI Expected: 0x%016" PRIX64 " but got 0x%016" PRIX64, instr_name, r1, test_case.r1, r2, test_case.r2, expected_hi, actual_hi)
    } else if (SHOULD_LOG_PASSED_TESTS) {
        passed("%s: (r%d)0x%016" PRIX64 ", (r%d)0x%016" PRIX64 " | LO Expected: 0x%016" PRIX64 " and got 0x%016" PRIX64, instr_name, r1, test_case.r1, r2, test_case.r2, expected_lo, actual_lo)
        passed("%s: (r%d)0x%016" PRIX64 ", (r%d)0x%016" PRIX64 " | HI Expected: 0x%016" PRIX64 " and got 0x%016" PRIX64, instr_name, r1, test_case.r1, r2, test_case.r2, expected_hi, actual_hi)
    }
}

#endif //N64_UNIT_H
