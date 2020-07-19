#include "rsp_vector_instructions.h"

#include <emmintrin.h>

#include "../common/log.h"
#include "rsp.h"
#include "rsp_rom.h"

#define vsvtvd vu_reg_t* vs = &rsp->vu_regs[instruction.cp2_vec.vs]; vu_reg_t* vt = &rsp->vu_regs[instruction.cp2_vec.vt]; vu_reg_t* vd = &rsp->vu_regs[instruction.cp2_vec.vd]

INLINE shalf clamp_signed(sdword value) {
    if (value < -32768) return -32768;
    if (value > 32767) return 32767;
    return value;
}

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
    int e = instruction.v.element;
    sbyte offset     = instruction.v.offset << 1;
    word address     = get_rsp_register(rsp, instruction.v.base) + offset * 8;
    unimplemented(e % 4 != 0, "LLV to unaligned element")

    for (int i = 0; i < 4; i++) {
        int element = i + e;
        unimplemented(element > 15, "LLV overflowing vector register")
        rsp->vu_regs[instruction.v.vt].bytes[15 - element] = rsp->read_byte(address + i);
    }
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
    sbyte offset = instruction.v.offset << 1;
    word address = get_rsp_register(rsp, instruction.v.base) + offset * 8;
    int e = instruction.v.element;

    for (int i = 0; i < 4; i++) {
        int element = i + e;
        unimplemented(element > 15, "SLV overflowing vector register")
        rsp->write_byte(address + i, rsp->vu_regs[instruction.v.vt].bytes[15 - element]);
    }
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
    word value = 0;
    switch (instruction.r.rd) {
        case 0: { // VCO
            for (int i = 0; i < 8; i++) {
                bool h = rsp->vco.h.elements[7 - i] != 0;
                bool l = rsp->vco.l.elements[7 - i] != 0;
                word mask = (l << i) | (h << (i + 8));
                value |= mask;
            }
            break;
        }
        case 1: { // VCC
            for (int i = 0; i < 8; i++) {
                bool h = rsp->vcc.h.elements[7 - i] != 0;
                bool l = rsp->vcc.l.elements[7 - i] != 0;
                word mask = (l << i) | (h << (i + 8));
                value |= mask;
            }
            break;
        }
        case 2: { // VCE
            for (int i = 0; i < 8; i++) {
                bool l = rsp->vce.elements[7 - i] != 0;
                value |= (l << i);
            }
            break;
            default:
                logfatal("CFC2 from unknown VU control register: %d", instruction.r.rd)
        }
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
    vsvtvd;

    // for i in 0..7
    for (int i = 0; i < 8; i++) {
        shalf vse = vs->signed_elements[i];
        shalf vte = vt->signed_elements[i];
        // result(16..0) = VS<i>(15..0) + VT<i>(15..0) + VCO(i)
        sword result = vse + vte + (rsp->vco.l.elements[i] != 0);
        // ACC<i>(15..0) = result(15..0)
        rsp->acc.l.elements[i] = result;
        // VD<i>(15..0) = clamp_signed(result(16..0))
        vd->elements[i] = clamp_signed(result);
        // VCO(i) = 0
        rsp->vco.l.elements[i] = 0;
        // VCO(i + 8) = 0
        rsp->vco.h.elements[i] = 0;
        // endfor
    }
}

RSP_VECTOR_INSTR(rsp_vec_vaddc) {
    logfatal("Unimplemented: rsp_vec_vaddc")
}

