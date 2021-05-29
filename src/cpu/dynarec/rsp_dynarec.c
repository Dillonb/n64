#include <log.h>
#include <rsp.h>
#include "rsp_dynarec.h"
#include "asm_emitter.h"
#include "dynarec_memory_management.h"

void* rsp_link_and_encode(dasm_State** d) {
    size_t code_size;
    dasm_link(d, &code_size);
#ifdef N64_LOG_COMPILATIONS
    printf("Generated %ld bytes of RSP code\n", code_size);
#endif
    void* buf = rsp_dynarec_bumpalloc(code_size);
    dasm_encode(d, buf);

    return buf;
}

void compile_new_rsp_block(rsp_dynarec_block_t* block, word address) {
    static dasm_State* d;
    static dasm_State** Dst;

    d = block_header();
    Dst = &d;

    int block_length = 0;
    int block_extra_cycles = 0;
    bool should_continue_block = true;
    word extra_cycles = 0;

    do {
        static mips_instruction_t instr;
        instr.raw = word_from_byte_array(N64RSP.sp_imem, address);

        dynarec_ir_t* ir = rsp_instruction_ir(instr, address);
        advance_rsp_pc(Dst);
        ir->compiler(Dst, instr, address, NULL, 0, &extra_cycles);
        block_length++;
        block_extra_cycles += extra_cycles;

        // limit ourselves to one instruction per block for now
        should_continue_block = false;
    } while(should_continue_block);

    end_rsp_block(Dst, block_length + block_extra_cycles);
    void* compiled = rsp_link_and_encode(Dst);
    dasm_free(Dst);

    block->run = compiled;
}

int rsp_missing_block_handler() {
    word pc = N64RSP.pc & 0x3FF;
    rsp_dynarec_block_t* block = &N64RSPDYNAREC->blockcache[pc];
    compile_new_rsp_block(block, N64RSP.pc << 2);
    return block->run(&N64RSP);
}

rsp_dynarec_t* rsp_dynarec_init(byte* codecache, size_t codecache_size) {
    rsp_dynarec_t* dynarec = calloc(1, sizeof(rsp_dynarec_t));

    dynarec->codecache_size = codecache_size;
    dynarec->codecache_used = 0;

    for (int i = 0; i < RSP_BLOCKCACHE_SIZE; i++) {
        dynarec->blockcache[i].run = rsp_missing_block_handler;
    }

    dynarec->codecache = codecache;

    return dynarec;
}

int rsp_dynarec_step() {
    rsp_dynarec_block_t* block = &N64RSPDYNAREC->blockcache[N64RSP.pc & 0x3FF];
    // temporary...
    if (block->run == NULL) {
        block->run = rsp_missing_block_handler;
    }
    return block->run(&N64RSP);
}
