#include "rdp.h"
#include <mem/mem_util.h>

#include <dlfcn.h>
#ifdef N64_MACOS
#include <limits.h>
#else
#include <linux/limits.h>

#endif
#include <stdbool.h>

#include "mupen_interface.h"
#include "parallel_rdp_wrapper.h"
#include "softrdp.h"
#include <log.h>
#include <frontend/render.h>

static void* plugin_handle = NULL;
static mupen_graphics_plugin_t graphics_plugin;
static word rdram_size_word = N64_RDRAM_SIZE; // GFX_INFO needs this to be sent as a uint32

#define RDP_COMMAND_BUFFER_SIZE 0xFFFFF
word rdp_command_buffer[RDP_COMMAND_BUFFER_SIZE];

#define FROM_RDRAM(address) word_from_byte_array(n64sys.mem.rdram, WORD_ADDRESS(address))
#define FROM_DMEM(address) word_from_byte_array(n64sys.rsp.sp_dmem, address)

static const int command_lengths[64] = {
        2, 2, 2, 2, 2, 2, 2, 2, 8, 12, 24, 28, 24, 28, 40, 44,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2,  2,  2,  2,  2,  2,  2,
        2, 2, 2, 2, 4, 4, 2, 2, 2, 2,  2,  2,  2,  2,  2,  2,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2,  2,  2,  2,  2,  2,  2
};

#define RDP_COMMAND_FULL_SYNC 0x29


void rdp_rendering_callback(int redrawn) {
    n64_poll_input();
    n64_render_screen();
}

#define ADDR_DPC_START_REG    0x04100000
#define ADDR_DPC_END_REG      0x04100004
#define ADDR_DPC_CURRENT_REG  0x04100008
#define ADDR_DPC_STATUS_REG   0x0410000C
#define ADDR_DPC_CLOCK_REG    0x04100010
#define ADDR_DPC_BUFBUSY_REG  0x04100014
#define ADDR_DPC_PIPEBUSY_REG 0x04100018
#define ADDR_DPC_TMEM_REG     0x0410001C


#define LOAD_SYM(var, name) do { \
    var = dlsym(plugin_handle, name); \
        if (var == NULL) { logfatal("Failed to load RDP plugin! Missing symbol: %s", name); } \
    } while(false)

void rdp_check_interrupts() {
    on_interrupt_change();
}

GFX_INFO get_gfx_info() {
    GFX_INFO gfx_info;
    gfx_info.HEADER = n64sys.mem.rom.rom;
    gfx_info.RDRAM = n64sys.mem.rdram;
    gfx_info.DMEM = n64sys.rsp.sp_dmem;
    gfx_info.IMEM = n64sys.rsp.sp_imem;

    gfx_info.MI_INTR_REG = &n64sys.mi.intr.raw;

    gfx_info.DPC_START_REG    = &n64sys.dpc.start;
    gfx_info.DPC_END_REG      = &n64sys.dpc.end;
    gfx_info.DPC_CURRENT_REG  = &n64sys.dpc.current;
    gfx_info.DPC_STATUS_REG   = &n64sys.dpc.status.raw;
    gfx_info.DPC_CLOCK_REG    = &n64sys.dpc.clock;
    gfx_info.DPC_BUFBUSY_REG  = &n64sys.dpc.bufbusy;
    gfx_info.DPC_PIPEBUSY_REG = &n64sys.dpc.pipebusy;
    gfx_info.DPC_TMEM_REG     = &n64sys.dpc.tmem;

    gfx_info.VI_STATUS_REG         = &n64sys.vi.status.raw;
    gfx_info.VI_ORIGIN_REG         = &n64sys.vi.vi_origin;
    gfx_info.VI_WIDTH_REG          = &n64sys.vi.vi_width;
    gfx_info.VI_INTR_REG           = &n64sys.vi.vi_v_intr;
    gfx_info.VI_V_CURRENT_LINE_REG = &n64sys.vi.v_current;
    gfx_info.VI_TIMING_REG         = &n64sys.vi.vi_burst.raw;
    gfx_info.VI_V_SYNC_REG         = &n64sys.vi.vsync;
    gfx_info.VI_H_SYNC_REG         = &n64sys.vi.hsync;
    gfx_info.VI_LEAP_REG           = &n64sys.vi.leap;
    gfx_info.VI_H_START_REG        = &n64sys.vi.hstart;
    gfx_info.VI_V_START_REG        = &n64sys.vi.vstart.raw;
    gfx_info.VI_V_BURST_REG        = &n64sys.vi.vburst;
    gfx_info.VI_X_SCALE_REG        = &n64sys.vi.xscale;
    gfx_info.VI_Y_SCALE_REG        = &n64sys.vi.yscale;

    gfx_info.CheckInterrupts = &rdp_check_interrupts;

    gfx_info.version = 2;

    gfx_info.SP_STATUS_REG = &n64sys.rsp.status.raw;
    gfx_info.RDRAM_SIZE = &rdram_size_word;

    return gfx_info;
}

