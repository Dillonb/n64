#ifndef N64_IR_CONTEXT_H
#define N64_IR_CONTEXT_H

#include <util.h>
#include <cpu/r4300i.h>

// Number of IR instructions that can be cached per block. 4x the max number of instructions per block - should be safe.
#define IR_CACHE_SIZE 4096
// Number of values that can be flushed conditionally per block when the block is exited early.
#define IR_FLUSH_CACHE_SIZE 100

#define IR_GPR_BASE 0
#define IR_FGR_BASE 32
#define IR_GPR(r) ((r) + IR_GPR_BASE)
#define IR_FGR(r) ((r) + IR_FGR_BASE)
#define IR_IS_GPR(r) ((r) >= IR_GPR_BASE && (r) < (IR_GPR_BASE + 32))
#define IR_IS_FGR(r) ((r) >= IR_FGR_BASE && (r) < (IR_FGR_BASE + 32))

typedef enum ir_condition {
    CONDITION_NOT_EQUAL,
    CONDITION_EQUAL,
    CONDITION_LESS_THAN_SIGNED,
    CONDITION_LESS_THAN_UNSIGNED,
    CONDITION_GREATER_THAN_SIGNED,
    CONDITION_GREATER_THAN_UNSIGNED,
    CONDITION_LESS_OR_EQUAL_TO_SIGNED,
    CONDITION_LESS_OR_EQUAL_TO_UNSIGNED,
    CONDITION_GREATER_OR_EQUAL_TO_SIGNED,
    CONDITION_GREATER_OR_EQUAL_TO_UNSIGNED
} ir_condition_t;

typedef enum ir_float_condition {
    CONDITION_FLOAT_LE,
    CONDITION_FLOAT_LT,
    CONDITION_FLOAT_EQ,
    CONDITION_FLOAT_NGE,
    CONDITION_FLOAT_NGT,
} ir_float_condition_t;

typedef enum ir_value_type {
    VALUE_TYPE_U8,
    VALUE_TYPE_S8,
    VALUE_TYPE_S16,
    VALUE_TYPE_U16,
    VALUE_TYPE_S32,
    VALUE_TYPE_U32,
    VALUE_TYPE_U64,
    VALUE_TYPE_S64
} ir_value_type_t;

#define CASE_SIZE_8 case VALUE_TYPE_U8: case VALUE_TYPE_S8
#define CASE_SIZE_16 case VALUE_TYPE_S16: case VALUE_TYPE_U16
#define CASE_SIZE_32 case VALUE_TYPE_S32: case VALUE_TYPE_U32
#define CASE_SIZE_64 case VALUE_TYPE_U64: case VALUE_TYPE_S64

typedef enum ir_float_value_type {
    FLOAT_VALUE_TYPE_INVALID,
    FLOAT_VALUE_TYPE_WORD,
    FLOAT_VALUE_TYPE_LONG,
    FLOAT_VALUE_TYPE_SINGLE,
    FLOAT_VALUE_TYPE_DOUBLE
} ir_float_value_type_t;

typedef enum ir_float_convert_mode {
    FLOAT_CONVERT_MODE_CONVERT,
    FLOAT_CONVERT_MODE_TRUNC,
    FLOAT_CONVERT_MODE_ROUND,
    FLOAT_CONVERT_MODE_FLOOR,
} ir_float_convert_mode_t;

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
        u64 value_u64;
        s64 value_s64;
    };
} ir_set_constant_t;

typedef struct ir_set_float_constant {
    ir_float_value_type_t format;
    union {
        s32 value_word;
        s64 value_long;
        float value_single;
        double value_double;
    } PACKED;
} ir_set_float_constant_t;

typedef struct ir_instruction_flush {
    u8 guest_reg;
    struct ir_instruction* item;
    struct ir_instruction_flush* next;
} ir_instruction_flush_t;

