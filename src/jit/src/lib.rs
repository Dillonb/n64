// Bindgen
#![allow(non_upper_case_globals)]
#![allow(non_camel_case_types)]
#![allow(non_snake_case)]

use std::mem;

use dgbir::{compiler::compile_vec, disassembler::disassemble_vec_function, util::flush_icache};
use log::{debug, info};
use mips_to_ir::to_ir;

mod mips_parser;
mod mips_to_ir;

include!(concat!(env!("OUT_DIR"), "/c_bindings_generated.rs"));

#[no_mangle]
pub unsafe extern "C" fn rs_jit_compile_new_block(
    block: &mut n64_dynarec_block_t,
    instructions: *mut u32,
    num_instructions: usize,
    virtual_address: u64,
    physical_address: u32,
    cpu: &r4300i_t,
) {
    let safe_code = std::slice::from_raw_parts(instructions, num_instructions);
    let parsed = mips_parser::parse(safe_code, virtual_address, physical_address);
    let mut func = to_ir(parsed, cpu);
    let baseaddr = dynarec_bumpalloc_get_next_allocation_ptr() as usize;
    debug!("{}", func);
    let compiled = compile_vec(&mut func, baseaddr);
    let code = &compiled.code;
    info!("{}", func);

    let alloc = dynarec_bumpalloc(code.len());
    std::ptr::copy_nonoverlapping(code.as_ptr(), alloc as *mut u8, code.len());
    flush_icache(unsafe { std::slice::from_raw_parts(alloc as *const u8, code.len()) });

    let f: unsafe extern "C" fn(*mut r4300i) -> i32 = mem::transmute(alloc);

    block.run = Some(f);
    block.host_size = code.len();
    block.guest_size = num_instructions * 4;

    info!("{}", disassemble_vec_function(&compiled));
}