void load_rdp_plugin(const char* filename) {
    char path[PATH_MAX] = "";
    if (filename[0] == '.' || filename[0] == '/') {
        snprintf(path, sizeof(path), "%s", filename);
    } else {
        snprintf(path, sizeof(path), "./%s", filename);
    }

    plugin_handle = dlopen(path, RTLD_NOW);
    if (plugin_handle == NULL) {
        logfatal("Failed to load RDP plugin. Please pass a path to a shared library file!");
    }

    init_mupen_interface();

    LOAD_SYM(graphics_plugin.PluginStartup, "PluginStartup");
    LOAD_SYM(graphics_plugin.PluginGetVersion, "PluginGetVersion");
    LOAD_SYM(graphics_plugin.ChangeWindow, "ChangeWindow");
    LOAD_SYM(graphics_plugin.InitiateGFX, "InitiateGFX");
    LOAD_SYM(graphics_plugin.MoveScreen, "MoveScreen");
    LOAD_SYM(graphics_plugin.ProcessDList, "ProcessDList");
    LOAD_SYM(graphics_plugin.ProcessRDPList, "ProcessRDPList");
    LOAD_SYM(graphics_plugin.RomClosed, "RomClosed");
    LOAD_SYM(graphics_plugin.RomOpen, "RomOpen");
    LOAD_SYM(graphics_plugin.ShowCFB, "ShowCFB");
    LOAD_SYM(graphics_plugin.UpdateScreen, "UpdateScreen");
    LOAD_SYM(graphics_plugin.ViStatusChanged, "ViStatusChanged");
    LOAD_SYM(graphics_plugin.ViWidthChanged, "ViWidthChanged");
    LOAD_SYM(graphics_plugin.ReadScreen2, "ReadScreen2");
    LOAD_SYM(graphics_plugin.SetRenderingCallback, "SetRenderingCallback");
    LOAD_SYM(graphics_plugin.FBRead, "FBRead");
    LOAD_SYM(graphics_plugin.FBWrite, "FBWrite");
    LOAD_SYM(graphics_plugin.FBGetFrameBufferInfo, "FBGetFrameBufferInfo");

    m64p_plugin_type plugin_type;
    int plugin_version;
    int api_version;
    const char* plugin_name;
    int capabilities;


    graphics_plugin.PluginGetVersion(&plugin_type, &plugin_version, &api_version, &plugin_name, &capabilities);
    if (plugin_type != M64PLUGIN_GFX) {
        logfatal("Plugin loaded successfully, but was not a graphics plugin!");
    }

    graphics_plugin.PluginStartup(NULL, NULL, NULL); // Null handle, null debug callbacks.

    GFX_INFO gfx_info = get_gfx_info();

    graphics_plugin.InitiateGFX(gfx_info);
    graphics_plugin.RomOpen();

    graphics_plugin.SetRenderingCallback(rdp_rendering_callback);

    // TODO: check plugin version, API version, etc for compatibility

    printf("Loaded RDP plugin %s\n", plugin_name);
}

void write_word_dpcreg(word address, word value) {
    switch (address) {
        case ADDR_DPC_START_REG:
            n64sys.dpc.start = value & 0xFFFFFF;
            n64sys.dpc.current = n64sys.dpc.start;
            break;
        case ADDR_DPC_END_REG:
            n64sys.dpc.end = value & 0xFFFFFF;
            rdp_run_command();
            break;
        case ADDR_DPC_CURRENT_REG:
            logfatal("Writing word to unimplemented DPC register: ADDR_DPC_CURRENT_REG");
        case ADDR_DPC_STATUS_REG:
            rdp_status_reg_write(value);
            break;
        case ADDR_DPC_CLOCK_REG:
            logfatal("Writing word to unimplemented DPC register: ADDR_DPC_CLOCK_REG");
        case ADDR_DPC_BUFBUSY_REG:
            logfatal("Writing word to unimplemented DPC register: ADDR_DPC_BUFBUSY_REG");
        case ADDR_DPC_PIPEBUSY_REG:
            logfatal("Writing word to unimplemented DPC register: ADDR_DPC_PIPEBUSY_REG");
        case ADDR_DPC_TMEM_REG:
            logfatal("Writing word to unimplemented DPC register: ADDR_DPC_TMEM_REG");
        default:
            logfatal("Writing word 0x%08X to address 0x%08X in unsupported region: REGION_DP_COMMAND_REGS", value, address);
    }
}

