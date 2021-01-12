#ifndef N64_ASM_EMITTER_H
#define N64_ASM_EMITTER_H

#include "dynarec.h"
#include <system/n64system.h>
#include <dynasm/dasm_proto.h>

dasm_State* block_header();
void advance_pc(r4300i_t* compile_time_cpu, dasm_State** Dst);
dynarec_instruction_category_t compile_instruction(dasm_State** Dst, mips_instruction_t instr, word address, word block_length, word* extra_cycles);
void end_block(dasm_State** Dst, int block_length);
void end_block_early_on_branch_taken(dasm_State** Dst, int block_length);
#endif //N64_ASM_EMITTER_H
