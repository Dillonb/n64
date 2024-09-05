proc_bitfield::bitfield! {
    #[derive(Clone, Copy, PartialEq, Eq)]
    pub struct MipsInstruction(pub u32): Debug, FromStorage, IntoStorage, DerefStorage {
        pub raw : u32 @ ..,

        pub imm:   u16 @ 0..=15,
        pub s_imm: i16 @ 0..=15,

        pub rt: u8 @ 16 .. 20,
        pub rs: u8 @ 21 .. 25,
        pub op: u8 @ 26 .. 31,
    }
}

pub fn parse(code: &[u32], virtual_address: u64, physical_address: u32) {
    let instructions = code.iter().map(|word| MipsInstruction(*word));

    let code_len = code.len();
    println!("Compiling {code_len} instructions at virtual address 0x{virtual_address:016X} and physical address 0x{physical_address:08X}");

    let mut addr = virtual_address;
    for instr in instructions {
        let x = instr.raw();
        let opcode = instr.op();
        println!("{addr:016X}\t{x:08X} (opcode {opcode:X})");
        addr += 4;
    }
}
