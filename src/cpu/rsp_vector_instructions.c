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
    vu_reg_t* vt = &rsp->vu_regs[instruction.cp2_vec.vt];

    sbyte offset = instruction.v.offset << 1;
    word address = get_rsp_register(rsp, instruction.v.base) + (offset / 2);

    vt->bytes[15 - instruction.v.element] = rsp->read_byte(address);
}

RSP_VECTOR_INSTR(rsp_lwc2_ldv) {
    sbyte offset = instruction.v.offset << 1;
    word address = get_rsp_register(rsp, instruction.v.base) + offset * 4;

    for (int i = 0; i < 8; i++) {
        int element = i + instruction.v.element;
        if (element > 15) {
            break;
        }
        rsp->vu_regs[instruction.v.vt].bytes[15 - element] = rsp->read_byte(address + i);
    }
}

RSP_VECTOR_INSTR(rsp_lwc2_lfv) {
    printf("Unimplemented: rsp_lwc2_lfv\n");
    exit(0);
}

RSP_VECTOR_INSTR(rsp_lwc2_lhv) {
    printf("Unimplemented: rsp_lwc2_lhv\n");
    exit(0);
}

RSP_VECTOR_INSTR(rsp_lwc2_llv) {
    int e = instruction.v.element;
    sword offset     = (sbyte)(instruction.v.offset << 1);
    word address     = get_rsp_register(rsp, instruction.v.base) + offset * 2;

    for (int i = 0; i < 4; i++) {
        int element = i + e;
        if (element > 15) {
            break;
        }
        rsp->vu_regs[instruction.v.vt].bytes[15 - element] = rsp->read_byte(address + i);
    }
}

RSP_VECTOR_INSTR(rsp_lwc2_lpv) {
    word base_address = get_rsp_register(rsp, instruction.v.base);
    sbyte base_offset = instruction.v.offset << 1;

    int e = instruction.v.element;

    base_address += ((sword)base_offset << 2);

    // Take into account how much the address is misaligned
    // since the accesses still wrap on the 8 byte boundary
    int address_offset = base_address & 7;
    base_address &= ~7;

    for(uint elem = 0; elem < 8; elem++) {
        int element_offset = (16 - e + (elem + address_offset)) & 0xF;

        half value = rsp->read_byte(base_address + element_offset);
        value <<= 8;
        rsp->vu_regs[instruction.v.vt].elements[7 - elem] = value;
    }
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
    int e = instruction.v.element;
    sbyte offset       = instruction.v.offset << 1;
    word address       = get_rsp_register(rsp, instruction.v.base) + offset * 8;
    int start = 16 - ((address & 0xF) - e);
    address &= 0xFFFFFFF0;

    for (int i = start; i < 16; i++) {
        rsp->vu_regs[instruction.v.vt].bytes[15 - (i & 0xF)] = rsp->read_byte(address++);
    }
}

RSP_VECTOR_INSTR(rsp_lwc2_lsv) {
    int e = instruction.v.element;
    sbyte offset     = instruction.v.offset << 1;
    word address     = get_rsp_register(rsp, instruction.v.base) + offset;
    half val = rsp->read_half(address);
    byte lo = val & 0xFF;
    byte hi = (val >> 8) & 0xFF;
    rsp->vu_regs[instruction.v.vt].bytes[15 - (e + 0)] = hi;
    if (e < 15) {
        rsp->vu_regs[instruction.v.vt].bytes[15 - (e + 1)] = lo;
    }
}

RSP_VECTOR_INSTR(rsp_lwc2_ltv) {
    word address = get_rsp_register(rsp, instruction.v.base);
    sbyte offset = + instruction.v.offset << 1;
    address += (sword)offset << 3;

    word start = get_rsp_register(rsp, instruction.v.vt);
    word end = start + 8;
    if (end > 32) {
        end = 32;
    }

    address = ((address + 8) & ~15) + (instruction.v.element & 1);
    for(int i = start; i < end; i++) {
        int byte_index = (8 - (instruction.v.element >> 1) + (i - start)) << 1;

        rsp->vu_regs[i].bytes[(byte_index + 0) & 15] = rsp->read_byte(address++);
        rsp->vu_regs[i].bytes[(byte_index + 1) & 15] = rsp->read_byte(address++);
    }
}

