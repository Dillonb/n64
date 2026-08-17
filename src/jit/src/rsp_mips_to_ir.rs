use std::mem::offset_of;

use dgbir::external_fn;
use dgbir::ir::{
    const_s16, const_s32, const_u128, const_u16, const_u32, const_u64, CompareType, DataType,
    ExternalFunction, IRBlockHandle, IRContext, IRFunction, InputSlot, MultiplyType, PackType,
    VectorHalf,
};
use log::{trace, warn};

use crate::{
    disassembler::disassemble_rsp_instruction,
    get_rsp_cp0_register,
    mips_parser::{BranchCondition, MipsInstructionBitfield},
    n64_rsp_read_byte_noinline, n64_rsp_read_half_noinline, n64_rsp_read_word_noinline,
    n64_rsp_write_byte_noinline, n64_rsp_write_half_noinline, n64_rsp_write_word_noinline,
    rsp_interpret_instruction, rsp_interpreter_fallback_until_no_branch,
    rsp_mips_parser::{ParsedRspInstruction, RspBranchInfo, RspOpcode},
    rsp_resolve_interpreter_handler, rsp_t, set_rsp_cp0_register,
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
    interpreter_fallback_until_no_branch: usize,
}

impl RspMipsToIrContext {
    fn read_byte(&self) -> ExternalFunction {
        external_fn!(n64_rsp_read_byte_noinline(_)).at(self.read_byte)
    }

    fn read_half(&self) -> ExternalFunction {
        external_fn!(n64_rsp_read_half_noinline(_)).at(self.read_half)
    }

    fn read_word(&self) -> ExternalFunction {
        external_fn!(n64_rsp_read_word_noinline(_)).at(self.read_word)
    }

    fn write_byte(&self) -> ExternalFunction {
        external_fn!(n64_rsp_write_byte_noinline(_, _)).at(self.write_byte)
    }

    fn write_half(&self) -> ExternalFunction {
        external_fn!(n64_rsp_write_half_noinline(_, _)).at(self.write_half)
    }

    fn write_word(&self) -> ExternalFunction {
        external_fn!(n64_rsp_write_word_noinline(_, _)).at(self.write_word)
    }

    fn get_rsp_cp0_register(&self) -> ExternalFunction {
        external_fn!(get_rsp_cp0_register(_)).at(self.get_rsp_cp0_register)
    }

    fn set_rsp_cp0_register(&self) -> ExternalFunction {
        external_fn!(set_rsp_cp0_register(_, _)).at(self.set_rsp_cp0_register)
    }

    fn interpreter_fallback_until_no_branch(&self) -> ExternalFunction {
        external_fn!(rsp_interpreter_fallback_until_no_branch())
            .at(self.interpreter_fallback_until_no_branch)
    }

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
            interpreter_fallback_until_no_branch: rsp_interpreter_fallback_until_no_branch
                as *const () as usize,
        }
    }
}

const ACC_HIGH: usize = 0;
const ACC_MID: usize = 1;
const ACC_LOW: usize = 2;

struct GuestRegisterManager {
    rsp_address: InputSlot,
    gprs: [Option<InputSlot>; 32],
    vu_regs: [Option<InputSlot>; 32],
    acc: [Option<InputSlot>; 3],
}