RSP_VECTOR_INSTR(rsp_vec_vand) {
    unimplemented(instruction.cp2_vec.e != 0, "Element != 0")
    for (int i = 0; i < 8; i++) {
        half result = rsp->vu_regs[instruction.cp2_vec.vt].elements[i] & rsp->vu_regs[instruction.cp2_vec.vs].elements[i];
        rsp->vu_regs[instruction.cp2_vec.vd].elements[i] = result;
        rsp->acc.l.elements[i] = result;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vch) {
    vsvtvd;

    for (int i = 0; i < 8; i++) {
        shalf vse = vs->signed_elements[i];
        shalf vte = vt->signed_elements[i];
        rsp->vco.l.elements[i] = (vs->elements[i] >> 15) != (vt->elements[i] >> 15);
        half vt_abs = rsp->vco.l.elements[i] != 0 ? -vte : vte;
        rsp->vce.elements[i] = rsp->vco.l.elements[i] != 0 && (vse == -vte - 1);
        rsp->vco.h.elements[i] = rsp->vce.elements[i] == 0 && (vse != (shalf)vt_abs);
        rsp->vcc.l.elements[i] = vse <= -vte;
        rsp->vcc.h.elements[i] = vse >= vte;
        bool clip = rsp->vco.l.elements[i] != 0 ? rsp->vcc.l.elements[i] != 0 : rsp->vcc.h.elements[i] != 0;
        rsp->acc.l.elements[i] = clip ? vt_abs : vs->elements[i];
        vd->elements[i] = rsp->acc.l.elements[i];
    }
}

RSP_VECTOR_INSTR(rsp_vec_vcl) {
    vsvtvd;

    for (int i = 0; i < 8; i++) {
        half vse = vs->elements[i];
        half vte = vt->elements[i];
        if (rsp->vco.l.elements[i] == 0 && rsp->vco.h.elements[i] == 0) {
            rsp->vcc.h.elements[i] = (sword)vse - (sword)vte >= 0;
        }
        if (rsp->vco.l.elements[i] != 0 && rsp->vco.h.elements[i] == 0) {
            bool lte = vse <= -vte;
            bool eql = vse == -vte;
            rsp->vcc.l.elements[i] = rsp->vce.elements[i] != 0 ? lte : eql;
        }
        bool clip = rsp->vco.l.elements[i] != 0 ? rsp->vcc.l.elements[i] != 0 : rsp->vcc.h.elements[i] != 0;
        half vt_abs = rsp->vco.l.elements[i] != 0 ? -vte : vte;
        if (clip) {
            rsp->acc.l.elements[i] = vt_abs;
        } else {
            rsp->acc.l.elements[i] = vse;
        }
        vd->elements[i] = rsp->acc.l.elements[i];
    }
}

RSP_VECTOR_INSTR(rsp_vec_vcr) {
    logfatal("Unimplemented: rsp_vec_vcr")
}

RSP_VECTOR_INSTR(rsp_vec_veq) {
    logfatal("Unimplemented: rsp_vec_veq")
}

RSP_VECTOR_INSTR(rsp_vec_vge) {
    vsvtvd;

    for (int i = 0; i < 8; i++) {
        bool eql = vs->signed_elements[i] == vt->signed_elements[i];
        bool neg = !(rsp->vco.l.elements[i] != 0 && rsp->vco.h.elements[i] != 0) && eql;

        rsp->vcc.l.elements[i] = neg || (vs->signed_elements[i] > vt->signed_elements[i]);
        rsp->acc.l.elements[i] = rsp->vcc.l.elements[i] != 0 ? vs->elements[i] : vt->elements[i];
        vd->elements[i] = rsp->acc.l.elements[i];
        rsp->vcc.h.elements[i] = 0;
        rsp->vco.h.elements[i] = 0;

    }
}

RSP_VECTOR_INSTR(rsp_vec_vlt) {
    vsvtvd;
    for (int i = 0; i < 8; i++) {
        bool eql = vs->elements[i] == vt->elements[i];
        bool neg = rsp->vco.h.elements[i] != 0 && rsp->vco.l.elements[i] != 0 && eql;
        rsp->vcc.l.elements[i] = neg || (vs->signed_elements[i] < vt->signed_elements[i]);
        rsp->acc.l.elements[i] = rsp->vcc.l.elements[i] != 0 ? vs->elements[i] : vt->elements[i];
        vd->elements[i] = rsp->acc.l.elements[i];
        rsp->vcc.h.elements[i] = 0;
        rsp->vco.h.elements[i] = 0;
        rsp->vco.l.elements[i] = 0;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vmacf) {
    for (int e = 0; e < 8; e++) {
        shalf multiplicand1 = rsp->vu_regs[instruction.cp2_vec.vt].elements[e];
        shalf multiplicand2 = rsp->vu_regs[instruction.cp2_vec.vs].elements[e];
        sword prod = multiplicand1 * multiplicand2;

        sdword acc_delta = prod * 2;
        sdword acc = get_rsp_accumulator(rsp, e) + acc_delta;

        shalf result = clamp_signed(acc >> 16);

        set_rsp_accumulator(rsp, e, acc);
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
        sdword acc = get_rsp_accumulator(rsp, e) + acc_delta;

        half result = clamp_unsigned(acc >> 16);

        set_rsp_accumulator(rsp, e, acc);
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
        sdword acc = get_rsp_accumulator(rsp, e) + acc_delta;

        shalf result = clamp_signed((sword)(acc >> 16));

        set_rsp_accumulator(rsp, e, acc);
        rsp->vu_regs[instruction.cp2_vec.vd].elements[e] = result;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vmadl) {
    for (int e = 0; e < 8; e++) {
        half multiplicand1 = rsp->vu_regs[instruction.cp2_vec.vt].elements[e];
        half multiplicand2 = rsp->vu_regs[instruction.cp2_vec.vs].elements[e];
        word prod = multiplicand1 * multiplicand2;

        dword acc_delta = prod >> 16;
        dword acc = get_rsp_accumulator(rsp, e) + acc_delta;

        set_rsp_accumulator(rsp, e, acc);
        half result = get_rsp_accumulator(rsp, e) & 0xFFFF; // TODO this isn't 100% correct, I think.
        rsp->vu_regs[instruction.cp2_vec.vd].elements[e] = result;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vmadm) {
    for (int e = 0; e < 8; e++) {
        half multiplicand1 = rsp->vu_regs[instruction.cp2_vec.vt].elements[e];
        shalf multiplicand2 = rsp->vu_regs[instruction.cp2_vec.vs].elements[e];
        sword prod = multiplicand1 * multiplicand2;

        sdword acc_delta = prod;
        sdword acc = get_rsp_accumulator(rsp, e);
        acc += acc_delta;

        shalf result = clamp_signed(acc >> 16);

        set_rsp_accumulator(rsp, e, acc);
        rsp->vu_regs[instruction.cp2_vec.vd].elements[e] = result;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vmadn) {
    for (int e = 0; e < 8; e++) {
        shalf multiplicand1 = rsp->vu_regs[instruction.cp2_vec.vt].elements[e];
        half multiplicand2 = rsp->vu_regs[instruction.cp2_vec.vs].elements[e];
        sword prod = multiplicand1 * multiplicand2;

        sdword acc_delta = prod;
        sdword acc = get_rsp_accumulator(rsp, e) + acc_delta;

        set_rsp_accumulator(rsp, e, acc);
        half result = get_rsp_accumulator(rsp, e) & 0xFFFF; // TODO this isn't 100% correct, I think.
        rsp->vu_regs[instruction.cp2_vec.vd].elements[e] = result;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vmov) {
    logfatal("Unimplemented: rsp_vec_vmov")
}

RSP_VECTOR_INSTR(rsp_vec_vmrg) {
    vsvtvd;

    for (int i = 0; i < 8; i++) {
        rsp->acc.l.elements[i] = rsp->vcc.l.elements[i] != 0 ? vs->elements[i] : vt->elements[i];
        vd->elements[i] = rsp->acc.l.elements[i];
    }
}

RSP_VECTOR_INSTR(rsp_vec_vmudh) {
    for (int e = 0; e < 8; e++) {
        sword multiplicand1 = (shalf)rsp->vu_regs[instruction.cp2_vec.vt].elements[e];
        sword multiplicand2 = (shalf)rsp->vu_regs[instruction.cp2_vec.vs].elements[e];
        sword prod = multiplicand1 * multiplicand2;

        //sdword acc = rsp->accumulator[e] >> 16;
        sdword acc = 0;
        acc += prod;

        shalf result = clamp_signed(acc);

        acc <<= 16;

        rsp->acc.l.elements[e] = acc;

        rsp->vu_regs[instruction.cp2_vec.vd].elements[e] = result;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vmudl) {
    for (int e = 0; e < 8; e++) {
        half multiplicand1 = rsp->vu_regs[instruction.cp2_vec.vt].elements[e];
        half multiplicand2 = rsp->vu_regs[instruction.cp2_vec.vs].elements[e];
        word prod = multiplicand1 * multiplicand2;

        dword acc = prod >> 16;

        set_rsp_accumulator(rsp, e, acc);
        half result = rsp->acc.l.elements[e]; // TODO this isn't 100% correct, I think.
        rsp->vu_regs[instruction.cp2_vec.vd].elements[e] = result;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vmudm) {
    for (int e = 0; e < 8; e++) {
        half multiplicand1 = rsp->vu_regs[instruction.cp2_vec.vt].elements[e];
        shalf multiplicand2 = rsp->vu_regs[instruction.cp2_vec.vs].elements[e];
        sword prod = multiplicand1 * multiplicand2;

        sdword acc = prod;

        shalf result = clamp_signed(acc >> 16);

        set_rsp_accumulator(rsp, e, acc);
        rsp->vu_regs[instruction.cp2_vec.vd].elements[e] = result;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vmudn) {
    for (int e = 0; e < 8; e++) {
        shalf multiplicand1 = rsp->vu_regs[instruction.cp2_vec.vt].elements[e];
        half multiplicand2 = rsp->vu_regs[instruction.cp2_vec.vs].elements[e];
        sword prod = multiplicand1 * multiplicand2;

        sdword acc = prod;

        set_rsp_accumulator(rsp, e, acc);
        half result = get_rsp_accumulator(rsp, e) & 0xFFFF; // TODO this isn't 100% correct, I think.
        rsp->vu_regs[instruction.cp2_vec.vd].elements[e] = result;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vmulf) {
    for (int e = 0; e < 8; e++) {
        shalf multiplicand1 = rsp->vu_regs[instruction.cp2_vec.vt].elements[e];
        shalf multiplicand2 = rsp->vu_regs[instruction.cp2_vec.vs].elements[e];
        sword prod = multiplicand1 * multiplicand2;

        sdword acc = (prod * 2) + 0x8000;

        shalf result = clamp_signed(acc >> 16);

        set_rsp_accumulator(rsp, e, acc);
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

        set_rsp_accumulator(rsp, e, acc);
        rsp->vu_regs[instruction.cp2_vec.vd].elements[e] = result;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vnand) {
    unimplemented(instruction.cp2_vec.e != 0, "Element != 0")
    for (int i = 0; i < 8; i++) {
        half result = ~(rsp->vu_regs[instruction.cp2_vec.vt].elements[i] & rsp->vu_regs[instruction.cp2_vec.vs].elements[i]);
        rsp->vu_regs[instruction.cp2_vec.vd].elements[i] = result;
        rsp->acc.l.elements[i] = result;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vne) {
    logfatal("Unimplemented: rsp_vec_vne")
}

RSP_VECTOR_INSTR(rsp_vec_vnop) {
    logfatal("Unimplemented: rsp_vec_vnop")
}

RSP_VECTOR_INSTR(rsp_vec_vnor) {
    unimplemented(instruction.cp2_vec.e != 0, "Element != 0")
    for (int i = 0; i < 8; i++) {
        half result = ~(rsp->vu_regs[instruction.cp2_vec.vt].elements[i] | rsp->vu_regs[instruction.cp2_vec.vs].elements[i]);
        rsp->vu_regs[instruction.cp2_vec.vd].elements[i] = result;
        rsp->acc.l.elements[i] = result;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vnxor) {
    unimplemented(instruction.cp2_vec.e != 0, "Element != 0")
    for (int i = 0; i < 8; i++) {
        half result = ~(rsp->vu_regs[instruction.cp2_vec.vt].elements[i] ^ rsp->vu_regs[instruction.cp2_vec.vs].elements[i]);
        rsp->vu_regs[instruction.cp2_vec.vd].elements[i] = result;
        rsp->acc.l.elements[i] = result;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vor) {
    unimplemented(instruction.cp2_vec.e != 0, "Element != 0")
    for (int i = 0; i < 8; i++) {
        half result = rsp->vu_regs[instruction.cp2_vec.vt].elements[i] | rsp->vu_regs[instruction.cp2_vec.vs].elements[i];
        rsp->vu_regs[instruction.cp2_vec.vd].elements[i] = result;
        rsp->acc.l.elements[i] = result;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vrcp) {
    logfatal("Unimplemented: rsp_vec_vrcp")
}

RSP_VECTOR_INSTR(rsp_vec_vrcph) {
    byte de = instruction.cp2_vec.vs;

    rsp->divin = rsp->vu_regs[instruction.cp2_vec.vt].elements[instruction.cp2_vec.e];
    rsp->divin_loaded = true;
    rsp->acc.l.single = rsp->vu_regs[instruction.cp2_vec.vt].single;
    rsp->vu_regs[instruction.cp2_vec.vd].elements[de] = rsp->divout;
}

RSP_VECTOR_INSTR(rsp_vec_vrcpl) {
    bool L = true;
    vu_reg_t* vt = &rsp->vu_regs[instruction.cp2_vec.vt];
    vu_reg_t* vd = &rsp->vu_regs[instruction.cp2_vec.vd];

    sword result = 0;
    sword input = L && rsp->divin_loaded ? rsp->divin << 16 | vt->elements[7 - (instruction.cp2_vec.e & 7)] : vt->signed_elements[7 - (instruction.cp2_vec.e & 7)];
    sword mask = input >> 31;
    sword data = input ^ mask;
    if(input > -32768) data -= mask;
    if(data == 0) {
        result = 0x7FFFFFFF;
    } else if(input == -32768) {
        result = 0xFFFF0000;
    } else {
        word shift = __builtin_clz(data);
        word index = ((dword)data << shift & 0x7FC00000) >> 22;
        result = rcp_rom[index];
        result = (0x10000 | result) << 14;
        result = result >> (31 - shift) ^ mask;
    }
    rsp->divin_loaded = false;
    rsp->divout = result >> 16;
    rsp->acc.l.single = vt->single;

    vd->elements[7 - (instruction.cp2_vec.vs)] = result;
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
            rsp->vu_regs[instruction.cp2_vec.vd].single = rsp->acc.h.single;
            break;
        case 0x9:
            rsp->vu_regs[instruction.cp2_vec.vd].single = rsp->acc.m.single;
            break;
        case 0xA:
            rsp->vu_regs[instruction.cp2_vec.vd].single = rsp->acc.l.single;
            break;
        default: // Not actually sure what the default behavior is here
            for (int i = 0; i < 8; i++) {
                rsp->vu_regs[instruction.cp2_vec.vd].elements[i] = 0x0000;
            }
            break;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vsub) {
    vsvtvd;

    for (int i = 0; i < 8; i++) {
        sword result = vs->signed_elements[i] - vt->signed_elements[i] - (rsp->vco.l.elements[i] != 0);
        rsp->acc.l.signed_elements[i] = result;
        vd->signed_elements[i] = clamp_signed(result);
        rsp->vco.l.elements[i] = 0;
        rsp->vco.h.elements[i] = 0;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vsubc) {
    vsvtvd;

    for (int i = 0; i < 8; i++) {
        word result = vs->elements[i] - vt->elements[i];
        half hresult = result & 0xFFFF;
        bool carry = (result >> 16) & 1;

        vd->elements[i] = hresult;
        rsp->acc.l.elements[i] = hresult;
        rsp->vco.l.elements[i] = carry;
        rsp->vco.h.elements[i] = result != 0; // not hresult, but I bet that'd also work
    }
}

RSP_VECTOR_INSTR(rsp_vec_vxor) {
    unimplemented(instruction.cp2_vec.e != 0, "Element != 0")
    for (int i = 0; i < 8; i++) {
        half result = rsp->vu_regs[instruction.cp2_vec.vt].elements[i] ^ rsp->vu_regs[instruction.cp2_vec.vs].elements[i];
        rsp->vu_regs[instruction.cp2_vec.vd].elements[i] = result;
        rsp->acc.l.elements[i] = result;
    }
}
