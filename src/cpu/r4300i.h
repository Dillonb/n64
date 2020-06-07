#ifndef N64_R4300I_H
#define N64_R4300I_H
#include <stdbool.h>

#include "../common/util.h"

typedef struct cp0 {

} cp0_t;

typedef struct r4300i {
    dword gpr[32];
    dword pc;
    dword mult_hi;
    dword mult_lo;
    bool llb;
    cp0_t cp0;
} r4300i_t;

void r4300i_step(r4300i_t* cpu, word instruction);

#endif //N64_R4300I_H
