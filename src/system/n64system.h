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
#define CPU_CYCLES_PER_FRAME (CPU_HERTZ / 60)
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
    OPENGL_VIDEO_TYPE,
    VULKAN_VIDEO_TYPE,
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
    word raw;
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
    word raw;
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

typedef struct n64_controller {
    bool plugged_in;
    union {
        byte byte1;
        struct {
            bool dp_right:1;
            bool dp_left:1;
            bool dp_down:1;
            bool dp_up:1;
            bool start:1;
            bool z:1;
            bool b:1;
            bool a:1;
        };
    };
    union {
        byte byte2;
        struct {
            bool c_right:1;
            bool c_left:1;
            bool c_down:1;
            bool c_up:1;
            bool r:1;
            bool l:1;
            bool zero:1;
            bool joy_reset:1;
        };
    };
    sbyte joy_x;
    sbyte joy_y;
} n64_controller_t;

typedef struct n64_dpc {
    word start;
    word end;
    word current;
    union {
        word raw;
        struct {
            bool xbus_dmem_dma:1;
            bool freeze:1;
            bool flush:1;
            bool start_gclk:1;
            bool tmem_busy:1;
            bool pipe_busy:1;
            bool cmd_busy:1;
            bool cbuf_ready:1;
            bool dma_busy:1;
            bool end_valid:1;
            bool start_valid:1;
            unsigned:21;
        };
    } status;
    word clock;
    word bufbusy;
    word pipebusy;
    word tmem;
} n64_dpc_t;

typedef struct n64_system {
    n64_mem_t mem;
    r4300i_t cpu;
    rsp_t rsp;
    n64_video_type_t video_type;
    struct {
        word init_mode;
        mi_intr_mask_t intr_mask;
        mi_intr_t intr;
    } mi;
    struct {
        vi_status_t status;
        word vi_origin;
        word vi_width;
        word vi_v_intr;
        vi_burst_t vi_burst;
        word vsync;
        word hsync;
        word leap;
        word hstart;
        union {
            word raw;
            struct {
                unsigned vend:10;
                unsigned:5;
                unsigned vstart:10;
                unsigned:6;
            };
        } vstart;
        word vburst;
        word xscale;
        word yscale;
        word v_current;
    } vi;
    struct {
        bool dma_enable;
        half dac_rate;
        byte bitrate;
        int dma_count;
        word dma_length[2];
        word dma_address[2];
        int cycles;

        struct {
            word frequency;
            word period;
            word precision;
        } dac;
    } ai;
    struct {
        n64_controller_t controllers[4];
    } si;
    n64_dpc_t dpc;
    n64_debugger_state_t debugger_state;
    n64_dynarec_t *dynarec;
    softrdp_state_t softrdp_state;
    bool use_interpreter;
    char rom_path[PATH_MAX];
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
extern n64_system_t n64sys;
#define N64CPU n64sys.cpu
#define N64CP0 N64CPU.cp0
#define N64RSP n64sys.rsp
#define N64DYNAREC n64sys.dynarec
#ifdef __cplusplus
}
#endif
#endif //N64_N64SYSTEM_H
