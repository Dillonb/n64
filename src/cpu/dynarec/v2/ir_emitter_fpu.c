#include <r4300i.h>
#include <disassemble.h>
#include "ir_emitter_fpu.h"
#include "ir_context.h"

void emit_ir_cvt(mips_instruction_t instruction, ir_float_value_type_t from, ir_float_value_type_t to, ir_float_convert_mode_t mode) {
    ir_instruction_t* source = ir_emit_load_guest_fgr(IR_FGR(instruction.fr.fs), from);
    ir_emit_float_convert(source, from, to, IR_FGR(instruction.fr.fd), mode);
}

#define CVT(from, to, mode) case FP_FMT_##from: emit_ir_cvt(instruction, FLOAT_VALUE_TYPE_##from, FLOAT_VALUE_TYPE_##to, FLOAT_CONVERT_MODE_##mode); break

IR_EMITTER(ldc1) {
    //checkcp1; // TODO: check cp1 is enabled
    logwarn("LDC1: TODO: Check CP1 is enabled");
    ir_instruction_t* address = ir_get_memory_access_address(instruction, BUS_LOAD);
    ir_emit_load(VALUE_TYPE_U64, address, IR_FGR(instruction.fi.ft));
}

IR_EMITTER(sdc1) {
    //checkcp1; // TODO: check cp1 is enabled
    logwarn("SDC1: TODO: Check CP1 is enabled");
    ir_instruction_t* address = ir_get_memory_access_address(instruction, BUS_STORE);
    ir_instruction_t* value = ir_emit_load_guest_fgr(IR_FGR(instruction.fi.ft), FLOAT_VALUE_TYPE_LONG);
    ir_emit_store(VALUE_TYPE_U64, address, value);
}

IR_EMITTER(lwc1) {
    //checkcp1; // TODO: check cp1 is enabled
    logwarn("LWC1: TODO: Check CP1 is enabled");
    ir_instruction_t* address = ir_get_memory_access_address(instruction, BUS_LOAD);
    ir_emit_load(VALUE_TYPE_U32, address, IR_FGR(instruction.fi.ft));
}

IR_EMITTER(swc1) {
    logwarn("SWC1: TODO: Check CP1 is enabled");
    ir_instruction_t* address = ir_get_memory_access_address(instruction, BUS_STORE);
    ir_instruction_t* value = ir_emit_load_guest_fgr(IR_FGR(instruction.fi.ft), FLOAT_VALUE_TYPE_WORD);
    ir_emit_store(VALUE_TYPE_U32, address, value);
}

IR_EMITTER(cfc1) {
    //checkcp1; // TODO: check cp1 is enabled
    logwarn("CFC1: TODO: Check CP1 is enabled");
    u8 fs = instruction.r.rd;
    switch (fs) {
        case 0:
            ir_emit_get_ptr(VALUE_TYPE_U32, &N64CPU.fcr0.raw, instruction.r.rt);
            break;
        case 31:
            ir_emit_get_ptr(VALUE_TYPE_U32, &N64CPU.fcr31.raw, instruction.r.rt);
            break;
        default:
            logfatal("This instruction is only defined when fs == 0 or fs == 31! (Throw an exception?)");
    }
}

IR_EMITTER(ctc1) {
    logwarn("CTC1: TODO: Check CP1 is enabled");
    //checkcp1; // TODO: check cp1 is enabled
    ir_instruction_t* value = ir_emit_load_guest_gpr(instruction.r.rt);
    u8 fs = instruction.r.rd;
    switch (fs) {
        case 0:
            logwarn("CTC1 FCR0: Wrote to read-only register FCR0!");
            break;
        case 31: {
            ir_instruction_t* mask = ir_emit_set_constant_u32(0x183ffff, NO_GUEST_REG);
            ir_instruction_t* masked = ir_emit_and(mask, value, NO_GUEST_REG);
            ir_emit_set_ptr(VALUE_TYPE_U32, &N64CPU.fcr31.raw, masked);
            break;
        }
        default:
            logfatal("This instruction is only defined when fs == 0 or fs == 31! (Throw an exception?)");
    }
}

