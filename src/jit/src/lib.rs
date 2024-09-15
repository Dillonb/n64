mod mips_parser;
mod ir;
mod ir_to_x64;

#[no_mangle]
pub unsafe extern fn rs_jit_compile_new_block(instructions: *mut u32, num_instructions: usize, virtual_address: u64, physical_address: u32) {
    let safe_code = std::slice::from_raw_parts(instructions, num_instructions);
    mips_parser::parse(safe_code, virtual_address, physical_address);
}
