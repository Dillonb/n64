#ifndef N64_MIPS32_H
#define N64_MIPS32_H
#include "r4300i.h"

void add(r4300i_t* cpu, mips32_instruction_t instruction);
void addi(r4300i_t* cpu, mips32_instruction_t instruction);
void addiu(r4300i_t* cpu, mips32_instruction_t instruction);
void addu(r4300i_t* cpu, mips32_instruction_t instruction);

void and(r4300i_t* cpu, mips32_instruction_t instruction);
void andi(r4300i_t* cpu, mips32_instruction_t instruction);

void beq(r4300i_t* cpu, mips32_instruction_t instruction);
void bne(r4300i_t* cpu, mips32_instruction_t instruction);

void mtc0(r4300i_t* cpu, mips32_instruction_t instruction);
void lui(r4300i_t* cpu, mips32_instruction_t instruction);
void lw(r4300i_t* cpu, mips32_instruction_t instruction);
void sw(r4300i_t* cpu, mips32_instruction_t instruction);
void ori(r4300i_t* cpu, mips32_instruction_t instruction);

#endif //N64_MIPS32_H