word read_word_dpcreg(word address) {
    switch (address) {
        case ADDR_DPC_START_REG:
            return n64sys.dpc.start;
        case ADDR_DPC_END_REG:
            return n64sys.dpc.end;
        case ADDR_DPC_CURRENT_REG:
            return n64sys.dpc.current;
        case ADDR_DPC_STATUS_REG:
            return n64sys.dpc.status.raw;
        case ADDR_DPC_CLOCK_REG:
            return n64sys.dpc.clock;
        case ADDR_DPC_BUFBUSY_REG:
            return n64sys.dpc.bufbusy;
        case ADDR_DPC_PIPEBUSY_REG:
            return n64sys.dpc.pipebusy;
        case ADDR_DPC_TMEM_REG:
            return n64sys.dpc.tmem;
        default:
            logfatal("Reading word from address 0x%08X in unsupported region: REGION_DP_COMMAND_REGS", address);
    }
}

void rdp_cleanup() {
    if (graphics_plugin.RomClosed) {
        graphics_plugin.RomClosed();
    }
}

INLINE void rdp_enqueue_command(int command_length, word* buffer) {
    switch (n64sys.video_type) {
        case UNKNOWN_VIDEO_TYPE:  logfatal("RDP enqueue command with video type UNKNOWN_VIDEO_TYPE");
        case OPENGL_VIDEO_TYPE:   logfatal("RDP enqueue command with video type OPENGL_VIDEO_TYPE");
        case VULKAN_VIDEO_TYPE:   parallel_rdp_enqueue_command(command_length, buffer); break;
        case SOFTWARE_VIDEO_TYPE: enqueue_command_softrdp(&n64sys.softrdp_state, command_length, buffer); break;
    }
}

INLINE void rdp_on_full_sync() {
    switch (n64sys.video_type) {
        case UNKNOWN_VIDEO_TYPE:  logfatal("RDP on full sync with video type UNKNOWN_VIDEO_TYPE");
        case OPENGL_VIDEO_TYPE:   logfatal("RDP on full sync with video type OPENGL_VIDEO_TYPE");
        case VULKAN_VIDEO_TYPE:   parallel_rdp_on_full_sync(); break;
        case SOFTWARE_VIDEO_TYPE: full_sync_softrdp(); break;
    }
}

