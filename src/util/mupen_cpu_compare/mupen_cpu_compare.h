#ifndef N64_MUPEN_CPU_COMPARE_H
#define N64_MUPEN_CPU_COMPARE_H

#include "../../cpu/r4300i.h"
#include "../../cpu/mips_instruction_decode.h"

typedef struct instruction_compare {
    bool valid;
    const char* name;
    void (*mine)(r4300i_t* cpu, mips_instruction_t instruction);
    void (*mupen)(r4300i_t* cpu, mips_instruction_t instruction);
} instruction_compare_t;

instruction_compare_t get_comparison(mips_instruction_type_t type);
#endif //N64_MUPEN_CPU_COMPARE_H