ir_instruction_t* get_cp1_compare() {
    ir_instruction_t* compare_mask = ir_emit_set_constant_u32(FCR31_COMPARE_MASK, NO_GUEST_REG);
    ir_instruction_t* fcr31 = ir_emit_get_ptr(VALUE_TYPE_U32, &N64CPU.fcr31.raw, NO_GUEST_REG);
    return ir_emit_and(fcr31, compare_mask, NO_GUEST_REG);
}

IR_EMITTER(bc1t) {
    ir_instruction_t* compare = get_cp1_compare();
    ir_instruction_t* zero = ir_emit_load_guest_gpr(0);
    ir_instruction_t* cond = ir_emit_check_condition(CONDITION_NOT_EQUAL, zero, compare, NO_GUEST_REG);
    ir_emit_conditional_branch(cond, instruction.i.immediate, virtual_address);
}

IR_EMITTER(bc1f) {
    ir_instruction_t* compare = get_cp1_compare();
    ir_instruction_t* zero = ir_emit_load_guest_gpr(0);
    ir_instruction_t* cond = ir_emit_check_condition(CONDITION_EQUAL, zero, compare, NO_GUEST_REG);
    ir_emit_conditional_branch(cond, instruction.i.immediate, virtual_address);
}

IR_EMITTER(bc1tl) {
    ir_instruction_t* zero = ir_emit_load_guest_gpr(0);
    ir_instruction_t* compare = get_cp1_compare();
    ir_instruction_t* cond = ir_emit_check_condition(CONDITION_NOT_EQUAL, zero, compare, NO_GUEST_REG);
    ir_emit_conditional_branch_likely(cond, instruction.i.immediate, virtual_address, index);
}

IR_EMITTER(bc1fl) {
    ir_instruction_t* zero = ir_emit_load_guest_gpr(0);
    ir_instruction_t* compare = get_cp1_compare();
    ir_instruction_t* cond = ir_emit_check_condition(CONDITION_EQUAL, zero, compare, NO_GUEST_REG);
    ir_emit_conditional_branch_likely(cond, instruction.i.immediate, virtual_address, index);
}

IR_EMITTER(mfc1) {
    logwarn("TODO: MFC1: check CP1 is enabled");
    ir_instruction_t* value = ir_emit_load_guest_fgr(IR_FGR(instruction.fr.fs), FLOAT_VALUE_TYPE_WORD);
    ir_emit_mov_reg_type(value, REGISTER_TYPE_GPR, VALUE_TYPE_S32, IR_GPR(instruction.r.rt));
}

IR_EMITTER(mtc1) {
    logwarn("TODO: check CP1 is enabled");
    //checkcp1;
    ir_instruction_t* value = ir_emit_load_guest_gpr(IR_GPR(instruction.r.rt));
    ir_emit_mov_reg_type(value, REGISTER_TYPE_FGR_32, VALUE_TYPE_U32, IR_FGR(instruction.r.rd));
}

IR_EMITTER(cp1_add) {
    //checkcp1;
    logwarn("TODO: check cp1 enabled");
    switch (instruction.fr.fmt) {
        case FP_FMT_DOUBLE:
            ir_emit_float_add(
                    ir_emit_load_guest_fgr(IR_FGR(instruction.fr.fs), FLOAT_VALUE_TYPE_DOUBLE),
                    ir_emit_load_guest_fgr(IR_FGR(instruction.fr.ft), FLOAT_VALUE_TYPE_DOUBLE),
                    FLOAT_VALUE_TYPE_DOUBLE,
                    IR_FGR(instruction.fr.fd));
            break;
        case FP_FMT_SINGLE:
            ir_emit_float_add(
                    ir_emit_load_guest_fgr(IR_FGR(instruction.fr.fs), FLOAT_VALUE_TYPE_SINGLE),
                    ir_emit_load_guest_fgr(IR_FGR(instruction.fr.ft), FLOAT_VALUE_TYPE_SINGLE),
                    FLOAT_VALUE_TYPE_SINGLE,
                    IR_FGR(instruction.fr.fd));
            break;
        default:
            logfatal("mips_cp1_invalid");
    }
}

