use itertools::izip;
use proc_bitfield::ConvRaw;

#[derive(ConvRaw, Debug)]
enum MipsOpcode {
    CP0    = 0b010000,
    CP1    = 0b010001,
    CP2    = 0b010010,
    CP3    = 0b010011,
    LD     = 0b110111,
    LUI    = 0b001111,
    ADDI   = 0b001000,
    ADDIU  = 0b001001,
    DADDI  = 0b011000,
    ANDI   = 0b001100,
    LBU    = 0b100100,
    LHU    = 0b100101,
    LH     = 0b100001,
    LW     = 0b100011,
    LWU    = 0b100111,
    BEQ    = 0b000100,
    BEQL   = 0b010100,
    BGTZ   = 0b000111,
    BGTZL  = 0b010111,
    BLEZ   = 0b000110,
    BLEZL  = 0b010110,
    BNE    = 0b000101,
    BNEL   = 0b010101,
    CACHE  = 0b101111,
    REGIMM = 0b000001,
    SPCL   = 0b000000,
    SB     = 0b101000,
    SH     = 0b101001,
    SD     = 0b111111,
    SW     = 0b101011,
    ORI    = 0b001101,
    J      = 0b000010,
    JAL    = 0b000011,
    SLTI   = 0b001010,
    SLTIU  = 0b001011,
    XORI   = 0b001110,
    DADDIU = 0b011001,
    LB     = 0b100000,
    LDC1   = 0b110101,
    SDC1   = 0b111101,
    LWC1   = 0b110001,
    SWC1   = 0b111001,
    LWL    = 0b100010,
    LWR    = 0b100110,
    SWL    = 0b101010,
    SWR    = 0b101110,
    LDL    = 0b011010,
    LDR    = 0b011011,
    SDL    = 0b101100,
    SDR    = 0b101101,
    LL     = 0b110000,
    LLD    = 0b110100,
    SC     = 0b111000,
    SCD    = 0b111100,
    RDHWR  = 0b011111
}

proc_bitfield::bitfield! {
    #[derive(Clone, Copy, PartialEq, Eq)]
    pub struct MipsInstruction(pub u32): Debug, FromStorage, IntoStorage, DerefStorage {
        pub raw : u32 @ ..,

        pub imm:   u16 @ 0..=15,
        pub s_imm: i16 @ 0..=15,

        pub rt: u8 @ 16 ..=20,
        pub rs: u8 @ 21 ..=25,

        pub op_bits: u8 @ 26 ..=31,
        pub op: u8 [unwrap MipsOpcode] @ 26 ..= 31
    }
}

pub fn parse(code: &[u32], virtual_address: u64, physical_address: u32) {
    let instructions = code.iter().map(|word| MipsInstruction(*word));
    let zipped = izip!(instructions, (virtual_address..).step_by(4), (physical_address..).step_by(4));

    let code_len = code.len();
    println!("Compiling {code_len} instructions at virtual address 0x{virtual_address:016X} and physical address 0x{physical_address:08X}");

    for (instruction, vaddr, paddr) in zipped {
        let iw = instruction.raw();
        let op_enum = instruction.op();
        println!("{vaddr:016X}\t{paddr:08X}\t{iw:08X} (opcode {op_enum:?})");
    }
}
