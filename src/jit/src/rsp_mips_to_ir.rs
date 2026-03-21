use std::mem::offset_of;

use dgbir::ir::{
    const_ptr, const_s16, const_s32, const_u16, const_u32, const_u64, CompareType, DataType,
    IRBlockHandle, IRContext, IRFunction, InputSlot,
};
use log::warn;

use crate::{
    disassembler::disassemble_rsp_instruction,
    get_rsp_cp0_register,
    mips_parser::{BranchCondition, MipsInstructionBitfield},
    n64_rsp_read_byte_noinline, n64_rsp_read_half_noinline, n64_rsp_read_word_noinline,
    n64_rsp_write_byte_noinline, n64_rsp_write_half_noinline, n64_rsp_write_word_noinline,
    rsp_mips_parser::{ParsedRspInstruction, RspBranchInfo, RspOpcode},
    rsp_t, set_rsp_cp0_register,
};

pub struct RspMipsToIrContext {
    read_byte: usize,
    read_half: usize,
    read_word: usize,
    write_byte: usize,
    write_half: usize,
    write_word: usize,
    get_rsp_cp0_register: usize,
    set_rsp_cp0_register: usize,
}

impl RspMipsToIrContext {
    pub fn default() -> Self {
        Self {
            read_byte: n64_rsp_read_byte_noinline as *const () as usize,
            read_half: n64_rsp_read_half_noinline as *const () as usize,
            read_word: n64_rsp_read_word_noinline as *const () as usize,
            write_byte: n64_rsp_write_byte_noinline as *const () as usize,
            write_half: n64_rsp_write_half_noinline as *const () as usize,
            write_word: n64_rsp_write_word_noinline as *const () as usize,

            get_rsp_cp0_register: get_rsp_cp0_register as *const () as usize,
            set_rsp_cp0_register: set_rsp_cp0_register as *const () as usize,
        }
    }
}

struct GuestRegisterManager {
    rsp_address: InputSlot,
    gprs: [Option<InputSlot>; 32],
    vu_regs: [Option<InputSlot>; 32],
}