RSP_VECTOR_INSTR(rsp_lwc2_luv) {
    word base_address = get_rsp_register(rsp, instruction.v.base);
    sbyte base_offset = instruction.v.offset << 1;

    int e = instruction.v.element;

    base_address += ((sword)base_offset << 2);

    // Take into account how much the address is misaligned
    // since the accesses still wrap on the 8 byte boundary
    int address_offset = base_address & 7;
    base_address &= ~7;

    for(uint elem = 0; elem < 8; elem++) {
        int element_offset = (16 - e + (elem + address_offset)) & 0xF;

        half value = rsp->read_byte(base_address + element_offset);
        value <<= 7;
        rsp->vu_regs[instruction.v.vt].elements[7 - elem] = value;
    }
}

RSP_VECTOR_INSTR(rsp_swc2_sbv) {
    sbyte offset = (instruction.v.offset << 1);
    offset >>= 1;

    word address = get_rsp_register(rsp, instruction.v.base) + offset;

    int element = instruction.v.element;
    rsp->write_byte(address, rsp->vu_regs[instruction.v.vt].bytes[15 - element]);
}

RSP_VECTOR_INSTR(rsp_swc2_sdv) {
    sbyte offset = instruction.v.offset << 1;
    word address = get_rsp_register(rsp, instruction.v.base) + offset * 4;

    for (int i = 0; i < 8; i++) {
        int element = i + instruction.v.element;
        rsp->write_byte(address + i, rsp->vu_regs[instruction.v.vt].bytes[15 - (element & 0xF)]);
    }
}

RSP_VECTOR_INSTR(rsp_swc2_sfv) {
    printf("Unimplemented: rsp_swc2_sfv\n");
    exit(0);
}

RSP_VECTOR_INSTR(rsp_swc2_shv) {
    printf("Unimplemented: rsp_swc2_shv\n");
    exit(0);
}

RSP_VECTOR_INSTR(rsp_swc2_slv) {
    int e = instruction.v.element;
    sword offset     = (sbyte)(instruction.v.offset << 1);
    word address     = get_rsp_register(rsp, instruction.v.base) + offset * 2;

    for (int i = 0; i < 4; i++) {
        int element = i + e;
        rsp->write_byte(address + i, rsp->vu_regs[instruction.v.vt].bytes[15 - (element & 0xF)]);
    }
}

RSP_VECTOR_INSTR(rsp_swc2_spv) {
    word address = get_rsp_register(rsp, instruction.v.base);
    sbyte base_offset = + instruction.v.offset << 1;
    address += ((sword)base_offset << 2);

    int start = instruction.v.element;
    int end = start + 8;

    for (int offset = start; offset < end; offset++) {
        if((offset & 15) < 8) {
            rsp->write_byte(address++, rsp->vu_regs[instruction.v.vt].bytes[15 - ((offset & 7) << 1)]);
        } else {
            rsp->write_byte(address++, rsp->vu_regs[instruction.v.vt].elements[7 - (offset & 7)] >> 7);
        }
    }
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
    sbyte offset = instruction.v.offset << 1;
    word address = get_rsp_register(rsp, instruction.v.base) + offset * 8;
    int start = instruction.v.element;
    int end = start + (address & 15);
    int base = 16 - (address & 15);
    address &= ~15;
    for(int i = start; i < end; i++) {
        rsp->write_byte(address++, rsp->vu_regs[instruction.v.vt].bytes[15 - ((i + base) & 0xF)]);
    }
}

RSP_VECTOR_INSTR(rsp_swc2_ssv) {
    sbyte offset = instruction.v.offset << 1;
    word address = get_rsp_register(rsp, instruction.v.base) + offset;

    int element = instruction.v.element;
    rsp->write_half(address, rsp->vu_regs[instruction.v.vt].elements[7 - (element / 2)]);
}

