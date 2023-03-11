#include <stdio.h>
#include <log.h>
#include <system/n64system.h>
#include <mem/pif.h>
#include <rdp/parallel_rdp_wrapper.h>
#include <imgui/imgui_ui.h>
#include <frontend/frontend.h>
#include <system/scheduler.h>
#include <disassemble.h>
#include <mem/n64bus.h>
#include <cpu/dynarec/dynarec.h>
#include <rsp.h>
#include <dynarec/v2/v2_compiler.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <settings.h>

r4300i_t* n64cpu_interpreter_ptr;
int mq_jit_to_interp_id = -1;
int mq_interp_to_jit_id = -1;

struct num_cycles_msg {
    long mtype;
    int cycles;
};

void send_cycles(int id, int cycles) {
    struct num_cycles_msg m;
    m.mtype = 1;
    m.cycles = cycles;
    if (msgsnd(id, &m, sizeof(m.cycles), 0) == -1) {
        perror("msgsnd");
        exit(1);
    }
}

int recv_cycles(int id) {
    struct num_cycles_msg m;
    if (msgrcv(id, &m, sizeof(m.cycles), 0, 0) == -1) {
        perror("msgrcv");
        exit(1);
    }
    return m.cycles;
}

int create_and_configure_mq(key_t key) {
    int mq_id = msgget(key, IPC_CREAT | 0777);
    if (mq_id == -1) {
        perror("msgget");
        exit(1);
    }
    struct msqid_ds mq_config;
    if (msgctl(mq_id, IPC_STAT, &mq_config) == -1) {
        perror("msgctl stat");
        exit(1);
    }
    mq_config.msg_qbytes = sizeof(int) + 1;
    if (msgctl(mq_id, IPC_SET, &mq_config) == -1) {
        perror("msgctl set");
        exit(1);
    }

    struct num_cycles_msg m;
    while (msgrcv(mq_id, &m, sizeof(m.cycles), 0, IPC_NOWAIT) != -1) {
        printf("Draining queue\n");
    }

    return mq_id;
}


bool compare() {
    bool good = true;
    good &= n64cpu_interpreter_ptr->pc == N64CPU.pc;
    good &= memcmp(n64cpu_interpreter_ptr->gpr, n64cpu_ptr->gpr, sizeof(u64) * 32) == 0;
    good &= memcmp(n64cpu_interpreter_ptr->f, n64cpu_ptr->f, sizeof(fgr_t) * 32) == 0;
    //good &= memcmp(n64sys_interpreter.mem.rdram, n64sys_dynarec.mem.rdram, N64_RDRAM_SIZE) == 0;

    good &= n64cpu_interpreter_ptr->cp0.index == n64cpu_ptr->cp0.index;
    good &= n64cpu_interpreter_ptr->cp0.random == n64cpu_ptr->cp0.random;
    good &= n64cpu_interpreter_ptr->cp0.entry_lo0.raw == n64cpu_ptr->cp0.entry_lo0.raw;
    good &= n64cpu_interpreter_ptr->cp0.entry_lo1.raw == n64cpu_ptr->cp0.entry_lo1.raw;
    good &= n64cpu_interpreter_ptr->cp0.context.raw == n64cpu_ptr->cp0.context.raw;
    good &= n64cpu_interpreter_ptr->cp0.page_mask.raw == n64cpu_ptr->cp0.page_mask.raw;
    good &= n64cpu_interpreter_ptr->cp0.wired == n64cpu_ptr->cp0.wired;
    good &= n64cpu_interpreter_ptr->cp0.bad_vaddr == n64cpu_ptr->cp0.bad_vaddr;
    good &= n64cpu_interpreter_ptr->cp0.count == n64cpu_ptr->cp0.count;
    good &= n64cpu_interpreter_ptr->cp0.entry_hi.raw == n64cpu_ptr->cp0.entry_hi.raw;
    good &= n64cpu_interpreter_ptr->cp0.compare == n64cpu_ptr->cp0.compare;
    good &= n64cpu_interpreter_ptr->cp0.status.raw == n64cpu_ptr->cp0.status.raw;
    good &= n64cpu_interpreter_ptr->cp0.cause.raw == n64cpu_ptr->cp0.cause.raw;
    good &= n64cpu_interpreter_ptr->cp0.EPC == n64cpu_ptr->cp0.EPC;
    good &= n64cpu_interpreter_ptr->cp0.PRId == n64cpu_ptr->cp0.PRId;
    good &= n64cpu_interpreter_ptr->cp0.config == n64cpu_ptr->cp0.config;
    good &= n64cpu_interpreter_ptr->cp0.lladdr == n64cpu_ptr->cp0.lladdr;
    good &= n64cpu_interpreter_ptr->cp0.watch_lo.raw == n64cpu_ptr->cp0.watch_lo.raw;
    good &= n64cpu_interpreter_ptr->cp0.watch_hi == n64cpu_ptr->cp0.watch_hi;
    good &= n64cpu_interpreter_ptr->cp0.x_context.raw == n64cpu_ptr->cp0.x_context.raw;
    good &= n64cpu_interpreter_ptr->cp0.parity_error == n64cpu_ptr->cp0.parity_error;
    good &= n64cpu_interpreter_ptr->cp0.cache_error == n64cpu_ptr->cp0.cache_error;
    good &= n64cpu_interpreter_ptr->cp0.tag_lo == n64cpu_ptr->cp0.tag_lo;
    good &= n64cpu_interpreter_ptr->cp0.tag_hi == n64cpu_ptr->cp0.tag_hi;
    good &= n64cpu_interpreter_ptr->cp0.error_epc == n64cpu_ptr->cp0.error_epc;

    return good;
}

