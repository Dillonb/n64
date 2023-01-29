#include "target_platform.h"

const int* get_preserved_registers() {
    const static int preserved_regs[] = {
            // rbx, rsp, rbp, r12, r13, r14, r15
            3, 4, 5, 12, 13, 14, 15
    };
    return preserved_regs;
}

int get_num_preserved_registers() {
    return 7;
}

const int* get_scratch_registers() {
    const static int scratch_regs[] = {
            // rax, rdi, rsi, rdx, rcx, r8, r9, r10, r11
            0, 7, 6, 2, 1, 8, 9, 10, 11
    };
    return scratch_regs;
}

int get_num_scratch_registers() {
    return 9;
}
