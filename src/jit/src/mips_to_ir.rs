use std::mem::offset_of;

use dgbir::ir::{
    const_ptr, const_s16, const_s32, const_s64, const_u16, const_u32, const_u64, CompareType,
    DataType, IRBlockHandle, IRContext, IRFunction, InputSlot, MultiplyType,
};

use crate::{
    bus_access, bus_access_BUS_LOAD, bus_access_BUS_STORE, cp0_status_updated,
    mips_parser::{
        BranchCondition, BranchInfo, MipsInstructionBitfield, MipsOpcode, ParsedMipsInstruction,
    },
    n64_read_physical_byte, n64_read_physical_word, n64_write_physical_byte,
    n64_write_physical_word, r4300i_t, reschedule_compare_interrupt, CP0_ENTRY_HI_WRITE_MASK,
    CP0_PAGEMASK_WRITE_MASK, CP0_STATUS_WRITE_MASK, R4300I_CP0_REG_21, R4300I_CP0_REG_22,
    R4300I_CP0_REG_23, R4300I_CP0_REG_24, R4300I_CP0_REG_25, R4300I_CP0_REG_31, R4300I_CP0_REG_7,
    R4300I_CP0_REG_BADVADDR, R4300I_CP0_REG_CACHEER, R4300I_CP0_REG_CAUSE, R4300I_CP0_REG_COMPARE,
    R4300I_CP0_REG_CONFIG, R4300I_CP0_REG_CONTEXT, R4300I_CP0_REG_COUNT, R4300I_CP0_REG_ENTRYHI,
    R4300I_CP0_REG_ENTRYLO0, R4300I_CP0_REG_ENTRYLO1, R4300I_CP0_REG_EPC, R4300I_CP0_REG_ERR_EPC,
    R4300I_CP0_REG_INDEX, R4300I_CP0_REG_LLADDR, R4300I_CP0_REG_PAGEMASK, R4300I_CP0_REG_PARITYER,
    R4300I_CP0_REG_PRID, R4300I_CP0_REG_RANDOM, R4300I_CP0_REG_STATUS, R4300I_CP0_REG_TAGHI,
    R4300I_CP0_REG_TAGLO, R4300I_CP0_REG_WATCHHI, R4300I_CP0_REG_WATCHLO, R4300I_CP0_REG_WIRED,
    R4300I_CP0_REG_XCONTEXT,
};

struct GuestRegisterManager {
    gprs: [Option<InputSlot>; 32],
    lo: Option<InputSlot>,
    hi: Option<InputSlot>,
    cpu_address: InputSlot,
}

