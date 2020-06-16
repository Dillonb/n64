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

#define OPC_CP     0b010000
#define OPC_LUI    0b001111
#define OPC_ADDI   0b001000
#define OPC_ADDIU  0b001001
#define OPC_ANDI   0b001100
#define OPC_LBU    0b100100
#define OPC_LW     0b100011
#define OPC_BEQ    0b000100
#define OPC_BEQL   0b010100
#define OPC_BGTZ   0b000111
#define OPC_BLEZL  0b010110
#define OPC_BNE    0b000101
#define OPC_BNEL   0b010101
#define OPC_CACHE  0b101111
#define OPC_REGIMM 0b000001
#define OPC_SPCL   0b000000
#define OPC_SB     0b101000
#define OPC_SW     0b101011
#define OPC_ORI    0b001101
#define OPC_J      0b000010
#define OPC_JAL    0b000011
#define OPC_SLTI   0b001010
#define OPC_XORI   0b001110
#define OPC_LB     0b100000

// Coprocessor
#define MTC0_MASK  0b11111111111000000000011111111111
#define MTC0_VALUE 0b01000000100000000000000000000000

// Special
#define FUNCT_NOP   0b000000
#define FUNCT_SRL   0b000010
#define FUNCT_SLLV  0b000100
#define FUNCT_SRLV  0b000110
#define FUNCT_JR    0b001000
#define FUNCT_MFHI  0b010000
#define FUNCT_MFLO  0b010010
#define FUNCT_MULTU 0b011001
#define FUNCT_ADD   0b100000
#define FUNCT_ADDU  0b100001
#define FUNCT_AND   0b100100
#define FUNCT_SUBU  0b100011
#define FUNCT_OR    0b100101
#define FUNCT_XOR   0b100110
#define FUNCT_SLT   0b101010
#define FUNCT_SLTU  0b101011

// REGIMM
#define RT_BGEZL  0b00011
#define RT_BGEZAL 0b10001

mips32_instruction_type_t decode_cp(r4300i_t* cpu, mips32_instruction_t instr) {
    if ((instr.raw & MTC0_MASK) == MTC0_VALUE) {
        return MIPS32_CP_MTC0;
    } else {
        logfatal("other/unknown MIPS32 Coprocessor: 0x%08X", instr.raw)
    }
}

mips32_instruction_type_t decode_special(r4300i_t* cpu, mips32_instruction_t instr) {
    switch (instr.r.funct) {
        case FUNCT_NOP:   return MIPS32_NOP;
        case FUNCT_SRL:   return MIPS32_SPC_SRL;
        case FUNCT_SLLV:  return MIPS32_SPC_SLLV;
        case FUNCT_SRLV:  return MIPS32_SPC_SRLV;
        case FUNCT_JR:    return MIPS32_SPC_JR;
        case FUNCT_MFHI:  return MIPS32_SPC_MFHI;
        case FUNCT_MFLO:  return MIPS32_SPC_MFLO;
        case FUNCT_MULTU: return MIPS32_SPC_MULTU;
        case FUNCT_ADD:   return MIPS32_SPC_ADD;
        case FUNCT_ADDU:  return MIPS32_SPC_ADDU;
        case FUNCT_AND:   return MIPS32_SPC_AND;
        case FUNCT_SUBU:  return MIPS32_SPC_SUBU;
        case FUNCT_OR:    return MIPS32_SPC_OR;
        case FUNCT_XOR:   return MIPS32_SPC_XOR;
        case FUNCT_SLT:   return MIPS32_SPC_SLT;
        case FUNCT_SLTU:  return MIPS32_SPC_SLTU;
        default: logfatal("other/unknown MIPS32 Special 0x%08X with FUNCT: %d%d%d%d%d%d", instr.raw,
                instr.funct0, instr.funct1, instr.funct2, instr.funct3, instr.funct4, instr.funct5)
    }
}

mips32_instruction_type_t decode_regimm(r4300i_t* cpu, mips32_instruction_t instr) {
    switch (instr.i.rt) {
        case RT_BGEZL:  return MIPS32_RI_BGEZL;
        case RT_BGEZAL: return MIPS32_RI_BGEZAL;
        default: logfatal("other/unknown MIPS32 REGIMM 0x%08X with RT: %d%d%d%d%d", instr.raw,
                          instr.rt0, instr.rt1, instr.rt2, instr.rt3, instr.rt4)
    }
}

