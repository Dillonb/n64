#include "n64system.h"
#include "scheduler.h"
#include "mprotect_utils.h"

#include <string.h>

#include <mem/n64bus.h>
#include <frontend/render.h>
#include <interface/vi.h>
#include <interface/ai.h>
#include <cpu/rsp.h>
#include <cpu/dynarec/dynarec.h>
#ifndef N64_WIN
#include <sys/mman.h>
#include <errno.h>
#include <unistd.h>
#else
#include <windows.h>
#include <memoryapi.h>
#endif
#include <mem/backup.h>
#include <frontend/game_db.h>
#include <metrics.h>
#include <frontend/device.h>
#include <interface/si.h>
#include <interface/pi.h>
#include <dynarec/rsp_dynarec.h>
#include <mem/pif.h>
#include <timing.h>

static bool should_quit = false;


n64_system_t n64sys;

// 32MiB codecache
#define CODECACHE_SIZE (1 << 25)
static u8 codecache[CODECACHE_SIZE] __attribute__((aligned(4096)));

// 32MiB RSP codecache
#define RSP_CODECACHE_SIZE (1 << 25)
static u8 rsp_codecache[RSP_CODECACHE_SIZE] __attribute__((aligned(4096)));

bool n64_should_quit() {
    return should_quit;
}

void n64_load_rom(const char* rom_path) {
    logalways("Loading %s", rom_path);
    load_n64rom(&n64sys.mem.rom, rom_path);
    n64sys.target_fps = n64sys.mem.rom.pal ? 50 : 60;
    gamedb_match(&n64sys);
    devices_init(n64sys.mem.save_type);
    init_savedata(&n64sys.mem, rom_path);
    if (n64sys.rom_path != rom_path) {
        strcpy(n64sys.rom_path, rom_path);
    }
}

void mprotect_codecache() {
    mprotect_rwx((u8*)&codecache, CODECACHE_SIZE, "codecache");
    mprotect_rwx((u8*)&rsp_codecache, RSP_CODECACHE_SIZE, "codecache");
}

void init_n64system(const char* rom_path, bool enable_frontend, bool enable_debug, n64_video_type_t video_type, bool use_interpreter) {
    memset(&n64sys, 0x00, sizeof(n64_system_t));
    memset(&N64CPU, 0x00, sizeof(N64CPU));
    memset(&N64RSP, 0x00, sizeof(N64RSP));
    init_mem(&n64sys.mem);

    n64sys.video_type = video_type;

    mprotect_codecache();
    n64sys.dynarec = n64_dynarec_init(codecache, CODECACHE_SIZE);
    N64RSP.dynarec = rsp_dynarec_init(rsp_codecache, RSP_CODECACHE_SIZE);

    if (enable_frontend) {
        render_init(video_type);
    }
#ifndef N64_WIN
    n64sys.debugger_state.enabled = enable_debug;
    if (enable_debug) {
        debugger_init();
    }
#endif
    n64sys.use_interpreter = use_interpreter;

    reset_n64system();

    if (rom_path != NULL) {
        n64_load_rom(rom_path);
    }
}

void reset_n64system() {
    force_persist_backup();
    if (n64sys.mem.save_data != NULL) {
        free(n64sys.mem.save_data);
        n64sys.mem.save_data = NULL;
    }
    if (n64sys.mem.mempack_data != NULL) {
        free(n64sys.mem.mempack_data);
        n64sys.mem.mempack_data = NULL;
    }
    N64CPU.branch = false;
    N64CPU.prev_branch = false;
    N64CPU.exception = false;


    for (int i = 0; i < SP_IMEM_SIZE / 4; i++) {
        N64RSP.icache[i].instruction.raw = 0;
        N64RSP.icache[i].handler = cache_rsp_instruction;
    }

    // RSP starts halted with PC 0
    N64RSP.status.halt = true;
    N64RSP.prev_pc = N64RSP.pc = N64RSP.next_pc = 0;

    n64sys.vi.vi_v_intr = 256;

    n64sys.dpc.status.raw = 0x80;


    n64sys.ai.dac.frequency = 44100;
    n64sys.ai.dac.precision = 16;
    n64sys.ai.dac.period = CPU_HERTZ / n64sys.ai.dac.frequency;

    N64CP0.status.raw = 0;
    N64CP0.status.bev = true;
    cp0_status_updated();
    N64CP0.cause.raw  = 0xB000007C;
    N64CP0.EPC        = 0xFFFFFFFFFFFFFFFF;
    N64CP0.PRId       = 0x00000B22;
    N64CP0.config     = 0x7006E463;
    N64CP0.error_epc  = 0xFFFFFFFFFFFFFFFF;

    N64CPU.fcr0.raw = 0xa00;

    memset(n64sys.mem.rdram, 0, N64_RDRAM_SIZE);
    memset(N64RSP.sp_dmem, 0, SP_DMEM_SIZE);
    memset(N64RSP.sp_imem, 0, SP_IMEM_SIZE);
    memset(n64sys.mem.pif_ram, 0, PIF_RAM_SIZE);

    n64sys.vi.num_halflines = 262;
    n64sys.vi.num_fields = 1;
    n64sys.vi.cycles_per_halfline = 1000;

    invalidate_dynarec_all_pages(n64sys.dynarec);

    scheduler_reset();
}

