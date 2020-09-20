#ifndef N64_MIPS_INSTRUCTION_DECODE_H
#define N64_MIPS_INSTRUCTION_DECODE_H

#include <stdbool.h>
#include <util.h>

typedef union mips_instruction {
    word raw;

    struct {
        unsigned:26;
        bool op5:1;
        bool op4:1;
        bool op3:1;
        bool op2:1;
        bool op1:1;
        bool op0:1;
    };

    struct {
        unsigned:26;
        unsigned op:6;
    };

    struct {
        unsigned immediate:16;
        unsigned rt:5;
        unsigned rs:5;
        unsigned op:6;
    } i;

    struct {
        unsigned offset:16;
        unsigned ft:5;
        unsigned base:5;
        unsigned op:6;
    } fi;

    struct {
        unsigned target:26;
        unsigned op:6;
    } j;

    struct {
        unsigned funct:6;
        unsigned sa:5;
        unsigned rd:5;
        unsigned rt:5;
        unsigned rs:5;
        unsigned op:6;
    } r;

    struct {
        unsigned funct:6;
        unsigned fd:5;
        unsigned fs:5;
        unsigned ft:5;
        unsigned fmt:5;
        unsigned op:6;
    } fr;

    struct {
        unsigned offset:7;
        unsigned element:4;
        unsigned funct:5;
        unsigned vt:5;
        unsigned base:5;
        unsigned op:6;
    } v;

    struct {
        unsigned funct:6;
        unsigned vd:5;
        unsigned vs:5;
        unsigned vt:5;
        unsigned e:4;
        bool is_vec:1;
        unsigned op:6;
    } cp2_vec;

    struct {
        unsigned:7;
        unsigned e:4;
        unsigned rd:5;
        unsigned rt:5;
        unsigned funct:5;
        unsigned op:6;
    } cp2_regmove;

    struct {
        unsigned funct5:1;
        unsigned funct4:1;
        unsigned funct3:1;
        unsigned funct2:1;
        unsigned funct1:1;
        unsigned funct0:1;
        unsigned:26;
    };

    struct {
        unsigned:16;
        unsigned rt4:1;
        unsigned rt3:1;
        unsigned rt2:1;
        unsigned rt1:1;
        unsigned rt0:1;
        unsigned:11;
    };

    struct {
        unsigned:21;
        unsigned rs4:1;
        unsigned rs3:1;
        unsigned rs2:1;
        unsigned rs1:1;
        unsigned rs0:1;
        unsigned:6;
    };

    struct {
        unsigned last11:11;
        unsigned:21;
    };

} mips_instruction_t;

