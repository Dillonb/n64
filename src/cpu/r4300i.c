#include "r4300i.h"
#include "../common/log.h"
#include "disassemble.h"

void r4300i_step(r4300i_t* cpu, word instruction) {
    char buf[50];
    disassemble32(cpu->pc, instruction, buf, 50);
    logfatal("Failed to decode instruction 0x%08X [%s]", instruction, buf)
}
