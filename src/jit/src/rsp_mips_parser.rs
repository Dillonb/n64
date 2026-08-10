use itertools::izip;
use log::info;

use crate::mips_parser::{
    BranchCondition, MipsCopRsField, MipsFunctField, MipsInstructionBitfield, MipsOpcodeField,
    MipsRegimmRtField, RspCop2VecField, RspLwc2, RspSwc2,
};

// Same as BranchInfo, but without likely
#[derive(Debug)]
pub struct RspBranchInfo {
    pub cond: BranchCondition,
    pub link: bool,
}

#[derive(Debug)]
pub enum RspOpcode {
    BRANCH(RspBranchInfo),

    // RSP main opcodes
    NOP,
    LUI,
    ADDI,
    ANDI,
    LBU,
    LHU,
    LH,
    LW,
    SB,
    SH,
    SW,
    ORI,
    J,
    JAL,
    SLTI,
    SLTIU,
    XORI,
    LB,

    // RSP CP0
    MTC0,
    MFC0,

    // RSP CP2 vector
    VEC_VABS,
    VEC_VADD,
    VEC_VADDC,
    VEC_VAND,
    VEC_VCH,
    VEC_VCL,
    VEC_VCR,
    VEC_VEQ,
    VEC_VGE,
    VEC_VLT,
    VEC_VMACF,
    VEC_VMACQ,
    VEC_VMACU,
    VEC_VMADH,
    VEC_VMADL,
    VEC_VMADM,
    VEC_VMADN,
    VEC_VMOV,
    VEC_VMRG,
    VEC_VMUDH,
    VEC_VMUDL,
    VEC_VMUDM,
    VEC_VMUDN,
    VEC_VMULF,
    VEC_VMULQ,
    VEC_VMULU,
    VEC_VNAND,
    VEC_VNE,
    VEC_VNOP,
    VEC_VNOR,
    VEC_VNXOR,
    VEC_VOR,
    VEC_VRCP,
    VEC_VRCPH_VRSQH,
    VEC_VRCPL,
    VEC_VRNDN,
    VEC_VRNDP,
    VEC_VRSQ,
    VEC_VRSQL,
    VEC_VSAR,
    VEC_VSUB,
    VEC_VSUBC,
    VEC_VXOR,
    VEC_VZERO,

    // RSP CP2 register moves
    CFC2,
    CTC2,
    MFC2,
    MTC2,

    // RSP special
    SLL,
    SRL,
    SRA,
    SRAV,
    SLLV,
    SRLV,
    JR,
    JALR,
    ADD,
    AND,
    SUB,
    OR,
    XOR,
    NOR,
    SLT,
    SLTU,
    BREAK,

    // RSP LWC2
    LBV,
    LDV,
    LFV,
    LHV,
    LLV,
    LPV,
    LQV,
    LRV,
    LSV,
    LTV,
    LUV,

    // RSP SWC2
    SBV,
    SDV,
    SFV,
    SHV,
    SLV,
    SPV,
    SQV,
    SRV,
    SSV,
    STV,
    SUV,
    SWV,
}

impl RspOpcode {
    pub fn is_branch(&self) -> bool {
        match self {
            RspOpcode::BRANCH(_) => true,
            RspOpcode::J => true,
            RspOpcode::JR => true,
            RspOpcode::JAL => true,
            RspOpcode::JALR => true,
            _ => false,
        }
    }
}

pub struct ParsedRspInstruction {
    pub addr: u16,
    pub instr: MipsInstructionBitfield,
    pub op: RspOpcode,
}

