use std::rc::Rc;

pub enum IrSize {
    U8,
    S8,
    U16,
    S16,
    U32,
    S32
}

pub enum MipsRegister {
    Gpr(u8),
    Cop0(u8),
}

pub enum IrInstruction {
    Nop,
    SetConstant,
    SetFloatConstant,
    Or(Rc<IrInstruction>, Rc<IrInstruction>),
    Xor(Rc<IrInstruction>, Rc<IrInstruction>),
    And(Rc<IrInstruction>, Rc<IrInstruction>),
    Sub,
    Not,
    Add(Rc<IrInstruction>, Rc<IrInstruction>),
    Shift,
    Store,
    Load,
    GetPtr(u64, IrSize),
    SetPtr(u64, IrSize),
    MaskAndCast,
    CheckCondition,

    // rework these
    SetCondBlockExitPc,
    SetBlockExitPc,
    CondBlockExit,

    TlbLookup,
    LoadGuestReg,
    FlushGuestReg,
    Multiply,
    Divide,
    Eret,
    Call,
    MovRegType,
    FloatConvert,
    FloatMultiply,
    FloatDivide,
    FloatAdd,
    FloatSub,
    FloatSqrt,
    FloatAbs,
    FloatNeg,
    FloatCheckCondition,
    InterpreterFallback
}
