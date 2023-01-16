#ifndef N64_IR_CONTEXT_H
#define N64_IR_CONTEXT_H

#include <util.h>

// Number of IR instructions that can be cached per block. 4x the max number of instructions per block - should be safe.
#define IR_CACHE_SIZE 4096

typedef struct ir_instruction {
    enum {
        IR_UNKNOWN,
        IR_SET_REG_CONSTANT
    } type;
    union {
        struct {
            u64 value;
        } set_reg_constant;
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
void ir_emit_set_register_constant(u8 guest_reg, u64 value);

#endif //N64_IR_CONTEXT_H