fn opcode_of_rsp_instruction(instr: &MipsInstructionBitfield) -> RspOpcode {
    match instr.op() {
        _ if instr.raw() == 0 => RspOpcode::NOP,
        MipsOpcodeField::LUI => RspOpcode::LUI,
        MipsOpcodeField::ADDIU => RspOpcode::ADDI,
        MipsOpcodeField::ADDI => RspOpcode::ADDI,
        MipsOpcodeField::ANDI => RspOpcode::ANDI,
        MipsOpcodeField::LBU => RspOpcode::LBU,
        MipsOpcodeField::LHU => RspOpcode::LHU,
        MipsOpcodeField::LH => RspOpcode::LH,
        MipsOpcodeField::LW => RspOpcode::LW,
        MipsOpcodeField::LWU => RspOpcode::LW,
        MipsOpcodeField::BEQ => RspOpcode::BRANCH(RspBranchInfo {
            cond: BranchCondition::EQ,
            link: false,
        }),
        MipsOpcodeField::BGTZ => RspOpcode::BRANCH(RspBranchInfo {
            cond: BranchCondition::GTZ,
            link: false,
        }),
        MipsOpcodeField::BLEZ => RspOpcode::BRANCH(RspBranchInfo {
            cond: BranchCondition::LEZ,
            link: false,
        }),
        MipsOpcodeField::BNE => RspOpcode::BRANCH(RspBranchInfo {
            cond: BranchCondition::NE,
            link: false,
        }),
        MipsOpcodeField::SB => RspOpcode::SB,
        MipsOpcodeField::SH => RspOpcode::SH,
        MipsOpcodeField::SW => RspOpcode::SW,
        MipsOpcodeField::ORI => RspOpcode::ORI,
        MipsOpcodeField::J => RspOpcode::J,
        MipsOpcodeField::JAL => RspOpcode::JAL,
        MipsOpcodeField::SLTI => RspOpcode::SLTI,
        MipsOpcodeField::SLTIU => RspOpcode::SLTIU,
        MipsOpcodeField::XORI => RspOpcode::XORI,
        MipsOpcodeField::LB => RspOpcode::LB,

        MipsOpcodeField::CP0 => {
            if instr.is_coprocessor_funct() {
                panic!("Invalid RSP CP0 instruction 0x{:08X}", instr.raw())
            } else {
                match instr.cop_op() {
                    MipsCopRsField::MT => RspOpcode::MTC0,
                    MipsCopRsField::MF => RspOpcode::MFC0,
                    rs => panic!(
                        "Invalid RSP CP0 instruction 0x{:08X} with rs {:?}",
                        instr.raw(),
                        rs
                    ),
                }
            }
        }
        MipsOpcodeField::CP1 => panic!("RSP CP1"), // Invalid
        MipsOpcodeField::CP2 => {
            if instr.is_cp2_vec() {
                match instr.rsp_cop2_vec() {
                    RspCop2VecField::VABS => RspOpcode::VEC_VABS,
                    RspCop2VecField::VADD => RspOpcode::VEC_VADD,
                    RspCop2VecField::VADDC => RspOpcode::VEC_VADDC,
                    RspCop2VecField::VAND => RspOpcode::VEC_VAND,
                    RspCop2VecField::VCH => RspOpcode::VEC_VCH,
                    RspCop2VecField::VCL => RspOpcode::VEC_VCL,
                    RspCop2VecField::VCR => RspOpcode::VEC_VCR,
                    RspCop2VecField::VEQ => RspOpcode::VEC_VEQ,
                    RspCop2VecField::VGE => RspOpcode::VEC_VGE,
                    RspCop2VecField::VLT => RspOpcode::VEC_VLT,
                    RspCop2VecField::VMACF => RspOpcode::VEC_VMACF,
                    RspCop2VecField::VMACQ => RspOpcode::VEC_VMACQ,
                    RspCop2VecField::VMACU => RspOpcode::VEC_VMACU,
                    RspCop2VecField::VMADH => RspOpcode::VEC_VMADH,
                    RspCop2VecField::VMADL => RspOpcode::VEC_VMADL,
                    RspCop2VecField::VMADM => RspOpcode::VEC_VMADM,
                    RspCop2VecField::VMADN => RspOpcode::VEC_VMADN,
                    RspCop2VecField::VMOV => RspOpcode::VEC_VMOV,
                    RspCop2VecField::VMRG => RspOpcode::VEC_VMRG,
                    RspCop2VecField::VMUDH => RspOpcode::VEC_VMUDH,
                    RspCop2VecField::VMUDL => RspOpcode::VEC_VMUDL,
                    RspCop2VecField::VMUDM => RspOpcode::VEC_VMUDM,
                    RspCop2VecField::VMUDN => RspOpcode::VEC_VMUDN,
                    RspCop2VecField::VMULF => RspOpcode::VEC_VMULF,
                    RspCop2VecField::VMULQ => RspOpcode::VEC_VMULQ,
                    RspCop2VecField::VMULU => RspOpcode::VEC_VMULU,
                    RspCop2VecField::VNAND => RspOpcode::VEC_VNAND,
                    RspCop2VecField::VNE => RspOpcode::VEC_VNE,
                    RspCop2VecField::VNOP => RspOpcode::VEC_VNOP,
                    RspCop2VecField::VNOR => RspOpcode::VEC_VNOR,
                    RspCop2VecField::VNXOR => RspOpcode::VEC_VNXOR,
                    RspCop2VecField::VOR => RspOpcode::VEC_VOR,
                    RspCop2VecField::VRCP => RspOpcode::VEC_VRCP,
                    RspCop2VecField::VRCPH => RspOpcode::VEC_VRCPH_VRSQH,
                    RspCop2VecField::VRCPL => RspOpcode::VEC_VRCPL,
                    RspCop2VecField::VRNDN => RspOpcode::VEC_VRNDN,
                    RspCop2VecField::VRNDP => RspOpcode::VEC_VRNDP,
                    RspCop2VecField::VRSQ => RspOpcode::VEC_VRSQ,
                    RspCop2VecField::VRSQH => RspOpcode::VEC_VRCPH_VRSQH,
                    RspCop2VecField::VRSQL => RspOpcode::VEC_VRSQL,
                    RspCop2VecField::VSAR => RspOpcode::VEC_VSAR,
                    RspCop2VecField::VSUB => RspOpcode::VEC_VSUB,
                    RspCop2VecField::VSUBC => RspOpcode::VEC_VSUBC,
                    RspCop2VecField::VXOR => RspOpcode::VEC_VXOR,
                    RspCop2VecField::VSUT
                    | RspCop2VecField::VADDB
                    | RspCop2VecField::VSUBB
                    | RspCop2VecField::VACCB
                    | RspCop2VecField::VSUCB
                    | RspCop2VecField::VSAD
                    | RspCop2VecField::VSAC
                    | RspCop2VecField::VSUM
                    | RspCop2VecField::X1E
                    | RspCop2VecField::X1F
                    | RspCop2VecField::X2E
                    | RspCop2VecField::X2F
                    | RspCop2VecField::VEXTT
                    | RspCop2VecField::VEXTQ
                    | RspCop2VecField::VEXTN
                    | RspCop2VecField::X3B
                    | RspCop2VecField::VINST
                    | RspCop2VecField::VINSQ
                    | RspCop2VecField::VINSN => RspOpcode::VEC_VZERO,
                    RspCop2VecField::VNULL => RspOpcode::VEC_VNOP,
                }
            } else {
                match instr.cop_op() {
                    MipsCopRsField::MF => RspOpcode::MFC2,
                    MipsCopRsField::CF => RspOpcode::CFC2,
                    MipsCopRsField::MT => RspOpcode::MTC2,
                    MipsCopRsField::CT => RspOpcode::CTC2,
                    _ => todo!(),
                }
            }
        }

        MipsOpcodeField::SPCL => match instr.funct() {
            MipsFunctField::SLL => RspOpcode::SLL,
            MipsFunctField::SRL => RspOpcode::SRL,
            MipsFunctField::SRA => RspOpcode::SRA,
            MipsFunctField::SRAV => RspOpcode::SRAV,
            MipsFunctField::SLLV => RspOpcode::SLLV,
            MipsFunctField::SRLV => RspOpcode::SRLV,
            MipsFunctField::JR => RspOpcode::JR,
            MipsFunctField::JALR => RspOpcode::JALR,
            MipsFunctField::ADD => RspOpcode::ADD,
            MipsFunctField::ADDU => RspOpcode::ADD,
            MipsFunctField::AND => RspOpcode::AND,
            MipsFunctField::SUB => RspOpcode::SUB,
            MipsFunctField::SUBU => RspOpcode::SUB,
            MipsFunctField::OR => RspOpcode::OR,
            MipsFunctField::XOR => RspOpcode::XOR,
            MipsFunctField::NOR => RspOpcode::NOR,
            MipsFunctField::SLT => RspOpcode::SLT,
            MipsFunctField::SLTU => RspOpcode::SLTU,
            MipsFunctField::BREAK => RspOpcode::BREAK,
            _ => panic!("Unsupported RSP funct field: {:?}", instr.funct()),
        },
        MipsOpcodeField::REGIMM => match instr.rt_op() {
            MipsRegimmRtField::BLTZ => RspOpcode::BRANCH(RspBranchInfo {
                cond: BranchCondition::LTZ,
                link: false,
            }),
            MipsRegimmRtField::BLTZAL => RspOpcode::BRANCH(RspBranchInfo {
                cond: BranchCondition::LTZ,
                link: true,
            }),
            MipsRegimmRtField::BGEZ => RspOpcode::BRANCH(RspBranchInfo {
                cond: BranchCondition::GEZ,
                link: false,
            }),
            MipsRegimmRtField::BGEZAL => RspOpcode::BRANCH(RspBranchInfo {
                cond: BranchCondition::GEZ,
                link: true,
            }),
            _ => panic!("Unsupported RSP regimm field: {:?}", instr.rt_op()),
        },
        MipsOpcodeField::LWC2 => match instr.rsp_lwc2() {
            RspLwc2::LBV => RspOpcode::LBV,
            RspLwc2::LDV => RspOpcode::LDV,
            RspLwc2::LFV => RspOpcode::LFV,
            RspLwc2::LHV => RspOpcode::LHV,
            RspLwc2::LLV => RspOpcode::LLV,
            RspLwc2::LPV => RspOpcode::LPV,
            RspLwc2::LQV => RspOpcode::LQV,
            RspLwc2::LRV => RspOpcode::LRV,
            RspLwc2::LSV => RspOpcode::LSV,
            RspLwc2::LTV => RspOpcode::LTV,
            RspLwc2::LUV => RspOpcode::LUV,
            RspLwc2::LWV => RspOpcode::NOP, // Only exists for SWC2: SWV
        },
        MipsOpcodeField::SWC2 => match instr.rsp_swc2() {
            RspSwc2::SBV => RspOpcode::SBV,
            RspSwc2::SDV => RspOpcode::SDV,
            RspSwc2::SFV => RspOpcode::SFV,
            RspSwc2::SHV => RspOpcode::SHV,
            RspSwc2::SLV => RspOpcode::SLV,
            RspSwc2::SPV => RspOpcode::SPV,
            RspSwc2::SQV => RspOpcode::SQV,
            RspSwc2::SRV => RspOpcode::SRV,
            RspSwc2::SSV => RspOpcode::SSV,
            RspSwc2::STV => RspOpcode::STV,
            RspSwc2::SUV => RspOpcode::SUV,
            RspSwc2::SWV => RspOpcode::SWV,
        },

        _ => panic!("Unsupported RSP opcode field: {:?}", instr.op()),
    }
}

pub fn parse_rsp(code: &[u32], address: u16) -> Vec<ParsedRspInstruction> {
    let instructions = code.iter().map(|word| MipsInstructionBitfield(*word));
    let parsed = izip!(
        instructions,
        (address..).step_by(4).map(|addr| addr & 0xFFF),
    )
    .map(|(instr, addr)| ParsedRspInstruction {
        addr,
        instr,
        op: opcode_of_rsp_instruction(&instr),
    })
    .collect::<Vec<_>>();

    let code_len = code.len();
    info!("Compiling {code_len} instructions at RSP address 0x{address:03X}");

    return parsed;
}
