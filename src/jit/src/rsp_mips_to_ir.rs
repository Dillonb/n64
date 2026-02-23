use dgbir::ir::{DataType, IRContext, IRFunction};

use crate::{
    rsp_mips_parser::{ParsedRspInstruction, RspOpcode},
    rsp_t,
};

pub struct RspMipsToIrContext {}

impl RspMipsToIrContext {
    pub fn default() -> Self {
        Self {}
    }
}

pub fn rsp_to_ir_ctx(
    ctx: RspMipsToIrContext,
    parsed: Vec<ParsedRspInstruction>,
    rsp: &rsp_t,
) -> IRFunction {
    let context = IRContext::new();
    let func = IRFunction::new(context);
    let mut block = func.new_block(vec![DataType::Ptr]);
    let rsp_address = block.input(0);

    for ParsedRspInstruction { addr, instr, op } in parsed {
        match op {
            RspOpcode::BRANCH(_) => todo!("RSP branch"),
            RspOpcode::NOP => todo!("RSP NOP"),
            RspOpcode::LUI => todo!("RSP LUI"),
            RspOpcode::ADDI => todo!("RSP ADDI"),
            RspOpcode::ANDI => todo!("RSP ANDI"),
            RspOpcode::LBU => todo!("RSP LBU"),
            RspOpcode::LHU => todo!("RSP LHU"),
            RspOpcode::LH => todo!("RSP LH"),
            RspOpcode::LW => todo!("RSP LW"),
            RspOpcode::BEQ => todo!("RSP BEQ"),
            RspOpcode::BGTZ => todo!("RSP BGTZ"),
            RspOpcode::BLEZ => todo!("RSP BLEZ"),
            RspOpcode::BNE => todo!("RSP BNE"),
            RspOpcode::SB => todo!("RSP SB"),
            RspOpcode::SH => todo!("RSP SH"),
            RspOpcode::SW => todo!("RSP SW"),
            RspOpcode::ORI => todo!("RSP ORI"),
            RspOpcode::J => todo!("RSP J"),
            RspOpcode::JAL => todo!("RSP JAL"),
            RspOpcode::SLTI => todo!("RSP SLTI"),
            RspOpcode::SLTIU => todo!("RSP SLTIU"),
            RspOpcode::XORI => todo!("RSP XORI"),
            RspOpcode::LB => todo!("RSP LB"),
            RspOpcode::MTC0 => todo!("RSP MTC0"),
            RspOpcode::MFC0 => todo!("RSP MFC0"),
            RspOpcode::VEC_VABS => todo!("RSP VEC_VABS"),
            RspOpcode::VEC_VADD => todo!("RSP VEC_VADD"),
            RspOpcode::VEC_VADDC => todo!("RSP VEC_VADDC"),
            RspOpcode::VEC_VAND => todo!("RSP VEC_VAND"),
            RspOpcode::VEC_VCH => todo!("RSP VEC_VCH"),
            RspOpcode::VEC_VCL => todo!("RSP VEC_VCL"),
            RspOpcode::VEC_VCR => todo!("RSP VEC_VCR"),
            RspOpcode::VEC_VEQ => todo!("RSP VEC_VEQ"),
            RspOpcode::VEC_VGE => todo!("RSP VEC_VGE"),
            RspOpcode::VEC_VLT => todo!("RSP VEC_VLT"),
            RspOpcode::VEC_VMACF => todo!("RSP VEC_VMACF"),
            RspOpcode::VEC_VMACQ => todo!("RSP VEC_VMACQ"),
            RspOpcode::VEC_VMACU => todo!("RSP VEC_VMACU"),
            RspOpcode::VEC_VMADH => todo!("RSP VEC_VMADH"),
            RspOpcode::VEC_VMADL => todo!("RSP VEC_VMADL"),
            RspOpcode::VEC_VMADM => todo!("RSP VEC_VMADM"),
            RspOpcode::VEC_VMADN => todo!("RSP VEC_VMADN"),
            RspOpcode::VEC_VMOV => todo!("RSP VEC_VMOV"),
            RspOpcode::VEC_VMRG => todo!("RSP VEC_VMRG"),
            RspOpcode::VEC_VMUDH => todo!("RSP VEC_VMUDH"),
            RspOpcode::VEC_VMUDL => todo!("RSP VEC_VMUDL"),
            RspOpcode::VEC_VMUDM => todo!("RSP VEC_VMUDM"),
            RspOpcode::VEC_VMUDN => todo!("RSP VEC_VMUDN"),
            RspOpcode::VEC_VMULF => todo!("RSP VEC_VMULF"),
            RspOpcode::VEC_VMULQ => todo!("RSP VEC_VMULQ"),
            RspOpcode::VEC_VMULU => todo!("RSP VEC_VMULU"),
            RspOpcode::VEC_VNAND => todo!("RSP VEC_VNAND"),
            RspOpcode::VEC_VNE => todo!("RSP VEC_VNE"),
            RspOpcode::VEC_VNOP => todo!("RSP VEC_VNOP"),
            RspOpcode::VEC_VNOR => todo!("RSP VEC_VNOR"),
            RspOpcode::VEC_VNXOR => todo!("RSP VEC_VNXOR"),
            RspOpcode::VEC_VOR => todo!("RSP VEC_VOR"),
            RspOpcode::VEC_VRCP => todo!("RSP VEC_VRCP"),
            RspOpcode::VEC_VRCPH_VRSQH => todo!("RSP VEC_VRCPH_VRSQH"),
            RspOpcode::VEC_VRCPL => todo!("RSP VEC_VRCPL"),
            RspOpcode::VEC_VRNDN => todo!("RSP VEC_VRNDN"),
            RspOpcode::VEC_VRNDP => todo!("RSP VEC_VRNDP"),
            RspOpcode::VEC_VRSQ => todo!("RSP VEC_VRSQ"),
            RspOpcode::VEC_VRSQL => todo!("RSP VEC_VRSQL"),
            RspOpcode::VEC_VSAR => todo!("RSP VEC_VSAR"),
            RspOpcode::VEC_VSUB => todo!("RSP VEC_VSUB"),
            RspOpcode::VEC_VSUBC => todo!("RSP VEC_VSUBC"),
            RspOpcode::VEC_VXOR => todo!("RSP VEC_VXOR"),
            RspOpcode::VEC_VZERO => todo!("RSP VEC_VZERO"),
            RspOpcode::CFC2 => todo!("RSP CFC2"),
            RspOpcode::CTC2 => todo!("RSP CTC2"),
            RspOpcode::MFC2 => todo!("RSP MFC2"),
            RspOpcode::MTC2 => todo!("RSP MTC2"),
            RspOpcode::SLL => todo!("RSP SLL"),
            RspOpcode::SRL => todo!("RSP SRL"),
            RspOpcode::SRA => todo!("RSP SRA"),
            RspOpcode::SRAV => todo!("RSP SRAV"),
            RspOpcode::SLLV => todo!("RSP SLLV"),
            RspOpcode::SRLV => todo!("RSP SRLV"),
            RspOpcode::JR => todo!("RSP JR"),
            RspOpcode::JALR => todo!("RSP JALR"),
            RspOpcode::ADD => todo!("RSP ADD"),
            RspOpcode::AND => todo!("RSP AND"),
            RspOpcode::SUB => todo!("RSP SUB"),
            RspOpcode::OR => todo!("RSP OR"),
            RspOpcode::XOR => todo!("RSP XOR"),
            RspOpcode::NOR => todo!("RSP NOR"),
            RspOpcode::SLT => todo!("RSP SLT"),
            RspOpcode::SLTU => todo!("RSP SLTU"),
            RspOpcode::BREAK => todo!("RSP BREAK"),
            RspOpcode::BLTZ => todo!("RSP BLTZ"),
            RspOpcode::BGEZ => todo!("RSP BGEZ"),
            RspOpcode::BGEZAL => todo!("RSP BGEZAL"),
            RspOpcode::BLTZAL => todo!("RSP BLTZAL"),
            RspOpcode::LBV => todo!("RSP LBV"),
            RspOpcode::LDV => todo!("RSP LDV"),
            RspOpcode::LFV => todo!("RSP LFV"),
            RspOpcode::LHV => todo!("RSP LHV"),
            RspOpcode::LLV => todo!("RSP LLV"),
            RspOpcode::LPV => todo!("RSP LPV"),
            RspOpcode::LQV => todo!("RSP LQV"),
            RspOpcode::LRV => todo!("RSP LRV"),
            RspOpcode::LSV => todo!("RSP LSV"),
            RspOpcode::LTV => todo!("RSP LTV"),
            RspOpcode::LUV => todo!("RSP LUV"),
            RspOpcode::SBV => todo!("RSP SBV"),
            RspOpcode::SDV => todo!("RSP SDV"),
            RspOpcode::SFV => todo!("RSP SFV"),
            RspOpcode::SHV => todo!("RSP SHV"),
            RspOpcode::SLV => todo!("RSP SLV"),
            RspOpcode::SPV => todo!("RSP SPV"),
            RspOpcode::SQV => todo!("RSP SQV"),
            RspOpcode::SRV => todo!("RSP SRV"),
            RspOpcode::SSV => todo!("RSP SSV"),
            RspOpcode::STV => todo!("RSP STV"),
            RspOpcode::SUV => todo!("RSP SUV"),
            RspOpcode::SWV => todo!("RSP SWV"),
        }
    }

    func
}

pub fn rsp_to_ir(parsed: Vec<ParsedRspInstruction>, rsp: &rsp_t) -> IRFunction {
    rsp_to_ir_ctx(RspMipsToIrContext::default(), parsed, rsp)
}
