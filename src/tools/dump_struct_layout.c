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

    FIELD(cp0_t, index);
    FIELD(cp0_t, random);
    FIELD(cp0_t, entry_lo0);
    FIELD(cp0_t, entry_lo1);
    FIELD(cp0_t, context);
    FIELD(cp0_t, page_mask);
    FIELD(cp0_t, wired);
    FIELD(cp0_t, bad_vaddr);
    FIELD(cp0_t, count);
    FIELD(cp0_t, entry_hi);
    FIELD(cp0_t, compare);
    FIELD(cp0_t, status);
    FIELD(cp0_t, cause);
    FIELD(cp0_t, EPC);
    FIELD(cp0_t, PRId);
    FIELD(cp0_t, config);
    FIELD(cp0_t, lladdr);
    FIELD(cp0_t, watch_lo);
    FIELD(cp0_t, watch_hi);
    FIELD(cp0_t, x_context);
    FIELD(cp0_t, parity_error);
    FIELD(cp0_t, cache_error);
    FIELD(cp0_t, tag_lo);
    FIELD(cp0_t, tag_hi);
    FIELD(cp0_t, error_epc);
    FIELD(cp0_t, open_bus);
    SIZEOF(cp0_t);

    FIELD(fcr0_t, raw);
    SIZEOF(fcr0_t);

    FIELD(fcr31_t, raw);
    SIZEOF(fcr31_t);

    return 0;
}
