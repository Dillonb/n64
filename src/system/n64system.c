#include "n64system.h"
#include "scheduler.h"
#include "mprotect_utils.h"
#include "scheduler_utils.h"

#include <frontend/http_api.h>
#include <string.h>

#include <mem/n64bus.h>
#include <frontend/render.h>
#include <interface/vi.h>
#include <interface/ai.h>
#include <cpu/rsp.h>
#ifdef N64_DYNAREC_ENABLED
#include <cpu/dynarec/dynarec.h>
#include <dynarec/rsp_dynarec.h>
#endif
#include <util.h>
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
#include <mem/pif.h>
#include <timing.h>

static bool should_quit = false;


n64_system_t n64sys;

static u8 codecache[CODECACHE_SIZE] __attribute__((aligned(4096)));

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

#ifdef LOG_CPU_STATE
FILE* log_file = NULL;
#endif

void init_n64system(const char* rom_path, bool enable_frontend, bool enable_debug, n64_video_type_t video_type, bool use_interpreter) {
    if (n64cpu_ptr) {
        logwarn("n64cpu already initialized");
    } else {
        n64cpu_ptr = malloc(sizeof(r4300i_t));
    }
#ifdef LOG_CPU_STATE
    log_file = fopen("cpu_log.bin", "wb");
    logalways("Opened log for writing");
#endif

    memset(&n64sys, 0x00, sizeof(n64_system_t));
    memset(&N64CPU, 0x00, sizeof(N64CPU));
    memset(&N64RSP, 0x00, sizeof(N64RSP));
    init_mem(&n64sys.mem);

    n64sys.video_type = video_type;

    mprotect_codecache();
#ifdef N64_DYNAREC_ENABLED
    n64_dynarec_init(codecache, CODECACHE_SIZE);
#endif
#ifdef N64_DYNAREC_V1_ENABLED
    N64RSP.dynarec = rsp_dynarec_init(rsp_codecache, RSP_CODECACHE_SIZE);
#endif

    if (enable_frontend) {
        render_init(video_type);
    }
    n64sys.debugger_state.enabled = enable_debug;
    if (enable_debug) {
        debugger_init();
    }

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
    if (n64sys.mem.mempak_data != NULL) {
        free(n64sys.mem.mempak_data);
        n64sys.mem.mempak_data = NULL;
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

#ifdef N64_DYNAREC_ENABLED
    invalidate_dynarec_all_pages();
#endif

    scheduler_reset();
    scheduler_enqueue_relative((u64)n64sys.vi.cycles_per_halfline, SCHEDULER_VI_HALFLINE);
}

#ifdef N64_DYNAREC_ENABLED
INLINE int jit_system_step() {
    int taken = n64_dynarec_step();
    N64CP0.count += taken;
    N64CP0.count &= 0x1FFFFFFFF;
    return taken;
}
#endif

INLINE int interpreter_system_step_matchjit(const int cycles) {
    for (int i = 0; i < cycles; i++) {
        r4300i_step();
#ifdef INSTANT_DMA
        N64CPU.fcr31.flag = 0;
        N64CPU.fcr31.cause = 0;
#endif
    N64CP0.count++;
    N64CP0.count &= 0x1FFFFFFFF;
    }

    return cycles;
}

#ifdef LOG_CPU_STATE
void log_cpu_state() {
    fwrite(&N64CPU.pc, sizeof(u64), 1, log_file);
    fwrite(&N64CPU.gpr, sizeof(u64), 32, log_file);
    fwrite(&N64CP0.cause.raw, sizeof(u32), 1, log_file);
    //fwrite(&N64CP0.status.raw, sizeof(u32), 1, log_file);
    fwrite(&n64sys.mi.intr.raw, sizeof(u32), 1, log_file);
    //fwrite(&n64sys.mi.intr_mask.raw, sizeof(u32), 1, log_file);
}
#endif

INLINE void interpreter_system_step() {
#ifdef N64_DEBUG_MODE
    check_breakpoint(N64CPU.pc);
    while (n64sys.debugger_state.broken) {
#ifdef N64_WIN
        Sleep(1);
#else
        usleep(1000);
#endif
        debugger_tick();
    }
#endif

#ifdef LOG_CPU_STATE
    log_cpu_state();
#endif
    r4300i_step();

    static int cpu_steps = 0;
    N64CP0.count++;
    N64CP0.count &= 0x1FFFFFFFF;
    cpu_steps++;

    if (N64RSP.status.halt) {
        cpu_steps = 0;
        N64RSP.steps = 0;
    } else {
        // 2 RSP steps per 3 CPU steps
        N64RSP.steps += (cpu_steps / 3) * 2;
        cpu_steps %= 3;

        rsp_run();
    }
}

void on_vi_halfline_complete(u64 time) {
    // VI timing
    s64 taken = time - n64sys.vi.last_halfline_at;
    n64sys.vi.last_halfline_at = time;

    u64 overshot;

    do {
        overshot = taken - n64sys.vi.cycles_per_halfline;

        n64sys.vi.halfline++;

        if (n64sys.vi.halfline > n64sys.vi.num_halflines) {
            n64sys.vi.halfline = 0;
            n64sys.vi.field++;
            if (n64sys.video_type != UNKNOWN_VIDEO_TYPE) {
                persist_backup();
                reset_all_metrics();
                ai_step(n64sys.vi.missing_cycles);
                rdp_update_screen();
            }
        }

        if (n64sys.vi.field > n64sys.vi.num_fields) {
            n64sys.vi.field = 0;
        }

        n64sys.vi.v_current = (n64sys.vi.halfline << 1) + n64sys.vi.field;
        check_vi_interrupt();
        taken -= n64sys.vi.cycles_per_halfline;
    } while (taken > n64sys.vi.cycles_per_halfline);

    u64 next_event = n64sys.vi.cycles_per_halfline - overshot;
    scheduler_enqueue_relative(next_event, SCHEDULER_VI_HALFLINE);
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
        case SCHEDULER_VI_HALFLINE:
            on_vi_halfline_complete(n64scheduler.scheduler_ticks);
            break;
        case SCHEDULER_RESET_SYSTEM:
            reset_n64system();
            n64_load_rom(n64sys.rom_path);
            pif_rom_execute();
            break;
        case SCHEDULER_COMPARE_INTERRUPT:
            N64CP0.cause.ip7 = true;
            r4300i_interrupt_update();
            reschedule_compare_interrupt(0);
            break;
        case SCHEDULER_HANDLE_INTERRUPT:
            if(N64CP0.status.ie && !N64CP0.status.exl && !N64CP0.status.erl) {
                N64CPU.prev_branch = N64CPU.branch;
                r4300i_handle_exception(N64CPU.pc, EXCEPTION_INTERRUPT, 0);
            }
            break;
        default:
            logfatal("Unknown scheduler event type");
    }
}

