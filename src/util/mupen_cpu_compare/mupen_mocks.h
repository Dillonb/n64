#ifndef N64_MUPEN_MOCKS_H
#define N64_MUPEN_MOCKS_H

#include "../../cpu/r4300i.h"
#include "../../cpu/mips_instruction_decode.h"

#define DECLARE_R4300 int64_t* sregs = (int64_t*)(r4300->gpr);

#define DECLARE_INSTRUCTION(NAME) static void NAME(r4300i_t* r4300, mips_instruction_t instruction)
#define ADD_TO_PC(x) /* ignore */

int r4300_read_aligned_word(r4300i_t* cpu, word address, word* dest) {
    *dest = cpu->read_word(address);
    return 1;
}

int r4300_read_aligned_dword(r4300i_t* cpu, word address, dword* dest) {
    *dest = cpu->read_dword(address);
    return 1;
}

int r4300_write_aligned_word(r4300i_t* cpu, word address, word value, word mask) {
    logfatal("Mask: 0x%08X", mask)
    //cpu->write_word(address, value);
}

int r4300_write_aligned_dword(r4300i_t* cpu, word address, dword value, dword mask) {
    logfatal("Mask: 0x%016lX", mask)
    //cpu->write_dword(address, value);
}

sdword* get_reg_sdword(r4300i_t* cpu, byte reg) {
    return (sdword*)cpu->gpr[reg];
}

#define SE8(a)  ((int64_t) ((int8_t) (a)))
#define SE16(a) ((int64_t) ((int16_t) (a)))
#define SE32(a) ((int64_t) ((int32_t) (a)))

#define rrt *((sdword*) &r4300->gpr[instruction.r.rt])
#define rrd *((sdword*) &r4300->gpr[instruction.r.rd])
#define rrs *((sdword*) &r4300->gpr[instruction.r.rs])
#define rsa (instruction.r.sa)
#define irt *((sdword*) &r4300->gpr[instruction.i.rt])
#define iimmediate ((shalf)instruction.i.immediate)
#define irs *((sdword*) &r4300->gpr[instruction.i.rs])
#define jinst_index instruction.j.target

#define rrt32 ((sword)r4300->gpr[instruction.r.rt])
#define rrs32 ((sword)r4300->gpr[instruction.r.rs])
#define irs32 ((sword)r4300->gpr[instruction.i.rs])

#define r4300_mult_hi(cpu) (&cpu->mult_hi)
#define r4300_mult_lo(cpu) (&cpu->mult_lo)
#define r4300_regs(cpu) cpu->gpr

#define PCADDR (r4300->pc - 4)

void mupen_jump(r4300i_t* r4300, mips_instruction_t instruction, word destination, bool condition, bool link, bool likely, bool cop1) {
    logfatal("Mupen jump")
}

#define DECLARE_JUMP(name, destination, condition, link, likely, cop1) \
static void name(r4300i_t* r4300, mips_instruction_t instruction) {      \
mupen_jump(r4300, instruction, destination, condition, link, likely, cop1);}

#endif //N64_MUPEN_MOCKS_H
