#include "rsp_ir_emitter.h"
#include <r4300i.h>
#include <rsp.h>

IR_RSP_EMITTER(ori) {
    logfatal("ori");
}

IR_RSP_EMITTER(instruction) {
    if (unlikely(instruction.raw == 0)) {
        return; // do nothing for NOP
    }

#ifdef LOG_ENABLED
        static char buf[50];
        if (n64_log_verbosity >= LOG_VERBOSITY_DEBUG) {
            disassemble(pc, instr.raw, buf, 50);
            logdebug("RSP [0x%08X]=0x%08X %s", pc, instr.raw, buf);
        }
#endif
        switch (instruction.op) {
            case OPC_LUI:   IR_RSP_UNIMPLEMENTED(OPC_LUI);
            case OPC_ADDIU: IR_RSP_UNIMPLEMENTED(OPC_ADDIU);
            case OPC_ADDI:  IR_RSP_UNIMPLEMENTED(OPC_ADDI);
            case OPC_ANDI:  IR_RSP_UNIMPLEMENTED(OPC_ANDI);
            case OPC_LBU:   IR_RSP_UNIMPLEMENTED(OPC_LBU);
            case OPC_LHU:   IR_RSP_UNIMPLEMENTED(OPC_LHU);
            case OPC_LH:    IR_RSP_UNIMPLEMENTED(OPC_LH);
            case OPC_LW:    IR_RSP_UNIMPLEMENTED(OPC_LW);
            case OPC_LWU:   IR_RSP_UNIMPLEMENTED(OPC_LWU);
            case OPC_BEQ:   IR_RSP_UNIMPLEMENTED(OPC_BEQ);
            //case OPC_BEQL:  return rsp_beql;
            case OPC_BGTZ:  IR_RSP_UNIMPLEMENTED(OPC_BGTZ);
            case OPC_BLEZ:  IR_RSP_UNIMPLEMENTED(OPC_BLEZ);
            case OPC_BNE:   IR_RSP_UNIMPLEMENTED(OPC_BNE);
            //case OPC_BNEL:  return rsp_bnel;
            //case OPC_CACHE: return rsp_cache;
            case OPC_SB:    IR_RSP_UNIMPLEMENTED(OPC_SB);
            case OPC_SH:    IR_RSP_UNIMPLEMENTED(OPC_SH);
            case OPC_SW:    IR_RSP_UNIMPLEMENTED(OPC_SW);
            case OPC_ORI:   CALL_IR_RSP_EMITTER(ori);
            case OPC_J:     IR_RSP_UNIMPLEMENTED(OPC_J);
            case OPC_JAL:   IR_RSP_UNIMPLEMENTED(OPC_JAL);
            case OPC_SLTI:  IR_RSP_UNIMPLEMENTED(OPC_SLTI);
            case OPC_SLTIU: IR_RSP_UNIMPLEMENTED(OPC_SLTIU);
            case OPC_XORI:  IR_RSP_UNIMPLEMENTED(OPC_XORI);
            case OPC_LB:    IR_RSP_UNIMPLEMENTED(OPC_LB);
            //case OPC_LWL:   return rsp_lwl;
            //case OPC_LWR:   return rsp_lwr;
            //case OPC_SWL:   return rsp_swl;
            //case OPC_SWR:   return rsp_swr;

            case OPC_CP0:      IR_RSP_UNIMPLEMENTED(OPC_CP0); //return rsp_cp0_decode(pc, instruction);
            case OPC_CP1:      IR_RSP_UNIMPLEMENTED(OPC_CP1);     //return rsp_cp1_decode(pc, instr);
            case OPC_CP2:      IR_RSP_UNIMPLEMENTED(OPC_CP2); //return rsp_cp2_decode(pc, instruction);
            case OPC_SPCL:     IR_RSP_UNIMPLEMENTED(OPC_SPCL); //return rsp_special_decode(pc, instruction);
            case OPC_REGIMM:   IR_RSP_UNIMPLEMENTED(OPC_REGIMM); //return rsp_regimm_decode(pc, instruction);
            case RSP_OPC_LWC2: IR_RSP_UNIMPLEMENTED(RSP_OPC_LWC2); //return rsp_lwc2_decode(pc, instruction);
            case RSP_OPC_SWC2: IR_RSP_UNIMPLEMENTED(RSP_OPC_SWC2); //return rsp_swc2_decode(pc, instruction);

            default:
#ifdef LOG_ENABLED
                if (n64_log_verbosity < LOG_VERBOSITY_DEBUG) {
                    disassemble(pc, instruction.raw, buf, 50);
                }
                logfatal("[RSP] Failed to decode instruction 0x%08X opcode %d%d%d%d%d%d [%s]",
                         instruction.raw, instruction.op0, instruction.op1, instruction.op2, instruction.op3, instruction.op4, instruction.op5, buf);
#else
                logfatal("[RSP] Failed to decode instruction 0x%08X opcode %d%d%d%d%d%d [UNKNOWN]",
                         instruction.raw, instruction.op0, instruction.op1, instruction.op2, instruction.op3, instruction.op4, instruction.op5);
#endif
        }
}