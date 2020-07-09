#ifndef N64_MUPEN_INTERFACE_H
#define N64_MUPEN_INTERFACE_H

#include "contrib/m64p_types.h"
#include "contrib/m64p_plugin.h"
#include "contrib/m64p_common.h"
#include "../system/n64system.h"

void init_mupen_interface(n64_system_t* system);

typedef struct mupen_graphics_plugin {
    const char* plugin_name;

    // Called on startup. Gives the plugin some handles back into the core.
    ptr_PluginStartup PluginStartup;
    // Called on shutdown to clean up plugin stuff.
    ptr_PluginShutdown PluginShutdown;
    // Called when plugin loaded to retrieve version info
    ptr_PluginGetVersion PluginGetVersion;


    // This emulator does not support fullscreen (yet?) so this is never called.
    ptr_ChangeWindow ChangeWindow;
    // Called as the plugin is loaded. This sets up the graphics context, and lets the plugin access parts of the emulator's memory.
    ptr_InitiateGFX InitiateGFX;
    // TODO? Angrylion does not use this, so this is never called.
    ptr_MoveScreen MoveScreen;
    // Used by HLE RSP plugins, and HLE RDP plugins. Never called.
    ptr_ProcessDList ProcessDList;
    // Called by the RSP when a command needs to be run.
    ptr_ProcessRDPList ProcessRDPList;
    // For cleanup
    ptr_RomClosed RomClosed;
    // Initialization
    ptr_RomOpen RomOpen;
    // TODO? Angrylion does not use this, so this is never called. Might be used for HLE RSP emulation?
    ptr_ShowCFB ShowCFB;
    // Called on every VI interrupt, whether or not it's been masked to actually happen
    ptr_UpdateScreen UpdateScreen;
    // Called when VI_STATUS_REG is changed
    ptr_ViStatusChanged ViStatusChanged;
    // Called when VI_WIDTH_REG is changed
    ptr_ViWidthChanged ViWidthChanged;
    // Seems to be used for screenshots?
    ptr_ReadScreen2 ReadScreen2;
    // Used to set a function to be called when a new frame is rendered.
    ptr_SetRenderingCallback SetRenderingCallback;
    ptr_FBRead FBRead;
    ptr_FBWrite FBWrite;
    ptr_FBGetFrameBufferInfo FBGetFrameBufferInfo;
} mupen_graphics_plugin_t;

#endif //N64_MUPEN_INTERFACE_H
