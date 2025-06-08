use crate::mips_parser::ParsedMipsInstruction;

pub fn to_ir(parsed: Vec<ParsedMipsInstruction>) {
    for instr in parsed {
        let iw = instr.instr.raw();
        let op_enum = instr.op;
        let vaddr = instr.vaddr;
        let paddr = instr.paddr;
        println!("{vaddr:016X}\t{paddr:08X}\t{iw:08X} (opcode {op_enum:?})");
    }
}