impl GuestRegisterManager {
    pub fn new(cpu_address: InputSlot) -> Self {
        let mut v = GuestRegisterManager {
            gprs: [None; 32],
            lo: None,
            hi: None,
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
            const_u32(bus_access),
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

fn set_pc(block: &mut IRBlockHandle, cpu_address: InputSlot, value: InputSlot) {
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

pub fn to_ir(parsed: Vec<ParsedMipsInstruction>, cpu: &r4300i_t) -> IRFunction {
    let context = IRContext::new();
    let func = IRFunction::new(context);
    let mut block = func.new_block(vec![DataType::Ptr]);

    let cpu_address = block.input(0);

    let mut guest_regs = GuestRegisterManager::new(cpu_address);

    let mut cycles = 0;

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
        match op {
            MipsOpcode::NOP => {}
            MipsOpcode::LD => todo!("LD"),
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
            MipsOpcode::LHU => todo!("LHU"),
            MipsOpcode::LH => todo!("LH"),
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

                let compare_type = match cond {
                    BranchCondition::EQ => CompareType::Equal,
                    BranchCondition::NE => CompareType::NotEqual,
                    BranchCondition::GTZ => {
                        rt_reg = 0;
                        CompareType::GreaterThanSigned
                    }
                    BranchCondition::LTZ => {
                        rt_reg = 0;
                        CompareType::LessThanSigned
                    }
                    BranchCondition::LEZ => {
                        rt_reg = 0;
                        CompareType::LessThanOrEqualSigned
                    }
                    BranchCondition::GEZ => {
                        rt_reg = 0;
                        CompareType::GreaterThanOrEqualSigned
                    }
                };

                let rs = guest_regs.get_gpr(&mut block, rs_reg);
                let rt = guest_regs.get_gpr(&mut block, rt_reg);

                let take_branch = block.compare(rs, compare_type, rt);

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

                set_pc(&mut taken_block, cpu_address, const_u64(taken_pc));
                set_pc(&mut not_taken_block, cpu_address, const_u64(not_taken_pc));

                if likely {
                    // For likely branches, flush all the regs here so we don't have to do it twice
                    // (if the branch is taken)
                    // Regs needed by the delay slot instruction will be reloaded
                    // TODO: it'd be best to somehow not flush registers needed by the delay slot
                    // instruction
                    guest_regs.flush_all(&mut block);
                }

                block.branch(
                    take_branch.val(),
                    taken_block.call(vec![]),
                    not_taken_block.call(vec![]),
                );

                block = func.new_block(vec![]);

                taken_block.jump(block.call(vec![]));
                if likely {
                    // Likely branches, return, don't execute the delay slot.
                    not_taken_block.ret(Some(const_s32(cycles + 1)));
                } else {
                    // Normal branches, continue and execute the delay slot.
                    not_taken_block.jump(block.call(vec![]));
                }
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
            MipsOpcode::SH => todo!("SH"),
            MipsOpcode::SD => todo!("SD"),
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

                set_pc(&mut block, cpu_address, const_u64(target));
            }
            MipsOpcode::JAL => {
                set_link_reg(&mut guest_regs, vaddr, 31);
                let upper_bits = vaddr & 0xFFFFFFFFF0000000;
                let target = (instr.j_target() as u64) << 2 | upper_bits;

                set_pc(&mut block, cpu_address, const_u64(target));
            }
            MipsOpcode::SLTI => todo!("SLTI"),
            MipsOpcode::SLTIU => {
                let rs = guest_regs.get_gpr(&mut block, instr.rs());
                let simm = const_s16(instr.s_imm());
                let result = block.compare(rs, CompareType::LessThanUnsigned, simm);
                guest_regs.set_gpr(instr.rt(), result.val());
            }
            MipsOpcode::XORI => {
                let rs = guest_regs.get_gpr(&mut block, instr.rs());
                let result = block.xor(DataType::U64, rs, const_u16(instr.imm()));
                guest_regs.set_gpr(instr.rt(), result.val());
            }
            MipsOpcode::DADDIU => todo!("DADDIU"),
            MipsOpcode::LB => todo!("LB"),
            MipsOpcode::LDC1 => todo!("LDC1"),
            MipsOpcode::SDC1 => todo!("SDC1"),
            MipsOpcode::LWC1 => todo!("LWC1"),
            MipsOpcode::SWC1 => todo!("SWC1"),
            MipsOpcode::LWL => todo!("LWL"),
            MipsOpcode::LWR => todo!("LWR"),
            MipsOpcode::SWL => todo!("SWL"),
            MipsOpcode::SWR => todo!("SWR"),
            MipsOpcode::LDL => todo!("LDL"),
            MipsOpcode::LDR => todo!("LDR"),
            MipsOpcode::SDL => todo!("SDL"),
            MipsOpcode::SDR => todo!("SDR"),
            MipsOpcode::LL => todo!("LL"),
            MipsOpcode::LLD => todo!("LLD"),
            MipsOpcode::SC => todo!("SC"),
            MipsOpcode::SCD => todo!("SCD"),
            MipsOpcode::RDHWR => todo!("RDHWR"),
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
                    todo!("MFC0 R4300I_CP0_REG_CAUSE")
                }
                R4300I_CP0_REG_COMPARE => {
                    todo!("MFC0 R4300I_CP0_REG_COMPARE")
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
                    todo!("MFC0 R4300I_CP0_REG_EPC")
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
                    todo!("MFC0 R4300I_CP0_REG_INDEX")
                }
                R4300I_CP0_REG_RANDOM => {
                    todo!("MFC0 R4300I_CP0_REG_INDEX")
                }
                R4300I_CP0_REG_COUNT => {
                    todo!("MFC0 R4300I_CP0_REG_COUNT")
                }
                _ => {
                    panic!("Unknown register in MFC0: {}", instr.rd());
                }
            },
            MipsOpcode::DMFC0 => todo!("DMFC0"),
            MipsOpcode::CFC0 => todo!("CFC0"),
            MipsOpcode::DCFC0 => todo!("DCFC0"),
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
                        todo!("MTC0 R4300I_CP0_REG_EPC")
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
            MipsOpcode::DMTC0 => todo!("DMTC0"),
            MipsOpcode::CTC0 => todo!("CTC0"),
            MipsOpcode::DCTC0 => todo!("DCTC0"),
            MipsOpcode::MFC1 => todo!("MFC1"),
            MipsOpcode::DMFC1 => todo!("DMFC1"),
            MipsOpcode::CFC1 => {
                checkcp1(&mut block, &mut guest_regs, true);

                let fs = instr.rd();
                let value = match fs {
                    0 => {
                        println!("Reading FCR0 - probably returning an invalid value!");
                        block.load_ptr(DataType::S32, cpu_address, offset_of!(r4300i_t, fcr0.raw))
                    }
                    31 => {
                        block.load_ptr(DataType::S32, cpu_address, offset_of!(r4300i_t, fcr31.raw))
                    }
                    _ => {
                        todo!("This instruction is only defined when fs == 0 or fs == 31! (Throw an exception?)");
                    }
                };

                guest_regs.set_gpr(instr.rt(), block.convert(DataType::S64, value.val()).val());
            }
            MipsOpcode::DCFC1 => todo!("DCFC1"),
            MipsOpcode::MTC1 => todo!("MTC1"),
            MipsOpcode::DMTC1 => todo!("DMTC1"),
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
                        block.write_ptr(
                            DataType::U32,
                            cpu_address,
                            offset_of!(r4300i_t, fcr31.raw),
                            masked.val(),
                        );
                        println!("TODO: check_fpu_exception();");
                    }
                    _ => {
                        todo!("This instruction is only defined when fs == 0 or fs == 31! (Throw an exception?)");
                    }
                }
            }
            MipsOpcode::DCTC1 => todo!("DCTC1"),
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
            MipsOpcode::SRAV => todo!("SRAV"),
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
                set_pc(&mut block, cpu_address, target);
            }
            MipsOpcode::JALR => {
                let target = guest_regs.get_gpr(&mut block, instr.rs());
                set_pc(&mut block, cpu_address, target);
                set_link_reg(&mut guest_regs, vaddr, instr.rd());
            }
            MipsOpcode::SYSCALL => todo!("SYSCALL"),
            MipsOpcode::SYNC => todo!("SYNC"),
            MipsOpcode::MFHI => {
                let hi = guest_regs.get_hi(&mut block);
                guest_regs.set_gpr(instr.rd(), hi);
            }
            MipsOpcode::MTHI => todo!("MTHI"),
            MipsOpcode::MFLO => {
                let lo = guest_regs.get_lo(&mut block);
                guest_regs.set_gpr(instr.rd(), lo);
            }
            MipsOpcode::MTLO => todo!("MTLO"),
            MipsOpcode::DSLLV => todo!("DSLLV"),
            MipsOpcode::DSRLV => todo!("DSRLV"),
            MipsOpcode::DSRAV => todo!("DSRAV"),
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

                let is_divide_by_zero = block.compare(divisor, CompareType::Equal, const_u32(0));

                let mut normal = func.new_block(vec![]);
                let mut divide_by_zero = func.new_block(vec![]);
                block.branch(
                    is_divide_by_zero.val(),
                    divide_by_zero.call(vec![]),
                    normal.call(vec![]),
                );

                let sign_extended_dividend =
                    divide_by_zero.convert_from(DataType::S32, DataType::S64, dividend);
                divide_by_zero.write_ptr(
                    DataType::S64,
                    cpu_address,
                    offset_of!(r4300i_t, mult_hi),
                    sign_extended_dividend.val(),
                );

                let is_dividend_gte_zero = divide_by_zero.compare(
                    dividend,
                    CompareType::GreaterThanOrEqualSigned,
                    const_s32(0),
                );
                let mut dividend_gte_zero = func.new_block(vec![]);
                let mut dividend_lt_zero = func.new_block(vec![]);
                divide_by_zero.branch(
                    is_dividend_gte_zero.val(),
                    dividend_gte_zero.call(vec![]),
                    dividend_lt_zero.call(vec![]),
                );

                dividend_gte_zero.write_ptr(
                    DataType::S64,
                    cpu_address,
                    offset_of!(r4300i_t, mult_lo),
                    const_s64(-1),
                );

                dividend_lt_zero.write_ptr(
                    DataType::S64,
                    cpu_address,
                    offset_of!(r4300i_t, mult_lo),
                    const_s64(1),
                );

                let result = normal.divide(DataType::S32, dividend, divisor);
                let quotient = normal.convert_from(DataType::S32, DataType::S64, result.at(0));
                let remainder = normal.convert_from(DataType::S32, DataType::S64, result.at(1));

                normal.write_ptr(
                    DataType::S64,
                    cpu_address,
                    offset_of!(r4300i_t, mult_lo),
                    quotient.val(),
                );
                normal.write_ptr(
                    DataType::S64,
                    cpu_address,
                    offset_of!(r4300i_t, mult_hi),
                    remainder.val(),
                );

                let end = func.new_block(vec![]);
                dividend_gte_zero.jump(end.call(vec![]));
                dividend_lt_zero.jump(end.call(vec![]));
                normal.jump(end.call(vec![]));

                block = end;
            }
            MipsOpcode::DIVU => todo!("DIVU"),
            MipsOpcode::DMULT => todo!("DMULT"),
            MipsOpcode::DMULTU => todo!("DMULTU"),
            MipsOpcode::DDIV => todo!("DDIV"),
            MipsOpcode::DDIVU => todo!("DDIVU"),
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
            MipsOpcode::NOR => todo!("NOR"),
            MipsOpcode::SLT => {
                let rs = guest_regs.get_gpr(&mut block, instr.rs());
                let rt = guest_regs.get_gpr(&mut block, instr.rt());
                let result = block.compare(rs, CompareType::LessThanSigned, rt);
                guest_regs.set_gpr(instr.rd(), result.val());
            }
            MipsOpcode::SLTU => {
                let rs = guest_regs.get_gpr(&mut block, instr.rs());
                let rt = guest_regs.get_gpr(&mut block, instr.rt());
                let result = block.compare(rs, CompareType::LessThanUnsigned, rt);
                guest_regs.set_gpr(instr.rd(), result.val());
            }
            MipsOpcode::DADD => todo!("DADD"),
            MipsOpcode::DADDU => todo!("DADDU"),
            MipsOpcode::DSUB => todo!("DSUB"),
            MipsOpcode::DSUBU => todo!("DSUBU"),
            MipsOpcode::TGE => todo!("TGE"),
            MipsOpcode::TGEU => todo!("TGEU"),
            MipsOpcode::TLT => todo!("TLT"),
            MipsOpcode::TLTU => todo!("TLTU"),
            MipsOpcode::TEQ => todo!("TEQ"),
            MipsOpcode::TNE => todo!("TNE"),
            MipsOpcode::DSLL => todo!("DSLL"),
            MipsOpcode::DSRL => todo!("DSRL"),
            MipsOpcode::DSRA => todo!("DSRA"),
            MipsOpcode::DSLL32 => todo!("DSLL32"),
            MipsOpcode::DSRL32 => todo!("DSRL32"),
            MipsOpcode::DSRA32 => todo!("DSRA32"),
            MipsOpcode::TLBWI => {
                println!("TLBWI: TODO, NOP for now");
            }
        }

        cycles += 1;
    }

    guest_regs.flush_all(&mut block);
    block.ret(Some(const_s32(cycles)));

    return func;
}
