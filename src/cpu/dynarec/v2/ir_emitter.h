#ifndef N64_IR_EMITTER_H
#define N64_IR_EMITTER_H

#include <util.h>
#include <mips_instruction_decode.h>
#include "ir_context.h"

#define IR_EMITTER(name) void emit_##name##_ir(mips_instruction_t instruction, int index, u64 virtual_address, u32 physical_address)
#define CALL_IR_EMITTER_NOBREAK(name) emit_##name##_ir(instruction, index, virtual_address, physical_address)
#define CALL_IR_EMITTER(name) CALL_IR_EMITTER_NOBREAK(name); break
#define IR_UNIMPLEMENTED(opc) logfatal("Unimplemented IR translation for instruction " #opc " at PC: 0x%016" PRIX64, virtual_address)

ir_instruction_t* ir_get_memory_access_address(int index, mips_instruction_t instruction, bus_access_t bus_access);
void ir_emit_conditional_branch(ir_instruction_t* condition, s16 offset, u64 virtual_address);
void ir_emit_conditional_branch_likely(ir_instruction_t* condition, s16 offset, u64 virtual_address, int index);

IR_EMITTER(instruction);

#endif //N64_IR_EMITTER_H