IR_EMITTER(cp1_sub) {
    //checkcp1;
    logwarn("TODO: check cp1 enabled");
    switch (instruction.fr.fmt) {
        case FP_FMT_DOUBLE:
            ir_emit_float_sub(
                    ir_emit_load_guest_fgr(IR_FGR(instruction.fr.fs), FLOAT_VALUE_TYPE_DOUBLE),
                    ir_emit_load_guest_fgr(IR_FGR(instruction.fr.ft), FLOAT_VALUE_TYPE_DOUBLE),
                    FLOAT_VALUE_TYPE_DOUBLE,
                    IR_FGR(instruction.fr.fd));
            break;
        case FP_FMT_SINGLE:
            ir_emit_float_sub(
                    ir_emit_load_guest_fgr(IR_FGR(instruction.fr.fs), FLOAT_VALUE_TYPE_SINGLE),
                    ir_emit_load_guest_fgr(IR_FGR(instruction.fr.ft), FLOAT_VALUE_TYPE_SINGLE),
                    FLOAT_VALUE_TYPE_SINGLE,
                    IR_FGR(instruction.fr.fd));
            break;
        default:
            logfatal("mips_cp1_invalid");
    }
}

IR_EMITTER(cp1_mult) {
    //checkcp1;
    logwarn("TODO: check cp1 enabled");
    switch (instruction.fr.fmt) {
        case FP_FMT_DOUBLE:
            ir_emit_float_mult(
                    ir_emit_load_guest_fgr(IR_FGR(instruction.fr.fs), FLOAT_VALUE_TYPE_DOUBLE),
                    ir_emit_load_guest_fgr(IR_FGR(instruction.fr.ft), FLOAT_VALUE_TYPE_DOUBLE),
                    FLOAT_VALUE_TYPE_DOUBLE,
                    IR_FGR(instruction.fr.fd));
            break;
        case FP_FMT_SINGLE:
            ir_emit_float_mult(
                    ir_emit_load_guest_fgr(IR_FGR(instruction.fr.fs), FLOAT_VALUE_TYPE_SINGLE),
                    ir_emit_load_guest_fgr(IR_FGR(instruction.fr.ft), FLOAT_VALUE_TYPE_SINGLE),
                    FLOAT_VALUE_TYPE_SINGLE,
                    IR_FGR(instruction.fr.fd));
            break;
        default:
            logfatal("mips_cp1_invalid");
    }
}

IR_EMITTER(cp1_div) {
    //checkcp1;
    logwarn("TODO: check cp1 enabled");
    switch (instruction.fr.fmt) {
        case FP_FMT_DOUBLE:
            ir_emit_float_div(
                    ir_emit_load_guest_fgr(IR_FGR(instruction.fr.fs), FLOAT_VALUE_TYPE_DOUBLE),
                    ir_emit_load_guest_fgr(IR_FGR(instruction.fr.ft), FLOAT_VALUE_TYPE_DOUBLE),
                    FLOAT_VALUE_TYPE_DOUBLE,
                    IR_FGR(instruction.fr.fd));
            break;
        case FP_FMT_SINGLE:
            ir_emit_float_div(
                    ir_emit_load_guest_fgr(IR_FGR(instruction.fr.fs), FLOAT_VALUE_TYPE_SINGLE),
                    ir_emit_load_guest_fgr(IR_FGR(instruction.fr.ft), FLOAT_VALUE_TYPE_SINGLE),
                    FLOAT_VALUE_TYPE_SINGLE,
                    IR_FGR(instruction.fr.fd));
            break;
        default:
            logfatal("mips_cp1_invalid");
    }
}

IR_EMITTER(cp1_trunc_l) {
    //checkcp1;
    logwarn("TODO: check cp1 enabled");
    switch (instruction.fr.fmt) {
        case FP_FMT_DOUBLE:
            logfatal("mips_cp_trunc_l_d");
        case FP_FMT_SINGLE:
            logfatal("mips_cp_trunc_l_s");
        default:
            logfatal("mips_cp1_invalid");
    }
}

