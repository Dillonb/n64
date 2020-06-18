#include "r4300i.h"
#include "../common/log.h"
#include "disassemble.h"
#include "mips.h"

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
#define OPC_LD     0b110111
#define OPC_LUI    0b001111
#define OPC_ADDI   0b001000
#define OPC_ADDIU  0b001001
#define OPC_DADDI  0b011000
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
#define OPC_SD     0b111111
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
#define FUNCT_SLL   0b000000
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
#define FUNCT_DADD  0b101100

// REGIMM
#define RT_BGEZL  0b00011
#define RT_BGEZAL 0b10001

mips_instruction_type_t decode_cp(r4300i_t* cpu, word pc, mips_instruction_t instr) {
    if ((instr.raw & MTC0_MASK) == MTC0_VALUE) {
        return MIPS_CP_MTC0;
    } else {
        char buf[50];
        disassemble(pc, instr.raw, buf, 50);
        logfatal("other/unknown MIPS Coprocessor: 0x%08X [%s]", instr.raw, buf)
    }
}

mips_instruction_type_t decode_special(r4300i_t* cpu, word pc, mips_instruction_t instr) {
    switch (instr.r.funct) {
        case FUNCT_SLL:   return MIPS_SPC_SLL;
        case FUNCT_SRL:   return MIPS_SPC_SRL;
        case FUNCT_SLLV:  return MIPS_SPC_SLLV;
        case FUNCT_SRLV:  return MIPS_SPC_SRLV;
        case FUNCT_JR:    return MIPS_SPC_JR;
        case FUNCT_MFHI:  return MIPS_SPC_MFHI;
        case FUNCT_MFLO:  return MIPS_SPC_MFLO;
        case FUNCT_MULTU: return MIPS_SPC_MULTU;
        case FUNCT_ADD:   return MIPS_SPC_ADD;
        case FUNCT_ADDU:  return MIPS_SPC_ADDU;
        case FUNCT_AND:   return MIPS_SPC_AND;
        case FUNCT_SUBU:  return MIPS_SPC_SUBU;
        case FUNCT_OR:    return MIPS_SPC_OR;
        case FUNCT_XOR:   return MIPS_SPC_XOR;
        case FUNCT_SLT:   return MIPS_SPC_SLT;
        case FUNCT_SLTU:  return MIPS_SPC_SLTU;
        case FUNCT_DADD:  return MIPS_SPC_DADD;
        default: {
            char buf[50];
            disassemble(pc, instr.raw, buf, 50);
            logfatal("other/unknown MIPS Special 0x%08X with FUNCT: %d%d%d%d%d%d [%s]", instr.raw,
                     instr.funct0, instr.funct1, instr.funct2, instr.funct3, instr.funct4, instr.funct5, buf)
        }
    }
}

mips_instruction_type_t decode_regimm(r4300i_t* cpu, word pc, mips_instruction_t instr) {
    switch (instr.i.rt) {
        case RT_BGEZL:  return MIPS_RI_BGEZL;
        case RT_BGEZAL: return MIPS_RI_BGEZAL;
        default: {
            char buf[50];
            disassemble(pc, instr.raw, buf, 50);
            logfatal("other/unknown MIPS REGIMM 0x%08X with RT: %d%d%d%d%d [%s]", instr.raw,
                     instr.rt0, instr.rt1, instr.rt2, instr.rt3, instr.rt4, buf)
        }
    }
}

mips_instruction_type_t decode(r4300i_t* cpu, word pc, mips_instruction_t instr) {
    char buf[50];
    if (n64_log_verbosity >= LOG_VERBOSITY_DEBUG) {
        disassemble(pc, instr.raw, buf, 50);
        logdebug("[0x%08X]=0x%08X %s", pc, instr.raw, buf)
    }
    if (instr.raw == 0) {
        return MIPS_NOP;
    }
    switch (instr.op) {
        case OPC_CP:     return decode_cp(cpu, pc, instr);
        case OPC_SPCL:   return decode_special(cpu, pc, instr);
        case OPC_REGIMM: return decode_regimm(cpu, pc, instr);

        case OPC_LD:    return MIPS_LD;
        case OPC_LUI:   return MIPS_LUI;
        case OPC_ADDIU: return MIPS_ADDIU;
        case OPC_ADDI:  return MIPS_ADDI;
        case OPC_DADDI: return MIPS_DADDI;
        case OPC_ANDI:  return MIPS_ANDI;
        case OPC_LBU:   return MIPS_LBU;
        case OPC_LW:    return MIPS_LW;
        case OPC_BEQ:   return MIPS_BEQ;
        case OPC_BEQL:  return MIPS_BEQL;
        case OPC_BGTZ:  return MIPS_BGTZ;
        case OPC_BLEZL: return MIPS_BLEZL;
        case OPC_BNE:   return MIPS_BNE;
        case OPC_BNEL:  return MIPS_BNEL;
        case OPC_CACHE: return MIPS_CACHE;
        case OPC_SB:    return MIPS_SB;
        case OPC_SD:    return MIPS_SD;
        case OPC_SW:    return MIPS_SW;
        case OPC_ORI:   return MIPS_ORI;
        case OPC_J:     return MIPS_J;
        case OPC_JAL:   return MIPS_JAL;
        case OPC_SLTI:  return MIPS_SLTI;
        case OPC_XORI:  return MIPS_XORI;
        case OPC_LB:    return MIPS_LB;
        default:
            if (n64_log_verbosity < LOG_VERBOSITY_DEBUG) {
                disassemble(pc, instr.raw, buf, 50);
            }
            logfatal("Failed to decode instruction 0x%08X opcode %d%d%d%d%d%d [%s]",
                     instr.raw, instr.op0, instr.op1, instr.op2, instr.op3, instr.op4, instr.op5, buf)
    }
}

