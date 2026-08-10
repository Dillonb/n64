#ifndef N64_RSP_DYNAREC_COMPARE_H
#define N64_RSP_DYNAREC_COMPARE_H

#include <stdbool.h>

// Set by RSP_COMPARE=1 in the environment.
bool rsp_compare_enabled();

// Runs one block on the dynarec and the same instructions on the interpreter, then reports any
// state that differs. Returns the number of instructions run.
int rsp_dynarec_step_compare();

#endif //N64_RSP_DYNAREC_COMPARE_H
