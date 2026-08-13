use capstone::{arch::BuildsCapstone, Capstone, InsnId};

use crate::mips_parser::{
    MipsCopRsField, MipsInstructionBitfield, MipsOpcodeField, RspLwc2, RspSwc2,
};

// fn get_mips_capstone() -> Capstone {
//     Capstone::new()
//         .mips()
//         .mode(capstone::arch::mips::ArchMode::Mips64)
//         .build()
//         .unwrap()
// }

fn get_rsp_capstone() -> Capstone {
    Capstone::new()
        .mips()
        .mode(capstone::arch::mips::ArchMode::Mips32)
        .build()
        .unwrap()
}

const GPR_NAMES: [&str; 32] = [
    "$zero", "$at", "$v0", "$v1", "$a0", "$a1", "$a2", "$a3", "$t0", "$t1", "$t2", "$t3", "$t4",
    "$t5", "$t6", "$t7", "$s0", "$s1", "$s2", "$s3", "$s4", "$s5", "$s6", "$s7", "$t8", "$t9",
    "$k0", "$k1", "$gp", "$sp", "$fp", "$ra",
];

const RSP_CP0_NAMES: [&str; 16] = [
    "$sp_mem_addr",  // 0
    "$sp_dram_addr", // 1
    "$sp_rd_len",    // 2
    "$sp_wr_len",    // 3
    "$sp_status",    // 4
    "$sp_dma_full",  // 5
    "$sp_dma_busy",  // 6
    "$sp_semaphore", // 7
    "$dp_start",     // 8
    "$dp_end",       // 9
    "$dp_current",   // 10
    "$dp_status",    // 11
    "$dp_clock",     // 12
    "$dp_busy",      // 13
    "$dp_pipe_busy", // 14
    "$dp_tmem_busy", // 15
];

// Gross hack - replace CP0 register names
fn fix_rsp_cp0_op_str(insn_id: InsnId, op_str: &str) -> String {
    const MFC0: u32 = 405;
    const MTC0: u32 = 435;

    let id = insn_id.0 as u32;
    if id != MFC0 && id != MTC0 {
        return op_str.to_string();
    }

    let mut parts = op_str.splitn(3, ',');
    let Some(gpr_part) = parts.next() else {
        return op_str.to_string();
    };
    let Some(cp0_part) = parts.next() else {
        return op_str.to_string();
    };
    let rest = parts.next();

    let cp0_trimmed = cp0_part.trim();
    let reg_idx = GPR_NAMES.iter().position(|&name| name == cp0_trimmed);

    match reg_idx {
        Some(idx) if idx < RSP_CP0_NAMES.len() => match rest {
            Some(r) => format!("{}, {},{}", gpr_part, RSP_CP0_NAMES[idx], r),
            None => format!("{}, {}", gpr_part, RSP_CP0_NAMES[idx]),
        },
        _ => op_str.to_string(),
    }
}

fn vec_elem_str(e: u8) -> String {
    if e == 0 {
        String::new()
    } else {
        format!("[{e}]")
    }
}

fn lswc2_elem_str(e: u8) -> String {
    if e == 0 {
        String::new()
    } else {
        format!("[{e}]")
    }
}

// Disassemble RSP-specific instructions that Capstone cannot decode:
// CP2 vector ops, LWC2/SWC2 vector loads/stores, and CFC2/CTC2/MFC2/MTC2.
fn disassemble_rsp_custom(instr: MipsInstructionBitfield) -> String {
    match instr.op() {
        MipsOpcodeField::CP2 => disassemble_rsp_cp2(instr),
        MipsOpcodeField::LWC2 => disassemble_rsp_lwc2(instr),
        MipsOpcodeField::SWC2 => disassemble_rsp_swc2(instr),
        _ => format!(".word\t0x{:08x}", instr.raw()),
    }
}

fn disassemble_rsp_cp2(instr: MipsInstructionBitfield) -> String {
    if instr.is_cp2_vec() {
        let vd = instr.cp2_vec_vd();
        let vs = instr.cp2_vec_vs();
        let vt = instr.cp2_vec_vt();
        let e = instr.cp2_vec_e();
        let funct = instr.rsp_cop2_vec();
        let mnemonic = format!("{funct:?}").to_lowercase();

        format!("{mnemonic}\t$v{vd}, $v{vs}, $v{vt}{}", vec_elem_str(e))
    } else {
        let rt = instr.cp2_regmove_rt();
        let rd = instr.cp2_regmove_rd();
        let e = instr.cp2_regmove_e();

        match instr.cop_op() {
            MipsCopRsField::MF => {
                format!(
                    "mfc2\t{}, $v{rd}{}",
                    GPR_NAMES[rt as usize],
                    lswc2_elem_str(e)
                )
            }
            MipsCopRsField::MT => {
                format!(
                    "mtc2\t{}, $v{rd}{}",
                    GPR_NAMES[rt as usize],
                    lswc2_elem_str(e)
                )
            }
            MipsCopRsField::CF => {
                format!("cfc2\t{}, $v{rd}", GPR_NAMES[rt as usize])
            }
            MipsCopRsField::CT => {
                format!("ctc2\t{}, $v{rd}", GPR_NAMES[rt as usize])
            }
            _ => format!(".word\t0x{:08x}", instr.raw()),
        }
    }
}