IR_EMITTER(cp1_round_l) {
    //checkcp1;
    logwarn("TODO: check cp1 enabled");
    switch (instruction.fr.fmt) {
        case FP_FMT_DOUBLE:
            logfatal("mips_cp_round_l_d");
        case FP_FMT_SINGLE:
            logfatal("mips_cp_round_l_s");
        default:
            logfatal("mips_cp1_invalid");
    }
}

IR_EMITTER(cp1_trunc_w) {
    //checkcp1;
    logwarn("TODO: check cp1 enabled");
    switch (instruction.fr.fmt) {
        CVT(DOUBLE, WORD, TRUNC);
        CVT(SINGLE, WORD, TRUNC);
        default:
            logfatal("mips_cp1_invalid");
    }
}

IR_EMITTER(cp1_floor_w) {
    //checkcp1;
    logwarn("TODO: check cp1 enabled");
    switch (instruction.fr.fmt) {
        CVT(DOUBLE, WORD, FLOOR);
        CVT(SINGLE, WORD, FLOOR);
        default:
            logfatal("mips_cp1_invalid");
    }
}

IR_EMITTER(cp1_round_w) {
    //checkcp1;
    logwarn("TODO: check cp1 enabled");
    switch (instruction.fr.fmt) {
        CVT(DOUBLE, WORD, ROUND);
        CVT(SINGLE, WORD, ROUND);
        default:
            logfatal("mips_cp1_invalid");
    }
}

IR_EMITTER(cp1_cvt_d) {
    //checkcp1;
    logwarn("TODO: check cp1 enabled");
    switch (instruction.fr.fmt) {
        CVT(SINGLE, DOUBLE, CONVERT);
        CVT(WORD,   DOUBLE, CONVERT);
        CVT(LONG,   DOUBLE, CONVERT);
        default:
            logfatal("mips_cp1_invalid");
    }
}

IR_EMITTER(cp1_cvt_l) {
    //checkcp1;
    logwarn("TODO: check cp1 enabled");
    switch (instruction.fr.fmt) {
        CVT(DOUBLE, LONG, CONVERT);
        CVT(SINGLE, LONG, CONVERT);
        default:
            logfatal("mips_cp1_invalid");
    }
}

IR_EMITTER(cp1_cvt_s) {
    //checkcp1;
    logwarn("TODO: check cp1 enabled");
    switch (instruction.fr.fmt) {
        CVT(DOUBLE, SINGLE, CONVERT);
        CVT(WORD,   SINGLE, CONVERT);
        CVT(LONG,   SINGLE, CONVERT);
        default:
            logfatal("mips_cp1_invalid");
    }
}

IR_EMITTER(cp1_cvt_w) {
    //checkcp1;
    logwarn("TODO: check cp1 enabled");
    switch (instruction.fr.fmt) {
        CVT(DOUBLE, WORD, CONVERT);
        CVT(SINGLE, WORD, CONVERT);
        default:
            logfatal("mips_cp1_invalid");
    }
}

IR_EMITTER(cp1_sqrt) {
    //checkcp1;
    logwarn("TODO: check cp1 enabled");
    switch (instruction.fr.fmt) {
        case FP_FMT_DOUBLE:
            logfatal("mips_cp_sqrt_d");
        case FP_FMT_SINGLE:
            logfatal("mips_cp_sqrt_s");
        default:
            logfatal("mips_cp1_invalid");
    }

}

IR_EMITTER(cp1_abs) {
    //checkcp1;
    logwarn("TODO: check cp1 enabled");
    switch (instruction.fr.fmt) {
        case FP_FMT_DOUBLE:
            logfatal("mips_cp_abs_d");
        case FP_FMT_SINGLE:
            logfatal("mips_cp_abs_s");
        default:
            logfatal("mips_cp1_invalid");
    }

}

