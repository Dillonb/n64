arch n64.cpu
endian msb

include "regs.inc"

origin $00000000
base $80000000

addiu t0, r0, 0
bnel t0, r0, 0x80000020
addiu t0, t4, 1
nop