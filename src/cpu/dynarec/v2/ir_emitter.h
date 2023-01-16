#ifndef N64_IR_EMITTER_H
#define N64_IR_EMITTER_H

#include <util.h>
#include <mips_instruction_decode.h>

void emit_instruction_ir(mips_instruction_t instr);

#endif //N64_IR_EMITTER_H