void print_colorcoded_u64(const char* name, u64 expected, u64 actual) {
    printf("%16s 0x%016lX 0x", name, expected);
    for (int offset = 56; offset >= 0; offset -= 8) {
        u64 good_byte = (expected >> offset) & 0xFF;
        u64 bad_byte = (actual >> offset) & 0xFF;
        printf("%s%02X%s", good_byte == bad_byte ? "" : COLOR_RED, (u8)bad_byte, good_byte == bad_byte ? "" : COLOR_END);
    }
    printf("%s" COLOR_END "\n", expected == actual ? COLOR_GREEN " OK!" : COLOR_RED " BAD!");
}

void print_state() {
    printf("expected (interpreter)  actual (dynarec)\n");
    print_colorcoded_u64("PC", n64cpu_interpreter_ptr->pc, N64CPU.pc);
    printf("\n");
    for (int i = 0; i < 32; i++) {
        print_colorcoded_u64(register_names[i], n64cpu_interpreter_ptr->gpr[i], N64CPU.gpr[i]);
    }
    printf("\n");
    for (int i = 0; i < 32; i++) {
        print_colorcoded_u64(cp1_register_names[i], n64cpu_interpreter_ptr->f[i].raw, N64CPU.f[i].raw);
    }

    printf("\n");

    print_colorcoded_u64("cp0 index", n64cpu_interpreter_ptr->cp0.index, N64CPU.cp0.index);
    print_colorcoded_u64("cp0 random", n64cpu_interpreter_ptr->cp0.random, N64CPU.cp0.random);
    print_colorcoded_u64("cp0 entry_lo0", n64cpu_interpreter_ptr->cp0.entry_lo0.raw, N64CPU.cp0.entry_lo0.raw);
    print_colorcoded_u64("cp0 entry_lo1", n64cpu_interpreter_ptr->cp0.entry_lo1.raw, N64CPU.cp0.entry_lo1.raw);
    print_colorcoded_u64("cp0 context", n64cpu_interpreter_ptr->cp0.context.raw, N64CPU.cp0.context.raw);
    print_colorcoded_u64("cp0 page_mask", n64cpu_interpreter_ptr->cp0.page_mask.raw, N64CPU.cp0.page_mask.raw);
    print_colorcoded_u64("cp0 wired", n64cpu_interpreter_ptr->cp0.wired, N64CPU.cp0.wired);
    print_colorcoded_u64("cp0 bad_vaddr", n64cpu_interpreter_ptr->cp0.bad_vaddr, N64CPU.cp0.bad_vaddr);
    print_colorcoded_u64("cp0 count", n64cpu_interpreter_ptr->cp0.count, N64CPU.cp0.count);
    print_colorcoded_u64("cp0 entry_hi", n64cpu_interpreter_ptr->cp0.entry_hi.raw, N64CPU.cp0.entry_hi.raw);
    print_colorcoded_u64("cp0 compare", n64cpu_interpreter_ptr->cp0.compare, N64CPU.cp0.compare);
    print_colorcoded_u64("cp0 status", n64cpu_interpreter_ptr->cp0.status.raw, N64CPU.cp0.status.raw);
    print_colorcoded_u64("cp0 cause", n64cpu_interpreter_ptr->cp0.cause.raw, N64CPU.cp0.cause.raw);
    print_colorcoded_u64("cp0 EPC", n64cpu_interpreter_ptr->cp0.EPC, N64CPU.cp0.EPC);
    print_colorcoded_u64("cp0 PRId", n64cpu_interpreter_ptr->cp0.PRId, N64CPU.cp0.PRId);
    print_colorcoded_u64("cp0 config", n64cpu_interpreter_ptr->cp0.config, N64CPU.cp0.config);
    print_colorcoded_u64("cp0 lladdr", n64cpu_interpreter_ptr->cp0.lladdr, N64CPU.cp0.lladdr);
    print_colorcoded_u64("cp0 watch_lo", n64cpu_interpreter_ptr->cp0.watch_lo.raw, N64CPU.cp0.watch_lo.raw);
    print_colorcoded_u64("cp0 watch_hi", n64cpu_interpreter_ptr->cp0.watch_hi, N64CPU.cp0.watch_hi);
    print_colorcoded_u64("cp0 x_context", n64cpu_interpreter_ptr->cp0.x_context.raw, N64CPU.cp0.x_context.raw);
    print_colorcoded_u64("cp0 parity_error", n64cpu_interpreter_ptr->cp0.parity_error, N64CPU.cp0.parity_error);
    print_colorcoded_u64("cp0 cache_error", n64cpu_interpreter_ptr->cp0.cache_error, N64CPU.cp0.cache_error);
    print_colorcoded_u64("cp0 tag_lo", n64cpu_interpreter_ptr->cp0.tag_lo, N64CPU.cp0.tag_lo);
    print_colorcoded_u64("cp0 tag_hi", n64cpu_interpreter_ptr->cp0.tag_hi, N64CPU.cp0.tag_hi);
    print_colorcoded_u64("cp0 error_epc", n64cpu_interpreter_ptr->cp0.error_epc, N64CPU.cp0.error_epc);

    /*
    for (int i = 0; i < N64_RDRAM_SIZE; i++) {
        if (n64sys_interpreter.mem.rdram[i] != n64sys_dynarec.mem.rdram[i]) {
            printf("%08X: %02X %02X\n", i, n64sys_interpreter.mem.rdram[i], n64sys_dynarec.mem.rdram[i]);
        }
    }
    */
}