void cp0_step(cp0_t* cp0) {
    cp0->count_stepper = !cp0->count_stepper;
    cp0->r[R4300I_CP0_REG_COUNT] += cp0->count_stepper;
    if (cp0->r[R4300I_CP0_REG_COUNT] == cp0->r[R4300I_CP0_REG_COMPARE]) {
        logwarn("TODO: Compare interrupt!")
    }
}

#define exec_instr(key, fn) case key: fn(cpu, instruction); break;

void r4300i_step(r4300i_t* cpu) {
    cp0_step(&cpu->cp0);
    dword pc = cpu->pc;

    mips_instruction_t instruction;
    instruction.raw = cpu->read_word(pc);
    cpu->pc += 4;

    switch (decode(cpu, pc, instruction)) {
        case MIPS_NOP: break;

        exec_instr(MIPS_LUI,   mips_lui)
        exec_instr(MIPS_LD,    mips_ld)
        exec_instr(MIPS_ADDI,  mips_addi)
        exec_instr(MIPS_ADDIU, mips_addiu)
        exec_instr(MIPS_DADDI, mips_daddi)
        exec_instr(MIPS_ANDI,  mips_andi)
        exec_instr(MIPS_LBU,   mips_lbu)
        exec_instr(MIPS_LW,    mips_lw)
        exec_instr(MIPS_BEQ,   mips_beq)
        exec_instr(MIPS_BLEZL, mips_blezl)
        exec_instr(MIPS_BNE,   mips_bne)
        exec_instr(MIPS_BNEL,  mips_bnel)
        exec_instr(MIPS_CACHE, mips_cache)
        exec_instr(MIPS_SB,    mips_sb)
        exec_instr(MIPS_SD,    mips_sd)
        exec_instr(MIPS_SW,    mips_sw)
        exec_instr(MIPS_ORI,   mips_ori)
        exec_instr(MIPS_J,     mips_j)
        exec_instr(MIPS_JAL,   mips_jal)
        exec_instr(MIPS_SLTI,  mips_slti)
        exec_instr(MIPS_BEQL,  mips_beql)
        exec_instr(MIPS_BGTZ,  mips_bgtz)
        exec_instr(MIPS_XORI,  mips_xori)
        exec_instr(MIPS_LB,    mips_lb)

        // Coprocessor
        exec_instr(MIPS_CP_MTC0, mips_mtc0)

        // Special
        exec_instr(MIPS_SPC_SLL,   mips_spc_sll)
        exec_instr(MIPS_SPC_SRL,   mips_spc_srl)
        exec_instr(MIPS_SPC_SLLV,  mips_spc_sllv)
        exec_instr(MIPS_SPC_SRLV,  mips_spc_srlv)
        exec_instr(MIPS_SPC_JR,    mips_spc_jr)
        exec_instr(MIPS_SPC_MFHI,  mips_spc_mfhi)
        exec_instr(MIPS_SPC_MFLO,  mips_spc_mflo)
        exec_instr(MIPS_SPC_MULTU, mips_spc_multu)
        exec_instr(MIPS_SPC_ADD,   mips_spc_add)
        exec_instr(MIPS_SPC_ADDU,  mips_spc_addu)
        exec_instr(MIPS_SPC_AND,   mips_spc_and)
        exec_instr(MIPS_SPC_SUBU,  mips_spc_subu)
        exec_instr(MIPS_SPC_OR,    mips_spc_or)
        exec_instr(MIPS_SPC_XOR,   mips_spc_xor)
        exec_instr(MIPS_SPC_SLT,   mips_spc_slt)
        exec_instr(MIPS_SPC_SLTU,  mips_spc_sltu)
        exec_instr(MIPS_SPC_DADD,  mips_spc_dadd)

        // REGIMM
        exec_instr(MIPS_RI_BGEZL,  mips_ri_bgezl)
        exec_instr(MIPS_RI_BGEZAL, mips_ri_bgezal)
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
