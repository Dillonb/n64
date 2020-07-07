#include "rdp.h"

#include <dlfcn.h>
#include <linux/limits.h>
#include <stdbool.h>

#include "mupen_interface.h"
#include "../common/log.h"

static void* plugin_handle = NULL;
static mupen_graphics_plugin_t graphics_plugin;
static word rdram_size_word = N64_RDRAM_SIZE; // GFX_INFO needs this to be sent as a uint32

#define LOAD_SYM(var, name) do { \
    var = dlsym(plugin_handle, name); \
        if (var == NULL) { logfatal("Failed to load RDP plugin! Missing symbol: %s", name) } \
    } while(false)

void rdp_check_interrupts() {
    logfatal("GFX plugin called CheckInterrupts!()")
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
        logfatal("Failed to load RDP plugin. Please pass a path to a shared library file!")
    }

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
        logfatal("Plugin loaded successfully, but was not a graphics plugin!")
    }

    GFX_INFO gfx_info;
    gfx_info.HEADER = system->mem.rom.rom;
    gfx_info.RDRAM = system->mem.rdram;
    gfx_info.DMEM = system->mem.sp_dmem;
    gfx_info.IMEM = system->mem.sp_imem;

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

    graphics_plugin.InitiateGFX(gfx_info);

    // TODO: check plugin version, API version, etc for compatibility

    printf("Loaded RDP plugin %s\n", plugin_name);
}
