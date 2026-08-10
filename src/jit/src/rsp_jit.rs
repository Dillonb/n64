use crate::{rsp_mips_parser::parse_rsp, rsp_mips_to_ir::rsp_to_ir, *};

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

fn rsp_read_instruction(rsp: &rsp_t, address: u16) -> mips_instruction_t {
    let bytes = rsp.sp_imem[address as usize..address as usize + 4]
        .try_into()
        .unwrap();
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
    rsp: &rsp_t,
) {
    let mut should_continue_block;
    let mut address = start_address;
    let mut instructions_left_in_block = -1;
    let mut prev_instr_category = InstructionCategory::Normal;

    let mut code = Vec::new();

    loop {
        let instr = rsp_read_instruction(rsp, address);
        let category = rsp_instruction_category(instr);

        let instr_raw = unsafe { instr.raw };

        code.push(instr_raw);
        unsafe {
            (*current_overlay).code[(address as usize) >> 2] = instr_raw;
            (*current_overlay).code_mask[(address as usize) >> 2] = 0xFFFFFFFF;
        }

        instructions_left_in_block -= 1;

        match category {
            InstructionCategory::Normal => {
                should_continue_block = instructions_left_in_block != 0;
            }
            InstructionCategory::Branch => {
                if prev_instr_category == InstructionCategory::Branch {
                    // Branch in a delay slot: end the block, rsp_to_ir will interpret it.
                    should_continue_block = false;
                } else {
                    should_continue_block = true;
                    instructions_left_in_block = 1; // Emit delay slot and then we're done.
                }
            }
            InstructionCategory::BlockEnder => {
                should_continue_block = false;
            }
        }

        address = next_rsp_address(address);
        prev_instr_category = category;

        if !should_continue_block {
            break;
        }
    }

    let parsed = parse_rsp(&code, start_address);
    let mut func = rsp_to_ir(parsed, rsp);

    let baseaddr = unsafe { rsp_dynarec_bumpalloc_get_next_allocation_ptr() as usize };
    let compiled = compile_vec(&mut func, baseaddr);

    let code = compiled.code;

    unsafe {
        let alloc = rsp_dynarec_bumpalloc(code.len());
        std::ptr::copy_nonoverlapping(code.as_ptr(), alloc as *mut u8, code.len());
        flush_icache(std::slice::from_raw_parts(alloc as *const u8, code.len()));
        let f: unsafe extern "C" fn(*mut rsp) -> i32 = mem::transmute(alloc);

        (*block).run = Some(f);
    }
}
