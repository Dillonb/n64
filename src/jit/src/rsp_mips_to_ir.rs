use std::mem::offset_of;

use dgbir::ir::{
    const_s16, const_s32, const_u16, const_u32, DataType, IRBlockHandle, IRContext, IRFunction,
    InputSlot,
};

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

struct GuestRegisterManager {
    rsp_address: InputSlot,
    gprs: [Option<InputSlot>; 32],
}

impl GuestRegisterManager {
    fn new(rsp_address: InputSlot) -> Self {
        let mut v = Self {
            rsp_address,
            gprs: [None; 32],
        };
        v.gprs[0] = Some(const_u32(0));
        v
    }

    pub fn set_gpr(&mut self, r: u8, value: InputSlot) {
        if r != 0 {
            self.gprs[r as usize] = Some(value);
        }
    }

    fn get_gpr(&mut self, block: &mut IRBlockHandle, r: u8) -> InputSlot {
        *self.gprs[r as usize].get_or_insert_with(|| {
            let offset = offset_of!(rsp_t, gpr) + (r as usize * std::mem::size_of::<u32>());
            block
                .load_ptr(DataType::U32, self.rsp_address, offset)
                .val()
        })
    }

    fn flush_all(&mut self, block: &mut IRBlockHandle, clear: bool) {
        self.gprs
            .iter_mut()
            .enumerate()
            .filter(|(i, reg)| *i != 0 && reg.is_some())
            .for_each(|(i, reg)| {
                if let Some(value) = if clear { reg.take() } else { *reg } {
                    let offset = offset_of!(rsp_t, gpr) + (i * std::mem::size_of::<u32>());
                    block.write_ptr(DataType::U32, self.rsp_address, offset, value);
                }
            });
    }
}

fn set_pc(
    pc_set_flag: &mut bool,
    block: &mut IRBlockHandle,
    rsp_address: InputSlot,
    value: InputSlot,
) {
    *pc_set_flag = true;
    let offset = offset_of!(rsp_t, pc);
    let next_pc_offset = offset_of!(rsp_t, next_pc);

    block.write_ptr(DataType::U16, rsp_address, offset, value);
    let next_pc = block.add(DataType::U16, value, const_u32(4));
    block.write_ptr(DataType::U16, rsp_address, next_pc_offset, next_pc.val());
}

pub fn rsp_to_ir_ctx(
    _ctx: RspMipsToIrContext,
    parsed: Vec<ParsedRspInstruction>,
    _rsp: &rsp_t,
) -> IRFunction {
    let context = IRContext::new();
    let func = IRFunction::new(context);
    let mut block = func.new_block(vec![DataType::Ptr]);
    let rsp_address = block.input(0);

    let mut guest_regs = GuestRegisterManager::new(rsp_address);

    let mut cycles = 0;
    let mut pc_set = false;

    if let Some(last) = parsed.last() {
        if last.op.is_branch() {
            todo!("RSP block ends with a branch. I don't think this can ever happen on the RSP, but it could be handled similarly to how the main CPU does.")
        }
    }

    for ParsedRspInstruction { addr: _, instr, op } in parsed {
        match op {
            RspOpcode::BRANCH(_) => todo!("RSP branch"),
            RspOpcode::NOP => todo!("RSP NOP"),
            RspOpcode::LUI => todo!("RSP LUI"),
            RspOpcode::ADDI => {
                let rs = guest_regs.get_gpr(&mut block, instr.rs());
                let result = block.add(DataType::S32, rs, const_s16(instr.s_imm()));
                guest_regs.set_gpr(instr.rt(), result.val());
            }
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
            RspOpcode::ORI => {
                let rs = guest_regs.get_gpr(&mut block, instr.rs());
                let result = block.or(DataType::U32, rs, const_u16(instr.imm()));
                guest_regs.set_gpr(instr.rt(), result.val());
            }
            RspOpcode::J => {
                set_pc(
                    &mut pc_set,
                    &mut block,
                    rsp_address,
                    const_u16(instr.j_target() as u16),
                );
            }
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
        cycles += 1;
    }

    if !pc_set {
        todo!("No branch in block, set PC based on length")
    }

    guest_regs.flush_all(&mut block, true);
    block.ret(Some(const_s32(cycles)));

    func
}

pub fn rsp_to_ir(parsed: Vec<ParsedRspInstruction>, rsp: &rsp_t) -> IRFunction {
    rsp_to_ir_ctx(RspMipsToIrContext::default(), parsed, rsp)
}
