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
#include <log.h>
#include <frontend/render.h>

static void* plugin_handle = NULL;
static mupen_graphics_plugin_t graphics_plugin;
static word rdram_size_word = N64_RDRAM_SIZE; // GFX_INFO needs this to be sent as a uint32

#define RDP_COMMAND_BUFFER_SIZE 0xFFFFF
word rdp_command_buffer[RDP_COMMAND_BUFFER_SIZE];

#define FROM_RDRAM(system, address) word_from_byte_array(system->mem.rdram, WORD_ADDRESS(address))
#define FROM_DMEM(system, address) word_from_byte_array(system->rsp.sp_dmem, address)

static const int command_lengths[64] = {
        2, 2, 2, 2, 2, 2, 2, 2, 8, 12, 24, 28, 24, 28, 40, 44,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2,  2,  2,  2,  2,  2,  2,
        2, 2, 2, 2, 4, 4, 2, 2, 2, 2,  2,  2,  2,  2,  2,  2,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2,  2,  2,  2,  2,  2,  2
};

#define RDP_COMMAND_FULL_SYNC 0x29


void rdp_rendering_callback(int redrawn) {
    n64_poll_input(mupen_interface_global_system);
    n64_render_screen(mupen_interface_global_system);
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
    on_interrupt_change(mupen_interface_global_system);
}

GFX_INFO get_gfx_info(n64_system_t* system) {
    GFX_INFO gfx_info;
    gfx_info.HEADER = system->mem.rom.rom;
    gfx_info.RDRAM = system->mem.rdram;
    gfx_info.DMEM = system->rsp.sp_dmem;
    gfx_info.IMEM = system->rsp.sp_imem;

    gfx_info.MI_INTR_REG = &system->mi.intr.raw;

    gfx_info.DPC_START_REG    = &system->dpc.start;
    gfx_info.DPC_END_REG      = &system->dpc.end;
    gfx_info.DPC_CURRENT_REG  = &system->dpc.current;
    gfx_info.DPC_STATUS_REG   = &system->dpc.status.raw;
    gfx_info.DPC_CLOCK_REG    = &system->dpc.clock;
    gfx_info.DPC_BUFBUSY_REG  = &system->dpc.bufbusy;
    gfx_info.DPC_PIPEBUSY_REG = &system->dpc.pipebusy;
    gfx_info.DPC_TMEM_REG     = &system->dpc.tmem;

    gfx_info.VI_STATUS_REG         = &system->vi.status.raw;
    gfx_info.VI_ORIGIN_REG         = &system->vi.vi_origin;
    gfx_info.VI_WIDTH_REG          = &system->vi.vi_width;
    gfx_info.VI_INTR_REG           = &system->vi.vi_v_intr;
    gfx_info.VI_V_CURRENT_LINE_REG = &system->vi.v_current;
    gfx_info.VI_TIMING_REG         = &system->vi.vi_burst.raw;
    gfx_info.VI_V_SYNC_REG         = &system->vi.vsync;
    gfx_info.VI_H_SYNC_REG         = &system->vi.hsync;
    gfx_info.VI_LEAP_REG           = &system->vi.leap;
    gfx_info.VI_H_START_REG        = &system->vi.hstart;
    gfx_info.VI_V_START_REG        = &system->vi.vstart.raw;
    gfx_info.VI_V_BURST_REG        = &system->vi.vburst;
    gfx_info.VI_X_SCALE_REG        = &system->vi.xscale;
    gfx_info.VI_Y_SCALE_REG        = &system->vi.yscale;

    gfx_info.CheckInterrupts = &rdp_check_interrupts;

    gfx_info.version = 2;

    gfx_info.SP_STATUS_REG = &system->rsp.status.raw;
    gfx_info.RDRAM_SIZE = &rdram_size_word;

    return gfx_info;
}

void load_rdp_plugin(n64_system_t* system, const char* filename) {
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

    init_mupen_interface(system);

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

    GFX_INFO gfx_info = get_gfx_info(system);

    graphics_plugin.InitiateGFX(gfx_info);
    graphics_plugin.RomOpen();

    graphics_plugin.SetRenderingCallback(rdp_rendering_callback);

    // TODO: check plugin version, API version, etc for compatibility

    printf("Loaded RDP plugin %s\n", plugin_name);
}

