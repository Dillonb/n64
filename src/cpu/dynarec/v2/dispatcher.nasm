extern n64cpu
global run_block
run_block:
    ; Push saved regs to stack. Each reg is 8 bytes and the stack must be 16-byte aligned.
    ; Stack is misaligned by 8 at first
    push rbx

    ; Stack is now aligned - push regs in pairs.
    push rsp
    push rbp

    push r12
    push r13

    push r14
    push r15

    ; r12 always holds a pointer to the CPU struct
    lea r12, [rel n64cpu]

    call rdi

    pop r15
    pop r14

    pop r13
    pop r12

    pop rbp
    pop rsp

    pop rbx

    ret