IR_EMITTER(cp1_mov) {
    //checkcp1;
    logwarn("TODO: check cp1 enabled");
    switch (instruction.fr.fmt) {
        case FP_FMT_DOUBLE:
            update_guest_reg_mapping(IR_FGR(instruction.fr.fd), ir_emit_load_guest_fgr(IR_FGR(instruction.fr.fs), FLOAT_VALUE_TYPE_DOUBLE));
            break;
        case FP_FMT_SINGLE:
            update_guest_reg_mapping(IR_FGR(instruction.fr.fd), ir_emit_load_guest_fgr(IR_FGR(instruction.fr.fs), FLOAT_VALUE_TYPE_SINGLE));
            break;
        default:
            logfatal("mips_cp1_invalid");
    }
}

IR_EMITTER(cp1_neg) {
    //checkcp1;
    logwarn("TODO: check cp1 enabled");
    switch (instruction.fr.fmt) {
        case FP_FMT_DOUBLE:
            logfatal("mips_cp_neg_d");
        case FP_FMT_SINGLE:
            logfatal("mips_cp_neg_s");
        default:
            logfatal("mips_cp1_invalid");
    }
}

IR_EMITTER(cp1_c_f) {
    //checkcp1;
    logwarn("TODO: check cp1 enabled");
    switch (instruction.fr.fmt) {
        case FP_FMT_DOUBLE:
            logfatal("mips_cp_c_f_d");
        case FP_FMT_SINGLE:
            logfatal("mips_cp_c_f_s");
        default:
            logfatal("mips_cp1_invalid");
    }
}

IR_EMITTER(cp1_c_un) {
    //checkcp1;
    logwarn("TODO: check cp1 enabled");
    switch (instruction.fr.fmt) {
        case FP_FMT_DOUBLE:
            logfatal("mips_cp_c_un_d");
        case FP_FMT_SINGLE:
            logfatal("mips_cp_c_un_s");
        default:
            logfatal("mips_cp1_invalid");
    }
}

IR_EMITTER(cp1_c_eq) {
    //checkcp1;
    logwarn("TODO: check cp1 enabled");
    switch (instruction.fr.fmt) {
        case FP_FMT_DOUBLE:
            ir_emit_float_check_condition(
                    CONDITION_FLOAT_EQ,
                    ir_emit_load_guest_fgr(IR_FGR(instruction.fr.fs), FLOAT_VALUE_TYPE_DOUBLE),
                    ir_emit_load_guest_fgr(IR_FGR(instruction.fr.ft), FLOAT_VALUE_TYPE_DOUBLE),
                    FLOAT_VALUE_TYPE_DOUBLE);
            break;
        case FP_FMT_SINGLE:
            ir_emit_float_check_condition(
                    CONDITION_FLOAT_EQ,
                    ir_emit_load_guest_fgr(IR_FGR(instruction.fr.fs), FLOAT_VALUE_TYPE_SINGLE),
                    ir_emit_load_guest_fgr(IR_FGR(instruction.fr.ft), FLOAT_VALUE_TYPE_SINGLE),
                    FLOAT_VALUE_TYPE_SINGLE);
            break;
        default:
            logfatal("mips_cp1_invalid");
    }
}

IR_EMITTER(cp1_c_ueq) {
    //checkcp1;
    logwarn("TODO: check cp1 enabled");
    switch (instruction.fr.fmt) {
        case FP_FMT_DOUBLE:
            logfatal("mips_cp_c_ueq_d");
        case FP_FMT_SINGLE:
            logfatal("mips_cp_c_ueq_s");
        default:
            logfatal("mips_cp1_invalid");
    }
}

IR_EMITTER(cp1_c_olt) {
    //checkcp1;
    logwarn("TODO: check cp1 enabled");
    switch (instruction.fr.fmt) {
        case FP_FMT_DOUBLE:
            logfatal("mips_cp_c_olt_d");
        case FP_FMT_SINGLE:
            logfatal("mips_cp_c_olt_s");
        default:
            logfatal("mips_cp1_invalid");
    }
}

IR_EMITTER(cp1_c_ult) {
    //checkcp1;
    logwarn("TODO: check cp1 enabled");
    switch (instruction.fr.fmt) {
        case FP_FMT_DOUBLE:
            logfatal("mips_cp_c_ult_d");
        case FP_FMT_SINGLE:
            logfatal("mips_cp_c_ult_s");
        default:
            logfatal("mips_cp1_invalid");
    }
}