void write_word_dpcreg(n64_system_t* system, word address, word value) {
    switch (address) {
        case ADDR_DPC_START_REG:
            system->dpc.start = value & 0xFFFFFF;
            system->dpc.current = system->dpc.start;
            break;
        case ADDR_DPC_END_REG:
            system->dpc.end = value & 0xFFFFFF;
            rdp_run_command(system);
            break;
        case ADDR_DPC_CURRENT_REG:
            logfatal("Writing word to unimplemented DPC register: ADDR_DPC_CURRENT_REG");
        case ADDR_DPC_STATUS_REG:
            rdp_status_reg_write(system, value);
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

word read_word_dpcreg(n64_system_t* system, word address) {
    switch (address) {
        case ADDR_DPC_START_REG:
            return system->dpc.start;
        case ADDR_DPC_END_REG:
            return system->dpc.end;
        case ADDR_DPC_CURRENT_REG:
            return system->dpc.current;
        case ADDR_DPC_STATUS_REG:
            return system->dpc.status.raw;
        case ADDR_DPC_CLOCK_REG:
            return system->dpc.clock;
        case ADDR_DPC_BUFBUSY_REG:
            return system->dpc.bufbusy;
        case ADDR_DPC_PIPEBUSY_REG:
            return system->dpc.pipebusy;
        case ADDR_DPC_TMEM_REG:
            return system->dpc.tmem;
        default:
            logfatal("Reading word from address 0x%08X in unsupported region: REGION_DP_COMMAND_REGS", address);
    }
}

void rdp_cleanup() {
    if (graphics_plugin.RomClosed) {
        graphics_plugin.RomClosed();
    }
}

void process_rdp_list(n64_system_t* system) {
    static int last_run_unprocessed_words = 0;

    n64_dpc_t* dpc = &system->dpc;

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
            word command_word = FROM_DMEM(system, (current + i));
            rdp_command_buffer[last_run_unprocessed_words + (i >> 2)] = command_word;
        }
    } else {
        if (end > 0x7FFFFFF || current > 0x7FFFFFF) {
            logwarn("Not running RDP commands, wanted to read past end of RDRAM!");
            return;
        }
        for (int i = 0; i < display_list_length; i += 4) {
            word command_word = FROM_RDRAM(system, current + i);
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
            parallel_rdp_enqueue_command(command_length, &rdp_command_buffer[buf_index]);
        }

        if (command == RDP_COMMAND_FULL_SYNC) {
            parallel_rdp_on_full_sync();
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

void rdp_run_command(n64_system_t* system) {
    //printf("Running commands from 0x%08X to 0x%08X\n", system->dpc.current, system->dpc.end);
    switch (system->video_type) {
        case OPENGL_VIDEO_TYPE:
            graphics_plugin.ProcessRDPList();
            break;
        case VULKAN_VIDEO_TYPE:
            process_rdp_list(system);
            break;
        default:
            logfatal("Unknown video type");
    }
}

void rdp_update_screen(n64_system_t* system) {
    switch (system->video_type) {
        case OPENGL_VIDEO_TYPE:
            graphics_plugin.UpdateScreen();
            break;
        case VULKAN_VIDEO_TYPE:
            update_screen_parallel_rdp(system);
            break;
        default:
            logfatal("Unknown video type");
    }
}

void rdp_status_reg_write(n64_system_t* system, word value) {
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

    if (status_write.clear_xbus_dmem_dma) system->dpc.status.xbus_dmem_dma = false;
    if (status_write.set_xbus_dmem_dma) system->dpc.status.xbus_dmem_dma = true;

    if (status_write.clear_freeze) system->dpc.status.freeze = false;
    if (status_write.set_freeze) system->dpc.status.freeze = true;

    if (status_write.clear_flush) system->dpc.status.flush = false;
    if (status_write.set_flush) system->dpc.status.flush = true;

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
