#ifndef N64_IR_CONTEXT_H
#define N64_IR_CONTEXT_H

#include <util.h>

// Number of IR instructions that can be cached per block. 4x the max number of instructions per block - should be safe.
#define IR_CACHE_SIZE 4096

typedef enum ir_condition {
    CONDITION_NOT_EQUAL
} ir_condition_t;

typedef enum ir_value_type {
    VALUE_TYPE_S16,
    VALUE_TYPE_U16,
    VALUE_TYPE_S32,
    VALUE_TYPE_U32,
    VALUE_TYPE_64
} ir_value_type_t;

typedef struct ir_set_constant {
    ir_value_type_t type;
    union {
        s16 value_s16;
        u16 value_u16;
        s32 value_s32;
        u32 value_u32;
        u64 value_64;
    };
} ir_set_constant_t;

typedef struct ir_instruction {
    enum {
        IR_NOP,
        IR_SET_CONSTANT,
        IR_OR,
        IR_AND,
        IR_ADD,
        IR_STORE,
        IR_LOAD,
        IR_MASK_AND_CAST,
        IR_CHECK_CONDITION,
        IR_SET_BLOCK_EXIT_PC
    } type;
    union {
        ir_set_constant_t set_constant;
        struct {
            int operand1;
            int operand2;
        } bin_op;
        struct {
            ir_value_type_t type;
            int address;
            int value;
        } store;
        struct {
            ir_value_type_t type;
            int address;
        } load;
        struct {
            ir_value_type_t type;
            int operand;
        } mask_and_cast;
        struct {
            int condition;
            int pc_if_true;
            int pc_if_false;
        } set_exit_pc;

        struct {
            ir_condition_t condition;
            int operand1;
            int operand2;
        } check_condition;
    };
} ir_instruction_t;

typedef struct ir_context {
    // Maps a guest register to the SSA value currently in it, as of the current context
    int guest_gpr_to_value[32];

    ir_instruction_t ir_cache[IR_CACHE_SIZE];
    int ir_cache_index;
} ir_context_t;

extern ir_context_t ir_context;

void ir_context_reset();
void ir_instr_to_string(int index, char* buf, size_t buf_size);

#define NO_GUEST_REG 0xFF

// Emit a constant to the IR, optionally associating it with a guest register.
int ir_emit_set_constant(ir_set_constant_t value, u8 guest_reg);
// Load a guest register, or return a reference to it if it's already loaded.
int ir_emit_load_guest_reg(u8 guest_reg);
// OR two values together.
int ir_emit_or(int operand, int operand2, u8 guest_reg);
// AND two values together.
int ir_emit_and(int operand, int operand2, u8 guest_reg);
// ADD two values together.
int ir_emit_add(int operand, int operand2, u8 guest_reg);
// STORE a typed value into memory at an address
int ir_emit_store(ir_value_type_t type, int address, int value);
// LOAD a typed value a register from an address
int ir_emit_load(ir_value_type_t type, int address, u8 guest_reg);
// mask and cast a value to a different type.
int ir_emit_mask_and_cast(int operand, ir_value_type_t type, u8 guest_reg);
// check two operands with a condition and return 0 or 1
int ir_emit_check_condition(ir_condition_t condition, int operand1, int operand2);
// set the block exit pc to one of two values based on a condition
int ir_emit_set_block_exit_pc(int condition, int pc_if_true, int pc_if_false);
// fall back to the interpreter for the next num_instructions instructions
int ir_emit_interpreter_fallback(int num_instructions);


// Emit an s16 constant to the IR, optionally associating it with a guest register.
INLINE int ir_emit_set_constant_s16(s16 value, u8 guest_reg) {
    ir_set_constant_t constant;
    constant.type = VALUE_TYPE_S16;
    constant.value_s16 = value;
    return ir_emit_set_constant(constant, guest_reg);
}

// Emit a u16 constant to the IR, optionally associating it with a guest register.
INLINE int ir_emit_set_constant_u16(u16 value, u8 guest_reg) {
    ir_set_constant_t constant;
    constant.type = VALUE_TYPE_U16;
    constant.value_u16 = value;
    return ir_emit_set_constant(constant, guest_reg);
}

// Emit a u64 constant to the IR, optionally associating it with a guest register.
INLINE int ir_emit_set_constant_64(u64 value, u8 guest_reg) {
    ir_set_constant_t constant;
    constant.type = VALUE_TYPE_64;
    constant.value_64 = value;
    return ir_emit_set_constant(constant, guest_reg);
}

#endif //N64_IR_CONTEXT_H
