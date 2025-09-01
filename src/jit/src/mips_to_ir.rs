use std::mem::offset_of;

use dgbir::ir::{
    const_ptr, const_s16, const_s32, const_s64, const_u16, const_u32, const_u64, CompareType,
    DataType, IRBlockHandle, IRContext, IRFunction, InputSlot, MultiplyType,
};

use crate::{
    bus_access, bus_access_BUS_LOAD, bus_access_BUS_STORE, cp0_status_updated, do_tlbp, do_tlbwi,
    mips_parser::{
        BranchCondition, BranchInfo, MipsInstructionBitfield, MipsOpcode, ParsedMipsInstruction,
    },
    n64_read_physical_byte, n64_read_physical_dword, n64_read_physical_half,
    n64_read_physical_word, n64_write_physical_byte, n64_write_physical_dword,
    n64_write_physical_half, n64_write_physical_word, n64cpu_ptr, r4300i_t,
    reschedule_compare_interrupt, CP0_ENTRY_HI_WRITE_MASK, CP0_PAGEMASK_WRITE_MASK,
    CP0_STATUS_WRITE_MASK, FCR31_COMPARE_MASK, FCR31_COMPARE_SHIFT, R4300I_CP0_REG_21,
    R4300I_CP0_REG_22, R4300I_CP0_REG_23, R4300I_CP0_REG_24, R4300I_CP0_REG_25, R4300I_CP0_REG_31,
    R4300I_CP0_REG_7, R4300I_CP0_REG_BADVADDR, R4300I_CP0_REG_CACHEER, R4300I_CP0_REG_CAUSE,
    R4300I_CP0_REG_COMPARE, R4300I_CP0_REG_CONFIG, R4300I_CP0_REG_CONTEXT, R4300I_CP0_REG_COUNT,
    R4300I_CP0_REG_ENTRYHI, R4300I_CP0_REG_ENTRYLO0, R4300I_CP0_REG_ENTRYLO1, R4300I_CP0_REG_EPC,
    R4300I_CP0_REG_ERR_EPC, R4300I_CP0_REG_INDEX, R4300I_CP0_REG_LLADDR, R4300I_CP0_REG_PAGEMASK,
    R4300I_CP0_REG_PARITYER, R4300I_CP0_REG_PRID, R4300I_CP0_REG_RANDOM, R4300I_CP0_REG_STATUS,
    R4300I_CP0_REG_TAGHI, R4300I_CP0_REG_TAGLO, R4300I_CP0_REG_WATCHHI, R4300I_CP0_REG_WATCHLO,
    R4300I_CP0_REG_WIRED, R4300I_CP0_REG_XCONTEXT, STATUS_ERL_MASK, STATUS_EXL_MASK,
};