typedef enum mips_instruction_type {
    MIPS_UNKNOWN,
    MIPS_LD,
    MIPS_LUI,
    MIPS_ADDI,
    MIPS_DADDI,
    MIPS_ADDIU,
    MIPS_ANDI,
    MIPS_LBU,
    MIPS_LHU,
    MIPS_LH,
    MIPS_LW,
    MIPS_LWU,
    MIPS_BLEZ,
    MIPS_BLEZL,
    MIPS_BNE,
    MIPS_BNEL,
    MIPS_CACHE,
    MIPS_BEQ,
    MIPS_BEQL,
    MIPS_BGTZ,
    MIPS_BGTZL,
    MIPS_NOP,
    MIPS_SB,
    MIPS_SH,
    MIPS_SW,
    MIPS_SD,
    MIPS_ORI,
    MIPS_J,
    MIPS_JAL,
    MIPS_SLTI,
    MIPS_SLTIU,
    MIPS_XORI,
    MIPS_DADDIU,
    MIPS_LB,
    MIPS_LDC1,
    MIPS_SDC1,
    MIPS_LWC1,
    MIPS_SWC1,
    MIPS_LWL,
    MIPS_LWR,
    MIPS_SWL,
    MIPS_SWR,
    MIPS_LDL,
    MIPS_LDR,
    MIPS_SDL,
    MIPS_SDR,

    // Coprocessor
    MIPS_CP_MFC0,
    MIPS_CP_MTC0,
    MIPS_CP_MFC1,
    MIPS_CP_MTC1,

    MIPS_ERET,
    MIPS_TLBWI,
    MIPS_TLBP,
    MIPS_TLBR,

    MIPS_CP_CTC1,
    MIPS_CP_CFC1,

    MIPS_CP_BC1F,
    MIPS_CP_BC1T,
    MIPS_CP_BC1FL,
    MIPS_CP_BC1TL,

    MIPS_CP_ADD_D,
    MIPS_CP_ADD_S,
    MIPS_CP_SUB_D,
    MIPS_CP_SUB_S,
    MIPS_CP_MUL_D,
    MIPS_CP_MUL_S,
    MIPS_CP_DIV_D,
    MIPS_CP_DIV_S,
    MIPS_CP_TRUNC_L_D,
    MIPS_CP_TRUNC_L_S,
    MIPS_CP_TRUNC_W_D,
    MIPS_CP_TRUNC_W_S,

    MIPS_CP_CVT_D_S,
    MIPS_CP_CVT_D_W,
    MIPS_CP_CVT_D_L,
    MIPS_CP_CVT_L_S,
    MIPS_CP_CVT_L_D,
    MIPS_CP_CVT_S_D,
    MIPS_CP_CVT_S_W,
    MIPS_CP_CVT_S_L,
    MIPS_CP_CVT_W_S,
    MIPS_CP_CVT_W_D,
    MIPS_CP_SQRT_S,
    MIPS_CP_SQRT_D,
    MIPS_CP_C_F_S,
    MIPS_CP_C_UN_S,
    MIPS_CP_C_EQ_S,
    MIPS_CP_C_UEQ_S,
    MIPS_CP_C_OLT_S,
    MIPS_CP_C_ULT_S,
    MIPS_CP_C_OLE_S,
    MIPS_CP_C_ULE_S,
    MIPS_CP_C_SF_S,
    MIPS_CP_C_NGLE_S,
    MIPS_CP_C_SEQ_S,
    MIPS_CP_C_NGL_S,
    MIPS_CP_C_LT_S,
    MIPS_CP_C_NGE_S,
    MIPS_CP_C_LE_S,
    MIPS_CP_C_NGT_S,
    MIPS_CP_C_F_D,
    MIPS_CP_C_UN_D,
    MIPS_CP_C_EQ_D,
    MIPS_CP_C_UEQ_D,
    MIPS_CP_C_OLT_D,
    MIPS_CP_C_ULT_D,
    MIPS_CP_C_OLE_D,
    MIPS_CP_C_ULE_D,
    MIPS_CP_C_SF_D,
    MIPS_CP_C_NGLE_D,
    MIPS_CP_C_SEQ_D,
    MIPS_CP_C_NGL_D,
    MIPS_CP_C_LT_D,
    MIPS_CP_C_NGE_D,
    MIPS_CP_C_LE_D,
    MIPS_CP_C_NGT_D,
    MIPS_CP_MOV_D,
    MIPS_CP_MOV_S,
    MIPS_CP_NEG_D,
    MIPS_CP_NEG_S,

    // Special
    MIPS_SPC_SLL,
    MIPS_SPC_SRL,
    MIPS_SPC_SRA,
    MIPS_SPC_SRAV,
    MIPS_SPC_SLLV,
    MIPS_SPC_SRLV,
    MIPS_SPC_JR,
    MIPS_SPC_JALR,
    MIPS_SPC_MFHI,
    MIPS_SPC_MTHI,
    MIPS_SPC_MFLO,
    MIPS_SPC_MTLO,
    MIPS_SPC_DSLLV,
    MIPS_SPC_MULT,
    MIPS_SPC_MULTU,
    MIPS_SPC_DIV,
    MIPS_SPC_DIVU,
    MIPS_SPC_DMULTU,
    MIPS_SPC_DDIV,
    MIPS_SPC_DDIVU,
    MIPS_SPC_ADD,
    MIPS_SPC_ADDU,
    MIPS_SPC_AND,
    MIPS_SPC_NOR,
    MIPS_SPC_SUB,
    MIPS_SPC_SUBU,
    MIPS_SPC_OR,
    MIPS_SPC_XOR,
    MIPS_SPC_SLT,
    MIPS_SPC_SLTU,
    MIPS_SPC_DADD,
    MIPS_SPC_DSLL,
    MIPS_SPC_DSLL32,
    MIPS_SPC_DSRA32,

    MIPS_SPC_BREAK,

    // REGIMM
    MIPS_RI_BLTZ,
    MIPS_RI_BLTZL,
    MIPS_RI_BGEZ,
    MIPS_RI_BGEZL,
    MIPS_RI_BGEZAL,

    // RSP

    RSP_LWC2_LBV,
    RSP_LWC2_LDV,
    RSP_LWC2_LFV,
    RSP_LWC2_LHV,
    RSP_LWC2_LLV,
    RSP_LWC2_LPV,
    RSP_LWC2_LQV,
    RSP_LWC2_LRV,
    RSP_LWC2_LSV,
    RSP_LWC2_LTV,
    RSP_LWC2_LUV,

    RSP_SWC2_SBV,
    RSP_SWC2_SDV,
    RSP_SWC2_SFV,
    RSP_SWC2_SHV,
    RSP_SWC2_SLV,
    RSP_SWC2_SPV,
    RSP_SWC2_SQV,
    RSP_SWC2_SRV,
    RSP_SWC2_SSV,
    RSP_SWC2_STV,
    RSP_SWC2_SUV,

    RSP_CFC2,
    RSP_CTC2,
    RSP_MFC2,
    RSP_MTC2,

    RSP_VEC_VABS,
    RSP_VEC_VADD,
    RSP_VEC_VADDC,
    RSP_VEC_VAND,
    RSP_VEC_VCH,
    RSP_VEC_VCL,
    RSP_VEC_VCR,
    RSP_VEC_VEQ,
    RSP_VEC_VGE,
    RSP_VEC_VLT,
    RSP_VEC_VMACF,
    RSP_VEC_VMACQ,
    RSP_VEC_VMACU,
    RSP_VEC_VMADH,
    RSP_VEC_VMADL,
    RSP_VEC_VMADM,
    RSP_VEC_VMADN,
    RSP_VEC_VMOV,
    RSP_VEC_VMRG,
    RSP_VEC_VMUDH,
    RSP_VEC_VMUDL,
    RSP_VEC_VMUDM,
    RSP_VEC_VMUDN,
    RSP_VEC_VMULF,
    RSP_VEC_VMULQ,
    RSP_VEC_VMULU,
    RSP_VEC_VNAND,
    RSP_VEC_VNE,
    RSP_VEC_VNOP,
    RSP_VEC_VNOR,
    RSP_VEC_VNXOR,
    RSP_VEC_VOR,
    RSP_VEC_VRCP,
    RSP_VEC_VRCPH,
    RSP_VEC_VRCPL,
    RSP_VEC_VRNDN,
    RSP_VEC_VRNDP,
    RSP_VEC_VRSQ,
    RSP_VEC_VRSQH,
    RSP_VEC_VRSQL,
    RSP_VEC_VSAR,
    RSP_VEC_VSUB,
    RSP_VEC_VSUBC,
    RSP_VEC_VXOR,
} mips_instruction_type_t;


#endif //N64_MIPS_INSTRUCTION_DECODE_H