typedef enum ir_register_type {
    REGISTER_TYPE_NONE,
    REGISTER_TYPE_GPR,
    REGISTER_TYPE_FGR_32,
    REGISTER_TYPE_FGR_64
} ir_register_type_t;

typedef struct ir_register_allocation {
    bool allocated;
    bool spilled;
    ir_register_type_t type;
    union {
        int host_reg;
        int spill_location;
    };
} ir_register_allocation_t;

INLINE ir_register_type_t float_val_to_reg_type(ir_float_value_type_t type) {
    switch (type) {
        case FLOAT_VALUE_TYPE_INVALID:
            logfatal("Getting register type of FLOAT_VALUE_TYPE_INVALID");
        case FLOAT_VALUE_TYPE_WORD:
        case FLOAT_VALUE_TYPE_SINGLE:
            return REGISTER_TYPE_FGR_32;
        case FLOAT_VALUE_TYPE_LONG:
        case FLOAT_VALUE_TYPE_DOUBLE:
            return REGISTER_TYPE_FGR_64;
    }
}

INLINE ir_register_allocation_t alloc_reg(int reg, ir_register_type_t type) {
    ir_register_allocation_t alloc;
    alloc.allocated = true;
    alloc.type = type;
    alloc.spilled = false;
    alloc.host_reg = reg;
    return alloc;
}

INLINE ir_register_allocation_t alloc_gpr(int reg) {
    return alloc_reg(reg, REGISTER_TYPE_GPR);
}

INLINE ir_register_allocation_t alloc_fgr_32(int reg) {
    return alloc_reg(reg, REGISTER_TYPE_FGR_32);
}

INLINE ir_register_allocation_t alloc_fgr_64(int reg) {
    return alloc_reg(reg, REGISTER_TYPE_FGR_64);
}

INLINE ir_register_allocation_t alloc_reg_spilled(int spill_location, ir_register_type_t type) {
    ir_register_allocation_t alloc;
    alloc.allocated = true;
    alloc.type = type;
    alloc.spilled = true;
    alloc.spill_location = spill_location;
    return alloc;
}

INLINE bool reg_alloc_equal(ir_register_allocation_t a, ir_register_allocation_t b) {
    if (a.allocated != b.allocated) {
        return false;
    }

    if (a.type != b.type) {
        return false;
    }

    if (a.spilled && b.spilled) {
        return a.spill_location == b.spill_location;
    } else if (!a.spilled && !b.spilled) {
        return a.host_reg == b.host_reg;
    }
    return false;
}

typedef struct dynarec_exception {
    u8 code;
    u64 virtual_address;
    int coprocessor_error;
} dynarec_exception_t;