RSP_VECTOR_INSTR(rsp_swc2_stv) {
    word address = get_rsp_register(rsp, instruction.v.base);
    sbyte offset = + instruction.v.offset << 1;
    address += (sword)offset << 3;
    int start_vu_reg = instruction.v.vt & 0b11000;

    int end_vu_reg = start_vu_reg + 8;
    if (end_vu_reg > 32) {
        end_vu_reg = 32;
    }

    int element = 8 - (instruction.v.element >> 1);
    word base = (address & 0xF) + (element << 1);
    word wrap_point = (address + 0x10) & 0xFFFFFFF8;
    base = (address & ~15) + (base & 0xF);
    address = base;
    for(int vu_reg = start_vu_reg; vu_reg < end_vu_reg; vu_reg++) {
        half val = rsp->vu_regs[vu_reg].elements[7 - (element++ & 7)];
        byte lo = val & 0xFF;
        byte hi = (val >> 8) & 0xFF;

        rsp->write_byte(address++, hi);
        if (address >= wrap_point) {
            address = base & ~0xF;
        }
        rsp->write_byte(address++, lo);
        if (address >= wrap_point) {
            address = base & ~0xF;
        }
    }
}

RSP_VECTOR_INSTR(rsp_swc2_suv) {
    word address = get_rsp_register(rsp, instruction.v.base);
    sbyte base_offset = + instruction.v.offset << 1;
    address += ((sword)base_offset << 2);

    int start = instruction.v.element;
    int end = start + 8;
    for(uint offset = start; offset < end; offset++) {
        if((offset & 15) < 8) {
            rsp->write_byte(address++, rsp->vu_regs[instruction.v.vt].elements[7 - (offset & 7)] >> 7);
        } else {
            rsp->write_byte(address++, rsp->vu_regs[instruction.v.vt].bytes[15 - ((offset & 7) << 1)]);
        }
    }
}

RSP_VECTOR_INSTR(rsp_cfc2) {
    shalf value = 0;
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

    set_rsp_register(rsp, instruction.r.rt, (sword)value);
}

RSP_VECTOR_INSTR(rsp_ctc2) {
    half value = get_rsp_register(rsp, instruction.r.rt) & 0xFFFF;
    switch (instruction.r.rd) {
        case 0: { // VCO
            printf("Copying %X to VCO\nVCO: ", value);
            for (int i = 0; i < 8; i++) {
                rsp->vco.h.elements[7 - i] = ((value >> (i + 8)) & 1) == 1;
                rsp->vco.l.elements[7 - i] = ((value >> i) & 1) == 1;
                printf("%d", rsp->vco.h.elements[7-i]);
            }
            for (int i = 0; i < 8; i++) {
                printf("%d", rsp->vco.l.elements[7-i]);
            }
            printf("\n");
            break;
        }
        case 1: { // VCC
            for (int i = 0; i < 8; i++) {
                rsp->vcc.h.elements[7 - i] = ((value >> (i + 8)) & 1) == 1;
                rsp->vcc.l.elements[7 - i] = ((value >> i) & 1) == 1;
            }
            break;
        }
        case 2: { // VCE
            for (int i = 0; i < 8; i++) {
                rsp->vce.elements[7 - i] = ((value >> i) & 1) == 1;
            }
            break;
        }
        default:
            logfatal("CTC2 to unknown VU control register: %d", instruction.r.rd)
    }
}

RSP_VECTOR_INSTR(rsp_mfc2) {
    byte hi = rsp->vu_regs[instruction.cp2_regmove.rd].bytes[15 - (instruction.cp2_regmove.e)];
    byte lo = rsp->vu_regs[instruction.cp2_regmove.rd].bytes[15 - ((instruction.cp2_regmove.e + 1) & 0xF)];
    shalf element = hi << 8 | lo;
    set_rsp_register(rsp, instruction.cp2_regmove.rt, (sword)element);
}

