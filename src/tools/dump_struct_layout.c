#include <stdio.h>
#include <stddef.h>
#include <cpu/rsp_types.h>

#define FIELD(type, field) \
    printf("%s.%s %zu %zu\n", #type, #field, offsetof(type, field), sizeof(((type*)0)->field))

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
    return 0;
}
