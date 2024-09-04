use std::ffi::c_void;

#[repr(C)]
pub struct JitBlock {
    // int (*run)(r4300i_t* cpu);
    pub run: *mut c_void,
    // size_t guest_size;
    pub guest_size: usize,
    // size_t host_size;
    pub host_size: usize,
    // n64_block_sysconfig_t sysconfig;
    pub sysconfig : u64,
    // u64 virtual_address;
    pub virtual_address : u64,
    // struct n64_dynarec_block* next; // for other sysconfigs
    pub next: *mut JitBlock
}

fn compile_new_block(_block: &mut JitBlock, _code_mask: &mut [u8], virtual_address: u64, physical_address: u32) {
    println!("Compiling a new block at virtual address 0x{virtual_address:016X} and physical address 0x{physical_address:08X}");
    // block.run = std::ptr::null_mut();
    // block.host_size = 1;
    // block.guest_size = 2;
    // block.sysconfig = 3;
    // block.virtual_address = 4;
    // block.next = std::ptr::null_mut();
}

#[no_mangle]
pub unsafe extern fn v3_compile_new_block(block: *mut JitBlock, code_mask: *mut u8, code_mask_size: usize, virtual_address: u64, physical_address: u32) {
    let safe_block = block.as_mut().unwrap_unchecked();
    let safe_code_mask = std::slice::from_raw_parts_mut(code_mask, code_mask_size);
    compile_new_block(safe_block, safe_code_mask, virtual_address, physical_address);
}
