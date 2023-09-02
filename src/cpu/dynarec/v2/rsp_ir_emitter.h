#ifndef N64_RSP_IR_EMITTER_H
#define N64_RSP_IR_EMITTER_H

#include <mips_instruction_decode.h>
#include <util.h>

#define IR_RSP_EMITTER(name) void emit_rsp_##name##_ir(mips_instruction_t instruction, int index, u16 address)
#define CALL_IR_RSP_EMITTER_NOBREAK(name) emit_rsp_##name##_ir(instruction, index, address)
#define CALL_IR_RSP_EMITTER(name) CALL_IR_RSP_EMITTER_NOBREAK(name); break

#define IR_RSP_UNIMPLEMENTED(opc) logfatal("Unimplemented IR translation for RSP instruction " #opc " at PC: 0x%03X", address)

IR_RSP_EMITTER(instruction);

#endif // N64_RSP_IR_EMITTER_H