fn is_fr_set() -> bool {
    return unsafe { (*n64cpu_ptr).cp0.status.__bindgen_anon_1.fr() } != 0;
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
enum FgrLoadState {
    Low32,
    High32,
    Full64,
}

struct GuestRegisterManager {
    gprs: [Option<InputSlot>; 32],
    fgrs: [Option<(FgrLoadState, InputSlot)>; 32],
    lo: Option<InputSlot>,
    hi: Option<InputSlot>,
    fcr31: Option<InputSlot>,
    cpu_address: InputSlot,
}

impl GuestRegisterManager {
    pub fn new(cpu_address: InputSlot) -> Self {
        let mut v = GuestRegisterManager {
            gprs: [None; 32],
            fgrs: [None; 32],
            lo: None,
            hi: None,
            fcr31: None,
            cpu_address,
        };
        v.gprs[0] = Some(const_u32(0)); // GPR[0] is always 0
        return v;
    }

    pub fn set_gpr(&mut self, r: u8, value: InputSlot) {
        if r != 0 {
            self.gprs[r as usize] = Some(value);
        }
    }

    fn get_gpr(&mut self, block: &mut IRBlockHandle, r: u8) -> InputSlot {
        *self.gprs[r as usize].get_or_insert_with(|| {
            let offset = offset_of!(r4300i_t, gpr) + (r as usize * std::mem::size_of::<u64>());
            block
                .load_ptr(DataType::U64, self.cpu_address, offset)
                .val()
        })
    }

    fn get_hi(&mut self, block: &mut IRBlockHandle) -> InputSlot {
        *self.hi.get_or_insert_with(|| {
            block
                .load_ptr(
                    DataType::U64,
                    self.cpu_address,
                    offset_of!(r4300i_t, mult_hi),
                )
                .val()
        })
    }

    fn set_hi(&mut self, value: InputSlot) {
        self.hi = Some(value);
    }

    fn get_lo(&mut self, block: &mut IRBlockHandle) -> InputSlot {
        *self.lo.get_or_insert_with(|| {
            block
                .load_ptr(
                    DataType::U64,
                    self.cpu_address,
                    offset_of!(r4300i_t, mult_lo),
                )
                .val()
        })
    }

    fn set_lo(&mut self, value: InputSlot) {
        self.lo = Some(value);
    }

    fn flush_fgr(
        &mut self,
        block: &mut IRBlockHandle,
        r: usize,
        load_state: FgrLoadState,
        value: InputSlot,
    ) {
        match load_state {
            FgrLoadState::Low32 => {
                let offset = offset_of!(r4300i_t, f) + (r * std::mem::size_of::<u64>());
                block.write_ptr(DataType::F32, self.cpu_address, offset, value);
            }
            FgrLoadState::High32 => {
                let offset = offset_of!(r4300i_t, f) + (r * std::mem::size_of::<u64>()) + std::mem::size_of::<u32>();
                block.write_ptr(DataType::F32, self.cpu_address, offset, value);
            }
            FgrLoadState::Full64 => {
                let offset = offset_of!(r4300i_t, f) + (r * std::mem::size_of::<u64>());
                block.write_ptr(DataType::F64, self.cpu_address, offset, value);
            }
        }

        self.fgrs[r as usize] = None;
    }

    fn get_fgr(&mut self, block: &mut IRBlockHandle, r: u8, tp: FgrLoadState) -> InputSlot {
        // Need to flush if the register is already loaded with a different type.
        // Don't need to reload if the register is not loaded at all.
        if let Some((load_state, value)) = self.fgrs[r as usize] {
            match (load_state, tp) {
                // Same state, no need to flush
                (FgrLoadState::Low32, FgrLoadState::Low32) => {}
                (FgrLoadState::Full64, FgrLoadState::Full64) => {}
                (FgrLoadState::High32, FgrLoadState::High32) => {}

                // FGR is loaded with the wrong bits, we need to flush and reload.
                (FgrLoadState::Low32, FgrLoadState::Full64)
                | (FgrLoadState::Low32, FgrLoadState::High32)
                | (FgrLoadState::High32, FgrLoadState::Low32)
                | (FgrLoadState::High32, FgrLoadState::Full64)
                | (FgrLoadState::Full64, FgrLoadState::High32) => {
                    self.flush_fgr(block, r as usize, load_state, value);
                }

                // FGR is loaded with the full 64, but we only need the low 32, this is fine, no
                // need to flush.
                (FgrLoadState::Full64, FgrLoadState::Low32) => {}
            }
        }

        let (_, value) = *self.fgrs[r as usize].get_or_insert_with(|| {
            let v = match tp {
                FgrLoadState::Low32 => block
                    .load_ptr(
                        DataType::F32,
                        self.cpu_address,
                        // ENDIANNESS: this will point at the low 32 bits of the 64 bit FGR
                        offset_of!(r4300i_t, f) + (r as usize * std::mem::size_of::<u64>()),
                    )
                    .val(),
                FgrLoadState::High32 => block
                    .load_ptr(
                        DataType::F32,
                        self.cpu_address,
                        // ENDIANNESS: this will point at the high 32 bits of the 64 bit FGR
                        offset_of!(r4300i_t, f) + (r as usize * std::mem::size_of::<u64>()) + std::mem::size_of::<u32>(),
                    )
                    .val(),
                FgrLoadState::Full64 => block
                    .load_ptr(
                        DataType::F64,
                        self.cpu_address,
                        offset_of!(r4300i_t, f) + (r as usize * std::mem::size_of::<u64>()),
                    )
                    .val(),
            };

            (tp, v)
        });
        return value;
    }

    fn set_fgr(&mut self, r: u8, value: InputSlot, tp: FgrLoadState) {
        self.fgrs[r as usize] = Some((tp, value));
    }

    // Set a 32 bit floating point register, respecting the FR bit.
    fn set_fgr_32bit_fr(&mut self, r: u8, value: InputSlot, block: &mut IRBlockHandle) {
        /*
        if (N64CPU.cp0.status.fr) {
            N64CPU.f[r].lo = value;
        } else {
            if (r & 1) {
                N64CPU.f[r & ~1].hi = value;
            } else {
                N64CPU.f[r].lo = value;
            }
        }
        */
        // Maybe try:
        // if fr || !(r & 1) {
        //   set lo
        // } else {
        //   set hi
        // }
        let fr = is_fr_set();

        if fr {
            todo!("set_fgr_32bit_fr with fr set");
        } else {
            let r_to_set = r & !1;
            let reg = self.get_fgr(block, r_to_set, FgrLoadState::Full64);
            let hi_mask = const_u64(0xFFFFFFFF00000000);
            let lo_mask = const_u64(0x00000000FFFFFFFF);

            if (r & 1) != 0 {
                let shifted_value = block.left_shift(DataType::U64, value, const_u16(32));
                let masked_reg = block.and(DataType::U64, reg, lo_mask);
                let result = block.or(DataType::U64, masked_reg.val(), shifted_value.val());
                self.set_fgr(r_to_set, result.val(), FgrLoadState::Full64);
            } else {
                let masked_value = block.and(DataType::U64, value, lo_mask);
                let masked_reg = block.and(DataType::U64, reg, hi_mask);
                let result = block.or(DataType::U64, masked_value.val(), masked_reg.val());
                self.set_fgr(r_to_set, result.val(), FgrLoadState::Full64);
            }
        }
    }

    fn set_fgr_64bit(&mut self, r: u8, value: InputSlot) {
        // No need to flush in this one since we're writing the full register.
        self.set_fgr(r, value, FgrLoadState::Full64);
    }

    fn get_fgr_32bit_fs(&mut self, block: &mut IRBlockHandle, fs: u8) -> InputSlot {
        let fs = if !is_fr_set() { fs & !1 } else { fs };

        return self.get_fgr(block, fs, FgrLoadState::Low32);
    }

    fn get_fgr_64bit_fr(&mut self, block: &mut IRBlockHandle, r: u8) -> InputSlot {
        let r = if !is_fr_set() { r & !1 } else { r };
        return self.get_fgr(block, r, FgrLoadState::Full64);
    }

    fn get_fgr_64bit_fs(&mut self, block: &mut IRBlockHandle, fs: u8) -> InputSlot {
        return self.get_fgr_64bit_fr(block, fs);
    }

    fn get_fgr_64bit_ft(&mut self, block: &mut IRBlockHandle, ft: u8) -> InputSlot {
        return self.get_fgr(block, ft, FgrLoadState::Full64);
    }

    fn get_fgr_32bit_ft(&mut self, block: &mut IRBlockHandle, ft: u8) -> InputSlot {
        return self.get_fgr(block, ft, FgrLoadState::Low32);
    }

    fn get_fgr_32bit_fr(&mut self, block: &mut IRBlockHandle, r: u8) -> InputSlot {
        let fr = is_fr_set();
        if fr {
            return self.get_fgr(block, r, FgrLoadState::Low32);
        } else {
            let r_to_get = r & !1;
            if (r & 1) != 0 {
                return self.get_fgr(block, r_to_get, FgrLoadState::High32);
            } else {
                return self.get_fgr(block, r_to_get, FgrLoadState::Low32);
            }
        }
    }

    fn get_fcr31(&mut self, block: &mut IRBlockHandle) -> InputSlot {
        *self.fcr31.get_or_insert_with(|| {
            block
                .load_ptr(
                    DataType::S32,
                    self.cpu_address,
                    offset_of!(r4300i_t, fcr31.raw),
                )
                .val()
        })
    }

    fn set_fcr31(&mut self, value: InputSlot) {
        self.fcr31 = Some(value);
    }

    fn get_fcr31_compare(&mut self, block: &mut IRBlockHandle) -> InputSlot {
        let fcr31 = self.get_fcr31(block);
        let masked = block.and(DataType::U32, fcr31, const_u32(FCR31_COMPARE_MASK));
        let shifted =
            block.right_shift(DataType::U32, masked.val(), const_u32(FCR31_COMPARE_SHIFT));
        return shifted.val();
    }

    fn set_fcr31_compare(&mut self, block: &mut IRBlockHandle, value: InputSlot) {
        let fcr31 = self.get_fcr31(block);

        let masked = block.and(DataType::U32, fcr31, const_u32(!FCR31_COMPARE_MASK));

        let shifted = block.left_shift(DataType::U32, value, const_u32(FCR31_COMPARE_SHIFT));

        let result = block.or(DataType::U32, masked.val(), shifted.val());

        self.set_fcr31(result.val());
    }

    fn flush_all(&mut self, block: &mut IRBlockHandle) {
        self.gprs
            .iter_mut()
            .enumerate()
            .filter(|(i, reg)| *i != 0 && reg.is_some())
            .for_each(|(i, reg)| {
                if let Some(value) = reg.take() {
                    let offset = offset_of!(r4300i_t, gpr) + (i * std::mem::size_of::<u64>());
                    block.write_ptr(DataType::U64, self.cpu_address, offset, value);
                }
            });

        let to_flush = self
            .fgrs
            .iter()
            .enumerate()
            .filter(|(_, reg)| reg.is_some())
            .map(|(r, _)| r)
            .collect::<Vec<_>>();

        to_flush.into_iter().for_each(|r| {
            if let Some((load_state, value)) = self.fgrs[r].take() {
                self.flush_fgr(block, r, load_state, value);
            }
        });

        if let Some(value) = self.lo.take() {
            block.write_ptr(
                DataType::U64,
                self.cpu_address,
                offset_of!(r4300i_t, mult_lo),
                value,
            );
        }

        if let Some(value) = self.hi.take() {
            block.write_ptr(
                DataType::U64,
                self.cpu_address,
                offset_of!(r4300i_t, mult_hi),
                value,
            );
        }

        if let Some(value) = self.fcr31.take() {
            block.write_ptr(
                DataType::U32,
                self.cpu_address,
                offset_of!(r4300i_t, fcr31.raw),
                value,
            );
        }
    }
}

fn get_paddr_for_loadstore(
    cpu: &r4300i_t,
    guest_regs: &mut GuestRegisterManager,
    func: &IRFunction,
    block: &mut IRBlockHandle,
    instr: MipsInstructionBitfield,
    bus_access: bus_access,
) -> InputSlot {
    let base = guest_regs.get_gpr(block, instr.rs());
    let virtual_address = block.add(DataType::U64, base, const_s16(instr.s_imm()));

    static mut physical: u32 = 0;
    static mut cached: bool = false;

    let physical_ptr = const_ptr(&raw const physical as usize);
    let cached_ptr = const_ptr(&raw const cached as usize);

    let resolve_virtual = const_ptr(cpu.cp0.resolve_virtual_address.unwrap() as usize);

    fn on_fail(vaddr: u64) {
        panic!("Failed to resolve virtual address 0x{:016X}", vaddr);
    }

    let success = block.call_function(
        resolve_virtual,
        Some(DataType::Bool),
        vec![
            virtual_address.val(),
            const_u32(bus_access as u32), // on Windows, this is an i32, need to convert.
            cached_ptr,
            physical_ptr,
        ],
    );

    let mut on_fail_block = func.new_block(vec![]);
    on_fail_block.call_function(
        const_ptr(on_fail as usize),
        None,
        vec![virtual_address.val()],
    );
    on_fail_block.ret(None);

    let on_success_block = func.new_block(vec![]);
    block.branch(
        success.val(),
        on_success_block.call(vec![]),
        on_fail_block.call(vec![]),
    );
    *block = on_success_block;

    return block.load_ptr(DataType::U32, physical_ptr, 0).val();
}

fn set_pc(
    pc_set_flag: &mut bool,
    block: &mut IRBlockHandle,
    cpu_address: InputSlot,
    value: InputSlot,
) {
    *pc_set_flag = true;
    let offset = offset_of!(r4300i_t, pc);
    let next_pc_offset = offset_of!(r4300i_t, next_pc);

    block.write_ptr(DataType::U64, cpu_address, offset, value);
    let next_pc = block.add(DataType::U64, value, const_u32(4));
    block.write_ptr(DataType::U64, cpu_address, next_pc_offset, next_pc.val());
}

fn set_link_reg(guest_regs: &mut GuestRegisterManager, vaddr: u64, mips_reg: u8) {
    // Skip the delay slot on return
    let vaddr = vaddr.wrapping_add(8);
    guest_regs.set_gpr(mips_reg, const_u64(vaddr));
}

fn checkcp1(
    _block: &mut IRBlockHandle,
    _guest_regs: &mut GuestRegisterManager,
    _preserve_cause: bool,
) {
    println!("TODO: check if CP1 is enabled in the JIT")
}

fn do_branch(
    link: bool,
    likely: bool,
    guest_regs: &mut GuestRegisterManager,
    vaddr: u64,
    func: &IRFunction,
    take_branch: InputSlot,
    instr: MipsInstructionBitfield,
    cpu_address: InputSlot,
    pc_set: &mut bool,
    block: &mut IRBlockHandle,
    cycles: i32,
) {
    if link {
        set_link_reg(guest_regs, vaddr, 31);
    }

    let mut taken_block = func.new_block(vec![]);
    let mut not_taken_block = func.new_block(vec![]);

    let taken_pc = vaddr
        .wrapping_add(4)
        .wrapping_add_signed((instr.s_imm() as i64) << 2);
    let not_taken_pc = vaddr.wrapping_add(8);

    println!(
        "Jumping to {:016X} if taken, continuing to {:016X} if not taken",
        taken_pc, not_taken_pc
    );

    set_pc(pc_set, &mut taken_block, cpu_address, const_u64(taken_pc));
    set_pc(
        pc_set,
        &mut not_taken_block,
        cpu_address,
        const_u64(not_taken_pc),
    );

    if likely {
        // For likely branches, flush all the regs here so we don't have to do it twice
        // (if the branch is taken)
        // Regs needed by the delay slot instruction will be reloaded
        // TODO: it'd be best to somehow not flush registers needed by the delay slot
        // instruction
        guest_regs.flush_all(block);
    }

    block.branch(
        take_branch,
        taken_block.call(vec![]),
        not_taken_block.call(vec![]),
    );

    *block = func.new_block(vec![]);

    taken_block.jump(block.call(vec![]));
    if likely {
        // Likely branches, return, don't execute the delay slot.
        not_taken_block.ret(Some(const_s32(cycles + 1)));
    } else {
        // Normal branches, continue and execute the delay slot.
        not_taken_block.jump(block.call(vec![]));
    }
}

fn do_fpu_compare(instr : &MipsInstructionBitfield, block: &mut IRBlockHandle, guest_regs: &mut GuestRegisterManager, ctp: CompareType) {
    match instr.fmt_datatype() {
        Some(DataType::F32) => {
            let fs = guest_regs.get_fgr_32bit_fs(block, instr.fs());
            let ft = guest_regs.get_fgr_32bit_ft(block, instr.ft());
            let result = block.compare(DataType::F32, fs, ctp, ft);
            guest_regs.set_fcr31_compare(block, result.val());
        }
        Some(DataType::F64) => {
            let fs = guest_regs.get_fgr_64bit_fs(block, instr.fs());
            let ft = guest_regs.get_fgr_64bit_ft(block, instr.ft());
            let result = block.compare(DataType::F64, fs, ctp, ft);
            guest_regs.set_fcr31_compare(block, result.val());
        }
        _ => panic!( "Unsupported datatype for FPU compare: {:?}", instr.fmt_datatype()),
    }
}

pub fn to_ir(parsed: Vec<ParsedMipsInstruction>, cpu: &r4300i_t) -> IRFunction {
    let context = IRContext::new();
    let func = IRFunction::new(context);
    let mut block = func.new_block(vec![DataType::Ptr]);

    let cpu_address = block.input(0);

    let mut guest_regs = GuestRegisterManager::new(cpu_address);

    let mut cycles = 0;
    let mut pc_set = false;

    let mut last_vaddr = 0;

    for (
        index,
        ParsedMipsInstruction {
            paddr: _paddr,
            vaddr,
            instr,
            op,
        },
    ) in parsed.into_iter().enumerate()
    {
        last_vaddr = vaddr;
        block.comment(format!("{:016X}: {:?}", vaddr, op));
        match op {
            MipsOpcode::NOP => {}
            MipsOpcode::LD => {
                let paddr = get_paddr_for_loadstore(
                    cpu,
                    &mut guest_regs,
                    &func,
                    &mut block,
                    instr,
                    bus_access_BUS_LOAD,
                );
                let value = block.call_function(
                    const_ptr(n64_read_physical_dword as usize),
                    Some(DataType::S64),
                    vec![paddr],
                );

                guest_regs.set_gpr(instr.rt(), value.val());
            }
            MipsOpcode::LUI => {
                let c = (instr.imm() as u32) << 16;
                guest_regs.set_gpr(instr.rt(), const_s32(c as i32));
            }
            MipsOpcode::ADDI => {
                let rs = guest_regs.get_gpr(&mut block, instr.rs());
                let result = block.add(DataType::S32, rs, const_s16(instr.s_imm()));
                let sign_extended = block.convert(DataType::S64, result.val());
                guest_regs.set_gpr(instr.rt(), sign_extended.val());
            }
            MipsOpcode::ADDIU => {
                // Identical to ADDI, but does not throw overflow exceptions (which are not
                // implemented yet anyway)
                let rs = guest_regs.get_gpr(&mut block, instr.rs());
                let result = block.add(DataType::S32, rs, const_s16(instr.s_imm()));
                let sign_extended = block.convert(DataType::S64, result.val());
                guest_regs.set_gpr(instr.rt(), sign_extended.val());
            }
            MipsOpcode::DADDI => {
                let rs = guest_regs.get_gpr(&mut block, instr.rs());
                let result = block.add(DataType::S64, rs, const_s16(instr.s_imm()));
                guest_regs.set_gpr(instr.rt(), result.val());
            }
            MipsOpcode::ANDI => {
                let rs = guest_regs.get_gpr(&mut block, instr.rs());
                let result = block.and(DataType::U64, rs, const_u16(instr.imm()));
                guest_regs.set_gpr(instr.rt(), result.val());
            }
            MipsOpcode::LBU => {
                let paddr = get_paddr_for_loadstore(
                    cpu,
                    &mut guest_regs,
                    &func,
                    &mut block,
                    instr,
                    bus_access_BUS_LOAD,
                );

                let value = block.call_function(
                    const_ptr(n64_read_physical_byte as usize),
                    Some(DataType::U8),
                    vec![paddr],
                );

                guest_regs.set_gpr(instr.rt(), value.val());
            }
            MipsOpcode::LHU => {
                let paddr = get_paddr_for_loadstore(
                    cpu,
                    &mut guest_regs,
                    &func,
                    &mut block,
                    instr,
                    bus_access_BUS_LOAD,
                );

                let value = block.call_function(
                    const_ptr(n64_read_physical_half as usize),
                    Some(DataType::U16),
                    vec![paddr],
                );

                guest_regs.set_gpr(instr.rt(), value.val());
            }
            MipsOpcode::LH => {
                let paddr = get_paddr_for_loadstore(
                    cpu,
                    &mut guest_regs,
                    &func,
                    &mut block,
                    instr,
                    bus_access_BUS_LOAD,
                );

                let value = block.call_function(
                    const_ptr(n64_read_physical_half as usize),
                    Some(DataType::S16),
                    vec![paddr],
                );

                let sign_extended = block.convert(DataType::S64, value.val());

                guest_regs.set_gpr(instr.rt(), sign_extended.val());
            }
            MipsOpcode::LW => {
                let paddr = get_paddr_for_loadstore(
                    cpu,
                    &mut guest_regs,
                    &func,
                    &mut block,
                    instr,
                    bus_access_BUS_LOAD,
                );
                let temp_value = block.call_function(
                    const_ptr(n64_read_physical_word as usize),
                    Some(DataType::S32),
                    vec![paddr],
                );

                let sign_extended = block.convert(DataType::S64, temp_value.val());

                guest_regs.set_gpr(instr.rt(), sign_extended.val());
            }
            MipsOpcode::LWU => {
                let paddr = get_paddr_for_loadstore(
                    cpu,
                    &mut guest_regs,
                    &func,
                    &mut block,
                    instr,
                    bus_access_BUS_LOAD,
                );

                let value = block.call_function(
                    const_ptr(n64_read_physical_word as usize),
                    Some(DataType::U32),
                    vec![paddr],
                );

                guest_regs.set_gpr(instr.rt(), value.val());
            }
            MipsOpcode::BRANCH(BranchInfo { cond, likely, link }) => {
                if link {
                    set_link_reg(&mut guest_regs, vaddr, 31);
                }

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

                let tp = if signed { DataType::S64 } else { DataType::U64 };
                let take_branch = block.compare(tp, rs, compare_type, rt);

                do_branch(
                    link,
                    likely,
                    &mut guest_regs,
                    vaddr,
                    &func,
                    take_branch.val(),
                    instr,
                    cpu_address,
                    &mut pc_set,
                    &mut block,
                    cycles,
                );
            }
            MipsOpcode::CACHE => {
                println!("TODO: Cache in the JIT (NOP for now)")
            }
            MipsOpcode::SB => {
                let paddr = get_paddr_for_loadstore(
                    cpu,
                    &mut guest_regs,
                    &func,
                    &mut block,
                    instr,
                    bus_access_BUS_STORE,
                );
                let to_write = guest_regs.get_gpr(&mut block, instr.rt());
                block.call_function(
                    const_ptr(n64_write_physical_byte as usize),
                    None,
                    vec![paddr, to_write],
                );
            }
            MipsOpcode::SH => {
                let paddr = get_paddr_for_loadstore(
                    cpu,
                    &mut guest_regs,
                    &func,
                    &mut block,
                    instr,
                    bus_access_BUS_STORE,
                );
                let to_write = guest_regs.get_gpr(&mut block, instr.rt());
                block.call_function(
                    const_ptr(n64_write_physical_half as usize),
                    None,
                    vec![paddr, to_write],
                );
            }
            MipsOpcode::SD => {
                let paddr = get_paddr_for_loadstore(
                    cpu,
                    &mut guest_regs,
                    &func,
                    &mut block,
                    instr,
                    bus_access_BUS_STORE,
                );
                let to_write = guest_regs.get_gpr(&mut block, instr.rt());
                block.call_function(
                    const_ptr(n64_write_physical_dword as usize),
                    None,
                    vec![paddr, to_write],
                );
            }
            MipsOpcode::SW => {
                let paddr = get_paddr_for_loadstore(
                    cpu,
                    &mut guest_regs,
                    &func,
                    &mut block,
                    instr,
                    bus_access_BUS_STORE,
                );
                let to_write = guest_regs.get_gpr(&mut block, instr.rt());
                block.call_function(
                    const_ptr(n64_write_physical_word as usize),
                    None,
                    vec![paddr, to_write],
                );
            }
            MipsOpcode::ORI => {
                let rs = guest_regs.get_gpr(&mut block, instr.rs());
                let result = block.or(DataType::U64, rs, const_u16(instr.imm()));
                guest_regs.set_gpr(instr.rt(), result.val());
            }
            MipsOpcode::J => {
                let upper_bits = vaddr & 0xFFFFFFFFF0000000;
                let target = (instr.j_target() as u64) << 2 | upper_bits;

                set_pc(&mut pc_set, &mut block, cpu_address, const_u64(target));
            }
            MipsOpcode::JAL => {
                set_link_reg(&mut guest_regs, vaddr, 31);
                let upper_bits = vaddr & 0xFFFFFFFFF0000000;
                let target = (instr.j_target() as u64) << 2 | upper_bits;

                set_pc(&mut pc_set, &mut block, cpu_address, const_u64(target));
            }
            MipsOpcode::SLTI => {
                let rs = guest_regs.get_gpr(&mut block, instr.rs());
                let simm = const_s16(instr.s_imm());
                let result = block.compare(DataType::S64, rs, CompareType::LessThan, simm);
                guest_regs.set_gpr(instr.rt(), result.val());
            }
            MipsOpcode::SLTIU => {
                let rs = guest_regs.get_gpr(&mut block, instr.rs());
                let simm = const_s16(instr.s_imm());
                let result = block.compare(DataType::U64, rs, CompareType::LessThan, simm);
                guest_regs.set_gpr(instr.rt(), result.val());
            }
            MipsOpcode::XORI => {
                let rs = guest_regs.get_gpr(&mut block, instr.rs());
                let result = block.xor(DataType::U64, rs, const_u16(instr.imm()));
                guest_regs.set_gpr(instr.rt(), result.val());
            }
            MipsOpcode::DADDIU => {
                // Identical to DADDI, but does not throw overflow exceptions (which are not
                // implemented yet anyway)
                let rs = guest_regs.get_gpr(&mut block, instr.rs());
                let result = block.add(DataType::S64, rs, const_s16(instr.s_imm()));
                guest_regs.set_gpr(instr.rt(), result.val());
            }
            MipsOpcode::LB => {
                let paddr = get_paddr_for_loadstore(
                    cpu,
                    &mut guest_regs,
                    &func,
                    &mut block,
                    instr,
                    bus_access_BUS_LOAD,
                );

                let value = block.call_function(
                    const_ptr(n64_read_physical_byte as usize),
                    Some(DataType::S8),
                    vec![paddr],
                );

                let sign_extended = block.convert(DataType::S64, value.val());

                guest_regs.set_gpr(instr.rt(), sign_extended.val());
            }
            MipsOpcode::LDC1 => {
                checkcp1(&mut block, &mut guest_regs, false);
                let paddr = get_paddr_for_loadstore(
                    cpu,
                    &mut guest_regs,
                    &func,
                    &mut block,
                    instr,
                    bus_access_BUS_LOAD,
                );
                let value = block.call_function(
                    const_ptr(n64_read_physical_dword as usize),
                    Some(DataType::U64),
                    vec![paddr],
                );

                guest_regs.set_fgr_64bit(instr.ft(), value.val());
            }
            MipsOpcode::SDC1 => {
                checkcp1(&mut block, &mut guest_regs, false);
                let paddr = get_paddr_for_loadstore(
                    cpu,
                    &mut guest_regs,
                    &func,
                    &mut block,
                    instr,
                    bus_access_BUS_STORE,
                );

                let value = guest_regs.get_fgr_64bit_fr(&mut block, instr.ft());
                // Convert from u64 to u64 to ensure we're in a GPR
                let value_converted = block.convert_from(DataType::U64, DataType::U64, value);
                block.call_function(
                    const_ptr(n64_write_physical_dword as usize),
                    None,
                    vec![paddr, value_converted.val()],
                );
            }
            MipsOpcode::LWC1 => {
                checkcp1(&mut block, &mut guest_regs, false);
                let paddr = get_paddr_for_loadstore(
                    cpu,
                    &mut guest_regs,
                    &func,
                    &mut block,
                    instr,
                    bus_access_BUS_LOAD,
                );
                let value = block.call_function(
                    const_ptr(n64_read_physical_word as usize),
                    Some(DataType::S32),
                    vec![paddr],
                );

                guest_regs.set_fgr_32bit_fr(instr.ft(), value.val(), &mut block);
            }
            MipsOpcode::SWC1 => {
                checkcp1(&mut block, &mut guest_regs, false);
                let paddr = get_paddr_for_loadstore(
                    cpu,
                    &mut guest_regs,
                    &func,
                    &mut block,
                    instr,
                    bus_access_BUS_STORE,
                );

                let value = guest_regs.get_fgr_32bit_fr(&mut block, instr.ft());
                // Convert from u32 to u32 to ensure we're in a GPR
                let value_converted = block.convert_from(DataType::U32, DataType::U32, value);
                block.call_function(
                    const_ptr(n64_write_physical_word as usize),
                    None,
                    vec![paddr, value_converted.val()],
                );
            }
            MipsOpcode::LWL => {
                let paddr = get_paddr_for_loadstore(
                    cpu,
                    &mut guest_regs,
                    &func,
                    &mut block,
                    instr,
                    bus_access_BUS_LOAD,
                );
                //u32 shift = (physical & 3) << 3;
                let masked_physical = block.and(DataType::U32, paddr, const_u32(3)).val();
                let shift = block.left_shift(DataType::U32, masked_physical, const_u32(3));
                //u32 mask = 0xFFFFFFFF << shift;
                let mask = block.left_shift(DataType::U32, const_u32(0xFFFFFFFF), shift.val());

                //u32 data = n64_read_physical_word(physical & ~3);
                let load_addr = block.and(DataType::U32, paddr, const_u32(!3));
                let data = block.call_function(
                    const_ptr(n64_read_physical_word as usize),
                    Some(DataType::U32),
                    vec![load_addr.val()],
                );

                //s32 result = (get_register(instruction.i.rt) & ~mask) | data << shift;
                //set_register(instruction.i.rt, (s64)result);
                let reg = guest_regs.get_gpr(&mut block, instr.rt());

                let inverse_mask = block.not(DataType::U32, mask.val());
                let reg_masked = block.and(DataType::U32, reg, inverse_mask.val());
                let shifted_data = block.left_shift(DataType::U32, data.val(), shift.val());
                let result = block.or(DataType::U32, reg_masked.val(), shifted_data.val());
                let sign_extended = block.convert_from(DataType::S32, DataType::S64, result.val());
                guest_regs.set_gpr(instr.rt(), sign_extended.val());
            }
            MipsOpcode::LWR => {
                let paddr = get_paddr_for_loadstore(
                    cpu,
                    &mut guest_regs,
                    &func,
                    &mut block,
                    instr,
                    bus_access_BUS_LOAD,
                );
                //u32 shift = ((address ^ 3) & 3) << 3;
                let xored_physical = block.xor(DataType::U32, paddr, const_u32(3)).val();
                let masked_physical = block.and(DataType::U32, xored_physical, const_u32(3)).val();
                let shift = block.left_shift(DataType::U32, masked_physical, const_u32(3));

                //u32 mask = 0xFFFFFFFF >> shift;
                let mask = block.right_shift(DataType::U32, const_u32(0xFFFFFFFF), shift.val());
                //u32 data = n64_read_physical_word(physical & ~3);
                let load_addr = block.and(DataType::U32, paddr, const_u32(!3));
                let data = block.call_function(
                    const_ptr(n64_read_physical_word as usize),
                    Some(DataType::U32),
                    vec![load_addr.val()],
                );
                //s32 result = (get_register(instruction.i.rt) & ~mask) | data >> shift;
                //set_register(instruction.i.rt, (s64)result);
                let reg = guest_regs.get_gpr(&mut block, instr.rt());
                let inverse_mask = block.not(DataType::U32, mask.val());
                let reg_masked = block.and(DataType::U32, reg, inverse_mask.val());
                let shifted_data = block.right_shift(DataType::U32, data.val(), shift.val());
                let result = block.or(DataType::U32, reg_masked.val(), shifted_data.val());
                let sign_extended = block.convert_from(DataType::S32, DataType::S64, result.val());
                guest_regs.set_gpr(instr.rt(), sign_extended.val());
            }
            MipsOpcode::SWL => {
                let physical = get_paddr_for_loadstore(
                    cpu,
                    &mut guest_regs,
                    &func,
                    &mut block,
                    instr,
                    bus_access_BUS_STORE,
                );
                //u32 shift = (physical & 3) << 3;
                let masked_physical = block.and(DataType::U32, physical, const_u32(3)).val();
                let shift = block.left_shift(DataType::U32, masked_physical, const_u32(3));
                // u32 mask = 0xFFFFFFFF >> shift;
                let mask = block.right_shift(DataType::U32, const_u32(0xFFFFFFFF), shift.val());

                //u32 data = n64_read_physical_word(physical & ~3);
                let data_addr = block.and(DataType::U32, physical, const_u32(!3));
                let data = block.call_function(
                    const_ptr(n64_read_physical_word as usize),
                    Some(DataType::U32),
                    vec![data_addr.val()],
                );

                //u32 oldreg = get_register(instruction.i.rt);
                let oldreg = guest_regs.get_gpr(&mut block, instr.rt());
                //n64_write_physical_word(physical & ~3, (data & ~mask) | (oldreg >> shift));
                let inverse_mask = block.not(DataType::U32, mask.val());
                let masked_data = block.and(DataType::U32, data.val(), inverse_mask.val());
                let shifted_reg = block.right_shift(DataType::U32, oldreg, shift.val());
                let result = block.or(DataType::U32, masked_data.val(), shifted_reg.val());
                block.call_function(
                    const_ptr(n64_write_physical_word as usize),
                    None,
                    vec![data_addr.val(), result.val()],
                );
            }
            MipsOpcode::SWR => {
                //ir_instruction_t* physical = ir_get_memory_access_address(index, instruction, BUS_STORE);
                let physical = get_paddr_for_loadstore(
                    cpu,
                    &mut guest_regs,
                    &func,
                    &mut block,
                    instr,
                    bus_access_BUS_STORE,
                );
                //u32 shift = ((address ^ 3) & 3) << 3;
                let xored_physical = block.xor(DataType::U32, physical, const_u32(3)).val();
                let masked_physical = block.and(DataType::U32, xored_physical, const_u32(3)).val();
                let shift = block.left_shift(DataType::U32, masked_physical, const_u32(3));
                //u32 mask = 0xFFFFFFFF << shift;
                let mask = block.left_shift(DataType::U32, const_u32(0xFFFFFFFF), shift.val());
                //u32 data = n64_read_physical_word(physical & ~3);
                let data_addr = block.and(DataType::U32, physical, const_u32(!3));
                let data = block.call_function(
                    const_ptr(n64_read_physical_word as usize),
                    Some(DataType::U32),
                    vec![data_addr.val()],
                );
                //u32 oldreg = get_register(instruction.i.rt);
                let oldreg = guest_regs.get_gpr(&mut block, instr.rt());
                //n64_write_physical_word(physical & ~3, (data & ~mask) | oldreg << shift);
                let inverse_mask = block.not(DataType::U32, mask.val());
                let masked_data = block.and(DataType::U32, data.val(), inverse_mask.val());
                let shifted_reg = block.left_shift(DataType::U32, oldreg, shift.val());
                let result = block.or(DataType::U32, masked_data.val(), shifted_reg.val());
                block.call_function(
                    const_ptr(n64_write_physical_word as usize),
                    None,
                    vec![data_addr.val(), result.val()],
                );
            }
            MipsOpcode::LDL => {
                let physical = get_paddr_for_loadstore(
                    cpu,
                    &mut guest_regs,
                    &func,
                    &mut block,
                    instr,
                    bus_access_BUS_LOAD,
                );
                //u32 shift = ((address ^ 0) & 7) << 3;
                let masked_physical = block.and(DataType::U32, physical, const_u32(7)).val();
                let shift = block.left_shift(DataType::U32, masked_physical, const_u32(3));
                //u64 mask = (u64) 0xFFFFFFFFFFFFFFFF << shift;
                let mask =
                    block.left_shift(DataType::U64, const_u64(0xFFFFFFFFFFFFFFFF), shift.val());

                //u64 data = n64_read_physical_dword(physical & ~7);
                let load_addr = block.and(DataType::U32, physical, const_u32(!7));
                let data = block.call_function(
                    const_ptr(n64_read_physical_dword as usize),
                    Some(DataType::U64),
                    vec![load_addr.val()],
                );

                //u64 result = (get_register(instruction.i.rt) & ~mask) | (data << shift);
                //set_register(instruction.i.rt, result);
                let reg = guest_regs.get_gpr(&mut block, instr.rt());
                let inverse_mask = block.not(DataType::U64, mask.val());
                let reg_masked = block.and(DataType::U64, reg, inverse_mask.val());
                let shifted_data = block.left_shift(DataType::U64, data.val(), shift.val());
                let result = block.or(DataType::U64, reg_masked.val(), shifted_data.val());
                guest_regs.set_gpr(instr.rt(), result.val());
            }
            MipsOpcode::LDR => {
                let physical = get_paddr_for_loadstore(
                    cpu,
                    &mut guest_regs,
                    &func,
                    &mut block,
                    instr,
                    bus_access_BUS_LOAD,
                );
                //u32 shift = ((address ^ 7) & 7) << 3;
                let xored_physical = block.xor(DataType::U32, physical, const_u32(7)).val();
                let masked_physical = block.and(DataType::U32, xored_physical, const_u32(7)).val();
                let shift = block.left_shift(DataType::U32, masked_physical, const_u32(3));
                //u64 mask = (u64) 0xFFFFFFFFFFFFFFFF >> shift;
                let mask =
                    block.right_shift(DataType::U64, const_u64(0xFFFFFFFFFFFFFFFF), shift.val());

                //u64 data = n64_read_physical_dword(physical & ~7);
                let load_addr = block.and(DataType::U32, physical, const_u32(!7));
                let data = block.call_function(
                    const_ptr(n64_read_physical_dword as usize),
                    Some(DataType::U64),
                    vec![load_addr.val()],
                );

                //u64 result = (get_register(instruction.i.rt) & ~mask) | (data >> shift);
                //set_register(instruction.i.rt, result);
                let reg = guest_regs.get_gpr(&mut block, instr.rt());
                let inverse_mask = block.not(DataType::U64, mask.val());
                let reg_masked = block.and(DataType::U64, reg, inverse_mask.val());
                let shifted_data = block.right_shift(DataType::U64, data.val(), shift.val());
                let result = block.or(DataType::U64, reg_masked.val(), shifted_data.val());
                guest_regs.set_gpr(instr.rt(), result.val());
            }
            MipsOpcode::SDL => {
                todo!("SDL")
            }
            MipsOpcode::SDR => {
                todo!("SDR")
            }
            MipsOpcode::LL => {
                todo!("LL")
            }
            MipsOpcode::LLD => {
                todo!("LLD")
            }
            MipsOpcode::SC => {
                todo!("SC")
            }
            MipsOpcode::SCD => {
                todo!("SCD")
            }
            MipsOpcode::RDHWR => {
                todo!("RDHWR")
            }
            MipsOpcode::MFC0 => match instr.rd() as u32 {
                R4300I_CP0_REG_ENTRYHI => {
                    let result = block.load_ptr(
                        DataType::S32,
                        cpu_address,
                        offset_of!(r4300i_t, cp0.entry_hi.raw),
                    );
                    let sign_extended = block.convert(DataType::S64, result.val());
                    guest_regs.set_gpr(instr.rt(), sign_extended.val());
                }
                R4300I_CP0_REG_STATUS => {
                    let result = block.load_ptr(
                        DataType::S32,
                        cpu_address,
                        offset_of!(r4300i_t, cp0.status.raw),
                    );
                    let sign_extended = block.convert(DataType::S64, result.val());
                    guest_regs.set_gpr(instr.rt(), sign_extended.val());
                }
                R4300I_CP0_REG_TAGLO => {
                    todo!("MFC0 R4300I_CP0_REG_TAGLO")
                }
                R4300I_CP0_REG_TAGHI => {
                    todo!("MFC0 R4300I_CP0_REG_TAGHI")
                }
                R4300I_CP0_REG_CAUSE => {
                    let result = block.load_ptr(
                        DataType::S32,
                        cpu_address,
                        offset_of!(r4300i_t, cp0.cause.raw),
                    );
                    let sign_extended = block.convert(DataType::S64, result.val());
                    guest_regs.set_gpr(instr.rt(), sign_extended.val());
                }
                R4300I_CP0_REG_COMPARE => {
                    let result = block.load_ptr(
                        DataType::S32,
                        cpu_address,
                        offset_of!(r4300i_t, cp0.compare),
                    );
                    let sign_extended = block.convert(DataType::S64, result.val());
                    guest_regs.set_gpr(instr.rt(), sign_extended.val());
                }
                R4300I_CP0_REG_ENTRYLO0 => {
                    todo!("MFC0 R4300I_CP0_REG_ENTRYLO0")
                }
                R4300I_CP0_REG_ENTRYLO1 => {
                    todo!("MFC0 R4300I_CP0_REG_ENTRYLO1")
                }
                R4300I_CP0_REG_PAGEMASK => {
                    todo!("MFC0 R4300I_CP0_REG_PAGEMASK")
                }
                R4300I_CP0_REG_EPC => {
                    let result =
                        block.load_ptr(DataType::S32, cpu_address, offset_of!(r4300i_t, cp0.EPC));
                    let sign_extended = block.convert(DataType::S64, result.val());
                    guest_regs.set_gpr(instr.rt(), sign_extended.val());
                }
                R4300I_CP0_REG_CONFIG => {
                    todo!("MFC0 R4300I_CP0_REG_CONFIG")
                }
                R4300I_CP0_REG_WATCHLO => {
                    todo!("MFC0 R4300I_CP0_REG_WATCHLO")
                }
                R4300I_CP0_REG_WATCHHI => {
                    todo!("MFC0 R4300I_CP0_REG_WATCHHI")
                }
                R4300I_CP0_REG_WIRED => {
                    todo!("MFC0 R4300I_CP0_REG_WIRED")
                }
                R4300I_CP0_REG_CONTEXT => {
                    todo!("MFC0 R4300I_CP0_REG_CONTEXT")
                }
                R4300I_CP0_REG_BADVADDR => {
                    todo!("MFC0 R4300I_CP0_REG_BADVADDR")
                }
                R4300I_CP0_REG_XCONTEXT => {
                    todo!("MFC0 R4300I_CP0_REG_XCONTEXT")
                }
                R4300I_CP0_REG_LLADDR => {
                    todo!("MFC0 R4300I_CP0_REG_LLADDR")
                }
                R4300I_CP0_REG_ERR_EPC => {
                    todo!("MFC0 R4300I_CP0_REG_ERR_EPC")
                }
                R4300I_CP0_REG_PRID => {
                    todo!("MFC0 R4300I_CP0_REG_PRID")
                }
                R4300I_CP0_REG_PARITYER => {
                    todo!("MFC0 R4300I_CP0_REG_PARITYER")
                }
                R4300I_CP0_REG_CACHEER => {
                    todo!("MFC0 R4300I_CP0_REG_CACHEER")
                }
                R4300I_CP0_REG_7 => {
                    todo!("MFC0 R4300I_CP0_REG_7")
                }
                R4300I_CP0_REG_21 => {
                    todo!("MFC0 R4300I_CP0_REG_21")
                }
                R4300I_CP0_REG_22 => {
                    todo!("MFC0 R4300I_CP0_REG_22")
                }
                R4300I_CP0_REG_23 => {
                    todo!("MFC0 R4300I_CP0_REG_23")
                }
                R4300I_CP0_REG_24 => {
                    todo!("MFC0 R4300I_CP0_REG_24")
                }
                R4300I_CP0_REG_25 => {
                    todo!("MFC0 R4300I_CP0_REG_25")
                }
                R4300I_CP0_REG_31 => {
                    todo!("MFC0 R4300I_CP0_REG_31")
                }
                R4300I_CP0_REG_INDEX => {
                    let result =
                        block.load_ptr(DataType::S32, cpu_address, offset_of!(r4300i_t, cp0.index));
                    let mask = const_u32(0x8000003F);
                    let masked_result = block.and(DataType::U32, result.val(), mask);
                    let sign_extended =
                        block.convert_from(DataType::S32, DataType::S64, masked_result.val());
                    guest_regs.set_gpr(instr.rt(), sign_extended.val());
                }
                R4300I_CP0_REG_RANDOM => {
                    todo!("MFC0 R4300I_CP0_REG_INDEX")
                }
                R4300I_CP0_REG_COUNT => {
                    let count =
                        block.load_ptr(DataType::U64, cpu_address, offset_of!(r4300i_t, cp0.count));

                    let adjusted = block.add(DataType::U64, count.val(), const_u32(index as u32));

                    let shifted = block.right_shift(DataType::U64, adjusted.val(), const_u16(1));

                    let sign_extended =
                        block.convert_from(DataType::S32, DataType::S64, shifted.val());

                    guest_regs.set_gpr(instr.rt(), sign_extended.val());
                }
                _ => {
                    panic!("Unknown register in MFC0: {}", instr.rd());
                }
            },
            MipsOpcode::DMFC0 => {
                todo!("DMFC0")
            }
            MipsOpcode::CFC0 => {
                todo!("CFC0")
            }
            MipsOpcode::DCFC0 => {
                todo!("DCFC0")
            }
            MipsOpcode::MTC0 => {
                let value = guest_regs.get_gpr(&mut block, instr.rt());

                match instr.rd() as u32 {
                    R4300I_CP0_REG_ENTRYHI => {
                        let mask = const_u64(CP0_ENTRY_HI_WRITE_MASK as u64);
                        let sign_extended = block.convert_from(DataType::S32, DataType::S64, value);
                        let masked_value = block.and(DataType::U64, sign_extended.val(), mask);
                        block.write_ptr(
                            DataType::U64,
                            cpu_address,
                            offset_of!(r4300i_t, cp0.entry_hi.raw),
                            masked_value.val(),
                        );
                    }
                    R4300I_CP0_REG_STATUS => {
                        let status_mask = const_u32(CP0_STATUS_WRITE_MASK);
                        let inverse_status_mask = block.not(DataType::U32, status_mask);
                        let old_status = block.load_ptr(
                            DataType::U32,
                            cpu_address,
                            offset_of!(r4300i_t, cp0.status.raw),
                        );
                        let old_status_masked =
                            block.and(DataType::U32, old_status.val(), inverse_status_mask.val());
                        let value_masked = block.and(DataType::U32, value, status_mask);
                        let new_status =
                            block.or(DataType::U32, value_masked.val(), old_status_masked.val());
                        block.write_ptr(
                            DataType::U32,
                            cpu_address,
                            offset_of!(r4300i_t, cp0.status.raw),
                            new_status.val(),
                        );
                        block.call_function(
                            const_ptr(cp0_status_updated as usize),
                            None,
                            vec![const_u32(index as u32)],
                        );
                    }
                    R4300I_CP0_REG_TAGLO => {
                        block.write_ptr(
                            DataType::U32,
                            cpu_address,
                            offset_of!(r4300i_t, cp0.tag_lo),
                            value,
                        );
                    }
                    R4300I_CP0_REG_TAGHI => {
                        block.write_ptr(
                            DataType::U32,
                            cpu_address,
                            offset_of!(r4300i_t, cp0.tag_hi),
                            value,
                        );
                    }
                    R4300I_CP0_REG_CAUSE => {
                        let cause_mask = const_u32(0x300);
                        let cause_masked = block.and(DataType::U32, value, cause_mask);

                        let inverse_cause_mask = block.not(DataType::U32, cause_mask);
                        let old_cause = block.load_ptr(
                            DataType::U32,
                            cpu_address,
                            offset_of!(r4300i_t, cp0.cause.raw),
                        );
                        let old_cause_masked =
                            block.and(DataType::U32, old_cause.val(), inverse_cause_mask.val());

                        let new_cause =
                            block.or(DataType::U32, old_cause_masked.val(), cause_masked.val());
                        block.write_ptr(
                            DataType::U32,
                            cpu_address,
                            offset_of!(r4300i_t, cp0.cause.raw),
                            new_cause.val(),
                        );
                    }
                    R4300I_CP0_REG_COMPARE => {
                        // Lower compare interrupt
                        let old_cause = block.load_ptr(
                            DataType::U32,
                            cpu_address,
                            offset_of!(r4300i_t, cp0.cause.raw),
                        );
                        let old_cause_masked =
                            block.and(DataType::U32, old_cause.val(), const_u32(!(1 << 15)));
                        block.write_ptr(
                            DataType::U32,
                            cpu_address,
                            offset_of!(r4300i_t, cp0.cause.raw),
                            old_cause_masked.val(),
                        );

                        block.write_ptr(
                            DataType::U32,
                            cpu_address,
                            offset_of!(r4300i_t, cp0.compare),
                            value,
                        );

                        block.call_function(
                            const_ptr(reschedule_compare_interrupt as usize),
                            None,
                            vec![const_u32(index as u32)],
                        );
                    }
                    R4300I_CP0_REG_ENTRYLO0 => {
                        block.write_ptr(
                            DataType::U32,
                            cpu_address,
                            offset_of!(r4300i_t, cp0.entry_lo0.raw),
                            value,
                        );
                    }
                    R4300I_CP0_REG_ENTRYLO1 => {
                        block.write_ptr(
                            DataType::U32,
                            cpu_address,
                            offset_of!(r4300i_t, cp0.entry_lo1.raw),
                            value,
                        );
                    }
                    R4300I_CP0_REG_PAGEMASK => {
                        let mask = const_u32(CP0_PAGEMASK_WRITE_MASK);
                        let masked = block.and(DataType::U32, value, mask);
                        block.write_ptr(
                            DataType::U32,
                            cpu_address,
                            offset_of!(r4300i_t, cp0.page_mask.raw),
                            masked.val(),
                        );
                    }
                    R4300I_CP0_REG_EPC => {
                        let sign_extended = block.convert_from(DataType::S32, DataType::S64, value);
                        block.write_ptr(
                            DataType::U64,
                            cpu_address,
                            offset_of!(r4300i_t, cp0.EPC),
                            sign_extended.val(),
                        );
                    }
                    R4300I_CP0_REG_CONFIG => {
                        todo!("MTC0 R4300I_CP0_REG_CONFIG")
                    }
                    R4300I_CP0_REG_WATCHLO => {
                        todo!("MTC0 R4300I_CP0_REG_WATCHLO")
                    }
                    R4300I_CP0_REG_WATCHHI => {
                        todo!("MTC0 R4300I_CP0_REG_WATCHHI")
                    }
                    R4300I_CP0_REG_WIRED => {
                        todo!("MTC0 R4300I_CP0_REG_WIRED")
                    }
                    R4300I_CP0_REG_CONTEXT => {
                        todo!("MTC0 R4300I_CP0_REG_CONTEXT")
                    }
                    R4300I_CP0_REG_BADVADDR => {
                        todo!("MTC0 R4300I_CP0_REG_BADVADDR")
                    }
                    R4300I_CP0_REG_XCONTEXT => {
                        todo!("MTC0 R4300I_CP0_REG_XCONTEXT")
                    }
                    R4300I_CP0_REG_LLADDR => {
                        todo!("MTC0 R4300I_CP0_REG_LLADDR")
                    }
                    R4300I_CP0_REG_ERR_EPC => {
                        todo!("MTC0 R4300I_CP0_REG_ERR_EPC")
                    }
                    R4300I_CP0_REG_PRID => {
                        todo!("MTC0 R4300I_CP0_REG_PRID")
                    }
                    R4300I_CP0_REG_PARITYER => {
                        todo!("MTC0 R4300I_CP0_REG_PARITYER")
                    }
                    R4300I_CP0_REG_CACHEER => {
                        todo!("MTC0 R4300I_CP0_REG_CACHEER")
                    }
                    R4300I_CP0_REG_7 => {
                        todo!("MTC0 R4300I_CP0_REG_7")
                    }
                    R4300I_CP0_REG_21 => {
                        todo!("MTC0 R4300I_CP0_REG_21")
                    }
                    R4300I_CP0_REG_22 => {
                        todo!("MTC0 R4300I_CP0_REG_22")
                    }
                    R4300I_CP0_REG_23 => {
                        todo!("MTC0 R4300I_CP0_REG_23")
                    }
                    R4300I_CP0_REG_24 => {
                        todo!("MTC0 R4300I_CP0_REG_24")
                    }
                    R4300I_CP0_REG_25 => {
                        todo!("MTC0 R4300I_CP0_REG_25")
                    }
                    R4300I_CP0_REG_31 => {
                        todo!("MTC0 R4300I_CP0_REG_31")
                    }
                    R4300I_CP0_REG_INDEX => {
                        block.write_ptr(
                            DataType::U32,
                            cpu_address,
                            offset_of!(r4300i_t, cp0.index),
                            value,
                        );
                    }
                    R4300I_CP0_REG_RANDOM => {
                        todo!("MTC0 R4300I_CP0_REG_INDEX")
                    }
                    R4300I_CP0_REG_COUNT => {
                        let value_u32 = block.convert(DataType::U32, value);
                        let value_shifted =
                            block.left_shift(DataType::U64, value_u32.val(), const_u16(1));
                        block.write_ptr(
                            DataType::U32,
                            cpu_address,
                            offset_of!(r4300i_t, cp0.count),
                            value_shifted.val(),
                        );
                        let reschedule_compare_interrupt =
                            const_ptr(reschedule_compare_interrupt as usize);
                        block.call_function(
                            reschedule_compare_interrupt,
                            None,
                            vec![const_u32(index as u32)],
                        );
                    }
                    _ => {
                        panic!("Unknown register in MTC0: {}", instr.rd());
                    }
                }
            }
            MipsOpcode::DMTC0 => {
                todo!("DMTC0")
            }
            MipsOpcode::CTC0 => {
                todo!("CTC0")
            }
            MipsOpcode::DCTC0 => {
                todo!("DCTC0")
            }
            MipsOpcode::MFC1 => {
                checkcp1(&mut block, &mut guest_regs, true);
                let value = guest_regs.get_fgr_32bit_fr(&mut block, instr.fs());
                let sign_extended = block.convert_from(DataType::S32, DataType::S64, value);
                guest_regs.set_gpr(instr.rt(), sign_extended.val());
            }
            MipsOpcode::DMFC1 => {
                todo!("DMFC1")
            }
            MipsOpcode::CFC1 => {
                checkcp1(&mut block, &mut guest_regs, true);

                let fs = instr.rd();
                let value = match fs {
                    0 => {
                        println!("Reading FCR0 - probably returning an invalid value!");
                        block
                            .load_ptr(DataType::S32, cpu_address, offset_of!(r4300i_t, fcr0.raw))
                            .val()
                    }
                    31 => guest_regs.get_fcr31(&mut block),
                    _ => {
                        todo!("This instruction is only defined when fs == 0 or fs == 31! (Throw an exception?)");
                    }
                };

                guest_regs.set_gpr(
                    instr.rt(),
                    block
                        .convert_from(DataType::S32, DataType::S64, value)
                        .val(),
                );
            }
            MipsOpcode::DCFC1 => {
                todo!("DCFC1")
            }
            MipsOpcode::MTC1 => {
                checkcp1(&mut block, &mut guest_regs, true);
                let value = guest_regs.get_gpr(&mut block, instr.rt());
                guest_regs.set_fgr_32bit_fr(instr.rd(), value, &mut block);
            }
            MipsOpcode::DMTC1 => {
                todo!("DMTC1")
            }
            MipsOpcode::CTC1 => {
                checkcp1(&mut block, &mut guest_regs, true);
                let fs = instr.rd();
                let value = guest_regs.get_gpr(&mut block, instr.rt());
                match fs {
                    0 => {
                        println!("CTC1 FCR0: Writing to read-only register FCR0!");
                    }
                    31 => {
                        let mask = const_u32(0x183ffff);
                        let masked = block.and(DataType::U32, value, mask);
                        guest_regs.set_fcr31(masked.val());
                        println!("TODO: check_fpu_exception();");
                    }
                    _ => {
                        todo!("This instruction is only defined when fs == 0 or fs == 31! (Throw an exception?)");
                    }
                }
            }
            MipsOpcode::DCTC1 => {
                todo!("DCTC1")
            }
            MipsOpcode::SLL => {
                let input = guest_regs.get_gpr(&mut block, instr.rt());
                let result = block.left_shift(DataType::S32, input, const_u16(instr.sa() as u16));
                let sign_extended = block.convert(DataType::S64, result.val());
                guest_regs.set_gpr(instr.rd(), sign_extended.val());
            }
            MipsOpcode::SRL => {
                let input = guest_regs.get_gpr(&mut block, instr.rt());
                let result = block.right_shift(DataType::U32, input, const_u16(instr.sa() as u16));
                let sign_extended = block.convert_from(DataType::S32, DataType::S64, result.val());
                guest_regs.set_gpr(instr.rd(), sign_extended.val());
            }
            MipsOpcode::SRA => {
                let input = guest_regs.get_gpr(&mut block, instr.rt());
                // SRA and SRAV are weird. They shift the entire 64 bit value and then sign extend
                // the low 32 bits.
                let result = block.right_shift(DataType::U64, input, const_u16(instr.sa() as u16));
                let sign_extended = block.convert_from(DataType::S32, DataType::S64, result.val());
                guest_regs.set_gpr(instr.rd(), sign_extended.val());
            }
            MipsOpcode::SRAV => {
                let input = guest_regs.get_gpr(&mut block, instr.rt());
                // SRA and SRAV are weird. They shift the entire 64 bit value and then sign extend
                // the low 32 bits.
                let shift_amount = guest_regs.get_gpr(&mut block, instr.rs());
                let shift_amount_masked =
                    block.and(DataType::U32, shift_amount, const_u32(0b11111));
                let result = block.right_shift(DataType::U64, input, shift_amount_masked.val());
                let sign_extended = block.convert_from(DataType::S32, DataType::S64, result.val());
                guest_regs.set_gpr(instr.rd(), sign_extended.val());
            }
            MipsOpcode::SLLV => {
                let rs = guest_regs.get_gpr(&mut block, instr.rs());
                let shift_amount = block.and(DataType::U32, rs, const_u32(0b11111));

                let rt = guest_regs.get_gpr(&mut block, instr.rt());
                let result = block.left_shift(DataType::U32, rt, shift_amount.val());
                let sign_extended = block.convert_from(DataType::S32, DataType::S64, result.val());

                guest_regs.set_gpr(instr.rd(), sign_extended.val());
            }
            MipsOpcode::SRLV => {
                let rs = guest_regs.get_gpr(&mut block, instr.rs());
                let shift_amount = block.and(DataType::U32, rs, const_u32(0b11111));

                let rt = guest_regs.get_gpr(&mut block, instr.rt());
                let result = block.right_shift(DataType::U32, rt, shift_amount.val());
                let sign_extended = block.convert_from(DataType::S32, DataType::S64, result.val());

                guest_regs.set_gpr(instr.rd(), sign_extended.val());
            }
            MipsOpcode::JR => {
                let target = guest_regs.get_gpr(&mut block, instr.rs());
                set_pc(&mut pc_set, &mut block, cpu_address, target);
            }
            MipsOpcode::JALR => {
                let target = guest_regs.get_gpr(&mut block, instr.rs());
                set_pc(&mut pc_set, &mut block, cpu_address, target);
                set_link_reg(&mut guest_regs, vaddr, instr.rd());
            }
            MipsOpcode::SYSCALL => {
                todo!("SYSCALL")
            }
            MipsOpcode::SYNC => {
                todo!("SYNC")
            }
            MipsOpcode::MFHI => {
                let hi = guest_regs.get_hi(&mut block);
                guest_regs.set_gpr(instr.rd(), hi);
            }
            MipsOpcode::MTHI => {
                let value = guest_regs.get_gpr(&mut block, instr.rs());
                guest_regs.set_hi(value);
            }
            MipsOpcode::MFLO => {
                let lo = guest_regs.get_lo(&mut block);
                guest_regs.set_gpr(instr.rd(), lo);
            }
            MipsOpcode::MTLO => {
                let value = guest_regs.get_gpr(&mut block, instr.rs());
                guest_regs.set_lo(value);
            }
            MipsOpcode::DSLLV => {
                todo!("DSLLV")
            }
            MipsOpcode::DSRLV => {
                todo!("DSRLV")
            }
            MipsOpcode::DSRAV => {
                todo!("DSRAV")
            }
            MipsOpcode::MULT => {
                let rs = guest_regs.get_gpr(&mut block, instr.rs());
                let rt = guest_regs.get_gpr(&mut block, instr.rt());

                let result =
                    block.multiply(DataType::S64, DataType::S32, MultiplyType::Split, rs, rt);

                let lo = block.convert_from(DataType::S32, DataType::S64, result.at(0));
                let hi = block.convert_from(DataType::S32, DataType::S64, result.at(1));

                guest_regs.set_lo(lo.val());
                guest_regs.set_hi(hi.val());
            }
            MipsOpcode::MULTU => {
                let rs = guest_regs.get_gpr(&mut block, instr.rs());
                let rt = guest_regs.get_gpr(&mut block, instr.rt());

                let result =
                    block.multiply(DataType::U64, DataType::U32, MultiplyType::Split, rs, rt);

                let lo = block.convert_from(DataType::S32, DataType::S64, result.at(0));
                let hi = block.convert_from(DataType::S32, DataType::S64, result.at(1));

                guest_regs.set_lo(lo.val());
                guest_regs.set_hi(hi.val());
            }
            MipsOpcode::DIV => {
                let dividend = guest_regs.get_gpr(&mut block, instr.rs());
                let divisor = guest_regs.get_gpr(&mut block, instr.rt());

                let is_divide_by_zero =
                    block.compare(DataType::U64, divisor, CompareType::Equal, const_u32(0));

                let mut normal = func.new_block(vec![]);
                let mut divide_by_zero = func.new_block(vec![]);
                block.branch(
                    is_divide_by_zero.val(),
                    divide_by_zero.call(vec![]),
                    normal.call(vec![]),
                );

                let sign_extended_dividend =
                    divide_by_zero.convert_from(DataType::S32, DataType::S64, dividend);

                let is_dividend_gte_zero = divide_by_zero.compare(
                    DataType::S32,
                    dividend,
                    CompareType::GreaterThanOrEqual,
                    const_s32(0),
                );
                let mut dividend_gte_zero = func.new_block(vec![]);
                let mut dividend_lt_zero = func.new_block(vec![]);
                divide_by_zero.branch(
                    is_dividend_gte_zero.val(),
                    dividend_gte_zero.call(vec![]),
                    dividend_lt_zero.call(vec![]),
                );

                let result = normal.divide(DataType::S32, dividend, divisor);
                let quotient = normal.convert_from(DataType::S32, DataType::S64, result.at(0));
                let remainder = normal.convert_from(DataType::S32, DataType::S64, result.at(1));

                // Takes mult_lo and mult_hi results as arguments (in that order)
                let end = func.new_block(vec![DataType::S64, DataType::S64]);
                dividend_gte_zero.jump(end.call(vec![const_s64(-1), sign_extended_dividend.val()]));
                dividend_lt_zero.jump(end.call(vec![const_s64(1), sign_extended_dividend.val()]));
                normal.jump(end.call(vec![quotient.val(), remainder.val()]));

                guest_regs.set_lo(end.input(0));
                guest_regs.set_hi(end.input(1));

                block = end;
            }
            MipsOpcode::DIVU => {
                let dividend = guest_regs.get_gpr(&mut block, instr.rs());
                let divisor = guest_regs.get_gpr(&mut block, instr.rt());

                let is_divide_by_zero =
                    block.compare(DataType::U32, divisor, CompareType::Equal, const_u32(0));

                let mut normal = func.new_block(vec![]);
                let mut divide_by_zero = func.new_block(vec![]);
                block.branch(
                    is_divide_by_zero.val(),
                    divide_by_zero.call(vec![]),
                    normal.call(vec![]),
                );

                let end = func.new_block(vec![DataType::S64, DataType::S64]);

                let sign_extended_dividend =
                    divide_by_zero.convert_from(DataType::S32, DataType::S64, dividend);
                divide_by_zero.jump(end.call(vec![const_s64(-1), sign_extended_dividend.val()]));

                let result = normal.divide(DataType::U32, dividend, divisor);
                let quotient = normal.convert_from(DataType::S32, DataType::S64, result.at(0));
                let remainder = normal.convert_from(DataType::S32, DataType::S64, result.at(1));
                normal.jump(end.call(vec![quotient.val(), remainder.val()]));
                guest_regs.set_lo(end.input(0));
                guest_regs.set_hi(end.input(1));

                block = end;
            }
            MipsOpcode::DMULT => {
                todo!("DMULT")
            }
            MipsOpcode::DMULTU => {
                let rs = guest_regs.get_gpr(&mut block, instr.rs());
                let rt = guest_regs.get_gpr(&mut block, instr.rt());

                let result =
                    block.multiply(DataType::U128, DataType::U64, MultiplyType::Split, rs, rt);

                let lo = result.at(0);
                let hi = result.at(1);

                guest_regs.set_lo(lo);
                guest_regs.set_hi(hi);
            }
            MipsOpcode::DDIV => {
                // TODO: some unimplemented cases in here:
                // s64 dividend = (s64)get_register(instruction.r.rs);
                // s64 divisor  = (s64)get_register(instruction.r.rt);
                // if (unlikely(divisor == 0)) {
                //     logwarn("Divide by zero");
                //     N64CPU.mult_hi = dividend;
                //     if (dividend >= 0) {
                //         N64CPU.mult_lo = (s64)-1;
                //     } else {
                //         N64CPU.mult_lo = (s64)1;
                //     }
                // } else if (unlikely(divisor == -1 && dividend == INT64_MIN)) {
                //     N64CPU.mult_lo = dividend;
                //     N64CPU.mult_hi = 0;
                // } else {
                //     s64 quotient  = (s64)(dividend / divisor);
                //     s64 remainder = (s64)(dividend % divisor);

                //     N64CPU.mult_lo = quotient;
                //     N64CPU.mult_hi = remainder;
                // }
                let dividend = guest_regs.get_gpr(&mut block, instr.rs());
                let divisor = guest_regs.get_gpr(&mut block, instr.rt());

                let is_divide_by_zero =
                    block.compare(DataType::S64, divisor, CompareType::Equal, const_s64(0));

                extern fn unimplemented_divide_by_zero() {
                    panic!("Unimplemented: Divide by zero exception handling for DDIV");
                }
                extern fn unimplemented_intmin_by_neg1() {
                    panic!("Unimplemented: INT64_MIN / -1 exception handling for DDIV");
                }

                let mut check_intmin_by_neg1 = func.new_block(vec![]);
                let mut divide_by_zero = func.new_block(vec![]);
                block.branch(
                    is_divide_by_zero.val(),
                    divide_by_zero.call(vec![]),
                    check_intmin_by_neg1.call(vec![]),
                );

                divide_by_zero.call_function(
                    const_ptr(unimplemented_divide_by_zero as usize),
                    None,
                    vec![],
                );
                divide_by_zero.ret(None);

                let mut intmin_by_neg1 = func.new_block(vec![]);
                intmin_by_neg1.call_function(
                    const_ptr(unimplemented_intmin_by_neg1 as usize),
                    None,
                    vec![],
                );
                intmin_by_neg1.ret(None);

                let mut normal = func.new_block(vec![]);
                {
                    // Check if operation is INT64_MIN / -1
                    let is_dividend_intmin = check_intmin_by_neg1.compare(
                        DataType::S64,
                        dividend,
                        CompareType::Equal,
                        const_s64(i64::MIN),
                    );
                    let is_divisor_neg1 = check_intmin_by_neg1.compare(
                        DataType::S64,
                        divisor,
                        CompareType::Equal,
                        const_s64(-1),
                    );
                    let both_conditions = check_intmin_by_neg1.and(DataType::Bool, is_divisor_neg1.val(), is_dividend_intmin.val());
                    check_intmin_by_neg1.branch(
                        both_conditions.val(),
                        intmin_by_neg1.call(vec![]),
                        normal.call(vec![]),
                    );
                }

                let end = func.new_block(vec![DataType::S64, DataType::S64]);

                let result = normal.divide(DataType::S64, dividend, divisor);
                let quotient = result.at(0);
                let remainder = result.at(1);
                normal.jump(end.call(vec![quotient, remainder]));

                guest_regs.set_lo(end.input(0));
                guest_regs.set_hi(end.input(1));

                block = end;
            }
            MipsOpcode::DDIVU => {
                let dividend = guest_regs.get_gpr(&mut block, instr.rs());
                let divisor = guest_regs.get_gpr(&mut block, instr.rt());

                let is_divide_by_zero =
                    block.compare(DataType::U64, divisor, CompareType::Equal, const_u32(0));

                let mut normal = func.new_block(vec![]);
                let mut divide_by_zero = func.new_block(vec![]);
                block.branch(
                    is_divide_by_zero.val(),
                    divide_by_zero.call(vec![]),
                    normal.call(vec![]),
                );

                let end = func.new_block(vec![DataType::S64, DataType::S64]);

                divide_by_zero.jump(end.call(vec![const_s64(-1), dividend]));

                let result = normal.divide(DataType::U64, dividend, divisor);
                let quotient = result.at(0);
                let remainder = result.at(1);
                normal.jump(end.call(vec![quotient, remainder]));
                guest_regs.set_lo(end.input(0));
                guest_regs.set_hi(end.input(1));

                block = end;
            }
            MipsOpcode::ADD => {
                // Identical to ADD, but does not throw overflow exceptions (which are not
                // implemented yet anyway)
                let rs = guest_regs.get_gpr(&mut block, instr.rs());
                let rt = guest_regs.get_gpr(&mut block, instr.rt());
                let result = block.add(DataType::S32, rs, rt);
                let sign_extended = block.convert(DataType::S64, result.val());
                guest_regs.set_gpr(instr.rd(), sign_extended.val());
            }
            MipsOpcode::ADDU => {
                // Identical to ADD, but does not throw overflow exceptions (which are not
                // implemented yet anyway)
                let rs = guest_regs.get_gpr(&mut block, instr.rs());
                let rt = guest_regs.get_gpr(&mut block, instr.rt());
                let result = block.add(DataType::S32, rs, rt);
                let sign_extended = block.convert(DataType::S64, result.val());
                guest_regs.set_gpr(instr.rd(), sign_extended.val());
            }
            MipsOpcode::AND => {
                let rs = guest_regs.get_gpr(&mut block, instr.rs());
                let rt = guest_regs.get_gpr(&mut block, instr.rt());
                let result = block.and(DataType::U64, rs, rt);
                guest_regs.set_gpr(instr.rd(), result.val());
            }
            MipsOpcode::SUB => {
                let rs = guest_regs.get_gpr(&mut block, instr.rs());
                let rt = guest_regs.get_gpr(&mut block, instr.rt());
                let result = block.subtract(DataType::S32, rs, rt);
                let sign_extended = block.convert(DataType::S64, result.val());
                guest_regs.set_gpr(instr.rd(), sign_extended.val());
            }
            MipsOpcode::SUBU => {
                // Identical to SUB, but does not throw overflow exceptions (which are not
                // implemented yet anyway)
                let rs = guest_regs.get_gpr(&mut block, instr.rs());
                let rt = guest_regs.get_gpr(&mut block, instr.rt());
                let result = block.subtract(DataType::S32, rs, rt);
                let sign_extended = block.convert(DataType::S64, result.val());
                guest_regs.set_gpr(instr.rd(), sign_extended.val());
            }
            MipsOpcode::OR => {
                let rs = guest_regs.get_gpr(&mut block, instr.rs());
                let rt = guest_regs.get_gpr(&mut block, instr.rt());
                let result = block.or(DataType::U64, rs, rt);
                guest_regs.set_gpr(instr.rd(), result.val());
            }
            MipsOpcode::XOR => {
                let rs = guest_regs.get_gpr(&mut block, instr.rs());
                let rt = guest_regs.get_gpr(&mut block, instr.rt());
                let result = block.xor(DataType::U64, rs, rt);
                guest_regs.set_gpr(instr.rd(), result.val());
            }
            MipsOpcode::NOR => {
                let rs = guest_regs.get_gpr(&mut block, instr.rs());
                let rt = guest_regs.get_gpr(&mut block, instr.rt());
                let or_result = block.or(DataType::U64, rs, rt);
                let result = block.not(DataType::U64, or_result.val());
                guest_regs.set_gpr(instr.rd(), result.val());
            }
            MipsOpcode::SLT => {
                let rs = guest_regs.get_gpr(&mut block, instr.rs());
                let rt = guest_regs.get_gpr(&mut block, instr.rt());
                let result = block.compare(DataType::S64, rs, CompareType::LessThan, rt);
                guest_regs.set_gpr(instr.rd(), result.val());
            }
            MipsOpcode::SLTU => {
                let rs = guest_regs.get_gpr(&mut block, instr.rs());
                let rt = guest_regs.get_gpr(&mut block, instr.rt());
                let result = block.compare(DataType::U64, rs, CompareType::LessThan, rt);
                guest_regs.set_gpr(instr.rd(), result.val());
            }
            MipsOpcode::DADD => {
                todo!("DADD")
            }
            MipsOpcode::DADDU => {
                todo!("DADDU")
            }
            MipsOpcode::DSUB => {
                todo!("DSUB")
            }
            MipsOpcode::DSUBU => {
                todo!("DSUBU")
            }
            MipsOpcode::TGE => {
                todo!("TGE")
            }
            MipsOpcode::TGEU => {
                todo!("TGEU")
            }
            MipsOpcode::TLT => {
                todo!("TLT")
            }
            MipsOpcode::TLTU => {
                todo!("TLTU")
            }
            MipsOpcode::TEQ => {
                todo!("TEQ")
            }
            MipsOpcode::TNE => {
                todo!("TNE")
            }
            MipsOpcode::DSLL => {
                let input = guest_regs.get_gpr(&mut block, instr.rt());
                let result = block.left_shift(DataType::U32, input, const_u16(instr.sa() as u16));
                guest_regs.set_gpr(instr.rd(), result.val());
            }
            MipsOpcode::DSRL => {
                todo!("DSRL")
            }
            MipsOpcode::DSRA => {
                todo!("DSRA")
            }
            MipsOpcode::DSLL32 => {
                let input = guest_regs.get_gpr(&mut block, instr.rt());
                let result =
                    block.left_shift(DataType::U64, input, const_u16(instr.sa() as u16 + 32));
                guest_regs.set_gpr(instr.rd(), result.val());
            }
            MipsOpcode::DSRL32 => {
                todo!("DSRL32")
            }
            MipsOpcode::DSRA32 => {
                let input = guest_regs.get_gpr(&mut block, instr.rt());
                let result =
                    block.right_shift(DataType::S64, input, const_u16(instr.sa() as u16 + 32));
                guest_regs.set_gpr(instr.rd(), result.val());
            }
            MipsOpcode::TLBWI => {
                let index = block.load_ptr(
                    DataType::U32,
                    cpu_address,
                    offset_of!(r4300i_t, cp0.index),
                );
                let masked_index = block.and(DataType::U32, index.val(), const_u32(0x8000003F));

                block.call_function(
                    const_ptr(do_tlbwi as usize),
                    None,
                    vec![masked_index.val()],
                );

            }
            MipsOpcode::TLBP => {
                block.call_function(const_ptr(do_tlbp as usize), None, vec![]);
            }
            MipsOpcode::ERET => {
                let status = block.load_ptr(
                    DataType::U32,
                    cpu_address,
                    offset_of!(r4300i_t, cp0.status.raw),
                );
                let erl_mask = const_u32(STATUS_ERL_MASK);
                let masked = block.and(DataType::U32, status.val(), erl_mask);
                let is_erl = block.compare(
                    DataType::U32,
                    masked.val(),
                    CompareType::NotEqual,
                    const_u32(0),
                );

                let mut block_erl = func.new_block(vec![]);
                let mut block_no_erl = func.new_block(vec![]);

                // if ERL is set, set the PC to error_epc
                block.branch(
                    is_erl.val(),
                    block_erl.call(vec![]),
                    block_no_erl.call(vec![]),
                );

                let error_epc = block_erl.load_ptr(
                    DataType::U64,
                    cpu_address,
                    offset_of!(r4300i_t, cp0.error_epc),
                );
                set_pc(&mut pc_set, &mut block_erl, cpu_address, error_epc.val());
                // Set erl to false
                let inverse_erl_mask = block_erl.not(DataType::U32, erl_mask);
                let masked_status =
                    block_erl.and(DataType::U32, status.val(), inverse_erl_mask.val());
                block_erl.write_ptr(
                    DataType::U32,
                    cpu_address,
                    offset_of!(r4300i_t, cp0.status.raw),
                    masked_status.val(),
                );

                // If erl is not set, set the PC to EPC
                let epc = block_no_erl.load_ptr(
                    DataType::U64,
                    cpu_address,
                    offset_of!(r4300i_t, cp0.EPC),
                );
                set_pc(&mut pc_set, &mut block_no_erl, cpu_address, epc.val());
                let inverse_exl_mask = block_no_erl.not(DataType::U32, const_u32(STATUS_EXL_MASK));
                let masked_status =
                    block_no_erl.and(DataType::U32, status.val(), inverse_exl_mask.val());
                block_no_erl.write_ptr(
                    DataType::U32,
                    cpu_address,
                    offset_of!(r4300i_t, cp0.status.raw),
                    masked_status.val(),
                );

                let mut end = func.new_block(vec![]);
                block_erl.jump(end.call(vec![]));
                block_no_erl.jump(end.call(vec![]));
                end.call_function(const_ptr(cp0_status_updated as usize), None, vec![]);

                block = end;
                println!("TODO: set llbit to false");
            }
            MipsOpcode::FPU_CVT_S => {
                checkcp1(&mut block, &mut guest_regs, false);
                match instr.fmt_datatype() {
                    Some(DataType::F64) => {
                        let fs = guest_regs.get_fgr_64bit_fs(&mut block, instr.fs());
                        let result = block.convert_from(DataType::F64, DataType::F32, fs);
                        guest_regs.set_fgr_64bit(instr.fd(), result.val());
                    }
                    Some(DataType::S32) => {
                        let fs = guest_regs.get_fgr_32bit_fs(&mut block, instr.fs());
                        let result = block.convert_from(DataType::S32, DataType::F32, fs);
                        guest_regs.set_fgr_64bit(instr.fd(), result.val());
                    }
                    Some(DataType::U64) => {
                        todo!()
                    }
                    _ => todo!("Fire unimplemented operation here"),
                }
            }
            MipsOpcode::FPU_CVT_D => {
                checkcp1(&mut block, &mut guest_regs, false);
                match instr.fmt_datatype() {
                    Some(DataType::F32) => {
                        let fs = guest_regs.get_fgr_32bit_fs(&mut block, instr.fs());
                        let result = block.convert_from(DataType::F32, DataType::F64, fs);
                        guest_regs.set_fgr_64bit(instr.fd(), result.val());
                    }
                    Some(DataType::S32) => {
                        let fs = guest_regs.get_fgr_32bit_fs(&mut block, instr.fs());
                        let result = block.convert_from(DataType::S32, DataType::F64, fs);
                        guest_regs.set_fgr_64bit(instr.fd(), result.val());
                    }
                    Some(DataType::S64) => {
                        todo!("cvt_d_l")
                    }
                    _ => todo!("Fire unimplemented operation here"),
                }
            }
            MipsOpcode::FPU_CVT_W => {
                checkcp1(&mut block, &mut guest_regs, false);
                match instr.fmt_datatype() {
                    Some(DataType::F32) => {
                        let fs = guest_regs.get_fgr_32bit_fs(&mut block, instr.fs());
                        let result = block.convert_from(DataType::F32, DataType::S32, fs);
                        guest_regs.set_fgr(instr.fd(), result.val(), FgrLoadState::Full64);
                    }
                    Some(DataType::F64) => {
                        let fs = guest_regs.get_fgr_64bit_fs(&mut block, instr.fs());
                        let result = block.convert_from(DataType::F64, DataType::S32, fs);
                        guest_regs.set_fgr(instr.fd(), result.val(), FgrLoadState::Full64);
                    }
                    _ => todo!("Fire unimplemented operation here"),
                }
            }
            MipsOpcode::FPU_CVT_L => {
                todo!("CVT_L")
            }
            MipsOpcode::FPU_ADD => match instr.fmt_datatype() {
                Some(DataType::F32) => {
                    let fs = guest_regs.get_fgr_32bit_fs(&mut block, instr.fs());
                    let ft = guest_regs.get_fgr_32bit_ft(&mut block, instr.ft());
                    let result = block.add(DataType::F32, fs, ft);
                    guest_regs.set_fgr(instr.fd(), result.val(), FgrLoadState::Full64);
                }
                Some(DataType::F64) => {
                    let fs = guest_regs.get_fgr_64bit_fs(&mut block, instr.fs());
                    let ft = guest_regs.get_fgr_64bit_ft(&mut block, instr.ft());
                    let result = block.add(DataType::F64, fs, ft);
                    guest_regs.set_fgr(instr.fd(), result.val(), FgrLoadState::Full64);
                }
                _ => todo!("Fire unimplemented operation here"),
            },
            MipsOpcode::FPU_SUB => match instr.fmt_datatype() {
                Some(DataType::F32) => {
                    let fs = guest_regs.get_fgr_32bit_fs(&mut block, instr.fs());
                    let ft = guest_regs.get_fgr_32bit_ft(&mut block, instr.ft());
                    let result = block.subtract(DataType::F32, fs, ft);
                    guest_regs.set_fgr(instr.fd(), result.val(), FgrLoadState::Full64);
                }
                Some(DataType::F64) => {
                    let fs = guest_regs.get_fgr_64bit_fs(&mut block, instr.fs());
                    let ft = guest_regs.get_fgr_64bit_ft(&mut block, instr.ft());
                    let result = block.subtract(DataType::F64, fs, ft);
                    guest_regs.set_fgr(instr.fd(), result.val(), FgrLoadState::Full64);
                }
                _ => todo!("Fire unimplemented operation here"),
            },
            MipsOpcode::FPU_MULT => match instr.fmt_datatype() {
                Some(DataType::F32) => {
                    let fs = guest_regs.get_fgr_32bit_fs(&mut block, instr.fs());
                    let ft = guest_regs.get_fgr_32bit_ft(&mut block, instr.ft());
                    let result = block.multiply(
                        DataType::F32,
                        DataType::F32,
                        MultiplyType::Combined,
                        fs,
                        ft,
                    );
                    guest_regs.set_fgr(instr.fd(), result.val(), FgrLoadState::Full64);
                }
                Some(DataType::F64) => {
                    let fs = guest_regs.get_fgr_64bit_fs(&mut block, instr.fs());
                    let ft = guest_regs.get_fgr_64bit_ft(&mut block, instr.ft());
                    let result = block.multiply(
                        DataType::F64,
                        DataType::F64,
                        MultiplyType::Combined,
                        fs,
                        ft,
                    );
                    guest_regs.set_fgr(instr.fd(), result.val(), FgrLoadState::Full64);
                }
                _ => todo!("Fire unimplemented operation here"),
            },
            MipsOpcode::FPU_DIV => match instr.fmt_datatype() {
                Some(DataType::F32) => {
                    let fs = guest_regs.get_fgr_32bit_fs(&mut block, instr.fs());
                    let ft = guest_regs.get_fgr_32bit_ft(&mut block, instr.ft());
                    let result = block.divide(DataType::F32, fs, ft);
                    guest_regs.set_fgr(instr.fd(), result.val(), FgrLoadState::Full64);
                }
                Some(DataType::F64) => {
                    let fs = guest_regs.get_fgr_64bit_fs(&mut block, instr.fs());
                    let ft = guest_regs.get_fgr_64bit_ft(&mut block, instr.ft());
                    let result = block.divide(DataType::F64, fs, ft);
                    guest_regs.set_fgr(instr.fd(), result.val(), FgrLoadState::Full64);
                }
                _ => todo!("Fire unimplemented operation here"),
            },
            MipsOpcode::FPU_SQRT => {
                checkcp1(&mut block, &mut guest_regs, false);
                match instr.fmt_datatype() {
                    Some(DataType::F32) => {
                        let fs = guest_regs.get_fgr_32bit_fs(&mut block, instr.fs());
                        let result = block.square_root(DataType::F32, fs);
                        guest_regs.set_fgr(instr.fd(), result.val(), FgrLoadState::Full64);
                    }
                    Some(DataType::F64) => {
                        let fs = guest_regs.get_fgr_64bit_fs(&mut block, instr.fs());
                        let result = block.square_root(DataType::F64, fs);
                        guest_regs.set_fgr(instr.fd(), result.val(), FgrLoadState::Full64);
                    }
                    _ => todo!("Fire unimplemented operation here"),
                }
            }
            MipsOpcode::FPU_ABS => {
                todo!("FPU_ABS")
            }
            MipsOpcode::FPU_MOV => match instr.fmt_datatype() {
                Some(DataType::F32) | Some(DataType::F64) => {
                    let fs = guest_regs.get_fgr_64bit_fs(&mut block, instr.fs());
                    guest_regs.set_fgr_64bit(instr.fd(), fs);
                }
                _ => todo!("Fire unimplemented operation here"),
            },
            MipsOpcode::FPU_ROUND_L => {
                todo!("FPU_ROUND_L")
            }
            MipsOpcode::FPU_TRUNC_L => {
                todo!("FPU_TRUNC_L")
            }
            MipsOpcode::FPU_CEIL_L => {
                todo!("FPU_CEIL_L")
            }
            MipsOpcode::FPU_FLOOR_L => {
                todo!("FPU_FLOOR_L")
            }
            MipsOpcode::FPU_ROUND_W => {
                todo!("FPU_ROUND_W")
            }
            MipsOpcode::FPU_TRUNC_W => match instr.fmt_datatype() {
                Some(DataType::F32) => {
                    let fs = guest_regs.get_fgr_32bit_fs(&mut block, instr.fs());
                    println!("TODO: round towards zero (specify rounding mode in IR instruction)");
                    let result = block.convert_from(DataType::F32, DataType::S32, fs);
                    guest_regs.set_fgr(instr.fd(), result.val(), FgrLoadState::Full64);
                }
                Some(DataType::F64) => {
                    let fs = guest_regs.get_fgr_64bit_fs(&mut block, instr.fs());
                    println!("TODO: round towards zero (specify rounding mode in IR instruction)");
                    let result = block.convert_from(DataType::F64, DataType::S32, fs);
                    guest_regs.set_fgr(instr.fd(), result.val(), FgrLoadState::Full64);
                }
                _ => panic!(
                    "Unsupported datatype for FPU_TRUNC_W: {:?}",
                    instr.fmt_datatype()
                ),
            },
            MipsOpcode::FPU_CEIL_W => {
                todo!("FPU_CEIL_W")
            }
            MipsOpcode::FPU_FLOOR_W => {
                todo!("FPU_FLOOR_W")
            }
            MipsOpcode::FPU_NEG => {
                checkcp1(&mut block, &mut guest_regs, false);
                match instr.fmt_datatype() {
                    Some(DataType::F32) => {
                        let fs = guest_regs.get_fgr_32bit_fs(&mut block, instr.fs());
                        let result = block.negate(DataType::F32, fs);
                        guest_regs.set_fgr(instr.fd(), result.val(), FgrLoadState::Full64);

                    },
                    Some(DataType::F64) => {
                        let fs = guest_regs.get_fgr_64bit_fs(&mut block, instr.fs());
                        let result = block.negate(DataType::F64, fs);
                        guest_regs.set_fgr(instr.fd(), result.val(), FgrLoadState::Full64);
                    }
                    _ => todo!("Fire unimplemented operation here"),
                }
            }
            MipsOpcode::FPU_C_F => {
                todo!("FPU_C_F")
            }
            MipsOpcode::FPU_C_UN => {
                todo!("FPU_C_UN")
            }
            MipsOpcode::FPU_C_EQ => {
                checkcp1(&mut block, &mut guest_regs, false);
                do_fpu_compare(&instr, &mut block, &mut guest_regs, CompareType::Equal);
            }
            MipsOpcode::FPU_C_UEQ => {
                todo!("FPU_C_UEQ")
            }
            MipsOpcode::FPU_C_OLT => {
                todo!("FPU_C_OLT")
            }
            MipsOpcode::FPU_C_ULT => {
                todo!("FPU_C_ULT")
            }
            MipsOpcode::FPU_C_OLE => {
                todo!("FPU_C_OLE")
            }
            MipsOpcode::FPU_C_ULE => {
                todo!("FPU_C_ULE")
            }
            MipsOpcode::FPU_C_SF => {
                todo!("FPU_C_SF")
            }
            MipsOpcode::FPU_C_NGLE => {
                todo!("FPU_C_NGLE")
            }
            MipsOpcode::FPU_C_SEQ => {
                todo!("FPU_C_SEQ")
            }
            MipsOpcode::FPU_C_NGL => {
                todo!("FPU_C_NGL")
            }
            MipsOpcode::FPU_C_LT => {
                checkcp1(&mut block, &mut guest_regs, false);
                do_fpu_compare(&instr, &mut block, &mut guest_regs, CompareType::LessThan);
            },
            MipsOpcode::FPU_C_NGE => {
                todo!("FPU_C_NGE")
            }
            MipsOpcode::FPU_C_LE => {
                checkcp1(&mut block, &mut guest_regs, false);
                do_fpu_compare(&instr, &mut block, &mut guest_regs, CompareType::LessThanOrEqual);
            },
            MipsOpcode::FPU_C_NGT => {
                todo!("FPU_C_NGT")
            }
            MipsOpcode::FPU_BC1F => {
                checkcp1(&mut block, &mut guest_regs, false);
                let dont_take_branch = guest_regs.get_fcr31_compare(&mut block);
                let take_branch = block.not(DataType::Bool, dont_take_branch).val();
                do_branch(
                    false,
                    false,
                    &mut guest_regs,
                    vaddr,
                    &func,
                    take_branch,
                    instr,
                    cpu_address,
                    &mut pc_set,
                    &mut block,
                    cycles,
                );
            }
            MipsOpcode::FPU_BC1T => {
                checkcp1(&mut block, &mut guest_regs, false);
                let take_branch = guest_regs.get_fcr31_compare(&mut block);
                do_branch(
                    false,
                    false,
                    &mut guest_regs,
                    vaddr,
                    &func,
                    take_branch,
                    instr,
                    cpu_address,
                    &mut pc_set,
                    &mut block,
                    cycles,
                );
            }
            MipsOpcode::FPU_BC1FL => {
                checkcp1(&mut block, &mut guest_regs, false);
                let dont_take_branch = guest_regs.get_fcr31_compare(&mut block);
                let take_branch = block.not(DataType::Bool, dont_take_branch).val();
                do_branch(
                    false,
                    true,
                    &mut guest_regs,
                    vaddr,
                    &func,
                    take_branch,
                    instr,
                    cpu_address,
                    &mut pc_set,
                    &mut block,
                    cycles,
                );
            }
            MipsOpcode::FPU_BC1TL => {
                checkcp1(&mut block, &mut guest_regs, false);
                let take_branch = guest_regs.get_fcr31_compare(&mut block);
                do_branch(
                    false,
                    true,
                    &mut guest_regs,
                    vaddr,
                    &func,
                    take_branch,
                    instr,
                    cpu_address,
                    &mut pc_set,
                    &mut block,
                    cycles,
                );
            }
        }

        cycles += 1;
    }

    if !pc_set {
        set_pc(
            &mut pc_set,
            &mut block,
            cpu_address,
            const_u64(last_vaddr + 4),
        );
    }

    guest_regs.flush_all(&mut block);
    block.ret(Some(const_s32(cycles)));

    return func;
}
