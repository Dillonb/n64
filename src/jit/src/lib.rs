// Bindgen
#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

use std::mem;

use dgbir::{compiler::compile, disassembler::disassemble};
use mips_to_ir::to_ir;

mod mips_parser;
mod mips_to_ir;

include!(concat!(env!("OUT_DIR"), "/c_bindings_generated.rs"));

#[no_mangle]
pub unsafe extern "C" fn rs_jit_compile_new_block(
    instructions: *mut u32,
    num_instructions: usize,
    virtual_address: u64,
    physical_address: u32,
    cpu: &r4300i_t,
) {
    println!("Compiling block at 0x{:016X}", virtual_address);
    let safe_code = std::slice::from_raw_parts(instructions, num_instructions);
    let parsed = mips_parser::parse(safe_code, virtual_address, physical_address);
    let mut func = to_ir(parsed, cpu);
    let compiled = compile(&mut func);

    println!("{}", func);
    println!("{}", compiled.allocations.lifetimes);

    let f: extern "C" fn(&r4300i_t) = unsafe { mem::transmute(compiled.ptr_entrypoint()) };
    println!("{}", disassemble(&compiled.code, f as u64));

    println!("Here goes nothing");
    f(cpu);
    println!("I survived!");
    println!("PC: {:016X}", cpu.pc);
    println!("Next PC: {:016X}", cpu.next_pc);

    if cpu.pc & 0xFFFFFFFF00000000 == 0 {
        panic!("Upper 32 bits of PC should not be zero!");
    }
}
