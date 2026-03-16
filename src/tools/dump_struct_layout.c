#include <stdio.h>
#include <stddef.h>
#include <cpu/r4300i.h>
#include <cpu/rsp_types.h>

#define FIELD(type, field) \
    printf("%s.%s %zu %zu\n", #type, #field, offsetof(type, field), sizeof(((type*)0)->field))

#define SIZEOF(type) \
    printf("sizeof:%s %zu\n", #type, sizeof(type))

int main(void) {
    FIELD(rsp_t, gpr);
    FIELD(rsp_t, prev_pc);
    FIELD(rsp_t, pc);
    FIELD(rsp_t, next_pc);
    FIELD(rsp_t, sp_dmem);
    FIELD(rsp_t, sp_imem);
    FIELD(rsp_t, steps);
    FIELD(rsp_t, status);
    FIELD(rsp_t, io);
    FIELD(rsp_t, io.mem_addr);
    FIELD(rsp_t, io.dram_addr);
    FIELD(rsp_t, io.shadow_mem_addr);
    FIELD(rsp_t, io.shadow_dram_addr);
    FIELD(rsp_t, io.dma);
    FIELD(rsp_t, icache);
    FIELD(rsp_t, vu_regs);
    FIELD(rsp_t, vcc);
    FIELD(rsp_t, vcc.l);
    FIELD(rsp_t, vcc.h);
    FIELD(rsp_t, vco);
    FIELD(rsp_t, vco.l);
    FIELD(rsp_t, vco.h);
    FIELD(rsp_t, vce);
    FIELD(rsp_t, acc);
    FIELD(rsp_t, acc.h);
    FIELD(rsp_t, acc.m);
    FIELD(rsp_t, acc.l);
    FIELD(rsp_t, sync);
    FIELD(rsp_t, divin);
    FIELD(rsp_t, divin_loaded);
    FIELD(rsp_t, divout);
    FIELD(rsp_t, semaphore_held);
    FIELD(rsp_t, dynarec);
    FIELD(rsp_t, zero);

    FIELD(r4300i_t, gpr);
    FIELD(r4300i_t, f);
    FIELD(r4300i_t, pc);
    FIELD(r4300i_t, next_pc);
    FIELD(r4300i_t, prev_pc);
    FIELD(r4300i_t, mult_hi);
    FIELD(r4300i_t, mult_lo);
    FIELD(r4300i_t, llbit);
    FIELD(r4300i_t, fcr0);
    FIELD(r4300i_t, fcr31);
    FIELD(r4300i_t, cp0);
    FIELD(r4300i_t, cp2_latch);
    FIELD(r4300i_t, icache);
    FIELD(r4300i_t, dcache);
    FIELD(r4300i_t, interrupts);
    FIELD(r4300i_t, branch);
    FIELD(r4300i_t, prev_branch);
    FIELD(r4300i_t, branch_likely_taken);
    FIELD(r4300i_t, exception);

    SIZEOF(rsp_t);
    SIZEOF(r4300i_t);

    return 0;
}
