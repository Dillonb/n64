#include "rsp_vector_instructions.h"

#include <emmintrin.h>

#include <log.h>
#include <tmmintrin.h>
#include "rsp.h"
#include "rsp_rom.h"

#define vsvtvd vu_reg_t* vs = &rsp->vu_regs[instruction.cp2_vec.vs]; vu_reg_t* vt = &rsp->vu_regs[instruction.cp2_vec.vt]; vu_reg_t* vd = &rsp->vu_regs[instruction.cp2_vec.vd]
#define defvte vu_reg_t vte = get_vte(rsp->vu_regs[instruction.cp2_vec.vt], instruction.cp2_vec.e)
#define elementzero unimplemented(instruction.cp2_vec.e != 0, "element was not zero!")

INLINE shalf clamp_signed(sdword value) {
    if (value < -32768) return -32768;
    if (value > 32767) return 32767;
    return value;
}

#define clamp_unsigned(x) ((x) < 0 ? 0 : ((x) > 32767 ? 65535 : x))

INLINE bool is_sign_extension(shalf high, shalf low) {
    if (high == 0) {
        return (low & 0x8000) == 0;
    } else if (high == -1) {
        return (low & 0x8000) == 0x8000;
    }
    return false;
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
            logfatal("vte where e > 15");
    }

    return vte;
}


#define SHIFT_AMOUNT_LBV_SBV 0
#define SHIFT_AMOUNT_LSV_SSV 1
#define SHIFT_AMOUNT_LLV_SLV 2
#define SHIFT_AMOUNT_LDV_SDV 3
#define SHIFT_AMOUNT_LQV_SQV 4
#define SHIFT_AMOUNT_LRV_SRV 4
#define SHIFT_AMOUNT_LPV_SPV 3
#define SHIFT_AMOUNT_LUV_SUV 3
#define SHIFT_AMOUNT_LHV_SHV 4
#define SHIFT_AMOUNT_LFV_SFV 4
#define SHIFT_AMOUNT_LTV_STV 4

INLINE int sign_extend_7bit_offset(byte offset, int shift_amount) {
    sbyte soffset = ((offset << 1) & 0x80) | offset;

    int ofs = soffset;
    return ofs << shift_amount;
}

word rcp(sword sinput) {
    if (sinput == 0) {
        return 0x7FFFFFFF;
    }

    word input = abs(sinput);
    int lshift = __builtin_clz(input) + 1;
    int rshift = 32 - lshift;
    int index = (input << lshift) >> 23;

    word rom = rcp_rom[index];
    word result = ((0x10000 | rom) << 14) >> rshift;

    if (input != sinput) {
        return ~result;
    } else {
        return result;
    }
}

word rsq(sword sinput) {
    if (sinput == 0) {
        return 0x7FFFFFFF;
    } else if (sinput == 0xFFFF8000) { // Only for RSQ special case
        return 0xFFFF0000;
    }

    word input = abs(sinput);
    int lshift = __builtin_clz(input) + 1;
    int rshift = (32 - lshift) >> 1; // Shifted by 1 instead of 0
    int index = (input << lshift) >> 24; // Shifted by 24 instead of 23

    word rom = rsq_rom[(index | ((lshift & 1) << 8))];
    word result = ((0x10000 | rom) << 14) >> rshift;

    if (input != sinput) {
        return ~result;
    } else {
        return result;
    }
}

RSP_VECTOR_INSTR(rsp_lwc2_lbv) {
    logdebug("rsp_lwc2_lbv");
    vu_reg_t* vt = &rsp->vu_regs[instruction.cp2_vec.vt];
    word address = get_rsp_register(rsp, instruction.v.base) + sign_extend_7bit_offset(instruction.v.offset, SHIFT_AMOUNT_LBV_SBV);

    vt->bytes[15 - instruction.v.element] = rsp->read_byte(address);
}

RSP_VECTOR_INSTR(rsp_lwc2_ldv) {
    logdebug("rsp_lwc2_ldv");
    word address = get_rsp_register(rsp, instruction.v.base) + sign_extend_7bit_offset(instruction.v.offset, SHIFT_AMOUNT_LDV_SDV);

    for (int i = 0; i < 8; i++) {
        int element = i + instruction.v.element;
        if (element > 15) {
            break;
        }
        rsp->vu_regs[instruction.v.vt].bytes[15 - element] = rsp->read_byte(address + i);
    }
}

RSP_VECTOR_INSTR(rsp_lwc2_lfv) {
    logdebug("rsp_lwc2_lfv");
    word address = get_rsp_register(rsp, instruction.v.base) + sign_extend_7bit_offset(instruction.v.offset, SHIFT_AMOUNT_LFV_SFV);
    int e = instruction.v.element;
    int start = e;
    int end = (start + 8);
    if (end > 15) {
        end = 15;
    }

    printf("LFV 0x%08X e %d\n", address, e);
    printf("%d -> %d (e = %d)\n", start, end, e);
    for (int i = e; i < end; i += 2) {
        printf("i: %d: ", i);
        half val = rsp->read_byte(address);
        int shift_amount = e & 7;
        if (shift_amount == 0) {
            shift_amount = 7;
        }
        printf("%02X << %d == ", val, shift_amount);
        val <<= shift_amount;
        printf("%04X\n", val);
        byte low = val & 0xFF;
        byte high = (val >> 8) & 0xFF;
        rsp->vu_regs[instruction.v.vt].bytes[15 - ((i + 0) & 15)] = high;
        printf("byte %d = 0x%02X\n", ((i + 0) & 15), high);
        rsp->vu_regs[instruction.v.vt].bytes[15 - ((i + 1) & 15)] = low;
        printf("byte %d = 0x%02X\n", ((i + 1) & 15), low);
        address += 4;
    }
}

