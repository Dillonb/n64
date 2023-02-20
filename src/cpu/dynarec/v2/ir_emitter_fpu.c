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
    logwarn("TODO: check CP1 is enabled, MFC1 unimplemented");
    ir_emit_set_constant_u16(0, instruction.r.rt);
}

IR_EMITTER(cp1_instruction) {
        // This function uses a series of two switch statements.
        // If the instruction doesn't use the RS field for the opcode, then control will fall through to the next
        // switch, and check the FUNCT. It may be worth profiling and seeing if it's faster to check FUNCT first at some point
        switch (instruction.r.rs) {
            case COP_CF: CALL_IR_EMITTER(cfc1);
            case COP_MF: CALL_IR_EMITTER(mfc1);
            case COP_DMF: IR_UNIMPLEMENTED(COP_DMF);
            case COP_MT:
                logwarn("Ignoring MTC1!");
            break;
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
        }
        //IR_UNIMPLEMENTED(SomeFPUInstruction);
        logwarn("Ignoring FPU instruction!");
        return;
}