impl GuestRegisterManager {
    fn new(rsp_address: InputSlot) -> Self {
        let mut v = Self {
            rsp_address,
            gprs: [None; 32],
            vu_regs: [None; 32],
            acc: [None; 3],
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

    fn acc_offset(index: usize) -> usize {
        offset_of!(rsp_t, acc) + index * std::mem::size_of::<u128>()
    }

    fn set_acc(&mut self, index: usize, value: InputSlot) {
        self.acc[index] = Some(value);
    }

    fn get_acc(&mut self, block: &mut IRBlockHandle, index: usize) -> InputSlot {
        *self.acc[index].get_or_insert_with(|| {
            block
                .load_ptr(DataType::VU16, self.rsp_address, Self::acc_offset(index))
                .val()
        })
    }

    fn set_acc_high(&mut self, value: InputSlot) {
        self.set_acc(ACC_HIGH, value);
    }

    fn set_acc_mid(&mut self, value: InputSlot) {
        self.set_acc(ACC_MID, value);
    }

    fn set_acc_low(&mut self, value: InputSlot) {
        self.set_acc(ACC_LOW, value);
    }

    #[allow(dead_code)]
    fn get_acc_high(&mut self, block: &mut IRBlockHandle) -> InputSlot {
        self.get_acc(block, ACC_HIGH)
    }

    #[allow(dead_code)]
    fn get_acc_mid(&mut self, block: &mut IRBlockHandle) -> InputSlot {
        self.get_acc(block, ACC_MID)
    }

    #[allow(dead_code)]
    fn get_acc_low(&mut self, block: &mut IRBlockHandle) -> InputSlot {
        self.get_acc(block, ACC_LOW)
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
            .enumerate()
            .filter(|(_, reg)| reg.is_some())
            .for_each(|(i, reg)| {
                if let Some(value) = if clear { reg.take() } else { *reg } {
                    let offset = offset_of!(rsp_t, vu_regs) + (i * std::mem::size_of::<u128>());
                    block.write_ptr(DataType::U128, self.rsp_address, offset, value);
                }
            });
        self.acc
            .iter_mut()
            .enumerate()
            .filter(|(_, slot)| slot.is_some())
            .for_each(|(i, slot)| {
                if let Some(value) = if clear { slot.take() } else { *slot } {
                    block.write_ptr(DataType::VU16, self.rsp_address, Self::acc_offset(i), value);
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
const SHIFT_AMOUNT_LQV_SQV: i32 = 4;
// const SHIFT_AMOUNT_LRV_SRV: i32 = 4;
// const SHIFT_AMOUNT_LPV_SPV: i32 = 3;
// const SHIFT_AMOUNT_LUV_SUV: i32 = 3;
// const SHIFT_AMOUNT_LHV_SHV: i32 = 4;
// const SHIFT_AMOUNT_LFV_SFV: i32 = 4;
// const SHIFT_AMOUNT_LTV_STV: i32 = 4;
// const SHIFT_AMOUNT_SWV: i32 = 4;

fn sign_extend_7bit_offset(offset: u8, shift_amount: i32) -> i32 {
    // Bit 6 is the sign bit, so copy it into bit 7 and sign extend from there.
    let soffset = (((offset << 1) & 0x80) | offset) as i8;
    let uofs = soffset as i32 as u32;
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

/// The element selection applied to vt by the CP2 vector instructions. Architectural element i
/// lives in lane 7 - i, so the patterns below are the usual element tables mirrored.
fn get_vte(block: &mut IRBlockHandle, vt: InputSlot, e: u8) -> InputSlot {
    const ELEMENTS: [[u8; 8]; 16] = [
        [0, 1, 2, 3, 4, 5, 6, 7],
        [0, 1, 2, 3, 4, 5, 6, 7],
        [0, 0, 2, 2, 4, 4, 6, 6],
        [1, 1, 3, 3, 5, 5, 7, 7],
        [0, 0, 0, 0, 4, 4, 4, 4],
        [1, 1, 1, 1, 5, 5, 5, 5],
        [2, 2, 2, 2, 6, 6, 6, 6],
        [3, 3, 3, 3, 7, 7, 7, 7],
        [0; 8],
        [1; 8],
        [2; 8],
        [3; 8],
        [4; 8],
        [5; 8],
        [6; 8],
        [7; 8],
    ];

    const PATTERNS: [u64; 16] = {
        let mut patterns = [0u64; 16];
        let mut e = 0;
        while e < 16 {
            let elements = ELEMENTS[e];
            let mut lane = 0;
            while lane < 8 {
                let src = 7 - elements[7 - lane];
                patterns[e] |= (src as u64) << (4 * lane);
                lane += 1;
            }
            e += 1;
        }
        patterns
    };

    if e <= 1 {
        return vt;
    }
    block
        .vector_swizzle(DataType::VU16, vt, PATTERNS[e as usize])
        .val()
}

/// `vs` and the element selected `vt`, which almost every CP2 vector instruction starts with.
fn vs_and_vte(
    block: &mut IRBlockHandle,
    guest_regs: &mut GuestRegisterManager,
    instr: MipsInstructionBitfield,
) -> (InputSlot, InputSlot) {
    let vs = guest_regs.get_vu_reg(block, instr.cp2_vec_vs());
    let vt = guest_regs.get_vu_reg(block, instr.cp2_vec_vt());
    (vs, get_vte(block, vt, instr.cp2_vec_e()))
}

/// Splits `2 * vs * vte` into the three accumulator halves. The product is 32 bits, so doubling
/// it needs 33, and the top half is the sign.
fn doubled_product(
    block: &mut IRBlockHandle,
    vs: InputSlot,
    vte: InputSlot,
) -> (InputSlot, InputSlot, InputSlot) {
    let lo = block.multiply(
        DataType::VS16,
        DataType::VS16,
        MultiplyType::Combined,
        vs,
        vte,
    );
    let hi = block.multiply(DataType::VS16, DataType::VS16, MultiplyType::High, vs, vte);

    let low = block.left_shift(DataType::VU16, lo.val(), const_u64(1));
    // The bit shifted out of the low half is the one shifted into the high half.
    let carried = block.right_shift(DataType::VU16, lo.val(), const_u64(15));
    let shifted = block.left_shift(DataType::VU16, hi.val(), const_u64(1));
    let mid = block.or(DataType::VU16, shifted.val(), carried.val());
    let high = block.right_shift(DataType::VS16, hi.val(), const_u64(15));
    (low.val(), mid.val(), high.val())
}

/// `clamp_signed(acc >> 16)`, which is acc.h and acc.m read as one signed 32 bit value.
fn clamp_acc_to_vd(
    block: &mut IRBlockHandle,
    acc_mid: InputSlot,
    acc_high: InputSlot,
) -> InputSlot {
    let low = block.vector_interleave(DataType::VS32, VectorHalf::Low, acc_mid, acc_high);
    let high = block.vector_interleave(DataType::VS32, VectorHalf::High, acc_mid, acc_high);
    block
        .vector_pack(DataType::VS16, PackType::Saturating, low.val(), high.val())
        .val()
}

fn rsp_load_u64(
    block: &mut IRBlockHandle,
    ctx: &RspMipsToIrContext,
    address: InputSlot,
) -> InputSlot {
    let low_address = block.add(DataType::U32, address, const_u16(4)).val();

    let v_high = block.call_function(ctx.read_word(), &[address]);
    let v_high = block.left_shift(DataType::U64, v_high.val(), const_u16(32));
    let v_low = block.call_function(ctx.read_word(), &[low_address]);
    block.or(DataType::U64, v_high.val(), v_low.val()).val()
}

/// Interprets one RSP instruction.
fn interpret_instruction(
    block: &mut IRBlockHandle,
    guest_regs: &mut GuestRegisterManager,
    op: &RspOpcode,
    addr: u16,
    instr: MipsInstructionBitfield,
) {
    // An interpreted branch would fight the PC handling here.
    assert!(
        !op.is_branch(),
        "Cannot interpret branch instruction {:?}",
        op
    );
    trace!("Falling back to the interpreter for {:?}", op);

    let handler = unsafe { rsp_resolve_interpreter_handler(addr as u32, instr.raw()) }
        .expect("rsp_resolve_interpreter_handler failed unexpectedly");

    guest_regs.flush_all(block, true);
    block.call_function(
        external_fn!(rsp_interpret_instruction(_)).at(handler as *const () as usize),
        &[const_u32(instr.raw())],
    );
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

    // A block ending in a branch has a branch in a delay slot, which the PC handling can't express.
    if let Some(last) = parsed.last() {
        if last.op.is_branch() {
            let cycles = block.call_function(ctx.interpreter_fallback_until_no_branch(), &[]);

            block.ret(Some(cycles.val()));
            return func;
        }
    }

    let mut last_addr = 0;
    for ParsedRspInstruction { addr, instr, op } in parsed {
        trace!("{}", disassemble_rsp_instruction(*instr, addr));
        last_addr = addr;
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
            RspOpcode::LUI => {
                guest_regs.set_gpr(instr.rt(), const_u32((instr.imm() as u32) << 16));
            }
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

                let value = block.call_function(ctx.read_byte(), &[addr.val()]);

                guest_regs.set_gpr(instr.rt(), value.val());
            }
            RspOpcode::LHU => {
                let base = guest_regs.get_gpr(&mut block, instr.rs());
                let addr = block.add(DataType::U32, base, const_s16(instr.s_imm()));

                let value = block.call_function(ctx.read_half(), &[addr.val()]);

                guest_regs.set_gpr(instr.rt(), value.val());
            }
            RspOpcode::LH => {
                let base = guest_regs.get_gpr(&mut block, instr.rs());
                let addr = block.add(DataType::U32, base, const_s16(instr.s_imm()));

                let value = block.call_function(ctx.read_half(), &[addr.val()]);

                let sign_extended = block.convert_from(DataType::S16, DataType::S32, value.val());

                guest_regs.set_gpr(instr.rt(), sign_extended.val());
            }
            RspOpcode::LW => {
                let base = guest_regs.get_gpr(&mut block, instr.rs());
                let addr = block.add(DataType::U32, base, const_s16(instr.s_imm()));

                let v = block.call_function(ctx.read_word(), &[addr.val()]);

                guest_regs.set_gpr(instr.rt(), v.val());
            }
            RspOpcode::SB => {
                let base = guest_regs.get_gpr(&mut block, instr.rs());
                let addr = block.add(DataType::U32, base, const_s16(instr.s_imm()));
                let value = guest_regs.get_gpr(&mut block, instr.rt());

                block.call_function(ctx.write_byte(), &[addr.val(), value]);
            }
            RspOpcode::SH => {
                let base = guest_regs.get_gpr(&mut block, instr.rs());
                let addr = block.add(DataType::U32, base, const_s16(instr.s_imm()));
                let value = guest_regs.get_gpr(&mut block, instr.rt());

                block.call_function(ctx.write_half(), &[addr.val(), value]);
            }
            RspOpcode::SW => {
                let base = guest_regs.get_gpr(&mut block, instr.rs());
                let addr = block.add(DataType::U32, base, const_s16(instr.s_imm()));
                let value = guest_regs.get_gpr(&mut block, instr.rt());

                block.call_function(ctx.write_word(), &[addr.val(), value]);
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
            // RspOpcode::SLTI => todo!("RSP SLTI"),
            // RspOpcode::SLTIU => todo!("RSP SLTIU"),
            RspOpcode::XORI => {
                let rs = guest_regs.get_gpr(&mut block, instr.rs());
                let result = block.xor(DataType::U32, rs, const_u16(instr.imm()));
                guest_regs.set_gpr(instr.rt(), result.val());
            }
            RspOpcode::LB => {
                let base = guest_regs.get_gpr(&mut block, instr.rs());
                let addr = block.add(DataType::U32, base, const_s16(instr.s_imm()));

                let value = block.call_function(ctx.read_byte(), &[addr.val()]);

                let sign_extended = block.convert_from(DataType::S8, DataType::S32, value.val());

                guest_regs.set_gpr(instr.rt(), sign_extended.val());
            }
            RspOpcode::MTC0 => {
                let value = guest_regs.get_gpr(&mut block, instr.rt());
                block.call_function(
                    ctx.set_rsp_cp0_register(),
                    &[const_u16(instr.rd() as u16), value],
                );
            }
            RspOpcode::MFC0 => {
                let value = block
                    .call_function(ctx.get_rsp_cp0_register(), &[const_u16(instr.rd() as u16)]);
                guest_regs.set_gpr(instr.rt(), value.val());
            }
            // RspOpcode::VEC_VABS => todo!("RSP VEC_VABS"),
            // RspOpcode::VEC_VADD => todo!("RSP VEC_VADD"),
            // RspOpcode::VEC_VADDC => todo!("RSP VEC_VADDC"),
            RspOpcode::VEC_VAND => {
                let (vs, vte) = vs_and_vte(&mut block, &mut guest_regs, instr);
                let result = block.and(DataType::VU16, vs, vte);
                guest_regs.set_acc_low(result.val());
                guest_regs.set_vu_reg(instr.cp2_vec_vd(), result.val());
            }
            // RspOpcode::VEC_VCH => todo!("RSP VEC_VCH"),
            // RspOpcode::VEC_VCL => todo!("RSP VEC_VCL"),
            // RspOpcode::VEC_VCR => todo!("RSP VEC_VCR"),
            // RspOpcode::VEC_VEQ => todo!("RSP VEC_VEQ"),
            // RspOpcode::VEC_VGE => todo!("RSP VEC_VGE"),
            // RspOpcode::VEC_VLT => todo!("RSP VEC_VLT"),
            RspOpcode::VEC_VMACF => {
                let vs = guest_regs.get_vu_reg(&mut block, instr.cp2_vec_vs());
                let vt = guest_regs.get_vu_reg(&mut block, instr.cp2_vec_vt());
                let vte = get_vte(&mut block, vt, instr.cp2_vec_e());

                let (delta_low, delta_mid, delta_high) = doubled_product(&mut block, vs, vte);

                let acc_low = guest_regs.get_acc_low(&mut block);
                let saturated = block.saturating_add(DataType::VU16, acc_low, delta_low);
                let new_low = block.add(DataType::VU16, acc_low, delta_low);
                let carry_low = block.compare(
                    DataType::VU16,
                    new_low.val(),
                    CompareType::NotEqual,
                    saturated.val(),
                );

                // acc.m takes the middle of the delta plus the carry, and either of those two
                // additions can carry into acc.h on its own.
                let acc_mid = guest_regs.get_acc_mid(&mut block);
                let saturated = block.saturating_add(DataType::VU16, acc_mid, delta_mid);
                let summed = block.add(DataType::VU16, acc_mid, delta_mid);
                let carry_sum = block.compare(
                    DataType::VU16,
                    summed.val(),
                    CompareType::NotEqual,
                    saturated.val(),
                );
                let new_mid = block.subtract(DataType::VU16, summed.val(), carry_low.val());
                // Adding one only wraps a lane that was already all ones, leaving it zero.
                let wrapped = block.compare(
                    DataType::VU16,
                    new_mid.val(),
                    CompareType::Equal,
                    const_u128(0),
                );
                let carry_inc = block.and(DataType::VU16, carry_low.val(), wrapped.val());
                let carry_mid = block.or(DataType::VU16, carry_sum.val(), carry_inc.val());

                let acc_high = guest_regs.get_acc_high(&mut block);
                let new_high = block.add(DataType::VU16, acc_high, delta_high);
                let new_high = block.subtract(DataType::VU16, new_high.val(), carry_mid.val());

                guest_regs.set_acc_low(new_low.val());
                guest_regs.set_acc_mid(new_mid.val());
                guest_regs.set_acc_high(new_high.val());

                let result = clamp_acc_to_vd(&mut block, new_mid.val(), new_high.val());
                guest_regs.set_vu_reg(instr.cp2_vec_vd(), result);
            }
            // RspOpcode::VEC_VMACQ => todo!("RSP VEC_VMACQ"),
            // RspOpcode::VEC_VMACU => todo!("RSP VEC_VMACU"),
            RspOpcode::VEC_VMADH => {
                let vs = guest_regs.get_vu_reg(&mut block, instr.cp2_vec_vs());
                let vt = guest_regs.get_vu_reg(&mut block, instr.cp2_vec_vt());
                let vte = get_vte(&mut block, vt, instr.cp2_vec_e());

                let lo = block.multiply(
                    DataType::VS16,
                    DataType::VS16,
                    MultiplyType::Combined,
                    vs,
                    vte,
                );
                let hi =
                    block.multiply(DataType::VS16, DataType::VS16, MultiplyType::High, vs, vte);

                let acc_mid = guest_regs.get_acc_mid(&mut block);
                let saturated = block.saturating_add(DataType::VU16, acc_mid, lo.val());
                let new_mid = block.add(DataType::VU16, acc_mid, lo.val());
                let carry = block.compare(
                    DataType::VU16,
                    new_mid.val(),
                    CompareType::NotEqual,
                    saturated.val(),
                );

                // The mask is all ones where a carry happened, so subtracting it adds one.
                let hi = block.subtract(DataType::VU16, hi.val(), carry.val());
                let acc_high = guest_regs.get_acc_high(&mut block);
                let new_high = block.add(DataType::VU16, acc_high, hi.val());

                guest_regs.set_acc_mid(new_mid.val());
                guest_regs.set_acc_high(new_high.val());

                let result = clamp_acc_to_vd(&mut block, new_mid.val(), new_high.val());
                guest_regs.set_vu_reg(instr.cp2_vec_vd(), result);
            }
            // RspOpcode::VEC_VMADL => todo!("RSP VEC_VMADL"),
            // RspOpcode::VEC_VMADM => todo!("RSP VEC_VMADM"),
            // RspOpcode::VEC_VMADN => todo!("RSP VEC_VMADN"),
            // RspOpcode::VEC_VMOV => todo!("RSP VEC_VMOV"),
            // RspOpcode::VEC_VMRG => todo!("RSP VEC_VMRG"),
            RspOpcode::VEC_VMUDH => {
                let vs = guest_regs.get_vu_reg(&mut block, instr.cp2_vec_vs());
                let vt = guest_regs.get_vu_reg(&mut block, instr.cp2_vec_vt());
                let vte = get_vte(&mut block, vt, instr.cp2_vec_e());

                // The product is 32 bits, held in acc.m and acc.h with acc.l zeroed.
                let lo = block.multiply(
                    DataType::VS16,
                    DataType::VS16,
                    MultiplyType::Combined,
                    vs,
                    vte,
                );
                let hi =
                    block.multiply(DataType::VS16, DataType::VS16, MultiplyType::High, vs, vte);
                guest_regs.set_acc_low(const_u64(0));
                guest_regs.set_acc_mid(lo.val());
                guest_regs.set_acc_high(hi.val());

                // vd is the product itself, clamped, which is the accumulator before the shift.
                let result = clamp_acc_to_vd(&mut block, lo.val(), hi.val());
                guest_regs.set_vu_reg(instr.cp2_vec_vd(), result);
            }
            // RspOpcode::VEC_VMUDL => todo!("RSP VEC_VMUDL"),
            // RspOpcode::VEC_VMUDM => todo!("RSP VEC_VMUDM"),
            // RspOpcode::VEC_VMUDN => todo!("RSP VEC_VMUDN"),
            RspOpcode::VEC_VMULF => {
                let vs = guest_regs.get_vu_reg(&mut block, instr.cp2_vec_vs());
                let vt = guest_regs.get_vu_reg(&mut block, instr.cp2_vec_vt());
                let vte = get_vte(&mut block, vt, instr.cp2_vec_e());

                let lo = block.multiply(
                    DataType::VS16,
                    DataType::VS16,
                    MultiplyType::Combined,
                    vs,
                    vte,
                );
                let hi =
                    block.multiply(DataType::VS16, DataType::VS16, MultiplyType::High, vs, vte);

                // 2 * prod + 0x8000 is 2 * (prod + 0x4000), which is one carry chain instead of
                // two. The inner add cannot overflow 32 bits because the product fits in 31.
                let round = const_u128(0x4000_4000_4000_4000_4000_4000_4000_4000);
                let saturated = block.saturating_add(DataType::VU16, lo.val(), round);
                let rounded_low = block.add(DataType::VU16, lo.val(), round);
                let carry = block.compare(
                    DataType::VU16,
                    rounded_low.val(),
                    CompareType::NotEqual,
                    saturated.val(),
                );
                let rounded_high = block.subtract(DataType::VU16, hi.val(), carry.val());

                let acc_low = block.left_shift(DataType::VU16, rounded_low.val(), const_u64(1));
                let carried = block.right_shift(DataType::VU16, rounded_low.val(), const_u64(15));
                let shifted = block.left_shift(DataType::VU16, rounded_high.val(), const_u64(1));
                let acc_mid = block.or(DataType::VU16, shifted.val(), carried.val());
                let acc_high = block.right_shift(DataType::VS16, rounded_high.val(), const_u64(15));

                guest_regs.set_acc_low(acc_low.val());
                guest_regs.set_acc_mid(acc_mid.val());
                guest_regs.set_acc_high(acc_high.val());

                let result = clamp_acc_to_vd(&mut block, acc_mid.val(), acc_high.val());
                guest_regs.set_vu_reg(instr.cp2_vec_vd(), result);
            }
            // RspOpcode::VEC_VMULQ => todo!("RSP VEC_VMULQ"),
            // RspOpcode::VEC_VMULU => todo!("RSP VEC_VMULU"),
            RspOpcode::VEC_VNAND => {
                let (vs, vte) = vs_and_vte(&mut block, &mut guest_regs, instr);
                let anded = block.and(DataType::VU16, vs, vte);
                let result = block.not(DataType::VU16, anded.val());
                guest_regs.set_acc_low(result.val());
                guest_regs.set_vu_reg(instr.cp2_vec_vd(), result.val());
            }
            // RspOpcode::VEC_VNE => todo!("RSP VEC_VNE"),
            RspOpcode::VEC_VNOP => {}
            RspOpcode::VEC_VNOR => {
                let (vs, vte) = vs_and_vte(&mut block, &mut guest_regs, instr);
                let ored = block.or(DataType::VU16, vs, vte);
                let result = block.not(DataType::VU16, ored.val());
                guest_regs.set_acc_low(result.val());
                guest_regs.set_vu_reg(instr.cp2_vec_vd(), result.val());
            }
            RspOpcode::VEC_VNXOR => {
                let (vs, vte) = vs_and_vte(&mut block, &mut guest_regs, instr);
                let xored = block.xor(DataType::VU16, vs, vte);
                let result = block.not(DataType::VU16, xored.val());
                guest_regs.set_acc_low(result.val());
                guest_regs.set_vu_reg(instr.cp2_vec_vd(), result.val());
            }
            RspOpcode::VEC_VOR => {
                let (vs, vte) = vs_and_vte(&mut block, &mut guest_regs, instr);
                let result = block.or(DataType::VU16, vs, vte);
                guest_regs.set_acc_low(result.val());
                guest_regs.set_vu_reg(instr.cp2_vec_vd(), result.val());
            }
            // RspOpcode::VEC_VRCP => todo!("RSP VEC_VRCP"),
            // RspOpcode::VEC_VRCPH_VRSQH => todo!("RSP VEC_VRCPH_VRSQH"),
            // RspOpcode::VEC_VRCPL => todo!("RSP VEC_VRCPL"),
            // RspOpcode::VEC_VRNDN => todo!("RSP VEC_VRNDN"),
            // RspOpcode::VEC_VRNDP => todo!("RSP VEC_VRNDP"),
            // RspOpcode::VEC_VRSQ => todo!("RSP VEC_VRSQ"),
            // RspOpcode::VEC_VRSQL => todo!("RSP VEC_VRSQL"),
            RspOpcode::VEC_VSAR => {
                // e selects an accumulator half here rather than an element of vt.
                let value = match instr.cp2_vec_e() {
                    0x8 => guest_regs.get_acc_high(&mut block),
                    0x9 => guest_regs.get_acc_mid(&mut block),
                    0xA => guest_regs.get_acc_low(&mut block),
                    _ => const_u128(0),
                };
                guest_regs.set_vu_reg(instr.cp2_vec_vd(), value);
            }
            // RspOpcode::VEC_VSUB => todo!("RSP VEC_VSUB"),
            // RspOpcode::VEC_VSUBC => todo!("RSP VEC_VSUBC"),
            RspOpcode::VEC_VXOR => {
                let (vs, vte) = vs_and_vte(&mut block, &mut guest_regs, instr);
                let result = block.xor(DataType::VU16, vs, vte);
                guest_regs.set_acc_low(result.val());
                guest_regs.set_vu_reg(instr.cp2_vec_vd(), result.val());
            }
            RspOpcode::VEC_VZERO => {
                // vd is zeroed, but the sum still lands in acc.l.
                let (vs, vte) = vs_and_vte(&mut block, &mut guest_regs, instr);
                let sum = block.add(DataType::VU16, vs, vte);
                guest_regs.set_acc_low(sum.val());
                guest_regs.set_vu_reg(instr.cp2_vec_vd(), const_u128(0));
            }
            // RspOpcode::CFC2 => todo!("RSP CFC2"),
            // RspOpcode::CTC2 => todo!("RSP CTC2"),
            // RspOpcode::MFC2 => todo!("RSP MFC2"),
            RspOpcode::MTC2 => {
                let e = instr.cp2_regmove_e();
                let value = guest_regs.get_gpr(&mut block, instr.cp2_regmove_rt());
                let value = block.and(DataType::U32, value, const_u32(0xFFFF));

                // The high byte of rt lands on element e and the low byte on e + 1, so the pair
                // sits at byte 14 - e. At e == 15 that goes negative, which drops the low byte.
                let (placed, mask) = if e < 15 {
                    let shift = const_u16((14 - e) as u16);
                    (
                        block.vector_left_shift_bytes(DataType::U128, value.val(), shift),
                        block.vector_left_shift_bytes(DataType::U128, const_u32(0xFFFF), shift),
                    )
                } else {
                    (
                        block.vector_right_shift_bytes(DataType::U128, value.val(), const_u16(1)),
                        block.vector_right_shift_bytes(
                            DataType::U128,
                            const_u32(0xFFFF),
                            const_u16(1),
                        ),
                    )
                };

                let inv_mask = block.not(DataType::U128, mask.val());
                let reg = guest_regs.get_vu_reg(&mut block, instr.cp2_regmove_rd());
                let kept = block.and(DataType::U128, reg, inv_mask.val());
                let result = block.or(DataType::U128, kept.val(), placed.val());
                guest_regs.set_vu_reg(instr.cp2_regmove_rd(), result.val());
            }
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
            RspOpcode::SRA => {
                let rt = guest_regs.get_gpr(&mut block, instr.rt());
                let result = block.right_shift(DataType::S32, rt, const_u16(instr.sa() as u16));
                guest_regs.set_gpr(instr.rd(), result.val());
            }
            // RspOpcode::SRAV => todo!("RSP SRAV"),
            RspOpcode::SLLV => {
                let rt = guest_regs.get_gpr(&mut block, instr.rt());
                let rs = guest_regs.get_gpr(&mut block, instr.rs());
                let result = block.left_shift(DataType::U32, rt, rs);
                guest_regs.set_gpr(instr.rd(), result.val());
            }
            RspOpcode::SRLV => {
                let rt = guest_regs.get_gpr(&mut block, instr.rt());
                let rs = guest_regs.get_gpr(&mut block, instr.rs());
                let result = block.right_shift(DataType::U32, rt, rs);
                guest_regs.set_gpr(instr.rd(), result.val());
            }
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
            RspOpcode::AND => {
                let rs = guest_regs.get_gpr(&mut block, instr.rs());
                let rt = guest_regs.get_gpr(&mut block, instr.rt());
                let result = block.and(DataType::U32, rs, rt);
                guest_regs.set_gpr(instr.rd(), result.val());
            }
            RspOpcode::SUB => {
                let rs = guest_regs.get_gpr(&mut block, instr.rs());
                let rt = guest_regs.get_gpr(&mut block, instr.rt());
                let result = block.subtract(DataType::S32, rs, rt);
                guest_regs.set_gpr(instr.rd(), result.val());
            }
            RspOpcode::OR => {
                let rs = guest_regs.get_gpr(&mut block, instr.rs());
                let rt = guest_regs.get_gpr(&mut block, instr.rt());
                let result = block.or(DataType::U32, rs, rt);
                guest_regs.set_gpr(instr.rd(), result.val());
            }
            RspOpcode::XOR => {
                let rs = guest_regs.get_gpr(&mut block, instr.rs());
                let rt = guest_regs.get_gpr(&mut block, instr.rt());
                let result = block.xor(DataType::U32, rs, rt);
                guest_regs.set_gpr(instr.rd(), result.val());
            }
            // RspOpcode::NOR => todo!("RSP NOR"),
            RspOpcode::SLT => {
                let rs = guest_regs.get_gpr(&mut block, instr.rs());
                let rt = guest_regs.get_gpr(&mut block, instr.rt());
                let result = block.compare(DataType::S32, rs, CompareType::LessThan, rt);
                guest_regs.set_gpr(instr.rd(), result.val());
            }
            // RspOpcode::SLTU => todo!("RSP SLTU"),
            // RspOpcode::BREAK => todo!("RSP BREAK"),
            // RspOpcode::LBV => todo!("RSP LBV"),
            RspOpcode::LDV => {
                let address =
                    get_lswc2_address(instr, &mut block, &mut guest_regs, SHIFT_AMOUNT_LDV_SDV);
                let e = instr.lswc2_e();

                let value = rsp_load_u64(&mut block, &ctx, address);
                let ones = const_u64(0xFFFFFFFFFFFFFFFF);

                // Past element 8 the bytes shift the other way, and the ones that run off the end
                // are dropped rather than wrapping.
                let (placed, mask) = if e <= 8 {
                    let shift = const_u16((8 - e) as u16);
                    (
                        block.vector_left_shift_bytes(DataType::U128, value, shift),
                        block.vector_left_shift_bytes(DataType::U128, ones, shift),
                    )
                } else {
                    let shift = const_u16((e - 8) as u16);
                    (
                        block.vector_right_shift_bytes(DataType::U128, value, shift),
                        block.vector_right_shift_bytes(DataType::U128, ones, shift),
                    )
                };

                let inv_mask = block.not(DataType::U128, mask.val());
                let reg = guest_regs.get_vu_reg(&mut block, instr.lswc2_vt());
                let masked = block.and(DataType::U128, reg, inv_mask.val());
                let result = block.or(DataType::U128, masked.val(), placed.val());
                guest_regs.set_vu_reg(instr.lswc2_vt(), result.val());
            }
            // RspOpcode::LFV => todo!("RSP LFV"),
            // RspOpcode::LHV => todo!("RSP LHV"),
            // RspOpcode::LLV => todo!("RSP LLV"),
            // RspOpcode::LPV => todo!("RSP LPV"),
            RspOpcode::LQV => {
                let address =
                    get_lswc2_address(instr, &mut block, &mut guest_regs, SHIFT_AMOUNT_LQV_SQV);
                let e = instr.lswc2_e();

                // The access runs from the address to the end of the 16 byte block containing it,
                // so load that whole block and discard what falls outside.
                let aligned = block.and(DataType::U32, address, const_u32(0xFFFFFFF0));
                let aligned_high = aligned.val();
                let aligned_low = block.add(DataType::U32, aligned_high, const_u16(8));
                let high = rsp_load_u64(&mut block, &ctx, aligned_high);
                let high = block.vector_left_shift_bytes(DataType::U128, high, const_u16(8));
                let low = rsp_load_u64(&mut block, &ctx, aligned_low.val());
                let loaded = block.or(DataType::U128, high.val(), low);

                // Shifting left by the misalignment drops the bytes before the address, and
                // shifting right by the element moves the rest into place. Anything past the end
                // of the register falls off both ends, which is exactly the clipping LQV wants.
                let misalignment = block.and(DataType::U32, address, const_u32(15));
                let misalignment = misalignment.val();
                let placed =
                    block.vector_left_shift_bytes(DataType::U128, loaded.val(), misalignment);
                let placed = block.vector_right_shift_bytes(
                    DataType::U128,
                    placed.val(),
                    const_u16(e as u16),
                );

                // The same shifts applied to an all ones value select the bytes actually written.
                let ones = block.vector_left_shift_bytes(
                    DataType::U128,
                    const_u64(u64::MAX),
                    const_u16(8),
                );
                let ones = block.or(DataType::U128, ones.val(), const_u64(u64::MAX));
                let mask = block.vector_left_shift_bytes(DataType::U128, ones.val(), misalignment);
                let mask =
                    block.vector_right_shift_bytes(DataType::U128, mask.val(), const_u16(e as u16));
                let inv_mask = block.not(DataType::U128, mask.val());

                let reg = guest_regs.get_vu_reg(&mut block, instr.lswc2_vt());
                let kept = block.and(DataType::U128, reg, inv_mask.val());
                let result = block.or(DataType::U128, kept.val(), placed.val());
                guest_regs.set_vu_reg(instr.lswc2_vt(), result.val());
            }
            // RspOpcode::LRV => todo!("RSP LRV"),
            // RspOpcode::LSV => todo!("RSP LSV"),
            // RspOpcode::LTV => todo!("RSP LTV"),
            // RspOpcode::LUV => todo!("RSP LUV"),
            // RspOpcode::SBV => todo!("RSP SBV"),
            RspOpcode::SDV => {
                let address =
                    get_lswc2_address(instr, &mut block, &mut guest_regs, SHIFT_AMOUNT_LDV_SDV);
                let e = instr.lswc2_e();
                let reg = guest_regs.get_vu_reg(&mut block, instr.lswc2_vt());

                // Extract 8 bytes from the VU register starting at element,
                // wrapping around with & 0xF, into a u64 value.
                let shift = 8i32 - e as i32;
                let value = if shift > 0 {
                    block
                        .vector_right_shift_bytes(DataType::U128, reg, const_u16(shift as u16))
                        .val()
                } else if shift < 0 {
                    // Wrapping case (element > 8): rotate via left-shift + right-shift + OR
                    let left = block.vector_left_shift_bytes(
                        DataType::U128,
                        reg,
                        const_u16((-shift) as u16),
                    );
                    let right = block.vector_right_shift_bytes(
                        DataType::U128,
                        reg,
                        const_u16((16 + shift) as u16),
                    );
                    block.or(DataType::U128, left.val(), right.val()).val()
                } else {
                    // element == 8: low 64 bits are already in position
                    reg
                };

                // Write as two 32-bit words
                let value = block
                    .convert_from(DataType::U128, DataType::U64, value)
                    .val();
                let high = block.right_shift(DataType::U64, value, const_u16(32));
                block.call_function(ctx.write_word(), &[address, high.val()]);
                let low_address = block.add(DataType::U32, address, const_u16(4));
                block.call_function(ctx.write_word(), &[low_address.val(), value]);
            }
            // RspOpcode::SFV => todo!("RSP SFV"),
            // RspOpcode::SHV => todo!("RSP SHV"),
            // RspOpcode::SLV => todo!("RSP SLV"),
            // RspOpcode::SPV => todo!("RSP SPV"),
            // RspOpcode::SQV => todo!("RSP SQV"),
            // RspOpcode::SRV => todo!("RSP SRV"),
            // RspOpcode::SSV => todo!("RSP SSV"),
            // RspOpcode::STV => todo!("RSP STV"),
            // RspOpcode::SUV => todo!("RSP SUV"),
            // RspOpcode::SWV => todo!("RSP SWV"),
            // Anything not compiled above runs through the interpreter.
            op => interpret_instruction(&mut block, &mut guest_regs, &op, addr, instr),
        }
        cycles += 1;
    }

    if !pc_set {
        // IMEM is 4KB and wraps.
        let next_addr = last_addr.wrapping_add(4) & 0xFFF;
        set_pc(&mut pc_set, &mut block, rsp_address, const_u16(next_addr));
    }

    guest_regs.flush_all(&mut block, true);
    block.ret(Some(const_s32(cycles)));

    func
}

pub fn rsp_to_ir(parsed: Vec<ParsedRspInstruction>, rsp: &rsp_t) -> IRFunction {
    rsp_to_ir_ctx(RspMipsToIrContext::default(), parsed, rsp)
}
