global run_block
run_block:
    ; Push saved regs to stack. Each reg is 8 bytes and the stack must be 16-byte aligned.
    push rbx
    push rsp

    push rbp
    push r12

    push r13
    push r14

    push r15

    sub rsp, 8 ; Keep stack aligned

    call rdi

    add rsp, 8
    pop r15


    pop r14
    pop r13

    pop r12
    pop rbp

    pop rsp
    pop rbx

    ret