IR_EMITTER(cp1_c_ole) {
    //checkcp1;
    logwarn("TODO: check cp1 enabled");
    switch (instruction.fr.fmt) {
        case FP_FMT_DOUBLE:
            logfatal("mips_cp_c_ole_d");
        case FP_FMT_SINGLE:
            logfatal("mips_cp_c_ole_s");
        default:
            logfatal("mips_cp1_invalid");
    }
}

IR_EMITTER(cp1_c_ule) {
    //checkcp1;
    logwarn("TODO: check cp1 enabled");
    switch (instruction.fr.fmt) {
        case FP_FMT_DOUBLE:
            logfatal("mips_cp_c_ule_d");
        case FP_FMT_SINGLE:
            logfatal("mips_cp_c_ule_s");
        default:
            logfatal("mips_cp1_invalid");
    }
}

IR_EMITTER(cp1_c_sf) {
    //checkcp1;
    logwarn("TODO: check cp1 enabled");
    switch (instruction.fr.fmt) {
        case FP_FMT_DOUBLE:
            logfatal("mips_cp_c_sf_d");
        case FP_FMT_SINGLE:
            logfatal("mips_cp_c_sf_s");
        default:
            logfatal("mips_cp1_invalid");
    }
}

IR_EMITTER(cp1_c_ngle) {
    //checkcp1;
    logwarn("TODO: check cp1 enabled");
    switch (instruction.fr.fmt) {
        case FP_FMT_DOUBLE:
            logfatal("mips_cp_c_ngle_d");
        case FP_FMT_SINGLE:
            logfatal("mips_cp_c_ngle_s");
        default:
            logfatal("mips_cp1_invalid");
    }
}

IR_EMITTER(cp1_c_seq) {
    //checkcp1;
    logwarn("TODO: check cp1 enabled");
    switch (instruction.fr.fmt) {
        case FP_FMT_DOUBLE:
            logfatal("mips_cp_c_seq_d");
        case FP_FMT_SINGLE:
            logfatal("mips_cp_c_seq_s");
        default:
            logfatal("mips_cp1_invalid");
    }
}

IR_EMITTER(cp1_c_ngl) {
    //checkcp1;
    logwarn("TODO: check cp1 enabled");
    switch (instruction.fr.fmt) {
        case FP_FMT_DOUBLE:
            logfatal("mips_cp_c_ngl_d");
        case FP_FMT_SINGLE:
            logfatal("mips_cp_c_ngl_s");
        default:
            logfatal("mips_cp1_invalid");
    }
}

IR_EMITTER(cp1_c_lt) {
    //checkcp1;
    logwarn("TODO: check cp1 enabled");
    switch (instruction.fr.fmt) {
        case FP_FMT_DOUBLE:
            ir_emit_float_check_condition(
                    CONDITION_FLOAT_LT,
                    ir_emit_load_guest_fgr(IR_FGR(instruction.fr.fs), FLOAT_VALUE_TYPE_DOUBLE),
                    ir_emit_load_guest_fgr(IR_FGR(instruction.fr.ft), FLOAT_VALUE_TYPE_DOUBLE),
                    FLOAT_VALUE_TYPE_DOUBLE);
            break;
        case FP_FMT_SINGLE:
            ir_emit_float_check_condition(
                    CONDITION_FLOAT_LT,
                    ir_emit_load_guest_fgr(IR_FGR(instruction.fr.fs), FLOAT_VALUE_TYPE_SINGLE),
                    ir_emit_load_guest_fgr(IR_FGR(instruction.fr.ft), FLOAT_VALUE_TYPE_SINGLE),
                    FLOAT_VALUE_TYPE_SINGLE);
            break;
        default:
            logfatal("mips_cp1_invalid");
    }
}