INLINE int jit_system_step() {
    /* Commented out for now since the game never actually reads cp0.random
     * TODO: when a game does, consider generating a random number rather than updating this every instruction
    if (N64CP0.random <= N64CP0.wired) {
        N64CP0.random = 31;
    } else {
        N64CP0.random--;
    }
     */

    if (unlikely(N64CPU.interrupts > 0)) {
        if(N64CP0.status.ie && !N64CP0.status.exl && !N64CP0.status.erl) {
            N64CPU.prev_branch = N64CPU.branch;
            r4300i_handle_exception(N64CPU.pc, EXCEPTION_INTERRUPT, 0);
            return CYCLES_PER_INSTR;
        }
    }
    static int cpu_steps = 0;
    int taken = n64_dynarec_step();
    {
        uint64_t oldcount = N64CP0.count >> 1;
        uint64_t newcount = (N64CP0.count + (taken * CYCLES_PER_INSTR)) >> 1;
        if (unlikely(oldcount < N64CP0.compare && newcount >= N64CP0.compare)) {
            N64CP0.cause.ip7 = true;
            loginfo("Compare interrupt! oldcount: 0x%08lX newcount: 0x%08lX compare 0x%08X", oldcount, newcount, N64CP0.compare);
            r4300i_interrupt_update();
        }
        N64CP0.count += taken;
        N64CP0.count &= 0x1FFFFFFFF;
    }
    cpu_steps += taken;

    if (!N64RSP.status.halt) {
        // 2 RSP steps per 3 CPU steps
        while (cpu_steps > 2) {
            N64RSP.steps += 2;
            cpu_steps -= 3;
        }

        rsp_dynarec_run();
    } else {
        N64RSP.steps = 0;
        cpu_steps = 0;
    }

    return taken;
}

INLINE int interpreter_system_step(const int cycles) {
#ifdef N64_DEBUG_MODE
#ifndef N64_WIN
    if (n64sys.debugger_state.enabled && check_breakpoint(&n64sys.debugger_state, N64CPU.pc)) {
        debugger_breakpoint_hit();
    }
    while (n64sys.debugger_state.broken) {
        usleep(1000);
        debugger_tick();
    }
#endif
#endif

    /* Commented out for now since the game never actually reads cp0.random
    if (N64CPU.cp0.random <= N64CPU.cp0.wired) {
        N64CPU.cp0.random = 31;
    } else {
        N64CPU.cp0.random--;
    }
     */

    if (unlikely(N64CPU.interrupts > 0)) {
        if(N64CP0.status.ie && !N64CP0.status.exl && !N64CP0.status.erl) {
            N64CPU.prev_branch = N64CPU.branch;
            r4300i_handle_exception(N64CPU.pc, EXCEPTION_INTERRUPT, 0);
            return CYCLES_PER_INSTR;
        }
    }

    for (int i = 0; i < cycles; i++) {
        r4300i_step();
    }
    const int taken = cycles;

    static int cpu_steps = 0;
    {
        uint64_t oldcount = N64CP0.count >> 1;
        uint64_t newcount = (N64CP0.count + (taken * CYCLES_PER_INSTR)) >> 1;
        if (unlikely(oldcount < N64CP0.compare && newcount >= N64CP0.compare)) {
            N64CP0.cause.ip7 = true;
            loginfo("Compare interrupt! oldcount: 0x%08lX newcount: 0x%08lX compare 0x%08X", oldcount, newcount, N64CP0.compare);
            r4300i_interrupt_update();
        }
        N64CP0.count += taken;
        N64CP0.count &= 0x1FFFFFFFF;
    }
    cpu_steps += taken;

    if (N64RSP.status.halt) {
        cpu_steps = 0;
        N64RSP.steps = 0;
    } else {
        // 2 RSP steps per 3 CPU steps
        while (cpu_steps > 2) {
            N64RSP.steps += 2;
            cpu_steps -= 3;
        }
        rsp_run();
    }

    return taken;
}

