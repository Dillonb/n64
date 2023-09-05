#ifndef N64_TARGET_PLATFORM_H
#define N64_TARGET_PLATFORM_H

#include "ir_context.h"

// Get the total number of GPRs on the platform
int get_num_gprs();
// Get the total number of FGRs on the platform
int get_num_fgrs();

// Get a list of the registers preserved under the target platform's calling convention
const int* get_preserved_gprs();
// Get the number of registers preserved under the target platform's calling convention
int get_num_preserved_gprs();

const int* get_available_fgrs();
int get_num_available_fgrs();

INLINE int get_num_temp_fgrs() {
    return get_num_fgrs() - get_num_available_fgrs();
}

// Get a list of the registers NOT preserved under the target platform's calling convention
const int* get_scratch_registers();
// Get the number of registers NOT preserved under the target platform's calling convention
int get_num_scratch_registers();

// Gets a list of the registers used for passing arguments to functions under the target platform's calling convention
const int* get_func_arg_registers();
// Gets the number of registers used for passing arguments to functions under the target platform's calling convention
int get_num_func_arg_registers();
// Gets the register used to hold a function's return value under the target platform's calling convention
int get_return_value_reg();
// Pointer to the CPU state
int get_cpu_state_reg();

// Gets a list of registers we can safely use for holding temporary values that have been spilled to memory.
// These should not overlap with the function arg registers, or be any registers needed for special purposes (like multiplication/division on x86)
// 3 are required.
const int* get_temp_registers_for_spilled();

// Gets whether a given value type is a valid immediate on the target platform
bool is_valid_immediate(ir_value_type_t value_type);

// Shortcuts
#define TMPREG1 get_scratch_registers()[0]
#define TMPREG2 get_scratch_registers()[1]

#endif //N64_TARGET_PLATFORM_H