IR_EMITTER(cp1_c_nge) {
    //checkcp1;
    logwarn("TODO: check cp1 enabled");
    switch (instruction.fr.fmt) {
        case FP_FMT_DOUBLE:
            ir_emit_float_check_condition(
                    CONDITION_FLOAT_NGE,
                    ir_emit_load_guest_fgr(IR_FGR(instruction.fr.fs), FLOAT_VALUE_TYPE_DOUBLE),
                    ir_emit_load_guest_fgr(IR_FGR(instruction.fr.ft), FLOAT_VALUE_TYPE_DOUBLE),
                    FLOAT_VALUE_TYPE_DOUBLE);
            break;
        case FP_FMT_SINGLE:
            ir_emit_float_check_condition(
                    CONDITION_FLOAT_NGE,
                    ir_emit_load_guest_fgr(IR_FGR(instruction.fr.fs), FLOAT_VALUE_TYPE_SINGLE),
                    ir_emit_load_guest_fgr(IR_FGR(instruction.fr.ft), FLOAT_VALUE_TYPE_SINGLE),
                    FLOAT_VALUE_TYPE_SINGLE);
            break;
        default:
            logfatal("mips_cp1_invalid");
    }
}

IR_EMITTER(cp1_c_le) {
    //checkcp1;
    logwarn("TODO: check cp1 enabled");
    switch (instruction.fr.fmt) {
        case FP_FMT_DOUBLE:
            ir_emit_float_check_condition(
                    CONDITION_FLOAT_LE,
                    ir_emit_load_guest_fgr(IR_FGR(instruction.fr.fs), FLOAT_VALUE_TYPE_DOUBLE),
                    ir_emit_load_guest_fgr(IR_FGR(instruction.fr.ft), FLOAT_VALUE_TYPE_DOUBLE),
                    FLOAT_VALUE_TYPE_DOUBLE);
            break;
        case FP_FMT_SINGLE:
            ir_emit_float_check_condition(
                    CONDITION_FLOAT_LE,
                    ir_emit_load_guest_fgr(IR_FGR(instruction.fr.fs), FLOAT_VALUE_TYPE_SINGLE),
                    ir_emit_load_guest_fgr(IR_FGR(instruction.fr.ft), FLOAT_VALUE_TYPE_SINGLE),
                    FLOAT_VALUE_TYPE_SINGLE);
            break;
        default:
            logfatal("mips_cp1_invalid");
    }
}

IR_EMITTER(cp1_c_ngt) {
    //checkcp1;
    logwarn("TODO: check cp1 enabled");
    switch (instruction.fr.fmt) {
        case FP_FMT_DOUBLE:
            ir_emit_float_check_condition(
                    CONDITION_FLOAT_NGT,
                    ir_emit_load_guest_fgr(IR_FGR(instruction.fr.fs), FLOAT_VALUE_TYPE_DOUBLE),
                    ir_emit_load_guest_fgr(IR_FGR(instruction.fr.ft), FLOAT_VALUE_TYPE_DOUBLE),
                    FLOAT_VALUE_TYPE_DOUBLE);
            break;
        case FP_FMT_SINGLE:
            ir_emit_float_check_condition(
                    CONDITION_FLOAT_NGT,
                    ir_emit_load_guest_fgr(IR_FGR(instruction.fr.fs), FLOAT_VALUE_TYPE_SINGLE),
                    ir_emit_load_guest_fgr(IR_FGR(instruction.fr.ft), FLOAT_VALUE_TYPE_SINGLE),
                    FLOAT_VALUE_TYPE_SINGLE);
            break;
        default:
            logfatal("mips_cp1_invalid");
    }
}

