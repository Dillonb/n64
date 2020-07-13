#include "rsp_vector_instructions.h"
#include "../common/log.h"
#include "rsp.h"

#define clamp_signed(x) ((x) < -32768 ? -32768 : ((x) > 32768 ? 32767 : x))
#define clamp_unsigned(x) ((x) < 0 ? 0 : ((x) > 32767 ? 65535 : x))

RSP_VECTOR_INSTR(rsp_lwc2_lbv) {
    logfatal("Unimplemented: rsp_lwc2_lbv")
}

RSP_VECTOR_INSTR(rsp_lwc2_ldv) {
    sbyte offset = instruction.v.offset << 1;
    word address = get_rsp_register(rsp, instruction.v.base) + offset * 8;

    for (int i = 0; i < 8; i++) {
        int element = i + instruction.v.element;
        unimplemented(element > 15, "LDV overflowing vector register")
        rsp->vu_regs[instruction.v.vt].bytes[15 - element] = rsp->read_byte(address + i);
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
    int e = instruction.v.element;
    sbyte offset     = instruction.v.offset << 1;
    word address     = get_rsp_register(rsp, instruction.v.base) + offset * 8;
    word end_address = ((address & ~15) + 15);

    for (int i = 0; address + i <= end_address && i + e < 16; i++) {
        rsp->vu_regs[instruction.v.vt].bytes[15 - (i + e)] = rsp->read_byte(address + i);
    }
}

RSP_VECTOR_INSTR(rsp_lwc2_lrv) {
    logfatal("Unimplemented: rsp_lwc2_lrv")
}

RSP_VECTOR_INSTR(rsp_lwc2_lsv) {
    int e = instruction.v.element;
    sbyte offset     = instruction.v.offset << 1;
    word address     = get_rsp_register(rsp, instruction.v.base) + offset * 8;
    unimplemented(e % 2 == 1, "LSV with uneven element!")
    rsp->vu_regs[instruction.v.vt].elements[7 - (e / 2)] = rsp->read_half(address);
}

RSP_VECTOR_INSTR(rsp_lwc2_ltv) {
    logfatal("Unimplemented: rsp_lwc2_ltv")
}

RSP_VECTOR_INSTR(rsp_lwc2_luv) {
    logfatal("Unimplemented: rsp_lwc2_luv")
}

RSP_VECTOR_INSTR(rsp_swc2_sbv) {
    sbyte offset = instruction.v.offset << 1;
    word address = get_rsp_register(rsp, instruction.v.base) + offset * 8;

    int element = instruction.v.element;
    rsp->write_byte(address, rsp->vu_regs[instruction.v.vt].bytes[7 - element]);
}

RSP_VECTOR_INSTR(rsp_swc2_sdv) {
    sbyte offset = instruction.v.offset << 1;
    word address = get_rsp_register(rsp, instruction.v.base) + offset * 8;

    for (int i = 0; i < 8; i++) {
        int element = i + instruction.v.element;
        unimplemented(element > 15, "SDV overflowing vector register")
        rsp->write_byte(address + i, rsp->vu_regs[instruction.v.vt].bytes[15 - element]);
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
    int e = instruction.v.element;
    sbyte offset     = instruction.v.offset << 1;
    word address     = get_rsp_register(rsp, instruction.v.base) + offset * 8;
    word end_address = ((address & ~15) + 15);

    for (int i = 0; address + i <= end_address; i++) {
        rsp->write_byte(address + i, rsp->vu_regs[instruction.v.vt].bytes[15 - ((i + e) & 15)]);
    }
}

RSP_VECTOR_INSTR(rsp_swc2_srv) {
    logfatal("Unimplemented: rsp_swc2_srv")
}

RSP_VECTOR_INSTR(rsp_swc2_ssv) {
    sbyte offset = instruction.v.offset << 1;
    word address = get_rsp_register(rsp, instruction.v.base) + offset * 8;

    int element = instruction.v.element;
    unimplemented(element % 2 != 0, "SSV: element is not even") // TODO: If discovered it's allowed to be uneven, at least make sure it's not 15.
    rsp->write_half(address, rsp->vu_regs[instruction.v.vt].elements[7 - (element / 2)]);
}

RSP_VECTOR_INSTR(rsp_swc2_stv) {
    logfatal("Unimplemented: rsp_swc2_stv")
}

RSP_VECTOR_INSTR(rsp_swc2_suv) {
    logfatal("Unimplemented: rsp_swc2_suv")
}

RSP_VECTOR_INSTR(rsp_cfc2) {
    word value;
    switch (instruction.r.rd) {
        case 0: // VCO
            value = rsp->vco.raw;
            break;
        case 1: // VCC
            value = rsp->vcc.raw;
            break;
        case 2: // VCE
            value = rsp->vce.raw;
            break;
        default: logfatal("CFC2 from unknown VU control register: %d", instruction.r.rd)
    }

    set_rsp_register(rsp, instruction.r.rt, value);
}

RSP_VECTOR_INSTR(rsp_ctc2) {
    logfatal("Unimplemented: rsp_ctc2")
}

RSP_VECTOR_INSTR(rsp_mfc2) {
    logfatal("Unimplemented: rsp_mfc2")
}

RSP_VECTOR_INSTR(rsp_mtc2) {
    half element = get_rsp_register(rsp, instruction.cp2_regmove.rt);
    rsp->vu_regs[instruction.cp2_regmove.rd].elements[7 - instruction.cp2_regmove.e] = element;
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
    for (int i = 0; i < 8; i++) {
        half result = rsp->vu_regs[instruction.cp2_vec.vt].elements[i] & rsp->vu_regs[instruction.cp2_vec.vs].elements[i];
        rsp->vu_regs[instruction.cp2_vec.vd].elements[i] = result;
    }
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
    for (int e = 0; e < 8; e++) {
        shalf multiplicand1 = rsp->vu_regs[instruction.cp2_vec.vt].elements[e];
        shalf multiplicand2 = rsp->vu_regs[instruction.cp2_vec.vs].elements[e];
        sword prod = multiplicand1 * multiplicand2;

        sdword acc_delta = prod * 2;
        sdword acc = rsp->accumulator[e].raw + acc_delta;

        shalf result = clamp_signed(acc >> 16);

        rsp->accumulator[e].raw = acc;
        rsp->vu_regs[instruction.cp2_vec.vd].elements[e] = result;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vmacq) {
    logfatal("Unimplemented: rsp_vec_vmacq")
}

RSP_VECTOR_INSTR(rsp_vec_vmacu) {
    for (int e = 0; e < 8; e++) {
        shalf multiplicand1 = rsp->vu_regs[instruction.cp2_vec.vt].elements[e];
        shalf multiplicand2 = rsp->vu_regs[instruction.cp2_vec.vs].elements[e];
        sword prod = multiplicand1 * multiplicand2;

        sdword acc_delta = prod * 2;
        sdword acc = rsp->accumulator[e].raw + acc_delta;

        half result = clamp_unsigned(acc >> 16);

        rsp->accumulator[e].raw = acc;
        rsp->vu_regs[instruction.cp2_vec.vd].elements[e] = result;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vmadh) {
    for (int e = 0; e < 8; e++) {
        shalf multiplicand1 = rsp->vu_regs[instruction.cp2_vec.vt].elements[e];
        shalf multiplicand2 = rsp->vu_regs[instruction.cp2_vec.vs].elements[e];
        sword prod = multiplicand1 * multiplicand2;
        word uprod = prod;

        dword acc_delta = (dword)uprod << 16;
        sdword acc = rsp->accumulator[e].raw + acc_delta;

        shalf result = clamp_signed((sword)(acc >> 16));

        rsp->accumulator[e].raw = acc;
        rsp->vu_regs[instruction.cp2_vec.vd].elements[e] = result;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vmadl) {
    for (int e = 0; e < 8; e++) {
        half multiplicand1 = rsp->vu_regs[instruction.cp2_vec.vt].elements[e];
        half multiplicand2 = rsp->vu_regs[instruction.cp2_vec.vs].elements[e];
        word prod = multiplicand1 * multiplicand2;

        dword acc_delta = prod >> 16;
        dword acc = rsp->accumulator[e].raw + acc_delta;

        rsp->accumulator[e].raw = acc;
        half result = rsp->accumulator[e].low; // TODO this isn't 100% correct, I think.
        rsp->vu_regs[instruction.cp2_vec.vd].elements[e] = result;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vmadm) {
    for (int e = 0; e < 8; e++) {
        half multiplicand1 = rsp->vu_regs[instruction.cp2_vec.vt].elements[e];
        shalf multiplicand2 = rsp->vu_regs[instruction.cp2_vec.vs].elements[e];
        sword prod = multiplicand1 * multiplicand2;

        sdword acc_delta = prod;
        sdword acc = rsp->accumulator[e].raw + acc_delta;

        shalf result = clamp_signed(acc >> 16);

        rsp->accumulator[e].raw = acc;
        rsp->vu_regs[instruction.cp2_vec.vd].elements[e] = result;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vmadn) {
    for (int e = 0; e < 8; e++) {
        shalf multiplicand1 = rsp->vu_regs[instruction.cp2_vec.vt].elements[e];
        half multiplicand2 = rsp->vu_regs[instruction.cp2_vec.vs].elements[e];
        sword prod = multiplicand1 * multiplicand2;

        sdword acc_delta = prod;
        sdword acc = rsp->accumulator[e].raw + acc_delta;

        rsp->accumulator[e].raw = acc;
        half result = rsp->accumulator[e].low; // TODO this isn't 100% correct, I think.
        rsp->vu_regs[instruction.cp2_vec.vd].elements[e] = result;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vmov) {
    logfatal("Unimplemented: rsp_vec_vmov")
}

RSP_VECTOR_INSTR(rsp_vec_vmrg) {
    logfatal("Unimplemented: rsp_vec_vmrg")
}

RSP_VECTOR_INSTR(rsp_vec_vmudh) {
    for (int e = 0; e < 8; e++) {
        shalf multiplicand1 = rsp->vu_regs[instruction.cp2_vec.vt].elements[e];
        shalf multiplicand2 = rsp->vu_regs[instruction.cp2_vec.vs].elements[e];
        sword prod = multiplicand1 * multiplicand2;

        sdword acc = prod << 16;

        half result = clamp_signed(acc >> 16);

        rsp->accumulator[e].raw = acc;
        rsp->vu_regs[instruction.cp2_vec.vd].elements[e] = result;
    }
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
    for (int e = 0; e < 8; e++) {
        shalf multiplicand1 = rsp->vu_regs[instruction.cp2_vec.vt].elements[e];
        shalf multiplicand2 = rsp->vu_regs[instruction.cp2_vec.vs].elements[e];
        sword prod = multiplicand1 * multiplicand2;

        sdword acc = (prod * 2) + 0x8000;

        shalf result = clamp_signed(acc >> 16);

        rsp->accumulator[e].raw = acc;
        rsp->vu_regs[instruction.cp2_vec.vd].elements[e] = result;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vmulq) {
    logfatal("Unimplemented: rsp_vec_vmulq")
}

RSP_VECTOR_INSTR(rsp_vec_vmulu) {
    for (int e = 0; e < 8; e++) {
        shalf multiplicand1 = rsp->vu_regs[instruction.cp2_vec.vt].elements[e];
        shalf multiplicand2 = rsp->vu_regs[instruction.cp2_vec.vs].elements[e];
        sword prod = multiplicand1 * multiplicand2;

        sdword acc = (prod * 2) + 0x8000;

        half result = clamp_unsigned(acc >> 16);

        rsp->accumulator[e].raw = acc;
        rsp->vu_regs[instruction.cp2_vec.vd].elements[e] = result;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vnand) {
    for (int i = 0; i < 8; i++) {
        half result = ~(rsp->vu_regs[instruction.cp2_vec.vt].elements[i] & rsp->vu_regs[instruction.cp2_vec.vs].elements[i]);
        rsp->vu_regs[instruction.cp2_vec.vd].elements[i] = result;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vne) {
    logfatal("Unimplemented: rsp_vec_vne")
}

RSP_VECTOR_INSTR(rsp_vec_vnop) {
    logfatal("Unimplemented: rsp_vec_vnop")
}

RSP_VECTOR_INSTR(rsp_vec_vnor) {
    for (int i = 0; i < 8; i++) {
        half result = ~(rsp->vu_regs[instruction.cp2_vec.vt].elements[i] | rsp->vu_regs[instruction.cp2_vec.vs].elements[i]);
        rsp->vu_regs[instruction.cp2_vec.vd].elements[i] = result;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vnxor) {
    for (int i = 0; i < 8; i++) {
        half result = ~(rsp->vu_regs[instruction.cp2_vec.vt].elements[i] ^ rsp->vu_regs[instruction.cp2_vec.vs].elements[i]);
        rsp->vu_regs[instruction.cp2_vec.vd].elements[i] = result;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vor) {
    for (int i = 0; i < 8; i++) {
        half result = rsp->vu_regs[instruction.cp2_vec.vt].elements[i] | rsp->vu_regs[instruction.cp2_vec.vs].elements[i];
        rsp->vu_regs[instruction.cp2_vec.vd].elements[i] = result;
    }
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
    switch (instruction.cp2_vec.e) {
        case 0x8:
            for (int i = 0; i < 8; i++) {
                rsp->vu_regs[instruction.cp2_vec.vd].elements[i] = rsp->accumulator[i].high;
            }
            break;
        case 0x9:
            for (int i = 0; i < 8; i++) {
                rsp->vu_regs[instruction.cp2_vec.vd].elements[i] = rsp->accumulator[i].middle;
            }
            break;
        case 0xA:
            for (int i = 0; i < 8; i++) {
                rsp->vu_regs[instruction.cp2_vec.vd].elements[i] = rsp->accumulator[i].low;
            }
            break;
        default: // Not actually sure what the default behavior is here
            for (int i = 0; i < 8; i++) {
                rsp->vu_regs[instruction.cp2_vec.vd].elements[i] = 0x0000;
            }
            break;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vsub) {
    logfatal("Unimplemented: rsp_vec_vsub")
}

RSP_VECTOR_INSTR(rsp_vec_vsubc) {
    logfatal("Unimplemented: rsp_vec_vsubc")
}

RSP_VECTOR_INSTR(rsp_vec_vxor) {
    for (int i = 0; i < 8; i++) {
        half result = rsp->vu_regs[instruction.cp2_vec.vt].elements[i] ^ rsp->vu_regs[instruction.cp2_vec.vs].elements[i];
        rsp->vu_regs[instruction.cp2_vec.vd].elements[i] = result;
    }
}
