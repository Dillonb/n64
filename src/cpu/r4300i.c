#include "r4300i.h"
#include "../common/log.h"
#include "disassemble.h"
#include "mips32.h"

#define MIPS32_CP    0b010000
#define MIPS32_LUI   0b001111
#define MIPS32_ADDI  0b001000
#define MIPS32_ADDIU 0b001001
#define MIPS32_LW    0b100011
#define MIPS32_BNE   0b000101
#define MIPS32_BEQ   0b000100
#define MIPS32_NOP   0b000000
#define MIPS32_SW    0b101011
#define MIPS32_ORI   0b001101
#define MIPS32_JAL   0b000011
#define MIPS32_SLTI  0b001010

#define MTC0_MASK  0b11111111111000000000011111111111
#define MTC0_VALUE 0b01000000100000000000000000000000

mips32_instruction_type_t decode_cp(r4300i_t* cpu, mips32_instruction_t instr) {
    if ((instr.raw & MTC0_MASK) == MTC0_VALUE) {
        return MTC0;
    } else {
        logfatal("other/unknown MIPS32 Coprocessor")
    }
}

mips32_instruction_type_t decode(r4300i_t* cpu, mips32_instruction_t instr) {
    char buf[50];
    disassemble32(cpu->pc, instr.raw, buf, 50);
    logdebug("[0x%08lX] %s", cpu->pc - 4, buf)
    switch (instr.op) {
        case MIPS32_CP:    return decode_cp(cpu, instr);
        case MIPS32_LUI:   return LUI;
        case MIPS32_ADDIU: return ADDIU;
        case MIPS32_ADDI:  return ADDI;
        case MIPS32_LW:    return LW;
        case MIPS32_BNE:   return BNE;
        case MIPS32_BEQ:   return BEQ;
        case MIPS32_NOP:   return NOP;
        case MIPS32_SW:    return SW;
        case MIPS32_ORI:   return ORI;
        case MIPS32_JAL:   return JAL;
        case MIPS32_SLTI:  return SLTI;
        default:
            logfatal("Failed to decode instruction 0x%08X opcode %d%d%d%d%d%d [%s]",
                     instr.raw, instr.op0, instr.op1, instr.op2, instr.op3, instr.op4, instr.op5, buf)
    }
}

void r4300i_step(r4300i_t* cpu) {
    mips32_instruction_t instruction;
    instruction.raw = cpu->read_word(cpu->pc);
    cpu->pc += 4;

    switch (decode(cpu, instruction)) {
        case MTC0:
            mtc0(cpu, instruction);
            break;
        case LUI:
            lui(cpu, instruction);
            break;
        case ADDI:
            addi(cpu, instruction);
            break;
        case ADDIU:
            addiu(cpu, instruction);
            break;
        case LW:
            lw(cpu, instruction);
            break;
        case BNE:
            bne(cpu, instruction);
            break;
        case BEQ:
            beq(cpu, instruction);
        case NOP:
            break;
        case SW:
            sw(cpu, instruction);
            break;
        case ORI:
            ori(cpu, instruction);
            break;
        case JAL:
            jal(cpu, instruction);
            break;
        case SLTI:
            slti(cpu, instruction);
            break;
        default:
            logfatal("Unknown instruction type!")
    }
}
