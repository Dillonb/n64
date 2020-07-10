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

RSP_VECTOR_INSTR(rsp_cfc2) {
    logfatal("Unimplemented: rsp_cfc2")
}

RSP_VECTOR_INSTR(rsp_ctc2) {
    logfatal("Unimplemented: rsp_ctc2")
}

RSP_VECTOR_INSTR(rsp_mfc2) {
    logfatal("Unimplemented: rsp_mfc2")
}

RSP_VECTOR_INSTR(rsp_mtc2) {
    half element = get_rsp_register(rsp, instruction.cp2_regmove.rt);
    rsp->vu_regs[instruction.cp2_regmove.rd].elements[instruction.cp2_regmove.e] = element;
}

RSP_VECTOR_INSTR(rsp_vec_vabs) {
    logfatal("Unimplemented: rsp_vec_vabs")
}

RSP_VECTOR_INSTR(rsp_vec_vadd) {
    logfatal("Unimplemented: rsp_vec_vadd")
}

RSP_VECTOR_INSTR(rsp_vec_vaddc) {
    logfatal("Unimplemented: rsp_vec_vaddc")
}

RSP_VECTOR_INSTR(rsp_vec_vand) {
    logfatal("Unimplemented: rsp_vec_vand")
}

RSP_VECTOR_INSTR(rsp_vec_vch) {
    logfatal("Unimplemented: rsp_vec_vch")
}

RSP_VECTOR_INSTR(rsp_vec_vcl) {
    logfatal("Unimplemented: rsp_vec_vcl")
}

RSP_VECTOR_INSTR(rsp_vec_vcr) {
    logfatal("Unimplemented: rsp_vec_vcr")
}

RSP_VECTOR_INSTR(rsp_vec_veq) {
    logfatal("Unimplemented: rsp_vec_veq")
}

RSP_VECTOR_INSTR(rsp_vec_vge) {
    logfatal("Unimplemented: rsp_vec_vge")
}

RSP_VECTOR_INSTR(rsp_vec_vlt) {
    logfatal("Unimplemented: rsp_vec_vlt")
}

RSP_VECTOR_INSTR(rsp_vec_vmacf) {
    logfatal("Unimplemented: rsp_vec_vmacf")
}

RSP_VECTOR_INSTR(rsp_vec_vmacq) {
    logfatal("Unimplemented: rsp_vec_vmacq")
}

RSP_VECTOR_INSTR(rsp_vec_vmacu) {
    logfatal("Unimplemented: rsp_vec_vmacu")
}

RSP_VECTOR_INSTR(rsp_vec_vmadh) {
    logfatal("Unimplemented: rsp_vec_vmadh")
}

RSP_VECTOR_INSTR(rsp_vec_vmadl) {
    logfatal("Unimplemented: rsp_vec_vmadl")
}

RSP_VECTOR_INSTR(rsp_vec_vmadm) {
    logfatal("Unimplemented: rsp_vec_vmadm")
}

RSP_VECTOR_INSTR(rsp_vec_vmadn) {
    logfatal("Unimplemented: rsp_vec_vmadn")
}

RSP_VECTOR_INSTR(rsp_vec_vmov) {
    logfatal("Unimplemented: rsp_vec_vmov")
}

RSP_VECTOR_INSTR(rsp_vec_vmrg) {
    logfatal("Unimplemented: rsp_vec_vmrg")
}

RSP_VECTOR_INSTR(rsp_vec_vmudh) {
    logfatal("Unimplemented: rsp_vec_vmudh")
}

RSP_VECTOR_INSTR(rsp_vec_vmudl) {
    logfatal("Unimplemented: rsp_vec_vmudl")
}

RSP_VECTOR_INSTR(rsp_vec_vmudm) {
    logfatal("Unimplemented: rsp_vec_vmudm")
}

RSP_VECTOR_INSTR(rsp_vec_vmudn) {
    logfatal("Unimplemented: rsp_vec_vmudn")
}

RSP_VECTOR_INSTR(rsp_vec_vmulf) {
    logfatal("Unimplemented: rsp_vec_vmulf")
}

RSP_VECTOR_INSTR(rsp_vec_vmulq) {
    logfatal("Unimplemented: rsp_vec_vmulq")
}

RSP_VECTOR_INSTR(rsp_vec_vmulu) {
    logfatal("Unimplemented: rsp_vec_vmulu")
}

RSP_VECTOR_INSTR(rsp_vec_vnand) {
    logfatal("Unimplemented: rsp_vec_vnand")
}

RSP_VECTOR_INSTR(rsp_vec_vne) {
    logfatal("Unimplemented: rsp_vec_vne")
}

RSP_VECTOR_INSTR(rsp_vec_vnop) {
    logfatal("Unimplemented: rsp_vec_vnop")
}

RSP_VECTOR_INSTR(rsp_vec_vnor) {
    logfatal("Unimplemented: rsp_vec_vnor")
}

RSP_VECTOR_INSTR(rsp_vec_vnxor) {
    logfatal("Unimplemented: rsp_vec_vnxor")
}

RSP_VECTOR_INSTR(rsp_vec_vor) {
    logfatal("Unimplemented: rsp_vec_vor")
}

RSP_VECTOR_INSTR(rsp_vec_vrcp) {
    logfatal("Unimplemented: rsp_vec_vrcp")
}

RSP_VECTOR_INSTR(rsp_vec_vrcph) {
    logfatal("Unimplemented: rsp_vec_vrcph")
}

RSP_VECTOR_INSTR(rsp_vec_vrcpl) {
    logfatal("Unimplemented: rsp_vec_vrcpl")
}

RSP_VECTOR_INSTR(rsp_vec_vrndn) {
    logfatal("Unimplemented: rsp_vec_vrndn")
}

RSP_VECTOR_INSTR(rsp_vec_vrndp) {
    logfatal("Unimplemented: rsp_vec_vrndp")
}

RSP_VECTOR_INSTR(rsp_vec_vrsq) {
    logfatal("Unimplemented: rsp_vec_vrsq")
}

RSP_VECTOR_INSTR(rsp_vec_vrsqh) {
    logfatal("Unimplemented: rsp_vec_vrsqh")
}

RSP_VECTOR_INSTR(rsp_vec_vrsql) {
    logfatal("Unimplemented: rsp_vec_vrsql")
}

RSP_VECTOR_INSTR(rsp_vec_vsar) {
    logfatal("Unimplemented: rsp_vec_vsar")
}

RSP_VECTOR_INSTR(rsp_vec_vsub) {
    logfatal("Unimplemented: rsp_vec_vsub")
}

RSP_VECTOR_INSTR(rsp_vec_vsubc) {
    logfatal("Unimplemented: rsp_vec_vsubc")
}

RSP_VECTOR_INSTR(rsp_vec_vxor) {
    logfatal("Unimplemented: rsp_vec_vxor")
}
