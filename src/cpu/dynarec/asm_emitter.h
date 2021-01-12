#ifndef N64_ASM_EMITTER_H
#define N64_ASM_EMITTER_H

#include "dynarec.h"
#include <system/n64system.h>

void compile_new_block(n64_dynarec_t* dynarec, r4300i_t* compile_time_cpu, n64_dynarec_block_t* block, word virtual_address, word physical_address);

#endif //N64_ASM_EMITTER_H
