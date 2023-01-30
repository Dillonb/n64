#include "target_platform.h"

typedef enum x86_64_register {
    REG_RAX,
    REG_RCX,
    REG_RDX,
    REG_RBX,
    REG_RSP,
    REG_RBP,
    REG_RSI,
    REG_RDI,
    REG_R8,
    REG_R9,
    REG_R10,
    REG_R11,
    REG_R12,
    REG_R13,
    REG_R14,
    REG_R15
} x86_64_register_t;

const int* get_preserved_registers() {
    const static int preserved_regs[] = {
            REG_RBX, REG_RSP, REG_RBP, REG_R12, REG_R13, REG_R14, REG_R15
    };

    return preserved_regs;
}

int get_num_preserved_registers() {
    return 7;
}

const int* get_scratch_registers() {
    const static int scratch_regs[] = {
            REG_RAX, REG_RDI, REG_RSI, REG_RDX, REG_RCX, REG_R8, REG_R9, REG_R10, REG_R11
    };
    return scratch_regs;
}

int get_num_scratch_registers() {
    return 9;
}

bool is_valid_immediate(ir_value_type_t value_type) {
    switch (value_type) {
        case VALUE_TYPE_S16:
        case VALUE_TYPE_U16:
        case VALUE_TYPE_S32:
        case VALUE_TYPE_U32:
            return true;

        case VALUE_TYPE_64:
            return false;
    }
}

const int* get_func_arg_registers() {
    const static int func_arg_regs[] = {
            REG_RDI,
            REG_RSI,
            REG_RDX,
            REG_RCX,
            REG_R8,
            REG_R9
    };
    return func_arg_regs;
}

int get_num_func_arg_registers() {
    return 6;
}

int get_return_value_reg() {
    return REG_RAX;
}
