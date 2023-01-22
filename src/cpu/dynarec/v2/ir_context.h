#ifndef N64_IR_CONTEXT_H
#define N64_IR_CONTEXT_H

#include <util.h>

// Number of IR instructions that can be cached per block. 4x the max number of instructions per block - should be safe.
#define IR_CACHE_SIZE 4096

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
        //s32 value_s32;
        //u32 value_u32;
        u64 value_64;
    };
} ir_set_constant_t;

typedef struct ir_instruction {
    enum {
        IR_UNKNOWN,
        IR_SET_CONSTANT,
        IR_OR,
        IR_AND,
        IR_ADD,
        IR_STORE,
        IR_LOAD,
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

#endif //N64_IR_CONTEXT_H
