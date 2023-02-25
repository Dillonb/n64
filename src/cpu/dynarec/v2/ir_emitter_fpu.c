#include <r4300i.h>
#include <disassemble.h>
#include "ir_emitter_fpu.h"
#include "ir_context.h"

IR_EMITTER(cfc1) {
    //checkcp1; // TODO: check cp1 is enabled
    logwarn("TODO: Check CP1 is enabled");
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
    ir_instruction_t* value = ir_emit_load_guest_reg(instruction.r.rt);
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

IR_EMITTER(bc1tl) {
    logwarn("BC1TL unimplemented");
    ir_instruction_t* cond = ir_emit_load_guest_reg(0); // Always false
    ir_emit_conditional_branch_likely(cond, instruction.i.immediate, virtual_address, index);
}

IR_EMITTER(mfc1) {
    logfatal("TODO: check CP1 is enabled, MFC1 unimplemented");
    ir_emit_set_constant_u16(0, instruction.r.rt);
}

IR_EMITTER(mtc1) {
    logwarn("TODO: check CP1 is enabled. Also, rewrite this function when a real FPU jit exists");
    //checkcp1;
    ir_instruction_t* value = ir_emit_load_guest_reg(IR_GPR(instruction.r.rt));
    ir_emit_mov_reg_type(value, REGISTER_TYPE_FGR, VALUE_TYPE_U32, IR_FGR(instruction.r.rd));
}
}

IR_EMITTER(cp1_instruction) {
    if (instruction.is_coprocessor_funct) {
        switch (instruction.fr.funct) {
            case COP_FUNCT_ADD: IR_UNIMPLEMENTED(COP_FUNCT_ADD);
            case COP_FUNCT_TLBR_SUB: IR_UNIMPLEMENTED(COP_FUNCT_TLBR_SUB);
            case COP_FUNCT_TLBWI_MULT: IR_UNIMPLEMENTED(COP_FUNCT_TLBWI_MULT);
            case COP_FUNCT_DIV: IR_UNIMPLEMENTED(COP_FUNCT_DIV);
            case COP_FUNCT_TRUNC_L: IR_UNIMPLEMENTED(COP_FUNCT_TRUNC_L);
            case COP_FUNCT_ROUND_L: IR_UNIMPLEMENTED(COP_FUNCT_ROUND_L);
            case COP_FUNCT_TRUNC_W: IR_UNIMPLEMENTED(COP_FUNCT_TRUNC_W);
            case COP_FUNCT_FLOOR_W: IR_UNIMPLEMENTED(COP_FUNCT_FLOOR_W);
            case COP_FUNCT_ROUND_W: IR_UNIMPLEMENTED(COP_FUNCT_ROUND_W);
            case COP_FUNCT_CVT_D: IR_UNIMPLEMENTED(COP_FUNCT_CVT_D);
            case COP_FUNCT_CVT_L: IR_UNIMPLEMENTED(COP_FUNCT_CVT_L);
            case COP_FUNCT_CVT_S: IR_UNIMPLEMENTED(COP_FUNCT_CVT_S);
            case COP_FUNCT_CVT_W: IR_UNIMPLEMENTED(COP_FUNCT_CVT_W);
            case COP_FUNCT_SQRT: IR_UNIMPLEMENTED(COP_FUNCT_SQRT);
            case COP_FUNCT_ABS: IR_UNIMPLEMENTED(COP_FUNCT_ABS);
            case COP_FUNCT_TLBWR_MOV: IR_UNIMPLEMENTED(COP_FUNCT_TLBWR_MOV);
            case COP_FUNCT_NEG: IR_UNIMPLEMENTED(COP_FUNCT_NEG);
            case COP_FUNCT_C_F: IR_UNIMPLEMENTED(COP_FUNCT_C_F);
            case COP_FUNCT_C_UN: IR_UNIMPLEMENTED(COP_FUNCT_C_UN);
            case COP_FUNCT_C_EQ: IR_UNIMPLEMENTED(COP_FUNCT_C_EQ);
            case COP_FUNCT_C_UEQ: IR_UNIMPLEMENTED(COP_FUNCT_C_UEQ);
            case COP_FUNCT_C_OLT: IR_UNIMPLEMENTED(COP_FUNCT_C_OLT);
            case COP_FUNCT_C_ULT: IR_UNIMPLEMENTED(COP_FUNCT_C_ULT);
            case COP_FUNCT_C_OLE: IR_UNIMPLEMENTED(COP_FUNCT_C_OLE);
            case COP_FUNCT_C_ULE: IR_UNIMPLEMENTED(COP_FUNCT_C_ULE);
            case COP_FUNCT_C_SF: IR_UNIMPLEMENTED(COP_FUNCT_C_SF);
            case COP_FUNCT_C_NGLE: IR_UNIMPLEMENTED(COP_FUNCT_C_NGLE);
            case COP_FUNCT_C_SEQ: IR_UNIMPLEMENTED(COP_FUNCT_C_SEQ);
            case COP_FUNCT_C_NGL: IR_UNIMPLEMENTED(COP_FUNCT_C_NGL);
            case COP_FUNCT_C_LT: IR_UNIMPLEMENTED(COP_FUNCT_C_LT);
            case COP_FUNCT_C_NGE: IR_UNIMPLEMENTED(COP_FUNCT_C_NGE);
            case COP_FUNCT_C_LE: IR_UNIMPLEMENTED(COP_FUNCT_C_LE);
            case COP_FUNCT_C_NGT: IR_UNIMPLEMENTED(COP_FUNCT_C_NGT);
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
                    case COP_BC_BCT: IR_UNIMPLEMENTED(COP_BC_BCT);
                    case COP_BC_BCF: IR_UNIMPLEMENTED(COP_BC_BCF);
                    case COP_BC_BCTL: CALL_IR_EMITTER(bc1tl);
                    case COP_BC_BCFL: IR_UNIMPLEMENTED(COP_BC_BCFL);
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