RSP_VECTOR_INSTR(rsp_mtc2) {
    half element = get_rsp_register(rsp, instruction.cp2_regmove.rt);
    byte lo = element & 0xFF;
    byte hi = (element >> 8) & 0xFF;
    rsp->vu_regs[instruction.cp2_regmove.rd].bytes[15 - (instruction.cp2_regmove.e + 0)] = hi;
    if (instruction.cp2_regmove.e < 0xF) {
        rsp->vu_regs[instruction.cp2_regmove.rd].bytes[15 - (instruction.cp2_regmove.e + 1)] = lo;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vabs) {
    printf("Unimplemented: rsp_vec_vabs\n");
    exit(0);
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
    vsvtvd;

    //for i in 0..7
    for (int i = 0; i < 8; i++) {
        half vse = vs->elements[i];
        half vte = vt->elements[i];
        //result(16..0) = VS<i>(15..0) + VT<i>(15..0)
        word result = vse + vte;
        //ACC<i>(15..0) = result(15..0)
        rsp->acc.l.elements[i] = result & 0xFFFF;
        //VD<i>(15..0) = result(15..0)
        vd->elements[i] = result & 0xFFFF;
        //VCO(i) = result(16)
        rsp->vco.l.elements[i] = (result >> 16) & 1;
        //VCO(i + 8) = 0
        rsp->vco.h.elements[i] = 0;
        //endfor
    }
}

RSP_VECTOR_INSTR(rsp_vec_vand) {
    //unimplemented(instruction.cp2_vec.e != 0, "Element != 0")
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
            rsp->vcc.h.elements[i] = (shalf)vse >= (shalf)vte;
        }

        if (rsp->vco.l.elements[i] != 0 && rsp->vco.h.elements[i] == 0) {
            bool lte = (shalf)vse <= -(shalf)vte;
            bool eql = (shalf)vse == -(shalf)vte;
            rsp->vcc.l.elements[i] = rsp->vce.elements[i] != 0 ? lte : eql;
        }
        bool clip = rsp->vco.l.elements[i] != 0 ? rsp->vcc.l.elements[i] : rsp->vcc.h.elements[i];
        half vtabs = rsp->vco.l.elements[i] != 0 ? -(half)vte : (half)vte;
        half acc = clip ? vtabs : vse;
        rsp->acc.l.elements[i] = acc;
        vd->elements[i] = acc;

    }
    for (int i = 0; i < 8; i++) {
        rsp->vco.l.elements[i] = 0;
        rsp->vco.h.elements[i] = 0;

        rsp->vce.elements[i] = 0;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vcr) {
    printf("Unimplemented: rsp_vec_vcr\n");
    exit(0);
}

