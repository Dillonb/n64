#ifndef N64_MUPEN_INTERFACE_H
#define N64_MUPEN_INTERFACE_H

#include "contrib/m64p_types.h"
#include "contrib/m64p_plugin.h"
#include "contrib/m64p_common.h"

typedef struct mupen_graphics_plugin {
    ptr_PluginGetVersion PluginGetVersion;
    ptr_ChangeWindow ChangeWindow;
    ptr_InitiateGFX InitiateGFX;
    ptr_MoveScreen MoveScreen;
    ptr_ProcessDList ProcessDList;
    ptr_ProcessRDPList ProcessRDPList;
    ptr_RomClosed RomClosed;
    ptr_RomOpen RomOpen;
    ptr_ShowCFB ShowCFB;
    ptr_UpdateScreen UpdateScreen;
    ptr_ViStatusChanged ViStatusChanged;
    ptr_ViWidthChanged ViWidthChanged;
    ptr_ReadScreen2 ReadScreen2;
    ptr_SetRenderingCallback SetRenderingCallback;
    ptr_FBRead FBRead;
    ptr_FBWrite FBWrite;
    ptr_FBGetFrameBufferInfo FBGetFrameBufferInfo;
} mupen_graphics_plugin_t;

#endif //N64_MUPEN_INTERFACE_H
