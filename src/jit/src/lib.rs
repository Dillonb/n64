// Bindgen
#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

use mips_to_ir::to_ir;

mod mips_parser;
mod mips_to_ir;

include!(concat!(env!("OUT_DIR"), "/c_bindings_generated.rs"));

#[no_mangle]
pub unsafe extern fn rs_jit_compile_new_block(instructions: *mut u32, num_instructions: usize, virtual_address: u64, physical_address: u32) {
    let safe_code = std::slice::from_raw_parts(instructions, num_instructions);
    let parsed = mips_parser::parse(safe_code, virtual_address, physical_address);
    to_ir(parsed);
}

#[no_mangle]
pub unsafe extern fn rs_jit_test_c_repr_struct(cpu: &r4300i_t) {
    for i in 0..32 {
        println!("GPR[{:2}]: {:016X}", i, cpu.gpr[i]);
    }
    println!("PC: {:016X}", cpu.pc);
    println!("Next PC: {:016X}", cpu.next_pc);
    println!("Previous PC: {:016X}", cpu.prev_pc);
}