RSP_VECTOR_INSTR(rsp_lwc2_lhv) {
    logdebug("rsp_lwc2_lhv");
    word address = get_rsp_register(rsp, instruction.v.base) + sign_extend_7bit_offset(instruction.v.offset, SHIFT_AMOUNT_LHV_SHV);

    word in_addr_offset = address & 0x7;
    address &= ~0x7;

    int e = instruction.v.element;

    for (int i = 0; i < 8; i++) {
        int ofs = ((16 - e) + (i * 2) + in_addr_offset) & 0xF;
        half val = rsp->read_byte(address + ofs);
        val <<= 7;
        rsp->vu_regs[instruction.v.vt].elements[7 - i] = val;
    }
}

RSP_VECTOR_INSTR(rsp_lwc2_llv) {
    logdebug("rsp_lwc2_llv");
    int e = instruction.v.element;
    word address = get_rsp_register(rsp, instruction.v.base) + sign_extend_7bit_offset(instruction.v.offset, SHIFT_AMOUNT_LLV_SLV);

    for (int i = 0; i < 4; i++) {
        int element = i + e;
        if (element > 15) {
            break;
        }
        rsp->vu_regs[instruction.v.vt].bytes[15 - element] = rsp->read_byte(address + i);
    }
}

RSP_VECTOR_INSTR(rsp_lwc2_lpv) {
    logdebug("rsp_lwc2_lpv");
    int e = instruction.v.element;
    word address = get_rsp_register(rsp, instruction.v.base) + sign_extend_7bit_offset(instruction.v.offset, SHIFT_AMOUNT_LPV_SPV);

    // Take into account how much the address is misaligned
    // since the accesses still wrap on the 8 byte boundary
    int address_offset = address & 7;
    address &= ~7;

    for(uint elem = 0; elem < 8; elem++) {
        int element_offset = (16 - e + (elem + address_offset)) & 0xF;

        half value = rsp->read_byte(address + element_offset);
        value <<= 8;
        rsp->vu_regs[instruction.v.vt].elements[7 - elem] = value;
    }
}

RSP_VECTOR_INSTR(rsp_lwc2_lqv) {
    logdebug("rsp_lwc2_lqv");
    int e = instruction.v.element;
    word address = get_rsp_register(rsp, instruction.v.base) + sign_extend_7bit_offset(instruction.v.offset, SHIFT_AMOUNT_LQV_SQV);
    word end_address = ((address & ~15) + 15);

    for (int i = 0; address + i <= end_address && i + e < 16; i++) {
        rsp->vu_regs[instruction.v.vt].bytes[15 - (i + e)] = rsp->read_byte(address + i);
    }
}

RSP_VECTOR_INSTR(rsp_lwc2_lrv) {
    logdebug("rsp_lwc2_lrv");
    int e = instruction.v.element;
    word address = get_rsp_register(rsp, instruction.v.base) + sign_extend_7bit_offset(instruction.v.offset, SHIFT_AMOUNT_LRV_SRV);
    int start = 16 - ((address & 0xF) - e);
    address &= 0xFFFFFFF0;

    for (int i = start; i < 16; i++) {
        rsp->vu_regs[instruction.v.vt].bytes[15 - (i & 0xF)] = rsp->read_byte(address++);
    }
}

RSP_VECTOR_INSTR(rsp_lwc2_lsv) {
    logdebug("rsp_lwc2_lsv");
    int e = instruction.v.element;
    word address = get_rsp_register(rsp, instruction.v.base) + sign_extend_7bit_offset(instruction.v.offset, SHIFT_AMOUNT_LSV_SSV);
    half val = rsp->read_half(address);
    byte lo = val & 0xFF;
    byte hi = (val >> 8) & 0xFF;
    rsp->vu_regs[instruction.v.vt].bytes[15 - (e + 0)] = hi;
    if (e < 15) {
        rsp->vu_regs[instruction.v.vt].bytes[15 - (e + 1)] = lo;
    }
}

RSP_VECTOR_INSTR(rsp_lwc2_ltv) {
    logdebug("rsp_lwc2_ltv");
    word base = get_rsp_register(rsp, instruction.v.base) + sign_extend_7bit_offset(instruction.v.offset, SHIFT_AMOUNT_LTV_STV);
    byte e = instruction.v.element;

    for (int i = 0; i < 8; i++) {
        word address = base;

        word offset = (i * 2) + e;

        half hi = rsp->read_byte(address + ((offset + 0) & 0xF));
        half lo = rsp->read_byte(address + ((offset + 1) & 0xF));

        int reg = (instruction.v.vt & 0x18) | ((i + (e >> 1)) & 0x7);

        rsp->vu_regs[reg].elements[7 - (i & 0x7)] = (hi << 8) | lo;
    }
}

