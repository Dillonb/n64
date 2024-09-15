use crate::ir::IrInstruction;
use dynasmrt::{dynasm, DynasmApi, DynasmLabelApi};

pub fn compile(code : &[IrInstruction]) {
    let mut ops = dynasmrt::x64::Assembler::new().unwrap();
    for instr in code {
        match instr {
            IrInstruction::Nop => todo!("IR Nop"),
            IrInstruction::SetConstant => todo!("IR SetConstant"),
            IrInstruction::SetFloatConstant => todo!("IR SetFloatConstant"),
            IrInstruction::Or(_, _) => todo!("IR Or"),
            IrInstruction::Xor(_, _) => todo!("IR Xor"),
            IrInstruction::And(_, _) => todo!("IR And"),
            IrInstruction::Sub => todo!("IR Sub"),
            IrInstruction::Not => todo!("IR Not"),
            IrInstruction::Add(_, _) => todo!("IR Add"),
            IrInstruction::Shift => todo!("IR Shift"),
            IrInstruction::Store => todo!("IR Store"),
            IrInstruction::Load => todo!("IR Load"),
            IrInstruction::GetPtr(address, size) => {
                dynasm!(ops
                    ; .arch x64
                    ; mov rax, rcx
                    // ; push [rsp + rsp]
                )
            },
            IrInstruction::SetPtr(_, _) => todo!("IR SetPtr"),
            IrInstruction::MaskAndCast => todo!("IR MaskAndCast"),
            IrInstruction::CheckCondition => todo!("IR CheckCondition"),
            IrInstruction::SetCondBlockExitPc => todo!("IR SetCondBlockExitPc"),
            IrInstruction::SetBlockExitPc => todo!("IR SetBlockExitPc"),
            IrInstruction::CondBlockExit => todo!("IR CondBlockExit"),
            IrInstruction::TlbLookup => todo!("IR TlbLookup"),
            IrInstruction::LoadGuestReg => todo!("IR LoadGuestReg"),
            IrInstruction::FlushGuestReg => todo!("IR FlushGuestReg"),
            IrInstruction::Multiply => todo!("IR Multiply"),
            IrInstruction::Divide => todo!("IR Divide"),
            IrInstruction::Eret => todo!("IR Eret"),
            IrInstruction::Call => todo!("IR Call"),
            IrInstruction::MovRegType => todo!("IR MovRegType"),
            IrInstruction::FloatConvert => todo!("IR FloatConvert"),
            IrInstruction::FloatMultiply => todo!("IR FloatMultiply"),
            IrInstruction::FloatDivide => todo!("IR FloatDivide"),
            IrInstruction::FloatAdd => todo!("IR FloatAdd"),
            IrInstruction::FloatSub => todo!("IR FloatSub"),
            IrInstruction::FloatSqrt => todo!("IR FloatSqrt"),
            IrInstruction::FloatAbs => todo!("IR FloatAbs"),
            IrInstruction::FloatNeg => todo!("IR FloatNeg"),
            IrInstruction::FloatCheckCondition => todo!("IR FloatCheckCondition"),
            IrInstruction::InterpreterFallback => todo!("IR InterpreterFallback"),
        }
    }

}