fn disassemble_rsp_lwc2(instr: MipsInstructionBitfield) -> String {
    let vt = instr.lswc2_vt();
    let base = instr.lswc2_base();
    let e = instr.lswc2_e();
    let offset = instr.lswc2_offset() as i8;
    // Sign-extend the 7-bit offset
    let offset = (offset << 1) >> 1;
    let subop = instr.rsp_lwc2();
    let mnemonic = format!("{subop:?}").to_lowercase();

    let shift = lwc2_offset_shift(subop);
    let effective_offset = (offset as i16) << shift;

    format!(
        "{mnemonic}\t$v{vt}{}, {effective_offset}({})",
        lswc2_elem_str(e),
        GPR_NAMES[base as usize]
    )
}

fn disassemble_rsp_swc2(instr: MipsInstructionBitfield) -> String {
    let vt = instr.lswc2_vt();
    let base = instr.lswc2_base();
    let e = instr.lswc2_e();
    let offset = instr.lswc2_offset() as i8;
    let offset = (offset << 1) >> 1;
    let subop = instr.rsp_swc2();
    let mnemonic = format!("{subop:?}").to_lowercase();

    let shift = swc2_offset_shift(subop);
    let effective_offset = (offset as i16) << shift;

    format!(
        "{mnemonic}\t$v{vt}{}, {effective_offset}({})",
        lswc2_elem_str(e),
        GPR_NAMES[base as usize]
    )
}

// The offset field is scaled by the data size of the vector load subopcode.
fn lwc2_offset_shift(subop: RspLwc2) -> u8 {
    match subop {
        RspLwc2::LBV => 0,
        RspLwc2::LSV => 1,
        RspLwc2::LLV => 2,
        RspLwc2::LDV => 3,
        RspLwc2::LQV => 4,
        RspLwc2::LRV => 4,
        RspLwc2::LPV => 3,
        RspLwc2::LUV => 3,
        RspLwc2::LHV => 4,
        RspLwc2::LFV => 4,
        RspLwc2::LTV => 4,
        RspLwc2::LWV => 0,
    }
}

fn swc2_offset_shift(subop: RspSwc2) -> u8 {
    match subop {
        RspSwc2::SBV => 0,
        RspSwc2::SSV => 1,
        RspSwc2::SLV => 2,
        RspSwc2::SDV => 3,
        RspSwc2::SQV => 4,
        RspSwc2::SRV => 4,
        RspSwc2::SPV => 3,
        RspSwc2::SUV => 3,
        RspSwc2::SHV => 4,
        RspSwc2::SFV => 4,
        RspSwc2::STV => 4,
        RspSwc2::SWV => 0,
    }
}

// pub fn disassemble_mips(code: &[u8], addr: u64) -> String {
//     let cs = get_mips_capstone();
//     let insns = cs.disasm_all(code, addr).unwrap();
//     insns
//         .iter()
//         .map(|insn| {
//             format!(
//                 "0x{:x}:\t{}\t{}",
//                 insn.address(),
//                 insn.mnemonic().unwrap(),
//                 insn.op_str().unwrap()
//             )
//         })
//         .collect::<Vec<String>>()
//         .join("\n")
// }

pub fn disassemble_rsp(code: &[u8], addr: u16) -> String {
    let cs = get_rsp_capstone();
    let mut results = Vec::new();
    let mut offset = 0usize;

    while offset + 4 <= code.len() {
        let chunk = &code[offset..offset + 4];
        let instr_addr = (addr as usize + offset) & 0xFFF;
        let raw = u32::from_le_bytes(chunk.try_into().unwrap());
        let bf = MipsInstructionBitfield(raw);

        let is_rsp_specific = matches!(
            bf.op(),
            MipsOpcodeField::CP2 | MipsOpcodeField::LWC2 | MipsOpcodeField::SWC2
        );

        let line = if is_rsp_specific {
            let disasm = disassemble_rsp_custom(bf);
            format!("0x{instr_addr:x}:\t{disasm}")
        } else if let Ok(insns) = cs.disasm_all(chunk, instr_addr as u64) {
            if let Some(insn) = insns.iter().next() {
                let mnemonic = insn.mnemonic().unwrap_or("???");
                let op_str = insn.op_str().unwrap_or("");
                let fixed = fix_rsp_cp0_op_str(insn.id(), op_str);
                format!("0x{:x}:\t{}\t{}", insn.address(), mnemonic, fixed)
            } else {
                format!("0x{instr_addr:x}:\t.word\t0x{raw:08x}")
            }
        } else {
            format!("0x{instr_addr:x}:\t.word\t0x{raw:08x}")
        };

        results.push(line);
        offset += 4;
    }

    results.join("\n")
}

// pub fn disassemble_mips_instruction(instr: u32, addr: u64) -> String {
//     disassemble_mips(&instr.to_le_bytes(), addr)
// }

pub fn disassemble_rsp_instruction(instr: u32, addr: u16) -> String {
    disassemble_rsp(&instr.to_le_bytes(), addr)
}
