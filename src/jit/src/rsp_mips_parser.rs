use itertools::izip;
use log::info;

use crate::mips_parser::{
    BranchCondition, MipsFunctField, MipsInstructionBitfield, MipsOpcodeField,
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
    BEQ,
    BGTZ,
    BLEZ,
    BNE,
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

    // RSP regimm
    BLTZ,
    BGEZ,
    BGEZAL,
    BLTZAL,

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
        MipsOpcodeField::BEQ => RspOpcode::BEQ,
        MipsOpcodeField::BGTZ => RspOpcode::BGTZ,
        MipsOpcodeField::BLEZ => RspOpcode::BLEZ,
        MipsOpcodeField::BNE => RspOpcode::BNE,
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

        MipsOpcodeField::CP0 => todo!("RSP CP0"),
        MipsOpcodeField::CP1 => todo!("RSP CP1"),
        MipsOpcodeField::CP2 => todo!("RSP CP2"),
        MipsOpcodeField::SPCL => match instr.funct() {
            // case FUNCT_SLL:    return rsp_spc_sll;
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
        MipsOpcodeField::REGIMM => todo!("RSP REGIMM"),
        MipsOpcodeField::LWC2 => todo!("RSP LWC2"),
        MipsOpcodeField::SWC2 => todo!("RSP SWC2"),

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
