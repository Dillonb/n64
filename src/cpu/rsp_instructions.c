#include "rsp_instructions.h"
#include "../common/log.h"

void rsp_branch_abs(rsp_t* rsp, word address) {
    rsp->branch_pc = address;

    // Execute one instruction before taking the branch_offset
    rsp->branch = true;
    rsp->branch_delay = 1;

    logtrace("Setting up a branch_offset (delayed by 1 instruction) to 0x%08X", rsp->branch_pc)
}


RSP_INSTR(rsp_ori) {
    set_rsp_register(rsp, instruction.i.rt, get_rsp_register(rsp, instruction.i.rs) | instruction.i.immediate);
}

RSP_INSTR(rsp_addi) {
    sword reg_addend = get_rsp_register(rsp, instruction.i.rs);
    shalf imm_addend = instruction.i.immediate;
    sword result = imm_addend + reg_addend;
    set_rsp_register(rsp, instruction.i.rt, result);
}

RSP_INSTR(rsp_j) {
    word target = instruction.j.target;
    target <<= 2;
    target |= ((rsp->pc - 4) & 0xF0000000); // PC is 4 ahead

    rsp_branch_abs(rsp, target);
}

RSP_INSTR(rsp_mfc0) {
    logfatal("RSP MFC0")
}
RSP_INSTR(rsp_mtc0) {
    logfatal("RSP MTC0")
}