void handle_scheduler_event(scheduler_event_t* event) {
    switch (event->type) {
        case SCHEDULER_SI_DMA_COMPLETE:
            on_si_dma_complete();
            break;
        case SCHEDULER_PI_DMA_COMPLETE:
            on_pi_dma_complete();
            break;
        case SCHEDULER_PI_BUS_WRITE_COMPLETE:
            on_pi_write_complete();
            break;
        default:
            logfatal("");
    }
}

int n64_system_step(bool dynarec, int steps) {
    int taken;
    if (dynarec) {
        taken = jit_system_step();
    } else {
        taken = interpreter_system_step(steps);
    }
    taken += pop_stalled_cycles();

    scheduler_event_t event;
    if (scheduler_tick(taken, &event)) {
        handle_scheduler_event(&event);
    }

    // VI timing
    n64sys.vi.halfline_cycles += taken;

    if (n64sys.vi.halfline_cycles > n64sys.vi.cycles_per_halfline) {
        n64sys.vi.halfline_cycles = 0;
        n64sys.vi.halfline++;
    }

    if (n64sys.vi.halfline > n64sys.vi.num_halflines) {
        n64sys.vi.halfline = 0;
        n64sys.vi.field++;
    }

    if (n64sys.vi.field > n64sys.vi.num_fields) {
        n64sys.vi.field = 0;
    }

    n64sys.vi.v_current = (n64sys.vi.halfline << 1) + n64sys.vi.field;
    check_vi_interrupt();


    return taken;
}

void check_vsync() {
    if (n64sys.vi.v_current == n64sys.vi.vsync >> 1) {
        rdp_update_screen();
    }
}

void jit_system_loop() {
    int cycles = 0;
    while (!should_quit) {
        switch (n64sys.action_queued) {
            case N64_ACTION_NONE:
                break;

            case N64_ACTION_RESET:
                reset_n64system();
                n64_load_rom(n64sys.rom_path);
                pif_rom_execute();
                break;
        }
        n64sys.action_queued = N64_ACTION_NONE;

        for (int field = 0; field < n64sys.vi.num_fields; field++) {
            int this_frame_cycles = 0;
            for (int line = 0; line < n64sys.vi.num_halflines; line++) {
                n64sys.vi.v_current = (line << 1) + field;
                check_vi_interrupt();

                while (cycles <= n64sys.vi.cycles_per_halfline) {
                    int taken = jit_system_step();
                    ai_step(taken);
                    static scheduler_event_t event;
                    if (scheduler_tick(taken, &event)) {
                        handle_scheduler_event(&event);
                    }
                    cycles += taken;
                    this_frame_cycles += taken;
#ifndef N64_WIN
                    n64sys.debugger_state.steps = 0;
#endif
                }
                cycles -= n64sys.vi.cycles_per_halfline;
            }
            check_vi_interrupt();
            rdp_update_screen();

            // Catch up audio if we didn't run the CPU long enough this frame
            int missed_cycles = CPU_CYCLES_PER_FRAME - this_frame_cycles;
            ai_step(missed_cycles);
        }
#ifdef N64_DEBUG_MODE
#ifndef N64_WIN
        if (n64sys.debugger_state.enabled) {
            debugger_tick();
        }
#endif
#endif
#ifdef LOG_ENABLED
        update_delayed_log_verbosity();
#endif
        persist_backup();
        reset_all_metrics();
    }
    force_persist_backup();
}