RSP_VECTOR_INSTR(rsp_lwc2_luv) {
    logdebug("rsp_lwc2_luv");
    word address = get_rsp_register(rsp, instruction.v.base) + sign_extend_7bit_offset(instruction.v.offset, SHIFT_AMOUNT_LUV_SUV);

    int e = instruction.v.element;

    // Take into account how much the address is misaligned
    // since the accesses still wrap on the 8 byte boundary
    int address_offset = address & 7;
    address &= ~7;

    for(uint elem = 0; elem < 8; elem++) {
        int element_offset = (16 - e + (elem + address_offset)) & 0xF;

        half value = rsp->read_byte(address + element_offset);
        value <<= 7;
        rsp->vu_regs[instruction.v.vt].elements[7 - elem] = value;
    }
}

RSP_VECTOR_INSTR(rsp_swc2_sbv) {
    logdebug("rsp_swc2_sbv");
    word address = get_rsp_register(rsp, instruction.v.base) + sign_extend_7bit_offset(instruction.v.offset, SHIFT_AMOUNT_LBV_SBV);

    int element = instruction.v.element;
    rsp->write_byte(address, rsp->vu_regs[instruction.v.vt].bytes[15 - element]);
}

RSP_VECTOR_INSTR(rsp_swc2_sdv) {
    logdebug("rsp_swc2_sdv");
    word address = get_rsp_register(rsp, instruction.v.base) + sign_extend_7bit_offset(instruction.v.offset, SHIFT_AMOUNT_LDV_SDV);

    for (int i = 0; i < 8; i++) {
        int element = i + instruction.v.element;
        rsp->write_byte(address + i, rsp->vu_regs[instruction.v.vt].bytes[15 - (element & 0xF)]);
    }
}

RSP_VECTOR_INSTR(rsp_swc2_sfv) {
    logdebug("rsp_swc2_sfv");
    logfatal("Unimplemented: rsp_swc2_sfv");
}

RSP_VECTOR_INSTR(rsp_swc2_shv) {
    logdebug("rsp_swc2_shv");
    word address = get_rsp_register(rsp, instruction.v.base) + sign_extend_7bit_offset(instruction.v.offset, SHIFT_AMOUNT_LHV_SHV);

    word in_addr_offset = address & 0x7;
    address &= ~0x7;

    int e = instruction.v.element;

    for (int i = 0; i < 8; i++) {
        int byte_index = (i * 2) + e;
        half val = rsp->vu_regs[instruction.v.vt].bytes[15 - (byte_index & 15)] << 1;
        val |= rsp->vu_regs[instruction.v.vt].bytes[15 - ((byte_index + 1) & 15)] >> 7;
        byte b = val & 0xFF;

        int ofs = in_addr_offset + (i * 2);
        rsp->write_byte(address + (ofs & 0xF), b);
    }
}

RSP_VECTOR_INSTR(rsp_swc2_slv) {
    logdebug("rsp_swc2_slv");
    int e = instruction.v.element;
    word address = get_rsp_register(rsp, instruction.v.base) + sign_extend_7bit_offset(instruction.v.offset, SHIFT_AMOUNT_LLV_SLV);

    for (int i = 0; i < 4; i++) {
        int element = i + e;
        rsp->write_byte(address + i, rsp->vu_regs[instruction.v.vt].bytes[15 - (element & 0xF)]);
    }
}

RSP_VECTOR_INSTR(rsp_swc2_spv) {
    logdebug("rsp_swc2_spv");
    word address = get_rsp_register(rsp, instruction.v.base) + sign_extend_7bit_offset(instruction.v.offset, SHIFT_AMOUNT_LPV_SPV);

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
    logdebug("rsp_swc2_sqv");
    int e = instruction.v.element;
    word address = get_rsp_register(rsp, instruction.v.base) + sign_extend_7bit_offset(instruction.v.offset, SHIFT_AMOUNT_LQV_SQV);
    word end_address = ((address & ~15) + 15);

    for (int i = 0; address + i <= end_address; i++) {
        rsp->write_byte(address + i, rsp->vu_regs[instruction.v.vt].bytes[15 - ((i + e) & 15)]);
    }
}

RSP_VECTOR_INSTR(rsp_swc2_srv) {
    logdebug("rsp_swc2_srv");
    word address = get_rsp_register(rsp, instruction.v.base) + sign_extend_7bit_offset(instruction.v.offset, SHIFT_AMOUNT_LRV_SRV);
    int start = instruction.v.element;
    int end = start + (address & 15);
    int base = 16 - (address & 15);
    address &= ~15;
    for(int i = start; i < end; i++) {
        rsp->write_byte(address++, rsp->vu_regs[instruction.v.vt].bytes[15 - ((i + base) & 0xF)]);
    }
}

RSP_VECTOR_INSTR(rsp_swc2_ssv) {
    logdebug("rsp_swc2_ssv");
    word address = get_rsp_register(rsp, instruction.v.base) + sign_extend_7bit_offset(instruction.v.offset, SHIFT_AMOUNT_LSV_SSV);

    int element = instruction.v.element;
    rsp->write_half(address, rsp->vu_regs[instruction.v.vt].elements[7 - (element / 2)]);
}

