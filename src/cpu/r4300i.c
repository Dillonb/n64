#include "r4300i.h"
#include "../common/log.h"
#include "disassemble.h"
#include "mips32.h"

const char* register_names[] = {
        "zero", "at", "v0", "v1", "a0", "a1", "a2", "a3", "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
        "s0",   "s1", "s2", "s3", "s4", "s5", "s6", "s7", "t8", "t9", "k0", "k1", "gp", "sp", "s8", "ra"
};

const char* cp0_register_names[] = {
        "Index", "Random", "EntryLo0", "EntryLo1", "Context", "PageMask", "Wired", "7", "BadVAddr", "Count", "EntryHi",
        "Compare", "Status", "Cause", "EPC", "PRId", "Config", "LLAddr", "WatchLo", "WatchHi", "XContext", "21", "22",
        "23", "24", "25", "Parity Error", "Cache Error", "TagLo", "TagHi"
};

#define MIPS32_CP     0b010000
#define MIPS32_LUI    0b001111
#define MIPS32_ADDI   0b001000
#define MIPS32_ADDIU  0b001001
#define MIPS32_ANDI   0b001100
#define MIPS32_LBU    0b100100
#define MIPS32_LW     0b100011
#define MIPS32_BEQ    0b000100
#define MIPS32_BEQL   0b010100
#define MIPS32_BLEZL  0b010110
#define MIPS32_BNE    0b000101
#define MIPS32_BNEL   0b010101
#define MIPS32_CACHE  0b101111
#define MIPS32_REGIMM 0b000001
#define MIPS32_SPCL   0b000000
#define MIPS32_SB     0b101000
#define MIPS32_SW     0b101011
#define MIPS32_ORI    0b001101
#define MIPS32_JAL    0b000011
#define MIPS32_SLTI   0b001010
#define MIPS32_XORI   0b001110

// Coprocessor
#define MTC0_MASK  0b11111111111000000000011111111111
#define MTC0_VALUE 0b01000000100000000000000000000000

// Special
#define FUNCT_NOP   0b000000
#define FUNCT_SRL   0b000010
#define FUNCT_JR    0b001000
#define FUNCT_MFHI  0b010000
#define FUNCT_MFLO  0b010010
#define FUNCT_MULTU 0b011001
#define FUNCT_ADD   0b100000
#define FUNCT_ADDU  0b100001
#define FUNCT_AND   0b100100
#define FUNCT_SUBU  0b100011
#define FUNCT_OR    0b100101
#define FUNCT_SLT   0b101010
#define FUNCT_SLTU  0b101011

// REGIMM
#define RT_BGEZL 0b00011

mips32_instruction_type_t decode_cp(r4300i_t* cpu, mips32_instruction_t instr) {
    if ((instr.raw & MTC0_MASK) == MTC0_VALUE) {
        return CP_MTC0;
    } else {
        logfatal("other/unknown MIPS32 Coprocessor: 0x%08X", instr.raw)
    }
}

mips32_instruction_type_t decode_special(r4300i_t* cpu, mips32_instruction_t instr) {
    switch (instr.r.funct) {
        case FUNCT_NOP:   return NOP;
        case FUNCT_SRL:   return SPC_SRL;
        case FUNCT_JR:    return SPC_JR;
        case FUNCT_MFHI:  return SPC_MFHI;
        case FUNCT_MFLO:  return SPC_MFLO;
        case FUNCT_MULTU: return SPC_MULTU;
        case FUNCT_ADD:   return SPC_ADD;
        case FUNCT_ADDU:  return SPC_ADDU;
        case FUNCT_AND:   return SPC_AND;
        case FUNCT_SUBU:  return SPC_SUBU;
        case FUNCT_OR:    return SPC_OR;
        case FUNCT_SLT:   return SPC_SLT;
        case FUNCT_SLTU:  return SPC_SLTU;
        default: logfatal("other/unknown MIPS32 Special 0x%08X with FUNCT: %d%d%d%d%d%d", instr.raw,
                instr.funct0, instr.funct1, instr.funct2, instr.funct3, instr.funct4, instr.funct5)
    }
}

