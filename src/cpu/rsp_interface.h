#ifndef N64_RSP_INTERFACE_H
#define N64_RSP_INTERFACE_H
#include "../system/n64system.h"

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

word read_word_spreg(n64_system_t* system, word address);
void write_word_spreg(n64_system_t* system, word address, word value);
void rsp_status_reg_write(n64_system_t* system, word value);
#endif //N64_RSP_INTERFACE_H