IR_EMITTER(cp1_instruction) {
    if (instruction.is_coprocessor_funct) {
        switch (instruction.fr.funct) {
            case COP_FUNCT_ADD:        CALL_IR_EMITTER(cp1_add);
            case COP_FUNCT_TLBR_SUB:   CALL_IR_EMITTER(cp1_sub);
            case COP_FUNCT_TLBWI_MULT: CALL_IR_EMITTER(cp1_mult);
            case COP_FUNCT_DIV:        CALL_IR_EMITTER(cp1_div);
            case COP_FUNCT_TRUNC_L:    CALL_IR_EMITTER(cp1_trunc_l);
            case COP_FUNCT_ROUND_L:    CALL_IR_EMITTER(cp1_round_l);
            case COP_FUNCT_TRUNC_W:    CALL_IR_EMITTER(cp1_trunc_w);
            case COP_FUNCT_FLOOR_W:    CALL_IR_EMITTER(cp1_floor_w);
            case COP_FUNCT_ROUND_W:    CALL_IR_EMITTER(cp1_round_w);
            case COP_FUNCT_CVT_D:      CALL_IR_EMITTER(cp1_cvt_d);
            case COP_FUNCT_CVT_L:      CALL_IR_EMITTER(cp1_cvt_l);
            case COP_FUNCT_CVT_S:      CALL_IR_EMITTER(cp1_cvt_s);
            case COP_FUNCT_CVT_W:      CALL_IR_EMITTER(cp1_cvt_w);
            case COP_FUNCT_SQRT:       CALL_IR_EMITTER(cp1_sqrt);
            case COP_FUNCT_ABS:        CALL_IR_EMITTER(cp1_abs);
            case COP_FUNCT_TLBWR_MOV:  CALL_IR_EMITTER(cp1_mov);
            case COP_FUNCT_NEG:        CALL_IR_EMITTER(cp1_neg);
            case COP_FUNCT_C_F:        CALL_IR_EMITTER(cp1_c_f);
            case COP_FUNCT_C_UN:       CALL_IR_EMITTER(cp1_c_un);
            case COP_FUNCT_C_EQ:       CALL_IR_EMITTER(cp1_c_eq);
            case COP_FUNCT_C_UEQ:      CALL_IR_EMITTER(cp1_c_ueq);
            case COP_FUNCT_C_OLT:      CALL_IR_EMITTER(cp1_c_olt);
            case COP_FUNCT_C_ULT:      CALL_IR_EMITTER(cp1_c_ult);
            case COP_FUNCT_C_OLE:      CALL_IR_EMITTER(cp1_c_ole);
            case COP_FUNCT_C_ULE:      CALL_IR_EMITTER(cp1_c_ule);
            case COP_FUNCT_C_SF:       CALL_IR_EMITTER(cp1_c_sf);
            case COP_FUNCT_C_NGLE:     CALL_IR_EMITTER(cp1_c_ngle);
            case COP_FUNCT_C_SEQ:      CALL_IR_EMITTER(cp1_c_seq);
            case COP_FUNCT_C_NGL:      CALL_IR_EMITTER(cp1_c_ngl);
            case COP_FUNCT_C_LT:       CALL_IR_EMITTER(cp1_c_lt);
            case COP_FUNCT_C_NGE:      CALL_IR_EMITTER(cp1_c_nge);
            case COP_FUNCT_C_LE:       CALL_IR_EMITTER(cp1_c_le);
            case COP_FUNCT_C_NGT:      CALL_IR_EMITTER(cp1_c_ngt);
        }
    } else {
        switch (instruction.r.rs) {
            case COP_CF: CALL_IR_EMITTER(cfc1);
            case COP_MF: CALL_IR_EMITTER(mfc1);
            case COP_DMF: IR_UNIMPLEMENTED(COP_DMF);
            case COP_MT: CALL_IR_EMITTER(mtc1);
            case COP_DMT: IR_UNIMPLEMENTED(COP_DMT);
            case COP_CT: CALL_IR_EMITTER(ctc1);
            case COP_BC:
                switch (instruction.r.rt) {
                    case COP_BC_BCT: CALL_IR_EMITTER(bc1t);
                    case COP_BC_BCF: CALL_IR_EMITTER(bc1f);
                    case COP_BC_BCTL: CALL_IR_EMITTER(bc1tl);
                    case COP_BC_BCFL: CALL_IR_EMITTER(bc1fl);
                    default: {
                        char buf[50];
                        disassemble(0, instruction.raw, buf, 50);
                        logfatal("other/unknown MIPS BC 0x%08X [%s]", instruction.raw, buf);
                    }
                }
                break;
            case COP_DCF:
            case COP_DCT:
                logfatal("Invalid CP1");
            case 0x9 ... 0xF:
                logfatal("Invalid");
        }
    }
}