mips32_instruction_type_t decode_regimm(r4300i_t* cpu, mips32_instruction_t instr) {
    switch (instr.i.rt) {
        case RT_BGEZL: return RI_BGEZL;
        default: logfatal("other/unknown MIPS32 REGIMM 0x%08X with RT: %d%d%d%d%d", instr.raw,
                          instr.rt0, instr.rt1, instr.rt2, instr.rt3, instr.rt4)
    }
}

mips32_instruction_type_t decode(r4300i_t* cpu, dword pc, mips32_instruction_t instr) {
    char buf[50];
    disassemble32(pc, instr.raw, buf, 50);
    logdebug("[0x%08lX] %s", pc, buf)
    switch (instr.op) {
        case MIPS32_CP:     return decode_cp(cpu, instr);
        case MIPS32_SPCL:   return decode_special(cpu, instr);
        case MIPS32_REGIMM: return decode_regimm(cpu, instr);

        case MIPS32_LUI:   return LUI;
        case MIPS32_ADDIU: return ADDIU;
        case MIPS32_ADDI:  return ADDI;
        case MIPS32_ANDI:  return ANDI;
        case MIPS32_LBU:   return LBU;
        case MIPS32_LW:    return LW;
        case MIPS32_BEQ:   return BEQ;
        case MIPS32_BEQL:  return BEQL;
        case MIPS32_BLEZL: return BLEZL;
        case MIPS32_BNE:   return BNE;
        case MIPS32_BNEL:  return BNEL;
        case MIPS32_CACHE: return CACHE;
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
    dword pc = cpu->pc;

    mips32_instruction_t instruction;
    instruction.raw = cpu->read_word(pc);
    cpu->pc += 4;

    switch (decode(cpu, pc, instruction)) {
        case NOP: break;

        exec_instr(LUI,   lui)
        exec_instr(ADDI,  addi)
        exec_instr(ADDIU, addiu)
        exec_instr(ANDI,  andi)
        exec_instr(LBU,   lbu)
        exec_instr(LW,    lw)
        exec_instr(BEQ,   beq)
        exec_instr(BLEZL, blezl)
        exec_instr(BNE,   bne)
        exec_instr(BNEL,  bnel)
        exec_instr(CACHE, cache)
        exec_instr(SB,    sb)
        exec_instr(SW,    sw)
        exec_instr(ORI,   ori)
        exec_instr(JAL,   jal)
        exec_instr(SLTI,  slti)
        exec_instr(BEQL,  beql)
        exec_instr(XORI,  xori)

        // Coprocessor
        exec_instr(CP_MTC0, mtc0)

        // Special
        exec_instr(SPC_SRL,   spc_srl)
        exec_instr(SPC_JR,    spc_jr)
        exec_instr(SPC_MFHI,  spc_mfhi)
        exec_instr(SPC_MFLO,  spc_mflo)
        exec_instr(SPC_MULTU, spc_multu)
        exec_instr(SPC_ADD,   spc_add)
        exec_instr(SPC_ADDU,  spc_addu)
        exec_instr(SPC_AND,   spc_and)
        exec_instr(SPC_SUBU,  spc_subu)
        exec_instr(SPC_OR,    spc_or)
        exec_instr(SPC_SLT,   spc_slt)
        exec_instr(SPC_SLTU,  spc_sltu)

        // REGIMM
        exec_instr(RI_BGEZL, ri_bgezl)
        default: logfatal("Unknown instruction type!")
    }

    if (cpu->branch) {
        if (cpu->branch_delay == 0) {
            logdebug("[BRANCH DELAY] Branching to 0x%08X", cpu->branch_pc)
            cpu->pc = cpu->branch_pc;
            cpu->branch = false;
        } else {
            logdebug("[BRANCH DELAY] Need to execute %d more instruction(s).", cpu->branch_delay)
            cpu->branch_delay--;
        }
    }

}
