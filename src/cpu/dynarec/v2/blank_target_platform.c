#include "target_platform.h"
#include <dynarec/dynarec.h>

// This is just a stub to get things compiling.

int get_num_gprs() {
    return 0;
}

int get_num_fgrs() {
    return 0;
}

const int* get_preserved_gprs() {
    const static int preserved_regs[] = { };

    return preserved_regs;
}

int get_num_preserved_gprs() {
    return 0;
}

const int* get_available_fgrs() {
    const static int available_regs[] = { };

    return available_regs;
}

int get_num_available_fgrs() {
    return 0;
}

const int* get_scratch_registers() {
    const static int scratch_regs[] = { };
    return scratch_regs;
}

int get_num_scratch_registers() {
    return 0;
}

bool is_valid_immediate(ir_value_type_t value_type) {
    return true;
}

const int* get_func_arg_registers() {
    const static int func_arg_regs[] = { };
    return func_arg_regs;
}

int get_num_func_arg_registers() {
    return 0;
}

int get_return_value_reg() {
    return 0;
}

const int* get_temp_registers_for_spilled() {
    const static int temp_registers_for_spilled[] = { };
    return temp_registers_for_spilled;
}

void v2_compiler_init_platformspecific() { }

void v2_emit_block(n64_dynarec_block_t *block, u32 physical_address) {
    logfatal("v2_emit_block() called for blank target platform");
}