void process_rdp_list() {
    static int last_run_unprocessed_words = 0;

    n64_dpc_t* dpc = &n64sys.dpc;

    // tell the game to not touch RDP stuff while we work
    dpc->status.freeze = true;

    // force align to 8 byte boundaries
    const word current = dpc->current & 0x00FFFFF8;
    const word end = dpc->end & 0x00FFFFF8;

    // How many bytes do we need to process?
    int display_list_length = end - current;

    if (display_list_length <= 0) {
        // No commands to run
        return;
    }

    if (display_list_length + (last_run_unprocessed_words * 4) > RDP_COMMAND_BUFFER_SIZE) {
        logfatal("Got a command of display_list_length %d / 0x%X (with %d unprocessed words from last run) - this overflows our buffer of size %d / 0x%X!",
                 display_list_length, display_list_length, last_run_unprocessed_words, RDP_COMMAND_BUFFER_SIZE, RDP_COMMAND_BUFFER_SIZE);
    }

    // read the commands into a buffer, 32 bits at a time.
    // we need to read the whole thing into a buffer before sending each command to the RDP
    // because commands have variable lengths
    if (dpc->status.xbus_dmem_dma) {
        for (int i = 0; i < display_list_length; i += 4) {
            word command_word = FROM_DMEM(current + i);
            rdp_command_buffer[last_run_unprocessed_words + (i >> 2)] = command_word;
        }
    } else {
        if (end > 0x7FFFFFF || current > 0x7FFFFFF) {
            logwarn("Not running RDP commands, wanted to read past end of RDRAM!");
            return;
        }
        for (int i = 0; i < display_list_length; i += 4) {
            word command_word = FROM_RDRAM(current + i);
            rdp_command_buffer[last_run_unprocessed_words + (i >> 2)] = command_word;
        }
    }

    int length_words = (display_list_length >> 2) + last_run_unprocessed_words;
    int buf_index = 0;

    bool processed_all = true;

    while (buf_index < length_words) {
        byte command = (rdp_command_buffer[buf_index] >> 24) & 0x3F;

        int command_length = command_lengths[command];

        // Check we actually have enough bytes left in the display list for this command, and save the remainder of the display list for the next run, if not.
        if ((buf_index + command_length) * 4 > display_list_length + (last_run_unprocessed_words * 4)) {
            // Copy remaining bytes back to the beginning of the display list, and save them for next run.
            last_run_unprocessed_words = length_words - buf_index;

            // Safe to allocate this on the stack because we'll only have a partial command left, and that _has_ to be pretty small.
            word temp[last_run_unprocessed_words];
            for (int i = 0; i < last_run_unprocessed_words; i++) {
                temp[i] = rdp_command_buffer[buf_index + i];
            }

            for (int i = 0; i < last_run_unprocessed_words; i++) {
                rdp_command_buffer[i] = temp[i];
            }

            processed_all = false;

            break;
        }


        // Don't need to process commands under 8
        if (command >= 8) {
            rdp_enqueue_command(command_length, &rdp_command_buffer[buf_index]);
        }

        if (command == RDP_COMMAND_FULL_SYNC) {
            rdp_on_full_sync();
            interrupt_raise(INTERRUPT_DP);
        }

        buf_index += command_length;
    }

    if (processed_all) {
        last_run_unprocessed_words = 0;
    }

    dpc->current = end;

    dpc->status.freeze = false;
}

void rdp_run_command() {
    //printf("Running commands from 0x%08X to 0x%08X\n", n64sys.dpc.current, n64sys.dpc.end);
    switch (n64sys.video_type) {
        case OPENGL_VIDEO_TYPE:
            graphics_plugin.ProcessRDPList();
            break;
        case VULKAN_VIDEO_TYPE:
        case SOFTWARE_VIDEO_TYPE:
            process_rdp_list();
            break;
        default:
            logfatal("Unknown video type");
    }
}

void rdp_update_screen() {
    switch (n64sys.video_type) {
        case OPENGL_VIDEO_TYPE:
            graphics_plugin.UpdateScreen();
            break;
        case VULKAN_VIDEO_TYPE:
            update_screen_parallel_rdp();
            break;
        case SOFTWARE_VIDEO_TYPE:
            n64_render_screen();
            break;
        default:
            logfatal("Unknown video type");
    }
}

void rdp_status_reg_write(word value) {
    union {
        word raw;
        struct {
            bool clear_xbus_dmem_dma:1;
            bool set_xbus_dmem_dma:1;
            bool clear_freeze:1;
            bool set_freeze:1;
            bool clear_flush:1;
            bool set_flush:1;
            bool clear_tmem_ctr:1;
            bool clear_pipe_ctr:1;
            bool clear_cmd_ctr:1;
            bool clear_clock_ctr:1;
            unsigned:22;
        };
    } status_write;

    status_write.raw = value;

    if (status_write.clear_xbus_dmem_dma) n64sys.dpc.status.xbus_dmem_dma = false;
    if (status_write.set_xbus_dmem_dma) n64sys.dpc.status.xbus_dmem_dma = true;

    if (status_write.clear_freeze) n64sys.dpc.status.freeze = false;
    if (status_write.set_freeze) n64sys.dpc.status.freeze = true;

    if (status_write.clear_flush) n64sys.dpc.status.flush = false;
    if (status_write.set_flush) n64sys.dpc.status.flush = true;

    if (status_write.clear_tmem_ctr) {
        //logwarn("Clear tmem ctr - deferring to RDP plugin");
    }
    if (status_write.clear_pipe_ctr) {
        //logwarn("Clear pipe ctr - deferring to RDP plugin");
    }
    if (status_write.clear_cmd_ctr) {
        //logwarn("Clear cmd ctr - deferring to RDP plugin");
    }
    if (status_write.clear_clock_ctr) {
        //logwarn("Clear clock ctr - deferring to RDP plugin");
    }
}
