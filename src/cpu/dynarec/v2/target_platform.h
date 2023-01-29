#ifndef N64_TARGET_PLATFORM_H
#define N64_TARGET_PLATFORM_H

#include "ir_context.h"

// Get a list of the registers preserved under the target platform's calling convention
const int* get_preserved_registers();
// Get the number of registers preserved under the target platform's calling convention
int get_num_preserved_registers();

// Get a list of the registers NOT preserved under the target platform's calling convention
const int* get_scratch_registers();
// Get the number of registers NOT preserved under the target platform's calling convention
int get_num_scratch_registers();

// Gets whether a given value type is a valid immediate on the target platform
bool is_valid_immediate(ir_value_type_t value_type);

INLINE int get_num_registers() {
    return get_num_preserved_registers() + get_num_scratch_registers();
}

#endif //N64_TARGET_PLATFORM_H