void run_compare_parent() {
    u64 start_pc = 0;
    int steps = 0;
    do {
        start_pc = N64CPU.pc;
        // Step jit
        steps = n64_system_step(true, -1);

        // Step interpreter
        send_cycles(mq_jit_to_interp_id, steps);
        // Wait for interpreter to be done
        recv_cycles(mq_interp_to_jit_id);
    } while (compare() && !n64_should_quit());
    send_cycles(mq_jit_to_interp_id, -1); // End the child process
    if (n64_should_quit()) {
        logalways("User requested quit, not doing a final compare");
        exit(0);
    }
    printf("Found a difference at pc: %016lX, ran for %d steps\n", start_pc, steps);
    printf("MIPS code:\n");
    u32 physical = resolve_virtual_address_or_die(start_pc, BUS_LOAD);
    n64_dynarec_block_t* block = &n64dynarec.blockcache[BLOCKCACHE_OUTER_INDEX(physical)][BLOCKCACHE_INNER_INDEX(physical)];
    if (physical >= N64_RDRAM_SIZE) {
        printf("outside of RDAM, can't disassemble (TODO)\n");
    } else {
        print_multi_guest(physical, &n64sys.mem.rdram[physical], block->guest_size);
    }
    printf("IR\n");
    print_ir_block();
    printf("Host code:\n");
    print_multi_host((uintptr_t)block->run, (u8*)block->run, block->host_size);
    print_state();
}

void run_compare_child() {
    while (true) {
        int cycles = recv_cycles(mq_jit_to_interp_id);
        if (cycles < 0) {
            logfatal("Child process done.");
        }
        int taken = n64_system_step(false, cycles);
        send_cycles(mq_interp_to_jit_id, taken);
    }
}

int main(int argc, char** argv) {
    n64_settings_init();
    log_set_verbosity(LOG_VERBOSITY_WARN);
#ifndef INSTANT_DMA
    logfatal("The emulator must be built with INSTANT_DMA for this tool to be effective! (TODO: and probably other DMAs, too)");
#endif
    if (argc != 2) {
        logfatal("Usage: %s <rom>", argv[0]);
    }
    const char* rom_path = argv[1];

    key_t shmem_key = ftok(argv[0], 1);

    key_t mq_jit_to_interp_key = ftok(argv[0], 2);
    mq_jit_to_interp_id = create_and_configure_mq(mq_jit_to_interp_key);
    key_t mq_interp_to_jit_key = ftok(argv[0], 3);
    mq_interp_to_jit_id = create_and_configure_mq(mq_interp_to_jit_key);

    int memory_id = shmget(shmem_key, sizeof(r4300i_t), IPC_CREAT | 0777);
    n64cpu_interpreter_ptr = shmat(memory_id, NULL, 0);

    pid_t pid = fork();
    bool is_child = pid == 0;

    if (is_child) {
        n64cpu_ptr = n64cpu_interpreter_ptr;
    }

    init_n64system(rom_path, true, false, VULKAN_VIDEO_TYPE, false);
    //init_n64system(rom_path, true, false, SOFTWARE_VIDEO_TYPE, false);
    softrdp_init(&n64sys.softrdp_state, (u8 *) &n64sys.mem.rdram);
    prdp_init_internal_swapchain();
    load_imgui_ui();
    register_imgui_event_handler(imgui_handle_event);
    n64_load_rom(rom_path);
    pif_rom_execute();


    u64 start_comparing_at = (s32)n64sys.mem.rom.header.program_counter;

    while (N64CPU.pc != start_comparing_at) {
        n64_system_step(false, 1);
    }

    logalways("ROM booted to %016lX, beginning comparison", start_comparing_at);

    if (is_child) {
        run_compare_child();
    } else {
        run_compare_parent();
    }
}