impl GuestRegisterManager {
    fn new(rsp_address: InputSlot) -> Self {
        let mut v = Self {
            rsp_address,
            gprs: [None; 32],
            vu_regs: [None; 32],
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

    fn set_vu_reg(&mut self, r: u8, value: InputSlot) {
        self.vu_regs[r as usize] = Some(value);
    }

    fn get_vu_reg(&mut self, block: &mut IRBlockHandle, r: u8) -> InputSlot {
        *self.vu_regs[r as usize].get_or_insert_with(|| {
            let offset = offset_of!(rsp_t, vu_regs) + (r as usize * std::mem::size_of::<u128>());
            block
                .load_ptr(DataType::U128, self.rsp_address, offset)
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
        self.vu_regs
            .iter_mut()
            .filter(|reg| reg.is_some())
            .enumerate()
            .for_each(|(i, reg)| {
                if let Some(value) = if clear { reg.take() } else { *reg } {
                    let offset = offset_of!(rsp_t, vu_regs) + (i * std::mem::size_of::<u128>());
                    block.write_ptr(DataType::U128, self.rsp_address, offset, value);
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

// const SHIFT_AMOUNT_LBV_SBV: i32 = 0;
// const SHIFT_AMOUNT_LSV_SSV: i32 = 1;
// const SHIFT_AMOUNT_LLV_SLV: i32 = 2;
const SHIFT_AMOUNT_LDV_SDV: i32 = 3;
// const SHIFT_AMOUNT_LQV_SQV: i32 = 4;
// const SHIFT_AMOUNT_LRV_SRV: i32 = 4;
// const SHIFT_AMOUNT_LPV_SPV: i32 = 3;
// const SHIFT_AMOUNT_LUV_SUV: i32 = 3;
// const SHIFT_AMOUNT_LHV_SHV: i32 = 4;
// const SHIFT_AMOUNT_LFV_SFV: i32 = 4;
// const SHIFT_AMOUNT_LTV_STV: i32 = 4;
// const SHIFT_AMOUNT_SWV: i32 = 4;

fn sign_extend_7bit_offset(offset: u8, shift_amount: i32) -> i32 {
    let soffset = ((offset << 1) & 0x80) | offset;
    let ofs = soffset as i32;
    let uofs = ofs as u32;
    (uofs << shift_amount) as i32
}

fn get_lswc2_address(
    instr: MipsInstructionBitfield,
    block: &mut IRBlockHandle,
    guest_regs: &mut GuestRegisterManager,
    shift_amount: i32,
) -> InputSlot {
    let base = guest_regs.get_gpr(block, instr.lswc2_base());
    let offset = sign_extend_7bit_offset(instr.lswc2_offset(), shift_amount);
    block.add(DataType::S32, base, const_s32(offset)).val()
}

fn rsp_load_u64(
    block: &mut IRBlockHandle,
    ctx: &RspMipsToIrContext,
    address: InputSlot,
) -> InputSlot {
    let v_high = block.call_function(const_ptr(ctx.read_word), Some(DataType::U32), vec![address]);
    let v_high = block.left_shift(DataType::U64, v_high.val(), const_u16(32));
    let v_low = block.call_function(
        const_ptr(ctx.read_word),
        Some(DataType::U32),
        vec![block.add(DataType::U32, address, const_u16(4)).val()],
    );
    block.or(DataType::U64, v_high.val(), v_low.val()).val()
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
        println!("{}", disassemble_rsp_instruction(*instr, addr));
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
            RspOpcode::LBU => {
                let base = guest_regs.get_gpr(&mut block, instr.rs());
                let addr = block.add(DataType::U32, base, const_s16(instr.s_imm()));

                let value = block.call_function(
                    const_ptr(ctx.read_byte),
                    Some(DataType::U8),
                    vec![addr.val()],
                );

                guest_regs.set_gpr(instr.rt(), value.val());
            }
            RspOpcode::LHU => {
                let base = guest_regs.get_gpr(&mut block, instr.rs());
                let addr = block.add(DataType::U32, base, const_s16(instr.s_imm()));

                let value = block.call_function(
                    const_ptr(ctx.read_half),
                    Some(DataType::U16),
                    vec![addr.val()],
                );

                guest_regs.set_gpr(instr.rt(), value.val());
            }
            RspOpcode::LH => {
                let base = guest_regs.get_gpr(&mut block, instr.rs());
                let addr = block.add(DataType::U32, base, const_s16(instr.s_imm()));

                let value = block.call_function(
                    const_ptr(ctx.read_half),
                    Some(DataType::S16),
                    vec![addr.val()],
                );

                let sign_extended = block.convert(DataType::S32, value.val());

                guest_regs.set_gpr(instr.rt(), sign_extended.val());
            }
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
            RspOpcode::SB => {
                let base = guest_regs.get_gpr(&mut block, instr.rs());
                let addr = block.add(DataType::U32, base, const_s16(instr.s_imm()));
                let value = guest_regs.get_gpr(&mut block, instr.rt());

                block.call_function(const_ptr(ctx.write_byte), None, vec![addr.val(), value]);
            }
            RspOpcode::SH => {
                let base = guest_regs.get_gpr(&mut block, instr.rs());
                let addr = block.add(DataType::U32, base, const_s16(instr.s_imm()));
                let value = guest_regs.get_gpr(&mut block, instr.rt());

                block.call_function(const_ptr(ctx.write_half), None, vec![addr.val(), value]);
            }
            RspOpcode::SW => {
                let base = guest_regs.get_gpr(&mut block, instr.rs());
                let addr = block.add(DataType::U32, base, const_s16(instr.s_imm()));
                let value = guest_regs.get_gpr(&mut block, instr.rt());

                block.call_function(const_ptr(ctx.write_word), None, vec![addr.val(), value]);
            }
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
            RspOpcode::JAL => {
                set_link_reg(&mut guest_regs, addr, 31);
                set_pc(
                    &mut pc_set,
                    &mut block,
                    rsp_address,
                    const_u16((instr.j_target() as u16) << 2),
                );
            }
            RspOpcode::SLTI => todo!("RSP SLTI"),
            RspOpcode::SLTIU => todo!("RSP SLTIU"),
            RspOpcode::XORI => todo!("RSP XORI"),
            RspOpcode::LB => {
                let base = guest_regs.get_gpr(&mut block, instr.rs());
                let addr = block.add(DataType::U32, base, const_s16(instr.s_imm()));

                let value = block.call_function(
                    const_ptr(ctx.read_byte),
                    Some(DataType::S8),
                    vec![addr.val()],
                );

                let sign_extended = block.convert(DataType::S32, value.val());

                guest_regs.set_gpr(instr.rt(), sign_extended.val());
            }
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
            RspOpcode::SLL => {
                let input = guest_regs.get_gpr(&mut block, instr.rt());
                let result = block.left_shift(DataType::U32, input, const_u16(instr.sa() as u16));
                guest_regs.set_gpr(instr.rd(), result.val());
            }
            RspOpcode::SRL => {
                let input = guest_regs.get_gpr(&mut block, instr.rt());
                let result = block.right_shift(DataType::U32, input, const_u16(instr.sa() as u16));
                guest_regs.set_gpr(instr.rd(), result.val());
            }
            RspOpcode::SRA => todo!("RSP SRA"),
            RspOpcode::SRAV => todo!("RSP SRAV"),
            RspOpcode::SLLV => todo!("RSP SLLV"),
            RspOpcode::SRLV => todo!("RSP SRLV"),
            RspOpcode::JR => {
                let target = guest_regs.get_gpr(&mut block, instr.rs());
                set_pc(&mut pc_set, &mut block, rsp_address, target);
            }
            RspOpcode::JALR => {
                let target = guest_regs.get_gpr(&mut block, instr.rs());
                set_pc(&mut pc_set, &mut block, rsp_address, target);
                set_link_reg(&mut guest_regs, addr, instr.rd());
            }
            RspOpcode::ADD => {
                let rs = guest_regs.get_gpr(&mut block, instr.rs());
                let rt = guest_regs.get_gpr(&mut block, instr.rt());
                let result = block.add(DataType::S32, rs, rt);
                guest_regs.set_gpr(instr.rd(), result.val());
            }
            RspOpcode::AND => todo!("RSP AND"),
            RspOpcode::SUB => todo!("RSP SUB"),
            RspOpcode::OR => todo!("RSP OR"),
            RspOpcode::XOR => todo!("RSP XOR"),
            RspOpcode::NOR => todo!("RSP NOR"),
            RspOpcode::SLT => todo!("RSP SLT"),
            RspOpcode::SLTU => todo!("RSP SLTU"),
            RspOpcode::BREAK => todo!("RSP BREAK"),
            RspOpcode::LBV => todo!("RSP LBV"),
            RspOpcode::LDV => {
                let address =
                    get_lswc2_address(instr, &mut block, &mut guest_regs, SHIFT_AMOUNT_LDV_SDV);
                let shift = 8 - instr.lswc2_e();

                let mask = block.left_shift(
                    DataType::U128,
                    const_u64(0xFFFFFFFFFFFFFFFF),
                    const_u16(shift as u16 * 8),
                );
                let inv_mask = block.not(DataType::U128, mask.val());

                let reg = guest_regs.get_vu_reg(&mut block, instr.lswc2_vt());
                let masked = block.and(DataType::U128, reg, inv_mask.val());

                let value = rsp_load_u64(&mut block, &ctx, address);

                let placed = block.left_shift(DataType::U128, value, const_u16(shift as u16 * 8));
                let result = block.or(DataType::U128, masked.val(), placed.val());
                guest_regs.set_vu_reg(instr.lswc2_vt(), result.val());
            }
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