RSP_VECTOR_INSTR(rsp_swc2_stv) {
    logdebug("rsp_swc2_stv");
    word base = get_rsp_register(rsp, instruction.v.base) + sign_extend_7bit_offset(instruction.v.offset, SHIFT_AMOUNT_LTV_STV);
    word in_addr_offset = base & 0x7;
    base &= ~0x7;

    byte e = instruction.v.element >> 1;

    for (int i = 0; i < 8; i++) {
        word address = base;

        word offset = (i * 2) + in_addr_offset;

        int reg = (instruction.v.vt & 0x18) | ((i + e) & 0x7);

        half val = rsp->vu_regs[reg].elements[7 - (i & 0x7)];
        half hi = (val >> 8) & 0xFF;
        half lo = (val >> 0) & 0xFF;

        rsp->write_byte(address + ((offset + 0) & 0xF), hi);
        rsp->write_byte(address + ((offset + 1) & 0xF), lo);
    }
}

RSP_VECTOR_INSTR(rsp_swc2_suv) {
    logdebug("rsp_swc2_suv");
    word address = get_rsp_register(rsp, instruction.v.base) + sign_extend_7bit_offset(instruction.v.offset, SHIFT_AMOUNT_LUV_SUV);

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
    logdebug("rsp_cfc2");
    shalf value = 0;
    switch (instruction.r.rd) {
        case 0: { // VCO
            value = rsp_get_vco(rsp);
            break;
        }
        case 1: { // VCC
            value = rsp_get_vcc(rsp);
            break;
        }
        case 2: { // VCE
            value = rsp_get_vce(rsp);
            break;
            default:
                logfatal("CFC2 from unknown VU control register: %d", instruction.r.rd);
        }
    }

    set_rsp_register(rsp, instruction.r.rt, (sword)value);
}