typedef struct ir_instruction {
    // Metadata
    struct ir_instruction* next;
    struct ir_instruction* prev;
    int index;
    bool dead_code;
    ir_register_allocation_t reg_alloc;
    int last_use;

    enum {
        IR_NOP,
        IR_SET_CONSTANT,
        IR_SET_FLOAT_CONSTANT,
        IR_OR,
        IR_XOR,
        IR_AND,
        IR_SUB,
        IR_NOT,
        IR_ADD,
        IR_SHIFT,
        IR_STORE,
        IR_LOAD,
        IR_GET_PTR,
        IR_SET_PTR,
        IR_MASK_AND_CAST,
        IR_CHECK_CONDITION,
        IR_SET_COND_BLOCK_EXIT_PC,
        IR_SET_BLOCK_EXIT_PC,
        IR_COND_BLOCK_EXIT,
        IR_TLB_LOOKUP,
        IR_LOAD_GUEST_REG,
        IR_FLUSH_GUEST_REG,
        IR_MULTIPLY,
        IR_DIVIDE,
        IR_ERET,
        IR_CALL,
        IR_MOV_REG_TYPE,
        IR_FLOAT_CONVERT,
        IR_FLOAT_MULTIPLY,
        IR_FLOAT_DIVIDE,
        IR_FLOAT_ADD,
        IR_FLOAT_SUB,
        IR_FLOAT_SQRT,
        IR_FLOAT_ABS,
        IR_FLOAT_NEG,
        IR_FLOAT_CHECK_CONDITION
    } type;
    union {
        ir_set_constant_t set_constant;
        ir_set_float_constant_t set_float_constant;
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
            ir_register_type_t reg_type;
            struct ir_instruction* address;
        } load;
        struct {
            ir_value_type_t type;
            uintptr_t ptr;
        } get_ptr;
        struct {
            ir_value_type_t type;
            uintptr_t ptr;
            struct ir_instruction* value;
        } set_ptr;
        struct {
            ir_value_type_t type;
            struct ir_instruction* operand;
        } mask_and_cast;
        struct {
            struct ir_instruction* operand;
        } unary_op;
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
            ir_register_type_t guest_reg_type;
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
        struct {
            int reg;
        } get_cp0;
        struct {
            int reg;
            struct ir_instruction* value;
        } set_cp0;
        struct {
            ir_instruction_flush_t* regs_to_flush;
            struct ir_instruction* condition;
            int block_length;
            bool has_exception;
            dynarec_exception_t exception;
        } cond_block_exit;
        struct {
            struct ir_instruction* operand1;
            struct ir_instruction* operand2;
            ir_value_type_t mult_div_type;
        } mult_div;
        struct {
            uintptr_t function;
            int num_args;
            struct ir_instruction* arguments[3];
        } call;
        struct {
            struct ir_instruction* value;
            ir_register_type_t new_type;
            ir_value_type_t size;
        } mov_reg_type;
        struct {
            struct ir_instruction* value;
            ir_float_value_type_t from_type;
            ir_float_value_type_t to_type;
            ir_float_convert_mode_t mode;
        } float_convert;
        struct {
            struct ir_instruction* operand;
            ir_float_value_type_t format;
        } float_unary_op;
        struct {
            struct ir_instruction* operand1;
            struct ir_instruction* operand2;
            ir_float_value_type_t format;
        } float_bin_op;
    };
    struct {
        struct ir_instruction* operand1;
        struct ir_instruction* operand2;
        ir_float_condition_t condition;
        ir_float_value_type_t format;
    } float_check_condition;
} ir_instruction_t;

typedef struct ir_context {
    /*
     * Maps a guest register to the SSA value currently in it, as of the current context
     * 0-31  - GPR
     * 32-63 - FGR
     */
    ir_instruction_t* guest_reg_to_value[64];
    ir_register_type_t guest_reg_to_reg_type[64];

    ir_instruction_t ir_cache[IR_CACHE_SIZE];
    ir_instruction_t* ir_cache_head;
    ir_instruction_t* ir_cache_tail;
    int ir_cache_index;

    ir_instruction_flush_t ir_flush_cache[IR_FLUSH_CACHE_SIZE];
    int ir_flush_cache_index;

    bool block_end_pc_ir_emitted;
    bool block_end_pc_compiled;

    bool cp1_checked;
} ir_context_t;

extern ir_context_t ir_context;

void ir_context_reset();
void ir_instr_to_string(ir_instruction_t* instr, char* buf, size_t buf_size);

#define NO_GUEST_REG 0xFF