void interpreter_system_loop() {
    int cycles = 0;
    while (!should_quit) {
        for (int field = 0; field < n64sys.vi.num_fields; field++) {
            int this_frame_cycles = 0;
            for (int line = 0; line < n64sys.vi.num_halflines; line++) {
                n64sys.vi.v_current = (line << 1) + field;
                check_vi_interrupt();

                while (cycles <= n64sys.vi.cycles_per_halfline) {
                    int taken = interpreter_system_step(1);
                    ai_step(taken);
                    static scheduler_event_t event;
                    if (scheduler_tick(taken, &event)) {
                        handle_scheduler_event(&event);
                    }
                    cycles += taken;
                    this_frame_cycles += taken;
#ifndef N64_WIN
                    n64sys.debugger_state.steps = 0;
#endif
                }
                cycles -= n64sys.vi.cycles_per_halfline;
            }
            check_vi_interrupt();
            rdp_update_screen();

            // Catch up audio if we didn't run the CPU long enough this frame
            int missed_cycles = CPU_CYCLES_PER_FRAME - this_frame_cycles;
            ai_step(missed_cycles);
        }
#ifdef N64_DEBUG_MODE
#ifndef N64_WIN
        if (n64sys.debugger_state.enabled) {
            debugger_tick();
        }
#endif
#endif
#ifdef LOG_ENABLED
        update_delayed_log_verbosity();
#endif
        persist_backup();
        reset_all_metrics();
    }
    force_persist_backup();
}

void n64_system_loop() {
    if (n64sys.use_interpreter) {
        interpreter_system_loop();
    } else {
        jit_system_loop();
    }
}

void n64_system_cleanup() {
    if (n64sys.dynarec != NULL) {
        free(n64sys.dynarec);
        n64sys.dynarec = NULL;
    }
#ifndef N64_WIN
    debugger_cleanup();
#endif

    free(n64sys.mem.rom.rom);
    n64sys.mem.rom.rom = NULL;

    free(n64sys.mem.rom.pif_rom);
    n64sys.mem.rom.pif_rom = NULL;
}

void n64_request_quit() {
    should_quit = true;
}

void on_interrupt_change() {
    bool interrupt = n64sys.mi.intr.raw & n64sys.mi.intr_mask.raw;
    loginfo("ip2 is now: %d", interrupt);
    N64CP0.cause.ip2 = interrupt;
    r4300i_interrupt_update();
}

void interrupt_raise(n64_interrupt_t interrupt) {
    switch (interrupt) {
        case INTERRUPT_VI:
            loginfo("Raising VI interrupt");
            n64sys.mi.intr.vi = true;
            break;
        case INTERRUPT_SI:
            loginfo("Raising SI interrupt");
            mark_metric(METRIC_SI_INTERRUPT);
            n64sys.mi.intr.si = true;
            break;
        case INTERRUPT_PI:
            loginfo("Raising PI interrupt");
            mark_metric(METRIC_PI_INTERRUPT);
            n64sys.mi.intr.pi = true;
            break;
        case INTERRUPT_AI:
            loginfo("Raising AI interrupt");
            mark_metric(METRIC_AI_INTERRUPT);
            n64sys.mi.intr.ai = true;
            break;
        case INTERRUPT_DP:
            loginfo("Raising DP interrupt");
            mark_metric(METRIC_DP_INTERRUPT);
            n64sys.mi.intr.dp = true;
            break;
        case INTERRUPT_SP:
            loginfo("Raising SP interrupt");
            mark_metric(METRIC_SP_INTERRUPT);
            n64sys.mi.intr.sp = true;
            break;
        default:
            logfatal("Raising unimplemented interrupt: %d", interrupt);
    }

    on_interrupt_change();
}

void interrupt_lower(n64_interrupt_t interrupt) {
    switch (interrupt) {
        case INTERRUPT_VI:
            n64sys.mi.intr.vi = false;
            loginfo("Lowering VI interrupt");
            break;
        case INTERRUPT_SI:
            n64sys.mi.intr.si = false;
            loginfo("Lowering SI interrupt");
            break;
        case INTERRUPT_PI:
            n64sys.mi.intr.pi = false;
            loginfo("Lowering PI interrupt");
            break;
        case INTERRUPT_DP:
            n64sys.mi.intr.dp = false;
            loginfo("Lowering DP interrupt");
            break;
        case INTERRUPT_AI:
            n64sys.mi.intr.ai = false;
            loginfo("Lowering DP interrupt");
            break;
        case INTERRUPT_SP:
            n64sys.mi.intr.sp = false;
            loginfo("Lowering SP interrupt");
            break;
        default:
            logfatal("Lowering unimplemented interrupt: %d", interrupt);
    }

    on_interrupt_change();
}

void n64_queue_action(n64_action_t action) {
    n64sys.action_queued = action;
}
