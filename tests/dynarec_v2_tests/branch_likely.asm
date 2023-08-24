arch n64.cpu
endian msb

include "regs.inc"

origin $00000000
base $80000000

//; These lines should be run with the interpreter, so the values of t0 and t1 are predictable
addiu t0, r0, 0
addiu t1, r0, 1
//; The rest should be run with the dynarec.
//; This sets up this register so it can't be eliminated with constant propagation first usage is here,
//; and the last usage is in the delay slot that should be skipped.
//; This way we can test that the registers are getting flushed correctly, and the delay slot is getting skipped correctly.
addiu t0, t0, 1
bnel t0, t1, 0x80000020
addiu t0, t0, 1