// Update a guest reg to point at a new value. Mostly used internally.
void update_guest_reg_mapping(u8 guest_reg, ir_instruction_t* value);
// Emit a constant to the IR, optionally associating it with a guest register.
ir_instruction_t* ir_emit_set_constant(ir_set_constant_t value, u8 guest_reg);
// Load a guest GPR, or return a reference to it if it's already loaded.
ir_instruction_t* ir_emit_load_guest_gpr(u8 guest_reg);
// Load a guest FGR, or return a reference to it if it's already loaded.
ir_instruction_t* ir_emit_load_guest_fgr(u8 guest_fgr, ir_float_value_type_t type);
// Flush a guest register back to memory. Emitted after a value's last usage to flush values back to memory.
ir_instruction_t* ir_emit_flush_guest_reg(ir_instruction_t* last_usage, ir_instruction_t* value, u8 guest_reg);
// OR two values together.
ir_instruction_t* ir_emit_or(ir_instruction_t* operand, ir_instruction_t* operand2, u8 guest_reg);
// XOR two values together.
ir_instruction_t* ir_emit_xor(ir_instruction_t* operand, ir_instruction_t* operand2, u8 guest_reg);
// AND two values together.
ir_instruction_t* ir_emit_and(ir_instruction_t* operand, ir_instruction_t* operand2, u8 guest_reg);
// subtract
ir_instruction_t* ir_emit_sub(ir_instruction_t* minuend, ir_instruction_t* subtrahend, ir_value_type_t type, u8 guest_reg);
// Bitwise NOT
ir_instruction_t* ir_emit_not(ir_instruction_t* operand, u8 guest_reg);
// Boolean NOT
ir_instruction_t* ir_emit_boolean_not(ir_instruction_t* operand, u8 guest_reg);
// ADD two values together.
ir_instruction_t* ir_emit_add(ir_instruction_t* operand, ir_instruction_t* operand2, u8 guest_reg);
// SHIFT a value of a given size in a given direction by a given amount
ir_instruction_t* ir_emit_shift(ir_instruction_t* operand, ir_instruction_t* amount, ir_value_type_t value_type, ir_shift_direction_t direction, u8 guest_reg);
// STORE a typed value into memory at an address
ir_instruction_t* ir_emit_store(ir_value_type_t type, ir_instruction_t* address, ir_instruction_t* value);
// LOAD a typed value a register from an address
ir_instruction_t* ir_emit_load(ir_value_type_t type, ir_instruction_t* address, u8 guest_reg);
// LOAD a typed value to a register from a host pointer.
ir_instruction_t* ir_emit_get_ptr(ir_value_type_t type, void* ptr, u8 guest_reg);
//STORE a typed value to a pointer
ir_instruction_t* ir_emit_set_ptr(ir_value_type_t type, void* ptr, ir_instruction_t* value);
// mask and cast a value to a different type.
ir_instruction_t* ir_emit_mask_and_cast(ir_instruction_t* operand, ir_value_type_t type, u8 guest_reg);
// check two operands with a condition and return 0 or 1
ir_instruction_t* ir_emit_check_condition(ir_condition_t condition, ir_instruction_t* operand1, ir_instruction_t* operand2, u8 guest_reg);
// set the block exit pc to one of two values based on a condition
ir_instruction_t* ir_emit_conditional_set_block_exit_pc(ir_instruction_t* condition, ir_instruction_t* pc_if_true, ir_instruction_t* pc_if_false);
// exit the block early if the condition is true
ir_instruction_t* ir_emit_conditional_block_exit(ir_instruction_t* condition, int index);
// exit the block early with an exception if the condition is true
ir_instruction_t* ir_emit_conditional_block_exit_exception(ir_instruction_t* condition, int index, dynarec_exception_t exception);
// set the block exit pc
ir_instruction_t* ir_emit_set_block_exit_pc(ir_instruction_t* address);
// fall back to the interpreter for the next num_instructions instructions
ir_instruction_t* ir_emit_interpreter_fallback(int num_instructions);
// lookup a memory address in the TLB
ir_instruction_t* ir_emit_tlb_lookup(ir_instruction_t* virtual_address, u8 guest_reg, bus_access_t bus_access);
// Multiply two values of type mult_div_type to get a double-sized result. Result must be accessed with ir_emit_get_ptr()
ir_instruction_t* ir_emit_multiply(ir_instruction_t* multiplicand1, ir_instruction_t* multiplicand2, ir_value_type_t multiplicand_type);
// Divide a value of type divide_type by a value of the same type. Result must be accessed with ir_emit_get_ptr()
ir_instruction_t* ir_emit_divide(ir_instruction_t* dividend, ir_instruction_t* divisor, ir_value_type_t divide_type);
// Run the MIPS ERET instruction
ir_instruction_t* ir_emit_eret();
// Call a function with no arguments. Result ignored.
void ir_emit_call_0(uintptr_t function);
// Call a function with one argument. Result ignored.
void ir_emit_call_1(uintptr_t function, ir_instruction_t* arg);
// Call a function with two arguments. Result ignored.
void ir_emit_call_2(uintptr_t function, ir_instruction_t* arg1, ir_instruction_t* arg2);
// Call a function with three arguments. Result ignored.
void ir_emit_call_3(uintptr_t function, ir_instruction_t* arg1, ir_instruction_t* arg2, ir_instruction_t* arg3);
// Move a value to a different register type
ir_instruction_t* ir_emit_mov_reg_type(ir_instruction_t* value, ir_register_type_t new_type, ir_value_type_t size, u8 new_reg);
// convert a float value to a different float value type
ir_instruction_t* ir_emit_float_convert(ir_instruction_t* value, ir_float_value_type_t from_type, ir_float_value_type_t to_type, u8 guest_reg, ir_float_convert_mode_t mode);
// Multiply two float values of type mult_type.
ir_instruction_t* ir_emit_float_mult(ir_instruction_t* multiplicand1, ir_instruction_t* multiplicand2, ir_float_value_type_t mult_type, u8 guest_reg);
// Divide two float values of type divide_type.
ir_instruction_t* ir_emit_float_div(ir_instruction_t* dividend, ir_instruction_t* divisor, ir_float_value_type_t divide_type, u8 guest_reg);
// Add two float values of type add_type.
ir_instruction_t* ir_emit_float_add(ir_instruction_t* operand1, ir_instruction_t* operand2, ir_float_value_type_t add_type, u8 guest_reg);
// Subtract two float values of type sub_type.
ir_instruction_t* ir_emit_float_sub(ir_instruction_t* operand1, ir_instruction_t* operand2, ir_float_value_type_t sub_type, u8 guest_reg);
// Get the square root of a value of type sqrt_type.
ir_instruction_t* ir_emit_float_sqrt(ir_instruction_t* operand, ir_float_value_type_t sqrt_type, u8 guest_reg);
// Get the absolute value of a value of type abs_type.
ir_instruction_t* ir_emit_float_abs(ir_instruction_t* operand, ir_float_value_type_t abs_type, u8 guest_reg);
// Get the negative value of a value of type neg_type.
ir_instruction_t* ir_emit_float_neg(ir_instruction_t* operand, ir_float_value_type_t neg_type, u8 guest_reg);
// Compare two floating point values, set the result to FCR31.compare
ir_instruction_t* ir_emit_float_check_condition(ir_float_condition_t cond, ir_instruction_t* operand1, ir_instruction_t* operand2, ir_float_value_type_t operand_type);


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

// Emit an s32 constant to the IR, optionally associating it with a guest register.
INLINE ir_instruction_t* ir_emit_set_constant_s32(s32 value, u8 guest_reg) {
    ir_set_constant_t constant;
    constant.type = VALUE_TYPE_S32;
    constant.value_s32 = value;
    return ir_emit_set_constant(constant, guest_reg);
}

// Emit a u32 constant to the IR, optionally associating it with a guest register.
INLINE ir_instruction_t* ir_emit_set_constant_u32(u32 value, u8 guest_reg) {
    ir_set_constant_t constant;
    constant.type = VALUE_TYPE_U32;
    constant.value_u32 = value;
    return ir_emit_set_constant(constant, guest_reg);
}

// Emit a u64 constant to the IR, optionally associating it with a guest register.
INLINE ir_instruction_t* ir_emit_set_constant_64(u64 value, u8 guest_reg) {
    ir_set_constant_t constant;
    constant.type = VALUE_TYPE_U64;
    constant.value_u64 = value;
    return ir_emit_set_constant(constant, guest_reg);
}

#endif //N64_IR_CONTEXT_H
