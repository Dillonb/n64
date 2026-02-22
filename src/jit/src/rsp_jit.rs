use dgbir::disassembler::{disassemble_mips, disassemble_mips_instruction};

use crate::{mips_parser::parse_rsp, *};

fn rsp_instruction_category(instr: mips_instruction_t) -> InstructionCategory {
    if unsafe { instr.raw } == 0 {
        return InstructionCategory::Normal;
    }

    let op = unsafe { instr.i.op() };
    let funct = unsafe { instr.r.funct() };

    match op {
        // Branches
        OPC_BEQ | OPC_BGTZ | OPC_BLEZ | OPC_BNE | OPC_J | OPC_JAL => InstructionCategory::Branch,

        // SPECIAL: only branches are JR/JALR
        OPC_SPCL => match funct {
            FUNCT_JR | FUNCT_JALR => InstructionCategory::Branch,
            FUNCT_BREAK => InstructionCategory::BlockEnder,
            _ => InstructionCategory::Normal,
        },

        // REGIMM: all RSP regimm instructions are branches
        OPC_REGIMM => InstructionCategory::Branch,

        // Everything else is Normal
        _ => InstructionCategory::Normal,
    }
}

fn rsp_read_instruction(address: u16) -> mips_instruction_t {
    let bytes = unsafe {
        n64rsp.sp_imem[address as usize..address as usize + 4]
            .try_into()
            .unwrap()
    };
    let raw = u32::from_le_bytes(bytes);
    mips_instruction_t { raw }
}

fn next_rsp_address(address: u16) -> u16 {
    address.wrapping_add(4) & 0xFFF
}

#[no_mangle]
pub extern "C" fn rs_jit_compile_new_rsp_block(
    block: *mut rsp_dynarec_block_t,
    start_address: u16,
    current_overlay: *mut rsp_code_overlay_t,
) {
    let mut should_continue_block;
    let mut address = start_address;
    let mut instructions_left_in_block = -1;
    let mut branch_in_block = false;
    let mut prev_instr_category = InstructionCategory::Normal;

    let mut code = Vec::new();

    loop {
        let instr = rsp_read_instruction(address);
        let category = rsp_instruction_category(instr);

        let instr_raw = unsafe { instr.raw };

        code.push(instr_raw);

        instructions_left_in_block -= 1;

        match category {
            InstructionCategory::Normal => {
                should_continue_block = instructions_left_in_block != 0;
            }
            InstructionCategory::Branch => {
                branch_in_block = true;
                if prev_instr_category == InstructionCategory::Branch {
                    todo!("Nested branches in RSP code");
                }

                should_continue_block = true;
                instructions_left_in_block = 1; // Emit delay slot and then we're done.
            }
            InstructionCategory::BlockEnder => {
                should_continue_block = false;
            }
            _ => panic!("Invalid instruction category for RSP: {:?}", category),
        }

        address = next_rsp_address(address);
        prev_instr_category = category;

        if !should_continue_block {
            break;
        }
    }

    let parsed = parse_rsp(&code, address);

    if !branch_in_block {
        todo!("Add code to flush PC")
    }

    todo!("RSP jit")
}
