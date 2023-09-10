#include "rsp_disassemble.h"
#include <mips_instruction_decode.h>
#include <rsp.h>

int disassemble_rsp_cp2(u32 address, mips_instruction_t instr, char* buf, int buflen) {
    snprintf(buf, buflen, "rsp cp2 (TODO)");
    return 1;
}

int disassemble_rsp_lwc2(u32 address, mips_instruction_t instr, char* buf, int buflen) {
    switch (instr.v.funct) {
        case LWC2_LBV: snprintf(buf, buflen, "lbv $v%d[%d], %02X($%s)", instr.v.vt, instr.v.element, instr.v.offset, register_names[instr.v.base]); break;
        case LWC2_LDV: snprintf(buf, buflen, "ldv $v%d[%d], %02X($%s)", instr.v.vt, instr.v.element, instr.v.offset, register_names[instr.v.base]); break;
        case LWC2_LFV: snprintf(buf, buflen, "lfv $v%d[%d], %02X($%s)", instr.v.vt, instr.v.element, instr.v.offset, register_names[instr.v.base]); break;
        case LWC2_LHV: snprintf(buf, buflen, "lhv $v%d[%d], %02X($%s)", instr.v.vt, instr.v.element, instr.v.offset, register_names[instr.v.base]); break;
        case LWC2_LLV: snprintf(buf, buflen, "llv $v%d[%d], %02X($%s)", instr.v.vt, instr.v.element, instr.v.offset, register_names[instr.v.base]); break;
        case LWC2_LPV: snprintf(buf, buflen, "lpv $v%d[%d], %02X($%s)", instr.v.vt, instr.v.element, instr.v.offset, register_names[instr.v.base]); break;
        case LWC2_LQV: snprintf(buf, buflen, "lqv $v%d[%d], %02X($%s)", instr.v.vt, instr.v.element, instr.v.offset, register_names[instr.v.base]); break;
        case LWC2_LRV: snprintf(buf, buflen, "lrv $v%d[%d], %02X($%s)", instr.v.vt, instr.v.element, instr.v.offset, register_names[instr.v.base]); break;
        case LWC2_LSV: snprintf(buf, buflen, "lsv $v%d[%d], %02X($%s)", instr.v.vt, instr.v.element, instr.v.offset, register_names[instr.v.base]); break;
        case LWC2_LTV: snprintf(buf, buflen, "ltv $v%d[%d], %02X($%s)", instr.v.vt, instr.v.element, instr.v.offset, register_names[instr.v.base]); break;
        case LWC2_LUV: snprintf(buf, buflen, "luv $v%d[%d], %02X($%s)", instr.v.vt, instr.v.element, instr.v.offset, register_names[instr.v.base]); break;
        case SWC2_SWV: snprintf(buf, buflen, "swv (rsp_nop)"); break;
        default:
            return 0;
    }
    return 1;
}

int disassemble_rsp_swc2(u32 address, mips_instruction_t instr, char* buf, int buflen) {
    switch (instr.v.funct) {
        case LWC2_LBV: snprintf(buf, buflen, "sbv $v%d[%d], %02X($%s)", instr.v.vt, instr.v.element, instr.v.offset, register_names[instr.v.base]); break;
        case LWC2_LDV: snprintf(buf, buflen, "sdv $v%d[%d], %02X($%s)", instr.v.vt, instr.v.element, instr.v.offset, register_names[instr.v.base]); break;
        case LWC2_LFV: snprintf(buf, buflen, "sfv $v%d[%d], %02X($%s)", instr.v.vt, instr.v.element, instr.v.offset, register_names[instr.v.base]); break;
        case LWC2_LHV: snprintf(buf, buflen, "shv $v%d[%d], %02X($%s)", instr.v.vt, instr.v.element, instr.v.offset, register_names[instr.v.base]); break;
        case LWC2_LLV: snprintf(buf, buflen, "slv $v%d[%d], %02X($%s)", instr.v.vt, instr.v.element, instr.v.offset, register_names[instr.v.base]); break;
        case LWC2_LPV: snprintf(buf, buflen, "spv $v%d[%d], %02X($%s)", instr.v.vt, instr.v.element, instr.v.offset, register_names[instr.v.base]); break;
        case LWC2_LQV: snprintf(buf, buflen, "sqv $v%d[%d], %02X($%s)", instr.v.vt, instr.v.element, instr.v.offset, register_names[instr.v.base]); break;
        case LWC2_LRV: snprintf(buf, buflen, "srv $v%d[%d], %02X($%s)", instr.v.vt, instr.v.element, instr.v.offset, register_names[instr.v.base]); break;
        case LWC2_LSV: snprintf(buf, buflen, "ssv $v%d[%d], %02X($%s)", instr.v.vt, instr.v.element, instr.v.offset, register_names[instr.v.base]); break;
        case LWC2_LTV: snprintf(buf, buflen, "stv $v%d[%d], %02X($%s)", instr.v.vt, instr.v.element, instr.v.offset, register_names[instr.v.base]); break;
        case LWC2_LUV: snprintf(buf, buflen, "suv $v%d[%d], %02X($%s)", instr.v.vt, instr.v.element, instr.v.offset, register_names[instr.v.base]); break;
        case SWC2_SWV: snprintf(buf, buflen, "swv $v%d[%d], %02X($%s)", instr.v.vt, instr.v.element, instr.v.offset, register_names[instr.v.base]); break;
        default:
            return 0;
    }
    return 1;
}

int disassemble_rsp_unique(u32 address, u32 raw, char* buf, int buflen) {
    mips_instruction_t instr;
    instr.raw = raw;
    switch (instr.op) {
        case OPC_CP2:      return disassemble_rsp_cp2(address, instr, buf, buflen);
        case RSP_OPC_LWC2: return disassemble_rsp_lwc2(address, instr, buf, buflen);
        case RSP_OPC_SWC2: return disassemble_rsp_swc2(address, instr, buf, buflen);
        default: return 0;
    }
}