RSP_VECTOR_INSTR(rsp_ctc2) {
    logdebug("rsp_ctc2");
    half value = get_rsp_register(rsp, instruction.r.rt) & 0xFFFF;
    switch (instruction.r.rd) {
        case 0: { // VCO
            for (int i = 0; i < 8; i++) {
                rsp->vco.h.elements[7 - i] = ((value >> (i + 8)) & 1) == 1;
                rsp->vco.l.elements[7 - i] = ((value >> i) & 1) == 1;
            }
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
            logfatal("CTC2 to unknown VU control register: %d", instruction.r.rd);
    }
}

RSP_VECTOR_INSTR(rsp_mfc2) {
    logdebug("rsp_mfc2");
    byte hi = rsp->vu_regs[instruction.cp2_regmove.rd].bytes[15 - (instruction.cp2_regmove.e)];
    byte lo = rsp->vu_regs[instruction.cp2_regmove.rd].bytes[15 - ((instruction.cp2_regmove.e + 1) & 0xF)];
    shalf element = hi << 8 | lo;
    set_rsp_register(rsp, instruction.cp2_regmove.rt, (sword)element);
}

RSP_VECTOR_INSTR(rsp_mtc2) {
    logdebug("rsp_mtc2");
    half element = get_rsp_register(rsp, instruction.cp2_regmove.rt);
    byte lo = element & 0xFF;
    byte hi = (element >> 8) & 0xFF;
    rsp->vu_regs[instruction.cp2_regmove.rd].bytes[15 - (instruction.cp2_regmove.e + 0)] = hi;
    if (instruction.cp2_regmove.e < 0xF) {
        rsp->vu_regs[instruction.cp2_regmove.rd].bytes[15 - (instruction.cp2_regmove.e + 1)] = lo;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vabs) {
    logdebug("rsp_vec_vabs");
    vsvtvd;
    defvte;

    __m128i res = _mm_sign_epi16(vte.single, vs->single);
    rsp->acc.l.single = res;
    vd->single = res;

}

RSP_VECTOR_INSTR(rsp_vec_vadd) {
    logdebug("rsp_vec_vadd");
    vsvtvd;
    defvte;

    for (int i = 0; i < 8; i++) {
        shalf vs_element = vs->signed_elements[i];
        shalf vte_element = vte.signed_elements[i];
        sword result = vs_element + vte_element + (rsp->vco.l.elements[i] != 0);
        rsp->acc.l.elements[i] = result;
        vd->elements[i] = clamp_signed(result);
        rsp->vco.l.elements[i] = 0;
        rsp->vco.h.elements[i] = 0;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vaddc) {
    logdebug("rsp_vec_vaddc");
    elementzero;
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
    logdebug("rsp_vec_vand");
    vsvtvd;
    defvte;

    for (int i = 0; i < 8; i++) {
        half result = vte.elements[i] & vs->elements[i];
        vd->elements[i] = result;
        rsp->acc.l.elements[i] = result;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vch) {
    logdebug("rsp_vec_vch");
    vsvtvd;
    defvte;

    for (int i = 0; i < 8; i++) {
        shalf vs_element = vs->signed_elements[i];
        shalf vte_element = vte.signed_elements[i];
        rsp->vco.l.elements[i] = (vs->elements[i] >> 15) != (vte.elements[i] >> 15);
        half vt_abs = rsp->vco.l.elements[i] != 0 ? -vte_element : vte_element;
        rsp->vce.elements[i] = rsp->vco.l.elements[i] != 0 && (vs_element == -vte_element - 1);
        rsp->vco.h.elements[i] = rsp->vce.elements[i] == 0 && (vs_element != (shalf)vt_abs);
        rsp->vcc.l.elements[i] = vs_element <= -vte_element;
        rsp->vcc.h.elements[i] = vs_element >= vte_element;
        bool clip = rsp->vco.l.elements[i] != 0 ? rsp->vcc.l.elements[i] != 0 : rsp->vcc.h.elements[i] != 0;
        rsp->acc.l.elements[i] = clip ? vt_abs : vs->elements[i];
        vd->elements[i] = rsp->acc.l.elements[i];
    }
}

RSP_VECTOR_INSTR(rsp_vec_vcl) {
    logdebug("rsp_vec_vcl");
    vsvtvd;
    defvte;
    for (int i = 0; i < 8; i++) {
        half vs_element = vs->elements[i];
        half vte_element = vte.elements[i];
        if (rsp->vco.l.elements[i] == 0 && rsp->vco.h.elements[i] == 0) {
            rsp->vcc.h.elements[i] = (shalf)vs_element >= (shalf)vte_element;
        }

        if (rsp->vco.l.elements[i] != 0 && rsp->vco.h.elements[i] == 0) {
            bool lte = (shalf)vs_element <= -(shalf)vte_element;
            bool eql = (shalf)vs_element == -(shalf)vte_element;
            rsp->vcc.l.elements[i] = rsp->vce.elements[i] != 0 ? lte : eql;
        }
        bool clip = rsp->vco.l.elements[i] != 0 ? rsp->vcc.l.elements[i] : rsp->vcc.h.elements[i];
        half vtabs = rsp->vco.l.elements[i] != 0 ? -(half)vte_element : (half)vte_element;
        half acc = clip ? vtabs : vs_element;
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
    logdebug("rsp_vec_vcr");
    logfatal("Unimplemented: rsp_vec_vcr");
    elementzero;
}

RSP_VECTOR_INSTR(rsp_vec_veq) {
    logdebug("rsp_vec_veq");
    vsvtvd;
    defvte;

    for (int i = 0; i < 8; i++) {
        rsp->vcc.l.elements[i] = (rsp->vco.h.elements[i] == 0) && (vs->elements[i] == vte.elements[i]);
        rsp->acc.l.elements[i] = rsp->vcc.l.elements[i] != 0 ? vs->elements[i] : vte.elements[i];
        vd->elements[i] = rsp->acc.l.elements[i];

        rsp->vcc.h.elements[i] = 0;
        rsp->vco.h.elements[i] = 0;
        rsp->vco.l.elements[i] = 0;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vge) {
    logdebug("rsp_vec_vge");
    vsvtvd;
    defvte;

    for (int i = 0; i < 8; i++) {
        bool eql = vs->signed_elements[i] == vte.signed_elements[i];
        bool neg = !(rsp->vco.l.elements[i] != 0 && rsp->vco.h.elements[i] != 0) && eql;

        rsp->vcc.l.elements[i] = neg || (vs->signed_elements[i] > vte.signed_elements[i]);
        rsp->acc.l.elements[i] = rsp->vcc.l.elements[i] != 0 ? vs->elements[i] : vte.elements[i];
        vd->elements[i] = rsp->acc.l.elements[i];
        rsp->vcc.h.elements[i] = 0;
        rsp->vco.h.elements[i] = 0;
        rsp->vco.l.elements[i] = 0;

    }
}

RSP_VECTOR_INSTR(rsp_vec_vlt) {
    logdebug("rsp_vec_vlt");
    vsvtvd;
    defvte;

    for (int i = 0; i < 8; i++) {
        bool eql = vs->elements[i] == vte.elements[i];
        bool neg = rsp->vco.h.elements[i] != 0 && rsp->vco.l.elements[i] != 0 && eql;
        rsp->vcc.l.elements[i] = neg || (vs->signed_elements[i] < vte.signed_elements[i]);
        rsp->acc.l.elements[i] = rsp->vcc.l.elements[i] != 0 ? vs->elements[i] : vte.elements[i];
        vd->elements[i] = rsp->acc.l.elements[i];
        rsp->vcc.h.elements[i] = 0;
        rsp->vco.h.elements[i] = 0;
        rsp->vco.l.elements[i] = 0;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vmacf) {
    logdebug("rsp_vec_vmacf");
    defvte;
    for (int e = 0; e < 8; e++) {
        shalf multiplicand1 = vte.elements[e];
        shalf multiplicand2 = rsp->vu_regs[instruction.cp2_vec.vs].elements[e];
        sword prod = multiplicand1 * multiplicand2;

        sdword acc_delta = prod;
        acc_delta *= 2;
        sdword acc = get_rsp_accumulator(rsp, e) + acc_delta;

        shalf result = clamp_signed(acc >> 16);

        set_rsp_accumulator(rsp, e, acc);
        rsp->vu_regs[instruction.cp2_vec.vd].elements[e] = result;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vmacq) {
    logdebug("rsp_vec_vmacq");
    logfatal("Unimplemented: rsp_vec_vmacq");
    elementzero;
}

RSP_VECTOR_INSTR(rsp_vec_vmacu) {
    logdebug("rsp_vec_vmacu");
    defvte;
    elementzero;
    for (int e = 0; e < 8; e++) {
        shalf multiplicand1 = vte.elements[e];
        shalf multiplicand2 = rsp->vu_regs[instruction.cp2_vec.vs].elements[e];
        sword prod = multiplicand1 * multiplicand2;

        sdword acc_delta = prod;
        acc_delta *= 2;
        sdword acc = get_rsp_accumulator(rsp, e) + acc_delta;

        half result = clamp_unsigned(acc >> 16);

        set_rsp_accumulator(rsp, e, acc);
        rsp->vu_regs[instruction.cp2_vec.vd].elements[e] = result;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vmadh) {
    logdebug("rsp_vec_vmadh");
    defvte;
    for (int e = 0; e < 8; e++) {
        shalf multiplicand1 = vte.elements[e];
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
    logdebug("rsp_vec_vmadl");
    defvte;
    for (int e = 0; e < 8; e++) {
        half multiplicand1 = vte.elements[e];
        half multiplicand2 = rsp->vu_regs[instruction.cp2_vec.vs].elements[e];
        word prod = multiplicand1 * multiplicand2;

        dword acc_delta = prod >> 16;
        dword acc = get_rsp_accumulator(rsp, e) + acc_delta;

        set_rsp_accumulator(rsp, e, acc);
        half result;
        if (is_sign_extension(rsp->acc.h.signed_elements[e], rsp->acc.m.signed_elements[e])) {
            result = rsp->acc.l.elements[e];
        } else if (rsp->acc.h.signed_elements[e] < 0) {
            result = 0;
        } else {
            result = 0xFFFF;
        }

        rsp->vu_regs[instruction.cp2_vec.vd].elements[e] = result;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vmadm) {
    logdebug("rsp_vec_vmadm");
    defvte;
    for (int e = 0; e < 8; e++) {
        half multiplicand1 = vte.elements[e];
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
    logdebug("rsp_vec_vmadn");
    defvte;
    for (int e = 0; e < 8; e++) {
        shalf multiplicand1 = vte.elements[e];
        half multiplicand2 = rsp->vu_regs[instruction.cp2_vec.vs].elements[e];
        sword prod = multiplicand1 * multiplicand2;

        sdword acc_delta = prod;
        sdword acc = get_rsp_accumulator(rsp, e) + acc_delta;

        set_rsp_accumulator(rsp, e, acc);

        half result;

        if (is_sign_extension(rsp->acc.h.signed_elements[e], rsp->acc.m.signed_elements[e])) {
            result = rsp->acc.l.elements[e];
        } else if (rsp->acc.h.signed_elements[e] < 0) {
            result = 0;
        } else {
            result = 0xFFFF;
        }

        rsp->vu_regs[instruction.cp2_vec.vd].elements[e] = result;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vmov) {
    logdebug("rsp_vec_vmov");
    vsvtvd;
    defvte;
    int de = instruction.cp2_vec.vs & 7;

    rsp->acc.l.single = vte.single;
    vd->elements[de] = vte.elements[de];
}

RSP_VECTOR_INSTR(rsp_vec_vmrg) {
    logdebug("rsp_vec_vmrg");
    vsvtvd;
    defvte;

    for (int i = 0; i < 8; i++) {
        rsp->acc.l.elements[i] = rsp->vcc.l.elements[i] != 0 ? vs->elements[i] : vte.elements[i];
        vd->elements[i] = rsp->acc.l.elements[i];

        rsp->vco.l.elements[i] = 0;
        rsp->vco.h.elements[i] = 0;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vmudh) {
    logdebug("rsp_vec_vmudh");
    defvte;
    for (int e = 0; e < 8; e++) {
        shalf multiplicand1 = vte.elements[e];
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
    logdebug("rsp_vec_vmudl");
    defvte;
    for (int e = 0; e < 8; e++) {
        half multiplicand1 = vte.elements[e];
        half multiplicand2 = rsp->vu_regs[instruction.cp2_vec.vs].elements[e];
        word prod = multiplicand1 * multiplicand2;

        dword acc = prod >> 16;

        set_rsp_accumulator(rsp, e, acc);
        half result;
        if (is_sign_extension(rsp->acc.h.signed_elements[e], rsp->acc.m.signed_elements[e])) {
            result = rsp->acc.l.elements[e];
        } else if (rsp->acc.h.signed_elements[e] < 0) {
            result = 0;
        } else {
            result = 0xFFFF;
        }

        rsp->vu_regs[instruction.cp2_vec.vd].elements[e] = result;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vmudm) {
    logdebug("rsp_vec_vmudm");
    defvte;
    for (int e = 0; e < 8; e++) {
        half multiplicand1 = vte.elements[e];
        shalf multiplicand2 = rsp->vu_regs[instruction.cp2_vec.vs].elements[e];
        sword prod = multiplicand1 * multiplicand2;

        sdword acc = prod;

        shalf result = clamp_signed(acc >> 16);

        set_rsp_accumulator(rsp, e, acc);
        rsp->vu_regs[instruction.cp2_vec.vd].elements[e] = result;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vmudn) {
    logdebug("rsp_vec_vmudn");
    defvte;
    for (int e = 0; e < 8; e++) {
        shalf multiplicand1 = vte.elements[e];
        half multiplicand2 = rsp->vu_regs[instruction.cp2_vec.vs].elements[e];
        sword prod = multiplicand1 * multiplicand2;

        sdword acc = prod;

        set_rsp_accumulator(rsp, e, acc);

        half result;
        if (is_sign_extension(rsp->acc.h.signed_elements[e], rsp->acc.m.signed_elements[e])) {
            result = rsp->acc.l.elements[e];
        } else if (rsp->acc.h.signed_elements[e] < 0) {
            result = 0;
        } else {
            result = 0xFFFF;
        }

        rsp->vu_regs[instruction.cp2_vec.vd].elements[e] = result;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vmulf) {
    logdebug("rsp_vec_vmulf");
    defvte;
    for (int e = 0; e < 8; e++) {
        shalf multiplicand1 = vte.elements[e];
        shalf multiplicand2 = rsp->vu_regs[instruction.cp2_vec.vs].elements[e];
        sword prod = multiplicand1 * multiplicand2;

        sdword acc = prod;
        acc = (acc * 2) + 0x8000;

        set_rsp_accumulator(rsp, e, acc);

        shalf result = clamp_signed(acc >> 16);
        rsp->vu_regs[instruction.cp2_vec.vd].elements[e] = result;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vmulq) {
    logdebug("rsp_vec_vmulq");
    logfatal("Unimplemented: rsp_vec_vmulq");
    elementzero;
}

RSP_VECTOR_INSTR(rsp_vec_vmulu) {
    logdebug("rsp_vec_vmulu");
    defvte;
    elementzero;
    for (int e = 0; e < 8; e++) {
        shalf multiplicand1 = vte.elements[e];
        shalf multiplicand2 = rsp->vu_regs[instruction.cp2_vec.vs].elements[e];
        sword prod = multiplicand1 * multiplicand2;

        sdword acc = prod;
        acc = (acc * 2) + 0x8000;

        half result = clamp_unsigned(acc >> 16);

        set_rsp_accumulator(rsp, e, acc);
        rsp->vu_regs[instruction.cp2_vec.vd].elements[e] = result;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vnand) {
    logdebug("rsp_vec_vnand");
    elementzero;
    for (int i = 0; i < 8; i++) {
        half result = ~(rsp->vu_regs[instruction.cp2_vec.vt].elements[i] & rsp->vu_regs[instruction.cp2_vec.vs].elements[i]);
        rsp->vu_regs[instruction.cp2_vec.vd].elements[i] = result;
        rsp->acc.l.elements[i] = result;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vne) {
    logdebug("rsp_vec_vne");
    vsvtvd;
    defvte;

    for (int i = 0; i < 8; i++) {
        rsp->vcc.l.elements[i] = (rsp->vco.h.elements[i] == 0) || (vs->elements[i] != vte.elements[i]);
        rsp->acc.l.elements[i] = rsp->vcc.l.elements[i] != 0 ? vs->elements[i] : vte.elements[i];
        vd->elements[i] = rsp->acc.l.elements[i];

        rsp->vcc.h.elements[i] = 0;
        rsp->vco.h.elements[i] = 0;
        rsp->vco.l.elements[i] = 0;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vnop) {
    logdebug("rsp_vec_vnop");
    logfatal("Unimplemented: rsp_vec_vnop");
    elementzero;
}

RSP_VECTOR_INSTR(rsp_vec_vnor) {
    logdebug("rsp_vec_vnor");
    elementzero;
    for (int i = 0; i < 8; i++) {
        half result = ~(rsp->vu_regs[instruction.cp2_vec.vt].elements[i] | rsp->vu_regs[instruction.cp2_vec.vs].elements[i]);
        rsp->vu_regs[instruction.cp2_vec.vd].elements[i] = result;
        rsp->acc.l.elements[i] = result;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vnxor) {
    logdebug("rsp_vec_vnxor");
    vsvtvd;
    defvte;
    for (int i = 0; i < 8; i++) {
        half result = ~(vte.elements[i] ^ vs->elements[i]);
        vd->elements[i] = result;
        rsp->acc.l.elements[i] = result;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vor) {
    logdebug("rsp_vec_vor");
    defvte;
    for (int i = 0; i < 8; i++) {
        half result =  vte.elements[i] | rsp->vu_regs[instruction.cp2_vec.vs].elements[i];
        rsp->vu_regs[instruction.cp2_vec.vd].elements[i] = result;
        rsp->acc.l.elements[i] = result;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vrcp) {
    logdebug("rsp_vec_vrcp");
    sword input;
    int e  = instruction.cp2_vec.e & 7;
    int de = instruction.cp2_vec.vs & 7;
    input = rsp->vu_regs[instruction.cp2_vec.vt].signed_elements[7 - e];
    word result = rcp(input);
    rsp->vu_regs[instruction.cp2_vec.vd].elements[7 - de] = result & 0xFFFF;
    rsp->divout = (result >> 16) & 0xFFFF;
    rsp->acc.l.single = rsp->vu_regs[instruction.cp2_vec.vt].single;
}

RSP_VECTOR_INSTR(rsp_vec_vrcpl) {
    logdebug("rsp_vec_vrcpl");
    sword input;
    int e  = instruction.cp2_vec.e & 7;
    int de = instruction.cp2_vec.vs & 7;
    if (rsp->divin_loaded) {
        input = (rsp->divin << 16) | rsp->vu_regs[instruction.cp2_vec.vt].elements[7 - e];
    } else {
        input = rsp->vu_regs[instruction.cp2_vec.vt].signed_elements[7 - e];
    }
    word result = rcp(input);
    rsp->vu_regs[instruction.cp2_vec.vd].elements[7 - de] = result & 0xFFFF;
    rsp->divout = (result >> 16) & 0xFFFF;
    rsp->divin = 0;
    rsp->divin_loaded = false;
    rsp->acc.l.single = rsp->vu_regs[instruction.cp2_vec.vt].single;
}

RSP_VECTOR_INSTR(rsp_vec_vrndn) {
    logdebug("rsp_vec_vrndn");
    logfatal("Unimplemented: rsp_vec_vrndn");
    elementzero;
}

RSP_VECTOR_INSTR(rsp_vec_vrndp) {
    logdebug("rsp_vec_vrndp");
    logfatal("Unimplemented: rsp_vec_vrndp");
    elementzero;
}

RSP_VECTOR_INSTR(rsp_vec_vrsq) {
    logdebug("rsp_vec_vrsq");
    sword input;
    int e  = instruction.cp2_vec.e & 7;
    int de = instruction.cp2_vec.vs & 7;
    input = rsp->vu_regs[instruction.cp2_vec.vt].signed_elements[7 - e];
    word result = rsq(input);
    rsp->vu_regs[instruction.cp2_vec.vd].elements[7 - de] = result & 0xFFFF;
    rsp->divout = (result >> 16) & 0xFFFF;
    rsp->acc.l.single = rsp->vu_regs[instruction.cp2_vec.vt].single;
}

RSP_VECTOR_INSTR(rsp_vec_vrcph_vrsqh) {
    logdebug("rsp_vec_vrcph_vrsqh");
    byte de = instruction.cp2_vec.vs;

    rsp->divin = rsp->vu_regs[instruction.cp2_vec.vt].elements[7 - instruction.cp2_vec.e];
    rsp->divin_loaded = true;
    rsp->acc.l.single = rsp->vu_regs[instruction.cp2_vec.vt].single;
    rsp->vu_regs[instruction.cp2_vec.vd].elements[7 - de] = rsp->divout;
}

RSP_VECTOR_INSTR(rsp_vec_vrsql) {
    elementzero;
    logdebug("rsp_vec_vrsql");
    sword input;
    int e  = instruction.cp2_vec.e & 7;
    int de = instruction.cp2_vec.vs & 7;
    if (rsp->divin_loaded) {
        input = (rsp->divin << 16) | rsp->vu_regs[instruction.cp2_vec.vt].elements[7 - e];
    } else {
        input = rsp->vu_regs[instruction.cp2_vec.vt].signed_elements[7 - e];
    }
    word result = rsq(input);
    rsp->vu_regs[instruction.cp2_vec.vd].elements[7 - de] = result & 0xFFFF;
    rsp->divout = (result >> 16) & 0xFFFF;
    rsp->divin = 0;
    rsp->divin_loaded = false;
    rsp->acc.l.single = rsp->vu_regs[instruction.cp2_vec.vt].single;
}

RSP_VECTOR_INSTR(rsp_vec_vsar) {
    logdebug("rsp_vec_vsar");
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
    logdebug("rsp_vec_vsub");
    vsvtvd;
    defvte;

    for (int i = 0; i < 8; i++) {
        sword result = vs->signed_elements[i] - vte.signed_elements[i] - (rsp->vco.l.elements[i] != 0);
        rsp->acc.l.signed_elements[i] = result;
        vd->signed_elements[i] = clamp_signed(result);
        rsp->vco.l.elements[i] = 0;
        rsp->vco.h.elements[i] = 0;
    }
}

RSP_VECTOR_INSTR(rsp_vec_vsubc) {
    logdebug("rsp_vec_vsubc");
    vsvtvd;
    defvte;

    for (int i = 0; i < 8; i++) {
        word result = vs->elements[i] - vte.elements[i];
        half hresult = result & 0xFFFF;
        bool carry = (result >> 16) & 1;

        vd->elements[i] = hresult;
        rsp->acc.l.elements[i] = hresult;
        rsp->vco.l.elements[i] = carry;
        rsp->vco.h.elements[i] = result != 0; // not hresult, but I bet that'd also work
    }
}

RSP_VECTOR_INSTR(rsp_vec_vxor) {
    logdebug("rsp_vec_vxor");
    elementzero;
    for (int i = 0; i < 8; i++) {
        half result = rsp->vu_regs[instruction.cp2_vec.vt].elements[i] ^ rsp->vu_regs[instruction.cp2_vec.vs].elements[i];
        rsp->vu_regs[instruction.cp2_vec.vd].elements[i] = result;
        rsp->acc.l.elements[i] = result;
    }
}
