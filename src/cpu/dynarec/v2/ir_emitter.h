#ifndef N64_IR_EMITTER_H
#define N64_IR_EMITTER_H

#include <util.h>
#include <mips_instruction_decode.h>

#define IR_EMITTER(name) void emit_##name##_ir(mips_instruction_t instruction, int index, u64 virtual_address, u32 physical_address)
#define CALL_IR_EMITTER(name) emit_##name##_ir(instruction, index, virtual_address, physical_address); break

IR_EMITTER(instruction);

#endif //N64_IR_EMITTER_H
