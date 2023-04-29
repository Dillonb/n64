#ifndef N64_N64SYSTEM_H
#define N64_N64SYSTEM_H
#ifdef __cplusplus
extern "C" {
#endif
#include <stdbool.h>
#include <mem/n64mem.h>
#include <cpu/r4300i.h>
#include <cpu/rsp_types.h>
#include <interface/vi_reg.h>
#include <debugger/debugger.h>
#include <debugger/debugger_types.h>
#include <rdp/softrdp.h>

#define CPU_HERTZ 93750000
#define CPU_CYCLES_PER_FRAME (CPU_HERTZ / n64sys.target_fps)
#define CYCLES_PER_INSTR 1

// The CPU runs at 93.75mhz. There are 60 frames per second, and 262 lines on the display.
// There are 1562500 cycles per frame.
// Because this doesn't divide nicely by 262, we have to run some lines for 1 more cycle than others.
// We call these the "long" lines, and the others the "short" lines.

// 5963*68+5964*194 == 1562500

#define NUM_SHORTLINES 68
#define NUM_LONGLINES  194

#define SHORTLINE_CYCLES 5963
#define LONGLINE_CYCLES  5964

typedef enum n64_video_type {
    UNKNOWN_VIDEO_TYPE,
    VULKAN_VIDEO_TYPE,
    QT_VULKAN_VIDEO_TYPE,
    SOFTWARE_VIDEO_TYPE
} n64_video_type_t;


typedef struct n64_dynarec n64_dynarec_t;

typedef enum n64_interrupt {
    INTERRUPT_VI,
    INTERRUPT_SI,
    INTERRUPT_PI,
    INTERRUPT_DP,
    INTERRUPT_AI,
    INTERRUPT_SP
} n64_interrupt_t;

typedef union mi_intr_mask {
    u32 raw;
    struct {
        bool sp:1;
        bool si:1;
        bool ai:1;
        bool vi:1;
        bool pi:1;
        bool dp:1;
        unsigned:26;
    };
} mi_intr_mask_t;

typedef union mi_intr {
    u32 raw;
    struct {
        bool sp:1;
        bool si:1;
        bool ai:1;
        bool vi:1;
        bool pi:1;
        bool dp:1;
        unsigned:26;
    };
} mi_intr_t;

typedef union n64_dpc_status {
    u32 raw;
    struct {
        u32 xbus_dmem_dma:1;
        u32 freeze:1;
        u32 flush:1;
        u32 start_gclk:1;
        u32 tmem_busy:1;
        u32 pipe_busy:1;
        u32 buf_busy:1;
        u32 cbuf_ready:1;
        u32 dma_busy:1;
        u32 end_valid:1;
        u32 start_valid:1;
        u32:21;
    } PACKED;
} n64_dpc_status_t;

ASSERTWORD(n64_dpc_status_t);

typedef struct n64_dpc {
    u32 start;
    u32 end;
    u32 current;
    n64_dpc_status_t status;
    u32 clock;
    u32 tmem;
} n64_dpc_t;

typedef union axis_scale {
    u32 raw;
    struct {
        unsigned scale_decimal:10;
        unsigned scale_integer:2;
        unsigned subpixel_offset_decimal:10;
        unsigned subpixel_offset_integer:2;
        unsigned:4;
    };
    struct {
        unsigned scale:12;
        unsigned subpixel_offset:12;
        unsigned:4;
    };
} axis_scale_t;

typedef union axis_start {
    u32 raw;
    struct {
        unsigned end:10;
        unsigned:6;
        unsigned start:10;
        unsigned:6;
    };
} axis_start_t;

typedef struct n64_system {
    n64_mem_t mem;
    n64_video_type_t video_type;
    struct {
        u32 init_mode;
        mi_intr_mask_t intr_mask;
        mi_intr_t intr;
    } mi;
    struct {
        // Timing
        u64 last_halfline_at; // Last halfline hit
        int field; // what field we're on
        int halfline; // what halfline we're on
        int halfline_cycles; // how many cycles into the halfline we are

        vi_status_t status;
        u32 vi_origin;
        u32 vi_width;
        u32 vi_v_intr;
        vi_burst_t vi_burst;
        u32 vsync;
        int num_halflines;
        int num_fields;
        int cycles_per_halfline;
        int missing_cycles;
        u32 hsync;
        u32 leap;
        axis_start_t hstart;
        axis_start_t vstart;
        u32 vburst;
        axis_scale_t xscale;
        axis_scale_t yscale;
        u32 v_current;
        int swaps;
    } vi;
    struct {
        bool dma_enable;
        u16 dac_rate;
        u8 bitrate;
        int dma_count;
        u32 dma_length[2];
        u32 dma_address[2];
        bool dma_address_carry;
        int cycles;

        struct {
            u32 frequency;
            u32 period;
            u32 precision;
        } dac;
    } ai;
    struct {
        bool dma_busy;
        bool dma_to_dram;
    } si;
    struct {
        bool dma_busy;
        bool io_busy;

        u32 latch;
    } pi;
    n64_dpc_t dpc;
#ifndef N64_WIN
    n64_debugger_state_t debugger_state;
#endif
    n64_dynarec_t *dynarec;
    softrdp_state_t softrdp_state;
    bool use_interpreter;
    char rom_path[PATH_MAX];
    unsigned target_fps;
} n64_system_t;

void init_n64system(const char* rom_path, bool enable_frontend, bool enable_debug, n64_video_type_t video_type, bool use_interpreter);
void reset_n64system();
bool n64_should_quit();
void n64_load_rom(const char* rom_path);

void n64_system_step(bool dynarec);
void n64_system_loop();
void n64_system_cleanup();
void n64_request_quit();
void interrupt_raise(n64_interrupt_t interrupt);
void interrupt_lower(n64_interrupt_t interrupt);
void on_interrupt_change();
void check_vsync();
void n64_queue_reset();
extern n64_system_t n64sys;
#define N64DYNAREC n64sys.dynarec
#define PIF_ROM_PATH (n64sys.mem.rom.pal ? "pif.pal.rom" : "pif.rom")
bool file_exists(const char* path);
#ifdef __cplusplus
}
#endif
#endif //N64_N64SYSTEM_H