mips32_instruction_type_t decode(r4300i_t* cpu, dword pc, mips32_instruction_t instr) {
    char buf[50];
    disassemble32(pc, instr.raw, buf, 50);
    logdebug("[0x%08lX] %s", pc, buf)
    switch (instr.op) {
        case OPC_CP:     return decode_cp(cpu, instr);
        case OPC_SPCL:   return decode_special(cpu, instr);
        case OPC_REGIMM: return decode_regimm(cpu, instr);

        case OPC_LUI:   return MIPS32_LUI;
        case OPC_ADDIU: return MIPS32_ADDIU;
        case OPC_ADDI:  return MIPS32_ADDI;
        case OPC_ANDI:  return MIPS32_ANDI;
        case OPC_LBU:   return MIPS32_LBU;
        case OPC_LW:    return MIPS32_LW;
        case OPC_BEQ:   return MIPS32_BEQ;
        case OPC_BEQL:  return MIPS32_BEQL;
        case OPC_BGTZ:  return MIPS32_BGTZ;
        case OPC_BLEZL: return MIPS32_BLEZL;
        case OPC_BNE:   return MIPS32_BNE;
        case OPC_BNEL:  return MIPS32_BNEL;
        case OPC_CACHE: return MIPS32_CACHE;
        case OPC_SB:    return MIPS32_SB;
        case OPC_SW:    return MIPS32_SW;
        case OPC_ORI:   return MIPS32_ORI;
        case OPC_J:     return MIPS32_J;
        case OPC_JAL:   return MIPS32_JAL;
        case OPC_SLTI:  return MIPS32_SLTI;
        case OPC_XORI:  return MIPS32_XORI;
        case OPC_LB:    return MIPS32_LB;
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
        case MIPS32_NOP: break;

        exec_instr(MIPS32_LUI,   mips32_lui)
        exec_instr(MIPS32_ADDI,  mips32_addi)
        exec_instr(MIPS32_ADDIU, mips32_addiu)
        exec_instr(MIPS32_ANDI,  mips32_andi)
        exec_instr(MIPS32_LBU,   mips32_lbu)
        exec_instr(MIPS32_LW,    mips32_lw)
        exec_instr(MIPS32_BEQ,   mips32_beq)
        exec_instr(MIPS32_BLEZL, mips32_blezl)
        exec_instr(MIPS32_BNE,   mips32_bne)
        exec_instr(MIPS32_BNEL,  mips32_bnel)
        exec_instr(MIPS32_CACHE, mips32_cache)
        exec_instr(MIPS32_SB,    mips32_sb)
        exec_instr(MIPS32_SW,    mips32_sw)
        exec_instr(MIPS32_ORI,   mips32_ori)
        exec_instr(MIPS32_J,     mips32_j)
        exec_instr(MIPS32_JAL,   mips32_jal)
        exec_instr(MIPS32_SLTI,  mips32_slti)
        exec_instr(MIPS32_BEQL,  mips32_beql)
        exec_instr(MIPS32_BGTZ,  mips32_bgtz)
        exec_instr(MIPS32_XORI,  mips32_xori)
        exec_instr(MIPS32_LB,    mips32_lb)

        // Coprocessor
        exec_instr(MIPS32_CP_MTC0, mips32_mtc0)

        // Special
        exec_instr(MIPS32_SPC_SRL,   mips32_spc_srl)
        exec_instr(MIPS32_SPC_SLLV,  mips32_spc_sllv)
        exec_instr(MIPS32_SPC_SRLV,  mips32_spc_srlv)
        exec_instr(MIPS32_SPC_JR,    mips32_spc_jr)
        exec_instr(MIPS32_SPC_MFHI,  mips32_spc_mfhi)
        exec_instr(MIPS32_SPC_MFLO,  mips32_spc_mflo)
        exec_instr(MIPS32_SPC_MULTU, mips32_spc_multu)
        exec_instr(MIPS32_SPC_ADD,   mips32_spc_add)
        exec_instr(MIPS32_SPC_ADDU,  mips32_spc_addu)
        exec_instr(MIPS32_SPC_AND,   mips32_spc_and)
        exec_instr(MIPS32_SPC_SUBU,  mips32_spc_subu)
        exec_instr(MIPS32_SPC_OR,    mips32_spc_or)
        exec_instr(MIPS32_SPC_XOR,   mips32_spc_xor)
        exec_instr(MIPS32_SPC_SLT,   mips32_spc_slt)
        exec_instr(MIPS32_SPC_SLTU,  mips32_spc_sltu)

        // REGIMM
        exec_instr(MIPS32_RI_BGEZL,  mips32_ri_bgezl)
        exec_instr(MIPS32_RI_BGEZAL, mips32_ri_bgezal)
        default: logfatal("Unknown instruction type!")
    }

    if (cpu->branch) {
        if (cpu->branch_delay == 0) {
            logtrace("[BRANCH DELAY] Branching to 0x%08X", cpu->branch_pc)
            cpu->pc = cpu->branch_pc;
            cpu->branch = false;
        } else {
            logtrace("[BRANCH DELAY] Need to execute %d more instruction(s).", cpu->branch_delay)
            cpu->branch_delay--;
        }
    }

}