RSP_VECTOR_INSTR(rsp_vec_veq) {
    printf("Unimplemented: rsp_vec_veq\n");
    exit(0);
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
        rsp->vco.l.elements[i] = 0;

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
    printf("Unimplemented: rsp_vec_vmacq\n");
    exit(0);
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
    printf("Unimplemented: rsp_vec_vmov\n");
    exit(0);
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
        shalf multiplicand1 = rsp->vu_regs[instruction.cp2_vec.vt].elements[e];
        shalf multiplicand2 = rsp->vu_regs[instruction.cp2_vec.vs].elements[e];
        sword prod = multiplicand1 * multiplicand2;

        sdword acc = prod;

        shalf result = clamp_signed(acc);

        acc <<= 16;
        set_rsp_accumulator(rsp, e, acc);

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
    printf("Unimplemented: rsp_vec_vmulq\n");
    exit(0);
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

INLINE vu_reg_t get_vte(vu_reg_t vt, byte e) {
    vu_reg_t vte;
    switch(e) {
        case 0 ... 1:
            return vt;
        case 2:
            vte.single = _mm_shufflehi_epi16(_mm_shufflelo_epi16(vt.single, 0b11110101), 0b11110101);
            break;
        case 3:
            vte.single = _mm_shufflehi_epi16(_mm_shufflelo_epi16(vt.single, 0b10100000), 0b10100000);
            break;
        case 4:
            vte.single = _mm_shufflehi_epi16(_mm_shufflelo_epi16(vt.single, 0b11111111), 0b11111111);
            break;
        case 5:
            vte.single = _mm_shufflehi_epi16(_mm_shufflelo_epi16(vt.single, 0b10101010), 0b10101010);
            break;
        case 6:
            vte.single = _mm_shufflehi_epi16(_mm_shufflelo_epi16(vt.single, 0b01010101), 0b01010101);
            break;
        case 7:
            vte.single = _mm_shufflehi_epi16(_mm_shufflelo_epi16(vt.single, 0b00000000), 0b00000000);
            break;
        case 8 ... 15:
            for (int i = 0; i < 8; i++) {
                vte.elements[i] = vt.elements[7 - (e - 8)];
            }
            break;
        default:
            logfatal("vte where e > 15")
    }

    return vte;
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
    printf("Unimplemented: rsp_vec_vne\n");
    exit(0);
}

RSP_VECTOR_INSTR(rsp_vec_vnop) {
    printf("Unimplemented: rsp_vec_vnop\n");
    exit(0);
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
    vu_reg_t vte = get_vte(rsp->vu_regs[instruction.cp2_vec.vt], instruction.cp2_vec.e);
    for (int i = 0; i < 8; i++) {
        half result =  vte.elements[i] | rsp->vu_regs[instruction.cp2_vec.vs].elements[i];
        rsp->vu_regs[instruction.cp2_vec.vd].elements[i] = result;
        rsp->acc.l.elements[i] = result;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vrcp) {
    printf("Unimplemented: rsp_vec_vrcp\n");
    exit(0);
}

RSP_VECTOR_INSTR(rsp_vec_vrcph) {
    byte de = instruction.cp2_vec.vs;

    rsp->divin = rsp->vu_regs[instruction.cp2_vec.vt].elements[7 - instruction.cp2_vec.e];
    rsp->divin_loaded = true;
    rsp->acc.l.single = rsp->vu_regs[instruction.cp2_vec.vt].single;
    rsp->vu_regs[instruction.cp2_vec.vd].elements[7 - de] = rsp->divout;
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
    printf("Unimplemented: rsp_vec_vrndn\n");
    exit(0);
}

RSP_VECTOR_INSTR(rsp_vec_vrndp) {
    printf("Unimplemented: rsp_vec_vrndp\n");
    exit(0);
}

RSP_VECTOR_INSTR(rsp_vec_vrsq) {
    vu_reg_t* vt = &rsp->vu_regs[instruction.cp2_vec.vt];
    vu_reg_t* vd = &rsp->vu_regs[instruction.cp2_vec.vd];

    sword result = 0;
    sword input = vt->signed_elements[7 - (instruction.cp2_vec.e & 7)];
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
        result = rsq_rom[index];
        result = (0x10000 | result) << 14;
        result = result >> (31 - shift) ^ mask;
    }
    rsp->divin_loaded = false;
    rsp->divout = result >> 16;
    rsp->acc.l.single = vt->single;

    vd->elements[7 - (instruction.cp2_vec.vs)] = result;
    exit(0);
}

RSP_VECTOR_INSTR(rsp_vec_vrsqh) {
    byte de = instruction.cp2_vec.vs;

    rsp->divin = rsp->vu_regs[instruction.cp2_vec.vt].elements[7 - instruction.cp2_vec.e];
    rsp->divin_loaded = true;
    rsp->acc.l.single = rsp->vu_regs[instruction.cp2_vec.vt].single;
    rsp->vu_regs[instruction.cp2_vec.vd].elements[7 - de] = rsp->divout;
}

RSP_VECTOR_INSTR(rsp_vec_vrsql) {
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
        result = rsq_rom[index];
        result = (0x10000 | result) << 14;
        result = result >> (31 - shift) ^ mask;
    }
    rsp->divin_loaded = false;
    rsp->divout = result >> 16;
    rsp->acc.l.single = vt->single;

    vd->elements[7 - (instruction.cp2_vec.vs)] = result;
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
