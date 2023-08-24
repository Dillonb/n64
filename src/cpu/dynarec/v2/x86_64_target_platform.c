#include "target_platform.h"
#include "x86_64_registers.h"

int get_num_gprs() {
    return 16;
}

int get_num_fgrs() {
    return 16; // xmm0-xmm15
}

const int* get_preserved_gprs() {
    const static int preserved_regs[] = {
#ifdef N64_WIN
            REG_RBX,
            REG_RBP,
            REG_RDI,
            REG_RSI,
            // Yes, it's preserved, but we can't use it for register allocation.
            //REG_RSP,
            // Holds the CPU state, can't use for register allocation.
            //REG_R12,
            REG_R13,
            REG_R14,
            REG_R15
#else
            REG_RBX,
            // Yes, it's preserved, but we can't use it for register allocation.
            //REG_RSP,
            REG_RBP,
            // holds the CPU state
            //REG_R12,
            REG_R13,
            REG_R14,
            REG_R15
#endif // N64_WIN
    };

    return preserved_regs;
}

int get_num_preserved_gprs() {
#ifdef N64_WIN
    return 7; // 9 if we include the stack pointer and r12, but we can't.
#else
    return 5; // 7 if we include the stack pointer and r12, but we can't.
#endif // N64_WIN
}

const int* get_available_fgrs() {
    const static int available_regs[] = {
            3,
            4,
            5,
            6,
            7,
            8,
            9,
            10,
            11,
            12,
            13,
            14,
            15
    };

    return available_regs;
}

int get_num_available_fgrs() {
    return 16 - 3; // Reserve the first 3
}

const int* get_scratch_registers() {
    const static int scratch_regs[] = {
#ifdef N64_WIN
            REG_RAX, REG_RCX, REG_RDX, REG_R8, REG_R9, REG_R10, REG_R11
#else
            REG_RAX, REG_RDI, REG_RSI, REG_RDX, REG_RCX, REG_R8, REG_R9, REG_R10, REG_R11
#endif // N64_WIN
    };
    return scratch_regs;
}

int get_num_scratch_registers() {
#ifdef N64_WIN
    return 7;
#else
    return 9;
#endif // N64_WIN
}

bool is_valid_immediate(ir_value_type_t value_type) {
    switch (value_type) {
        case VALUE_TYPE_S8:
        case VALUE_TYPE_U8:
        case VALUE_TYPE_S16:
        case VALUE_TYPE_U16:
        case VALUE_TYPE_S32:
        case VALUE_TYPE_U32:
            return true;

        case VALUE_TYPE_U64:
        case VALUE_TYPE_S64:
            return false;
    }
}

const int* get_func_arg_registers() {
    const static int func_arg_regs[] = {
#ifdef N64_WIN
            REG_RCX,
            REG_RDX,
            REG_R8
            // Used as a temp register for spilled values.
            // REG_R9
#else
            REG_RDI,
            REG_RSI,
            REG_RDX,
            REG_RCX,
            REG_R8
            // Used as a temp register for spilled values.
            // REG_R9
#endif // N64_WIN
    };
    return func_arg_regs;
}

int get_num_func_arg_registers() {
#ifdef N64_WIN
    return 3;
#else
    return 5;
#endif // N64_WIN
}

int get_return_value_reg() {
    return REG_RAX;
}

// Can't be RAX or RDX (used for multiplies) and also can't overlap with get_func_arg_registers()
const int* get_temp_registers_for_spilled() {
    const static int temp_registers_for_spilled[] = {
            REG_R9, REG_R10, REG_R11 // these work for both System-V and Microsoft calling conventions
    };
    return temp_registers_for_spilled;
}