#include "rsp.h"
#include "../common/log.h"
#include "../mem/addresses.h"
#include "../cpu/mips.h"
#include "disassemble.h"

#define ADDR_SP_MEM_ADDR_REG  0x04040000
#define ADDR_SP_DRAM_ADDR_REG 0x04040004
#define ADDR_SP_RD_LEN_REG    0x04040008
#define ADDR_SP_WR_LEN_REG    0x0404000C
#define ADDR_SP_STATUS_REG    0x04040010
#define ADDR_SP_DMA_FULL_REG  0x04040014
#define ADDR_SP_DMA_BUSY_REG  0x04040018
#define ADDR_SP_SEMAPHORE_REG 0x0404001C
#define ADDR_SP_PC_REG        0x04080000
#define ADDR_SP_IBIST_REG     0x04080004

#define exec_instr(key, fn) case key: fn(rsp, instruction); break;

typedef union sp_status_write {
    word raw;
    struct {
        bool clear_halt:1;
        bool set_halt:1;
        bool clear_broke:1;
        bool clear_intr:1;
        bool set_intr:1;
        bool clear_sstep:1;
        bool set_sstep:1;
        bool clear_intr_on_break:1;
        bool set_intr_on_break:1;
        bool clear_signal_0:1;
        bool set_signal_0:1;
        bool clear_signal_1:1;
        bool set_signal_1:1;
        bool clear_signal_2:1;
        bool set_signal_2:1;
        bool clear_signal_3:1;
        bool set_signal_3:1;
        bool clear_signal_4:1;
        bool set_signal_4:1;
        bool clear_signal_5:1;
        bool set_signal_5:1;
        bool clear_signal_6:1;
        bool set_signal_6:1;
        bool clear_signal_7:1;
        bool set_signal_7:1;
        unsigned:7;
    };
} sp_status_write_t;

#define CLEAR_SET(VAL, CLEAR, SET) if (CLEAR) {VAL = false; } if (SET) { VAL = true; }

INLINE void status_reg_write(n64_system_t* system, word value) {
    sp_status_write_t write;
    write.raw = value;

    CLEAR_SET(system->rsp_status.halt,          write.clear_halt,          write.set_halt)
    CLEAR_SET(system->rsp_status.broke,         write.clear_broke,         false)
    if (write.clear_intr) {
        logwarn("TODO: Clearing RSP intr?")
    }
    if (write.set_intr) {
        logwarn("TODO: Setting RSP intr?")
    }
    CLEAR_SET(system->rsp_status.single_step,   write.clear_sstep,         write.set_sstep)
    CLEAR_SET(system->rsp_status.intr_on_break, write.clear_intr_on_break, write.set_intr_on_break)
    CLEAR_SET(system->rsp_status.signal_0,      write.clear_signal_0,      write.set_signal_0)
    CLEAR_SET(system->rsp_status.signal_1,      write.clear_signal_1,      write.set_signal_1)
    CLEAR_SET(system->rsp_status.signal_2,      write.clear_signal_2,      write.set_signal_2)
    CLEAR_SET(system->rsp_status.signal_3,      write.clear_signal_3,      write.set_signal_3)
    CLEAR_SET(system->rsp_status.signal_4,      write.clear_signal_4,      write.set_signal_4)
    CLEAR_SET(system->rsp_status.signal_5,      write.clear_signal_5,      write.set_signal_5)
    CLEAR_SET(system->rsp_status.signal_6,      write.clear_signal_6,      write.set_signal_6)
    CLEAR_SET(system->rsp_status.signal_7,      write.clear_signal_7,      write.set_signal_7)
}

word read_word_spreg(n64_system_t* system, word address) {
    switch (address) {
        case ADDR_SP_PC_REG:
            return system->rsp.pc;
        case ADDR_SP_STATUS_REG:
            return system->rsp_status.raw;
        default:
            logfatal("Reading word from unknown/unsupported address 0x%08X in region: REGION_SP_REGS", address)
    }
}

