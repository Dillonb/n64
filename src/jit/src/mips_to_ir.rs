use std::mem::offset_of;

use dgbir::ir::{
    const_ptr, const_s16, const_u16, const_u32, const_u64, CompareType, DataType, IRBlockHandle,
    IRContext, IRFunction, InputSlot,
};

use crate::{
    bus_access, bus_access_BUS_LOAD, bus_access_BUS_STORE,
    mips_parser::{
        BranchCondition, BranchInfo, MipsInstructionBitfield, MipsOpcode, ParsedMipsInstruction,
    },
    n64_read_physical_word, n64_write_physical_word, r4300i_t,
};

struct GuestRegisterManager {
    gprs: [Option<InputSlot>; 32],
    cpu_address: InputSlot,
}

impl GuestRegisterManager {
    pub fn new(cpu_address: InputSlot) -> Self {
        let mut v = GuestRegisterManager {
            gprs: [None; 32],
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

    fn get_register(&mut self, block: &mut IRBlockHandle, r: u8) -> InputSlot {
        *self.gprs[r as usize].get_or_insert_with(|| {
            let offset = offset_of!(r4300i_t, gpr) + (r as usize * std::mem::size_of::<u64>());
            block
                .load_ptr(DataType::U64, self.cpu_address, offset)
                .val()
        })
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
    let base = guest_regs.get_register(block, instr.rs());
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

pub fn to_ir(parsed: Vec<ParsedMipsInstruction>, cpu: &r4300i_t) -> IRFunction {
    let context = IRContext::new();
    let func = IRFunction::new(context);
    let mut block = func.new_block(vec![DataType::Ptr]);

    let cpu_address = block.input(0);

    let mut guest_regs = GuestRegisterManager::new(cpu_address);

    for ParsedMipsInstruction {
        paddr: _paddr,
        vaddr,
        instr,
        op,
    } in parsed
    {
        match op {
            MipsOpcode::NOP => {}
            MipsOpcode::LD => todo!("LD"),
            MipsOpcode::LUI => {
                let c = (instr.imm() as u32) << 16;
                guest_regs.set_gpr(instr.rt(), const_u32(c));
            }
            MipsOpcode::ADDI => todo!("ADDI"),
            MipsOpcode::ADDIU => todo!("ADDIU"),
            MipsOpcode::DADDI => todo!("DADDI"),
            MipsOpcode::ANDI => {
                let rs = guest_regs.get_register(&mut block, instr.rs());
                let result = block.and(DataType::U64, rs, const_u16(instr.imm()));
                guest_regs.set_gpr(instr.rt(), result.val());
            }
            MipsOpcode::LBU => todo!("LBU"),
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
            MipsOpcode::LWU => todo!("LWU"),
            MipsOpcode::BRANCH(BranchInfo { cond, likely, link }) => {
                assert_eq!(likely, false, "Likely branches are not supported yet");
                assert_eq!(link, false, "Link branches are not supported yet");

                let compare_type = match cond {
                    BranchCondition::EQ => todo!(),
                    BranchCondition::NE => CompareType::NotEqual,
                    BranchCondition::GTZ => todo!(),
                    BranchCondition::LTZ => todo!(),
                    BranchCondition::LEZ => todo!(),
                    BranchCondition::GEZ => todo!(),
                };

                let rs = guest_regs.get_register(&mut block, instr.rs());
                let rt = guest_regs.get_register(&mut block, instr.rt());

                let take_branch = block.compare(rs, compare_type, rt);

                let mut taken_block = func.new_block(vec![]);
                let mut not_taken_block = func.new_block(vec![]);

                let taken_pc = vaddr.wrapping_add(4).wrapping_add_signed((instr.s_imm() as i64) << 2);
                let not_taken_pc = vaddr.wrapping_add(8);

                println!(
                    "Jumping to {:016X} if taken, continuing to {:016X} if not taken",
                    taken_pc, not_taken_pc
                );

                set_pc(&mut taken_block, cpu_address, const_u64(taken_pc));
                set_pc(&mut not_taken_block, cpu_address, const_u64(not_taken_pc));

                block.branch(
                    take_branch.val(),
                    taken_block.call(vec![]),
                    not_taken_block.call(vec![]),
                );

                block = func.new_block(vec![]);

                taken_block.jump(block.call(vec![]));
                not_taken_block.jump(block.call(vec![]));
            }
            MipsOpcode::CACHE => todo!("CACHE"),
            MipsOpcode::SB => todo!("SB"),
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
                let to_write = guest_regs.get_register(&mut block, instr.rt());
                block.call_function(
                    const_ptr(n64_write_physical_word as usize),
                    None,
                    vec![paddr, to_write],
                );
            }
            MipsOpcode::ORI => {
                let rs = guest_regs.get_register(&mut block, instr.rs());
                let result = block.or(DataType::U64, rs, const_u16(instr.imm()));
                guest_regs.set_gpr(instr.rt(), result.val());
            }
            MipsOpcode::J => todo!("J"),
            MipsOpcode::JAL => todo!("JAL"),
            MipsOpcode::SLTI => todo!("SLTI"),
            MipsOpcode::SLTIU => todo!("SLTIU"),
            MipsOpcode::XORI => todo!("XORI"),
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
            MipsOpcode::MFC0 => todo!("MFC0"),
            MipsOpcode::DMFC0 => todo!("DMFC0"),
            MipsOpcode::CFC0 => todo!("CFC0"),
            MipsOpcode::DCFC0 => todo!("DCFC0"),
            MipsOpcode::MTC0 => todo!("MTC0"),
            MipsOpcode::DMTC0 => todo!("DMTC0"),
            MipsOpcode::CTC0 => todo!("CTC0"),
            MipsOpcode::DCTC0 => todo!("DCTC0"),
            MipsOpcode::MFC1 => todo!("MFC1"),
            MipsOpcode::DMFC1 => todo!("DMFC1"),
            MipsOpcode::CFC1 => todo!("CFC1"),
            MipsOpcode::DCFC1 => todo!("DCFC1"),
            MipsOpcode::MTC1 => todo!("MTC1"),
            MipsOpcode::DMTC1 => todo!("DMTC1"),
            MipsOpcode::CTC1 => todo!("CTC1"),
            MipsOpcode::DCTC1 => todo!("DCTC1"),
            MipsOpcode::SLL => todo!("SLL"),
            MipsOpcode::SRL => todo!("SRL"),
            MipsOpcode::SRA => todo!("SRA"),
            MipsOpcode::SRAV => todo!("SRAV"),
            MipsOpcode::SLLV => todo!("SLLV"),
            MipsOpcode::SRLV => todo!("SRLV"),
            MipsOpcode::JR => {
                let target = guest_regs.get_register(&mut block, instr.rs());
                set_pc(&mut block, cpu_address, target);
            },
            MipsOpcode::JALR => todo!("JALR"),
            MipsOpcode::SYSCALL => todo!("SYSCALL"),
            MipsOpcode::SYNC => todo!("SYNC"),
            MipsOpcode::MFHI => todo!("MFHI"),
            MipsOpcode::MTHI => todo!("MTHI"),
            MipsOpcode::MFLO => todo!("MFLO"),
            MipsOpcode::MTLO => todo!("MTLO"),
            MipsOpcode::DSLLV => todo!("DSLLV"),
            MipsOpcode::DSRLV => todo!("DSRLV"),
            MipsOpcode::DSRAV => todo!("DSRAV"),
            MipsOpcode::MULT => todo!("MULT"),
            MipsOpcode::MULTU => todo!("MULTU"),
            MipsOpcode::DIV => todo!("DIV"),
            MipsOpcode::DIVU => todo!("DIVU"),
            MipsOpcode::DMULT => todo!("DMULT"),
            MipsOpcode::DMULTU => todo!("DMULTU"),
            MipsOpcode::DDIV => todo!("DDIV"),
            MipsOpcode::DDIVU => todo!("DDIVU"),
            MipsOpcode::ADD => todo!("ADD"),
            MipsOpcode::ADDU => todo!("ADDU"),
            MipsOpcode::AND => todo!("AND"),
            MipsOpcode::SUB => todo!("SUB"),
            MipsOpcode::SUBU => todo!("SUBU"),
            MipsOpcode::OR => todo!("OR"),
            MipsOpcode::XOR => todo!("XOR"),
            MipsOpcode::NOR => todo!("NOR"),
            MipsOpcode::SLT => todo!("SLT"),
            MipsOpcode::SLTU => todo!("SLTU"),
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
        }
    }

    guest_regs.flush_all(&mut block);
    block.ret(None);

    return func;
}
