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

#[no_mangle]
pub extern fn v3_compile_new_block(block: *mut JitBlock, code_mask: *mut c_void, virtual_address: u64, physical_address: u32) {
    panic!("v3_compile_new_block()");
}
