#include "r4300i.h"
#include "../common/log.h"
#include "disassemble.h"
#include "mips32.h"

#define MIPS32_CP    0b010000
#define MIPS32_LUI   0b001111
#define MIPS32_ADDI  0b001000
#define MIPS32_ADDIU 0b001001
#define MIPS32_ANDI  0b001100
#define MIPS32_LW    0b100011
#define MIPS32_BEQ   0b000100
#define MIPS32_BEQL  0b010100
#define MIPS32_BNE   0b000101
#define MIPS32_NOP   0b000000
#define MIPS32_SB    0b101000
#define MIPS32_SW    0b101011
#define MIPS32_ORI   0b001101
#define MIPS32_JAL   0b000011
#define MIPS32_SLTI  0b001010
#define MIPS32_XORI  0b001110

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
        case MIPS32_ANDI:  return ANDI;
        case MIPS32_LW:    return LW;
        case MIPS32_BEQ:   return BEQ;
        case MIPS32_BEQL:  return BEQL;
        case MIPS32_BNE:   return BNE;
        case MIPS32_NOP:   return NOP;
        case MIPS32_SB:    return SB;
        case MIPS32_SW:    return SW;
        case MIPS32_ORI:   return ORI;
        case MIPS32_JAL:   return JAL;
        case MIPS32_SLTI:  return SLTI;
        case MIPS32_XORI:  return XORI;
        default:
            logfatal("Failed to decode instruction 0x%08X opcode %d%d%d%d%d%d [%s]",
                     instr.raw, instr.op0, instr.op1, instr.op2, instr.op3, instr.op4, instr.op5, buf)
    }
}

#define exec_instr(key, fn) case key: fn(cpu, instruction); break;

void r4300i_step(r4300i_t* cpu) {
    mips32_instruction_t instruction;
    instruction.raw = cpu->read_word(cpu->pc);
    cpu->pc += 4;

    if (cpu->branch) {
        if (cpu->branch_delay == 0) {
            cpu->pc = cpu->branch_pc;
            cpu->branch = false;
        } else {
            cpu->branch_delay--;
        }
    }

    switch (decode(cpu, instruction)) {
        case NOP: break;

        exec_instr(MTC0,  mtc0)
        exec_instr(LUI,   lui)
        exec_instr(ADDI,  addi)
        exec_instr(ADDIU, addiu)
        exec_instr(ANDI,  andi)
        exec_instr(LW,    lw)
        exec_instr(BNE,   bne)
        exec_instr(BEQ,   beq)
        exec_instr(SB,    sb)
        exec_instr(SW,    sw)
        exec_instr(ORI,   ori)
        exec_instr(JAL,   jal)
        exec_instr(SLTI,  slti)
        exec_instr(BEQL,  beql)
        exec_instr(XORI,  xori)

        default: logfatal("Unknown instruction type!")
    }
}