int n64_system_step(bool dynarec, int steps) {
    static int cpu_steps = 0;

    int taken;
    if (dynarec) {
#ifdef N64_DYNAREC_ENABLED
        taken = jit_system_step();
#else
        logfatal("Dynarec is not enabled!");
#endif
    } else {
        taken = interpreter_system_step_matchjit(steps);
    }
    taken += pop_stalled_cycles();

    cpu_steps += taken;

    scheduler_event_t event;
    if (scheduler_tick(taken, &event)) {
        handle_scheduler_event(&event);

        ai_step(cpu_steps);
        if (!N64RSP.status.halt) {
            // 2 RSP steps per 3 CPU steps
            N64RSP.steps += (cpu_steps / 3) * 2;
            cpu_steps %= 3;
#ifdef N64_DYNAREC_V1_ENABLED
            rsp_dynarec_run();
#else
            rsp_run();
#endif
        } else {
            N64RSP.steps = 0;
            cpu_steps = 0;
        }
    }

    ai_step(taken);
    return taken;
}

void check_vsync() {
    if (n64sys.vi.v_current == n64sys.vi.vsync >> 1) {
        rdp_update_screen();
    }
}

void n64_queue_reset() {
    scheduler_enqueue_relative(0, SCHEDULER_RESET_SYSTEM);
}

#ifdef N64_DYNAREC_ENABLED
void jit_system_loop() {
    while (!should_quit) {
        static int cpu_steps = 0;
        while (true) {
            int taken = jit_system_step();
            cpu_steps += taken;
            static scheduler_event_t event;
            if (scheduler_tick(taken, &event)) {
                handle_scheduler_event(&event);
                break;
            }
        }

        ai_step(cpu_steps);
        if (!N64RSP.status.halt) {
            // 2 RSP steps per 3 CPU steps
            N64RSP.steps += (cpu_steps / 3) * 2;
            cpu_steps %= 3;

#ifdef N64_DYNAREC_V1_ENABLED
            rsp_dynarec_run();
#else
            rsp_run();
#endif
        } else {
            N64RSP.steps = 0;
            cpu_steps = 0;
        }
    }
    force_persist_backup();
}
#endif

void interpreter_system_loop() {
    while (!should_quit) {
        interpreter_system_step();
        ai_step(1);
        static scheduler_event_t event;
        if (scheduler_tick(1, &event)) {
            handle_scheduler_event(&event);
        }
    }
}

void n64_system_loop() {
#ifdef N64_DYNAREC_ENABLED
    if (n64sys.use_interpreter) {
#endif
        interpreter_system_loop();
#ifdef N64_DYNAREC_ENABLED
    } else {
        jit_system_loop();
    }
#endif
}

void n64_system_cleanup() {
#ifndef N64_WIN
    debugger_cleanup();
#endif

    free(n64sys.mem.rom.rom);
    n64sys.mem.rom.rom = NULL;

    free(n64sys.mem.rom.pif_rom);
    n64sys.mem.rom.pif_rom = NULL;
    http_api_stop();
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