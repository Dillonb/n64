#ifndef N64_IR_CONTEXT_H
#define N64_IR_CONTEXT_H

#include <util.h>
#include <cpu/r4300i.h>

// Number of IR instructions that can be cached per block. 4x the max number of instructions per block - should be safe.
#define IR_CACHE_SIZE 4096

typedef enum ir_condition {
    CONDITION_NOT_EQUAL,
    CONDITION_EQUAL,
    CONDITION_LESS_THAN,
    CONDITION_GREATER_THAN
} ir_condition_t;

typedef enum ir_value_type {
    VALUE_TYPE_U8,
    VALUE_TYPE_S8,
    VALUE_TYPE_S16,
    VALUE_TYPE_U16,
    VALUE_TYPE_S32,
    VALUE_TYPE_U32,
    VALUE_TYPE_64
} ir_value_type_t;

typedef enum ir_shift_direction {
    SHIFT_DIRECTION_LEFT,
    SHIFT_DIRECTION_RIGHT
} ir_shift_direction_t;

typedef struct ir_set_constant {
    ir_value_type_t type;
    union {
        s8 value_s8;
        u8 value_u8;
        s16 value_s16;
        u16 value_u16;
        s32 value_s32;
        u32 value_u32;
        u64 value_64;
    };
} ir_set_constant_t;

typedef struct ir_instruction {
    // Metadata
    struct ir_instruction* next;
    struct ir_instruction* prev;
    int index;
    bool dead_code;
    int allocated_host_register;

    enum {
        IR_NOP,
        IR_SET_CONSTANT,
        IR_OR,
        IR_AND,
        IR_ADD,
        IR_SHIFT,
        IR_STORE,
        IR_LOAD,
        IR_MASK_AND_CAST,
        IR_CHECK_CONDITION,
        IR_SET_COND_BLOCK_EXIT_PC,
        IR_SET_BLOCK_EXIT_PC,
        IR_TLB_LOOKUP,
        IR_LOAD_GUEST_REG,
        IR_FLUSH_GUEST_REG
    } type;
    union {
        ir_set_constant_t set_constant;
        struct {
            struct ir_instruction* operand1;
            struct ir_instruction* operand2;
        } bin_op;
        struct {
            ir_value_type_t type;
            struct ir_instruction* address;
            struct ir_instruction* value;
        } store;
        struct {
            ir_value_type_t type;
            struct ir_instruction* address;
        } load;
        struct {
            ir_value_type_t type;
            struct ir_instruction* operand;
        } mask_and_cast;
        struct {
            struct ir_instruction* address;
        } set_exit_pc;
        struct {
            struct ir_instruction* condition;
            struct ir_instruction* pc_if_true;
            struct ir_instruction* pc_if_false;
        } set_cond_exit_pc;
        struct {
            ir_condition_t condition;
            struct ir_instruction* operand1;
            struct ir_instruction* operand2;
        } check_condition;
        struct {
            struct ir_instruction* virtual_address;
            bus_access_t bus_access;
        } tlb_lookup;
        struct {
            u8 guest_reg;
        } load_guest_reg;
        struct {
            struct ir_instruction* value;
            u8 guest_reg;
        } flush_guest_reg;
        struct {
            struct ir_instruction* operand;
            struct ir_instruction* amount;
            ir_value_type_t type;
            ir_shift_direction_t direction;
        } shift;
    };
} ir_instruction_t;

typedef struct ir_context {
    // Maps a guest register to the SSA value currently in it, as of the current context
    ir_instruction_t* guest_gpr_to_value[32];

    ir_instruction_t ir_cache[IR_CACHE_SIZE];
    ir_instruction_t* ir_cache_head;
    ir_instruction_t* ir_cache_tail;
    int ir_cache_index;

    bool block_end_pc_set;
} ir_context_t;

extern ir_context_t ir_context;

void ir_context_reset();
void ir_instr_to_string(ir_instruction_t* instr, char* buf, size_t buf_size);

#define NO_GUEST_REG 0xFF

// Emit a constant to the IR, optionally associating it with a guest register.
ir_instruction_t* ir_emit_set_constant(ir_set_constant_t value, u8 guest_reg);
// Load a guest register, or return a reference to it if it's already loaded.
ir_instruction_t* ir_emit_load_guest_reg(u8 guest_reg);
// Flush a guest register back to memory. Emitted at the end of the block to flush values back to memory.
ir_instruction_t* ir_emit_flush_guest_reg(ir_instruction_t* value, u8 guest_reg);
// OR two values together.
ir_instruction_t* ir_emit_or(ir_instruction_t* operand, ir_instruction_t* operand2, u8 guest_reg);
// AND two values together.
ir_instruction_t* ir_emit_and(ir_instruction_t* operand, ir_instruction_t* operand2, u8 guest_reg);
// ADD two values together.
ir_instruction_t* ir_emit_add(ir_instruction_t* operand, ir_instruction_t* operand2, u8 guest_reg);
// SHIFT a value of a given size in a given direction by a given amount
ir_instruction_t* ir_emit_shift(ir_instruction_t* operand, ir_instruction_t* amount, ir_value_type_t value_type, ir_shift_direction_t direction, u8 guest_reg);
// STORE a typed value into memory at an address
ir_instruction_t* ir_emit_store(ir_value_type_t type, ir_instruction_t* address, ir_instruction_t* value);
// LOAD a typed value a register from an address
ir_instruction_t* ir_emit_load(ir_value_type_t type, ir_instruction_t* address, u8 guest_reg);
// mask and cast a value to a different type.
ir_instruction_t* ir_emit_mask_and_cast(ir_instruction_t* operand, ir_value_type_t type, u8 guest_reg);
// check two operands with a condition and return 0 or 1
ir_instruction_t* ir_emit_check_condition(ir_condition_t condition, ir_instruction_t* operand1, ir_instruction_t* operand2, u8 guest_reg);
// set the block exit pc to one of two values based on a condition
ir_instruction_t* ir_emit_conditional_set_block_exit_pc(ir_instruction_t* condition, ir_instruction_t* pc_if_true, ir_instruction_t* pc_if_false);
// set the block exit pc
ir_instruction_t* ir_emit_set_block_exit_pc(ir_instruction_t* address);
// fall back to the interpreter for the next num_instructions instructions
ir_instruction_t* ir_emit_interpreter_fallback(int num_instructions);
// lookup a memory address in the TLB
ir_instruction_t* ir_emit_tlb_lookup(ir_instruction_t* virtual_address, u8 guest_reg, bus_access_t bus_access);


// Emit an s16 constant to the IR, optionally associating it with a guest register.
INLINE ir_instruction_t* ir_emit_set_constant_s16(s16 value, u8 guest_reg) {
    ir_set_constant_t constant;
    constant.type = VALUE_TYPE_S16;
    constant.value_s16 = value;
    return ir_emit_set_constant(constant, guest_reg);
}

// Emit a u16 constant to the IR, optionally associating it with a guest register.
INLINE ir_instruction_t* ir_emit_set_constant_u16(u16 value, u8 guest_reg) {
    ir_set_constant_t constant;
    constant.type = VALUE_TYPE_U16;
    constant.value_u16 = value;
    return ir_emit_set_constant(constant, guest_reg);
}

// Emit a u64 constant to the IR, optionally associating it with a guest register.
INLINE ir_instruction_t* ir_emit_set_constant_64(u64 value, u8 guest_reg) {
    ir_set_constant_t constant;
    constant.type = VALUE_TYPE_64;
    constant.value_64 = value;
    return ir_emit_set_constant(constant, guest_reg);
}

#endif //N64_IR_CONTEXT_H