void write_word_spreg(n64_system_t* system, word address, word value) {
    switch (address) {
        case ADDR_SP_MEM_ADDR_REG:
            system->sp.mem_addr.raw = value;
            printf("SP mem addr: 0x%08X\n", value);
            break;
        case ADDR_SP_DRAM_ADDR_REG:
            system->sp.dram_addr.raw = value;
            break;
        case ADDR_SP_RD_LEN_REG: {
            system->sp.dma_read.raw = value;
            word length = (system->sp.dma_read.length | 7) + 1;
            for (int i = 0; i < system->sp.dma_read.count + 1; i++) {
                word mem_addr = system->sp.mem_addr.address + (system->sp.mem_addr.imem ? SREGION_SP_IMEM : SREGION_SP_DMEM);
                for (int j = 0; j < length; j++) {
                    byte val = system->rsp.read_byte(system->sp.dram_addr.address + j);
                    logtrace("SP DMA: Copying 0x%02X from 0x%08X to 0x%08X", val, system->sp.dram_addr.address + j, mem_addr + j)
                    system->rsp.write_byte(mem_addr + j, val);
                }

                system->sp.dram_addr.address += length + system->sp.dma_read.skip;
                system->sp.mem_addr.address += length;
            }
            break;
        }
        case ADDR_SP_WR_LEN_REG:
            logfatal("Write to unsupported SP reg: ADDR_SP_WR_LEN_REG")
        case ADDR_SP_STATUS_REG:
            status_reg_write(system, value);
            break;
        case ADDR_SP_DMA_FULL_REG:
            logfatal("Write to unsupported SP reg: ADDR_SP_DMA_FULL_REG")
        case ADDR_SP_DMA_BUSY_REG:
            logfatal("Write to unsupported SP reg: ADDR_SP_DMA_BUSY_REG")
        case ADDR_SP_SEMAPHORE_REG:
            logfatal("Write to unsupported SP reg: ADDR_SP_SEMAPHORE_REG")
        case ADDR_SP_PC_REG:
            system->rsp.pc = value;
            break;
        case ADDR_SP_IBIST_REG:
            logfatal("Write to unsupported SP reg: ADDR_SP_IBIST_REG")
        default:
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_SP_REGS", value, address)
    }
}

mips_instruction_type_t rsp_cp0_decode(r4300i_t* rsp, word pc, mips_instruction_t instr) {
    if (instr.last11 == 0) {
        switch (instr.r.rs) {
            case COP_MT: return MIPS_CP_MTC0;
            case COP_MF: return MIPS_CP_MFC0;
            default: {
                char buf[50];
                disassemble(pc, instr.raw, buf, 50);
                logfatal("other/unknown MIPS RSP CP0 0x%08X with rs: %d%d%d%d%d [%s]", instr.raw,
                         instr.rs0, instr.rs1, instr.rs2, instr.rs3, instr.rs4, buf)
            }
        }
    } else {
        switch (instr.fr.funct) {
            default: {
                char buf[50];
                disassemble(pc, instr.raw, buf, 50);
                logfatal("other/unknown MIPS RSP CP0 0x%08X with FUNCT: %d%d%d%d%d%d [%s]", instr.raw,
                         instr.funct0, instr.funct1, instr.funct2, instr.funct3, instr.funct4, instr.funct5, buf)
            }
        }
    }
}

mips_instruction_type_t rsp_instruction_decode(r4300i_t* rsp, word pc, mips_instruction_t instr) {
        char buf[50];
        if (n64_log_verbosity >= LOG_VERBOSITY_DEBUG) {
            disassemble(pc, instr.raw, buf, 50);
            logdebug("RSP [0x%08X]=0x%08X %s", pc, instr.raw, buf)
        }
        if (instr.raw == 0) {
            return MIPS_NOP;
        }
        switch (instr.op) {
            case OPC_ORI:  return MIPS_ORI;
            case OPC_J:    return MIPS_J;
            case OPC_ADDI: return MIPS_ADDI;
            case OPC_LW:   return MIPS_LW;
            case OPC_CP0:  return rsp_cp0_decode(rsp, pc, instr);
            default:
                if (n64_log_verbosity < LOG_VERBOSITY_DEBUG) {
                    disassemble(pc, instr.raw, buf, 50);
                }
                logfatal("Failed to r4300i_instruction_decode instruction 0x%08X opcode %d%d%d%d%d%d [%s]",
                         instr.raw, instr.op0, instr.op1, instr.op2, instr.op3, instr.op4, instr.op5, buf)
        }
}

INLINE void rsp_cp0_step(cp0_t* cp0) {
    if (cp0->random <= cp0->wired) {
        cp0->random = 31;
    } else {
        cp0->random--;
    }
    cp0->count += 2;
}

void rsp_step(r4300i_t* rsp) {
    rsp_cp0_step(&rsp->cp0); // TODO: does the RSP have a CP0?
    dword pc = rsp->pc;
    mips_instruction_t instruction;
    instruction.raw = rsp->read_word(pc);

    rsp->pc += 4;

    switch (rsp_instruction_decode(rsp, pc, instruction)) {
        exec_instr(MIPS_ORI,  mips_ori)
        exec_instr(MIPS_J,    mips_j)
        exec_instr(MIPS_ADDI, mips_addi)
        exec_instr(MIPS_LW,   mips_lw)

        exec_instr(MIPS_CP_MTC0, mips_mtc0)
        exec_instr(MIPS_CP_MFC0, mips_mfc0)
        default:
            logfatal("Unknown instruction!")
    }
}
