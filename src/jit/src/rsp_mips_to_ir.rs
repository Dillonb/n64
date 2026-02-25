use std::mem::offset_of;

use dgbir::{
    disassembler::disassemble_mips_instruction,
    ir::{
        const_ptr, const_s16, const_s32, const_u16, const_u32, CompareType, DataType,
        IRBlockHandle, IRContext, IRFunction, InputSlot,
    },
};
use log::warn;

use crate::{
    get_rsp_cp0_register,
    mips_parser::{BranchCondition, MipsInstructionBitfield},
    n64_rsp_read_byte_noinline, n64_rsp_read_half_noinline, n64_rsp_read_word_noinline,
    n64_rsp_write_byte_noinline, n64_rsp_write_half_noinline, n64_rsp_write_word_noinline,
    rsp_mips_parser::{ParsedRspInstruction, RspBranchInfo, RspOpcode},
    rsp_t, set_rsp_cp0_register,
};

pub struct RspMipsToIrContext {
    _read_byte: usize,
    _read_half: usize,
    read_word: usize,
    _write_byte: usize,
    _write_half: usize,
    _write_word: usize,
    get_rsp_cp0_register: usize,
    set_rsp_cp0_register: usize,
}

impl RspMipsToIrContext {
    pub fn default() -> Self {
        Self {
            _read_byte: n64_rsp_read_byte_noinline as usize,
            _read_half: n64_rsp_read_half_noinline as usize,
            read_word: n64_rsp_read_word_noinline as usize,
            _write_byte: n64_rsp_write_byte_noinline as usize,
            _write_half: n64_rsp_write_half_noinline as usize,
            _write_word: n64_rsp_write_word_noinline as usize,

            get_rsp_cp0_register: get_rsp_cp0_register as usize,
            set_rsp_cp0_register: set_rsp_cp0_register as usize,
        }
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

    let value = block.right_shift(DataType::U16, value, const_u16(2)).val();

    block.write_ptr(DataType::U16, rsp_address, offset, value);
    let next_pc = block.add(DataType::U16, value, const_u32(1));
    block.write_ptr(DataType::U16, rsp_address, next_pc_offset, next_pc.val());
}

fn set_link_reg(guest_regs: &mut GuestRegisterManager, addr: u16, mips_reg: u8) {
    // Skip the delay slot on return
    let addr = addr.wrapping_add(8);
    guest_regs.set_gpr(mips_reg, const_u16(addr));
}

fn do_branch(
    link: bool,
    guest_regs: &mut GuestRegisterManager,
    addr: u16,
    func: &IRFunction,
    take_branch: InputSlot,
    instr: MipsInstructionBitfield,
    cpu_address: InputSlot,
    pc_set: &mut bool,
    block: &mut IRBlockHandle,
) {
    if link {
        set_link_reg(guest_regs, addr, 31);
    }

    let mut taken_block = func.new_block(vec![]);
    let mut not_taken_block = func.new_block(vec![]);

    let taken_pc = addr
        .wrapping_add(4)
        .wrapping_add_signed((instr.s_imm()) << 2);
    let not_taken_pc = addr.wrapping_add(8);

    warn!(
        "Jumping to {:03X} if taken, continuing to {:03X} if not taken",
        taken_pc, not_taken_pc
    );

    set_pc(pc_set, &mut taken_block, cpu_address, const_u16(taken_pc));
    set_pc(
        pc_set,
        &mut not_taken_block,
        cpu_address,
        const_u16(not_taken_pc),
    );

    block.branch(
        take_branch,
        taken_block.call(vec![]),
        not_taken_block.call(vec![]),
    );

    *block = func.new_block(vec![]);

    taken_block.jump(block.call(vec![]));
    // Continue and execute the delay slot.
    not_taken_block.jump(block.call(vec![]));
}

pub fn rsp_to_ir_ctx(
    ctx: RspMipsToIrContext,
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

    println!("--------------");
    for ParsedRspInstruction { addr, instr, op } in parsed {
        println!("{}", disassemble_mips_instruction(*instr, addr as u64));
        match op {
            RspOpcode::BRANCH(RspBranchInfo { cond, link }) => {
                let rs_reg = instr.rs();
                let mut rt_reg = instr.rt();

                let (signed, compare_type) = match cond {
                    BranchCondition::EQ => (false, CompareType::Equal),
                    BranchCondition::NE => (false, CompareType::NotEqual),
                    BranchCondition::GTZ => {
                        rt_reg = 0;
                        (true, CompareType::GreaterThan)
                    }
                    BranchCondition::LTZ => {
                        rt_reg = 0;
                        (true, CompareType::LessThan)
                    }
                    BranchCondition::LEZ => {
                        rt_reg = 0;
                        (true, CompareType::LessThanOrEqual)
                    }
                    BranchCondition::GEZ => {
                        rt_reg = 0;
                        (true, CompareType::GreaterThanOrEqual)
                    }
                };

                let rs = guest_regs.get_gpr(&mut block, rs_reg);
                let rt = guest_regs.get_gpr(&mut block, rt_reg);

                let tp = if signed { DataType::S32 } else { DataType::U32 };
                let take_branch = block.compare(tp, rs, compare_type, rt);

                do_branch(
                    link,
                    &mut guest_regs,
                    addr,
                    &func,
                    take_branch.val(),
                    instr,
                    rsp_address,
                    &mut pc_set,
                    &mut block,
                );
            }
            RspOpcode::NOP => {}
            RspOpcode::LUI => todo!("RSP LUI"),
            RspOpcode::ADDI => {
                let rs = guest_regs.get_gpr(&mut block, instr.rs());
                let result = block.add(DataType::S32, rs, const_s16(instr.s_imm()));
                guest_regs.set_gpr(instr.rt(), result.val());
            }
            RspOpcode::ANDI => {
                let rs = guest_regs.get_gpr(&mut block, instr.rs());
                let result = block.and(DataType::U32, rs, const_u16(instr.imm()));
                guest_regs.set_gpr(instr.rt(), result.val());
            }
            RspOpcode::LBU => todo!("RSP LBU"),
            RspOpcode::LHU => todo!("RSP LHU"),
            RspOpcode::LH => todo!("RSP LH"),
            RspOpcode::LW => {
                let base = guest_regs.get_gpr(&mut block, instr.rs());
                let addr = block.add(DataType::U32, base, const_s16(instr.s_imm()));

                let v = block.call_function(
                    const_ptr(ctx.read_word),
                    Some(DataType::S32),
                    vec![addr.val()],
                );

                guest_regs.set_gpr(instr.rt(), v.val());
            }
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
                    const_u16((instr.j_target() as u16) << 2),
                );
            }
            RspOpcode::JAL => todo!("RSP JAL"),
            RspOpcode::SLTI => todo!("RSP SLTI"),
            RspOpcode::SLTIU => todo!("RSP SLTIU"),
            RspOpcode::XORI => todo!("RSP XORI"),
            RspOpcode::LB => todo!("RSP LB"),
            RspOpcode::MTC0 => {
                let value = guest_regs.get_gpr(&mut block, instr.rt());
                block.call_function(
                    const_ptr(ctx.set_rsp_cp0_register),
                    None,
                    vec![const_u16(instr.rd() as u16), value],
                );
            }
            RspOpcode::MFC0 => {
                let value = block.call_function(
                    const_ptr(ctx.get_rsp_cp0_register),
                    Some(DataType::U32),
                    vec![const_u16(instr.rd() as u16)],
                );
                guest_regs.set_gpr(instr.rt(), value.val());
            }
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
