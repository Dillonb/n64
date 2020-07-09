#include "rsp_vector_instructions.h"
#include "../common/log.h"
#include "rsp.h"

RSP_VECTOR_INSTR(rsp_lwc2_lbv) {
    logfatal("Unimplemented: rsp_lwc2_lbv")
}

RSP_VECTOR_INSTR(rsp_lwc2_ldv) {
    sbyte offset = instruction.v.offset << 1;
    word address = get_rsp_register(rsp, instruction.v.base) + offset * 8;

    for (int i = 0; i < 8; i++) {
        int element = i + instruction.v.element;
        unimplemented(element > 15, "LDV overflowing vector register")
        rsp->vu_regs[instruction.v.vt].bytes[element] = rsp->read_byte(address + i);
    }
}

RSP_VECTOR_INSTR(rsp_lwc2_lfv) {
    logfatal("Unimplemented: rsp_lwc2_lfv")
}

RSP_VECTOR_INSTR(rsp_lwc2_lhv) {
    logfatal("Unimplemented: rsp_lwc2_lhv")
}

RSP_VECTOR_INSTR(rsp_lwc2_llv) {
    logfatal("Unimplemented: rsp_lwc2_llv")
}

RSP_VECTOR_INSTR(rsp_lwc2_lpv) {
    logfatal("Unimplemented: rsp_lwc2_lpv")
}

RSP_VECTOR_INSTR(rsp_lwc2_lqv) {
    unimplemented(instruction.v.element != 0, "LQV with element != 0!")

    sbyte offset     = instruction.v.offset << 1;
    word address     = get_rsp_register(rsp, instruction.v.base) + offset * 8;
    word end_address = ((address & ~15) + 15);

    for (int i = 0; address + i <= end_address; i++) {
        rsp->vu_regs[instruction.v.vt].bytes[i] = rsp->read_byte(address + i);
    }
}

RSP_VECTOR_INSTR(rsp_lwc2_lrv) {
    logfatal("Unimplemented: rsp_lwc2_lrv")
}

RSP_VECTOR_INSTR(rsp_lwc2_lsv) {
    logfatal("Unimplemented: rsp_lwc2_lsv")
}

RSP_VECTOR_INSTR(rsp_lwc2_ltv) {
    logfatal("Unimplemented: rsp_lwc2_ltv")
}

RSP_VECTOR_INSTR(rsp_lwc2_luv) {
    logfatal("Unimplemented: rsp_lwc2_luv")
}

RSP_VECTOR_INSTR(rsp_swc2_sbv) {
    logfatal("Unimplemented: rsp_swc2_sbv")
}

RSP_VECTOR_INSTR(rsp_swc2_sdv) {
    sbyte offset = instruction.v.offset << 1;
    word address = get_rsp_register(rsp, instruction.v.base) + offset * 8;

    for (int i = 0; i < 8; i++) {
        int element = i + instruction.v.element;
        unimplemented(element > 15, "SDV overflowing vector register")
        rsp->write_byte(address + i, rsp->vu_regs[instruction.v.vt].bytes[element]);
    }
}

RSP_VECTOR_INSTR(rsp_swc2_sfv) {
    logfatal("Unimplemented: rsp_swc2_sfv")
}

RSP_VECTOR_INSTR(rsp_swc2_shv) {
    logfatal("Unimplemented: rsp_swc2_shv")
}

RSP_VECTOR_INSTR(rsp_swc2_slv) {
    logfatal("Unimplemented: rsp_swc2_slv")
}

RSP_VECTOR_INSTR(rsp_swc2_spv) {
    logfatal("Unimplemented: rsp_swc2_spv")
}

RSP_VECTOR_INSTR(rsp_swc2_sqv) {
    logfatal("Unimplemented: rsp_swc2_sqv")
}

RSP_VECTOR_INSTR(rsp_swc2_srv) {
    logfatal("Unimplemented: rsp_swc2_srv")
}

RSP_VECTOR_INSTR(rsp_swc2_ssv) {
    logfatal("Unimplemented: rsp_swc2_ssv")
}

RSP_VECTOR_INSTR(rsp_swc2_stv) {
    logfatal("Unimplemented: rsp_swc2_stv")
}

RSP_VECTOR_INSTR(rsp_swc2_suv) {
    logfatal("Unimplemented: rsp_swc2_suv")
}
