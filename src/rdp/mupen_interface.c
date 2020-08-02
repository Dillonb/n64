#include "mupen_interface.h"
#include "../common/log.h"
#include "../frontend/render.h"

#include <stdbool.h>
#include <stdio.h>
#include <SDL_video.h>

n64_system_t* mupen_interface_global_system;

#define PARAM(name, value) if (strcmp(param, name) == 0) { return value; }

void init_mupen_interface(n64_system_t* system) {
    mupen_interface_global_system = system;
    printf("Initialized Mupen64Plus plugin interface\n");
}

EXPORT m64p_error CALL ConfigOpenSection(const char* section, m64p_handle* handle) {
    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL ConfigSaveSection(const char* section) {
    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL ConfigSetDefaultInt(m64p_handle handle, const char* param, int value, const char* help) {
    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL ConfigSetDefaultFloat(m64p_handle handle, const char* param, float value, const char* help) {
    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL ConfigSetDefaultBool(m64p_handle handle, const char* param, int value, const char* help) {
    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL ConfigSetDefaultString(m64p_handle handle, const char* param, const char* value, const char* help) {
    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL VidExt_Init() {
    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL VidExt_GL_SetAttribute(m64p_GLattr attr, int arg1) {
    return M64ERR_SUCCESS;
}
EXPORT m64p_error CALL VidExt_SetVideoMode(int width, int height, int bits_per_pixel, m64p_video_mode mode, m64p_video_flags flags) {
    return M64ERR_SUCCESS;
}

EXPORT void* CALL VidExt_GL_GetProcAddress(const char* proc) {
    void* pointer = SDL_GL_GetProcAddress(proc);
    if (pointer == NULL) {
        logfatal("VidExt_GL_GetProcAddress: could not get address of %s!", proc)
    }

    return pointer;
}

EXPORT int CALL ConfigGetParamInt(m64p_handle handle, const char* param) {
    PARAM("ScreenWidth",     N64_SCREEN_X * SCREEN_SCALE)
    PARAM("ScreenHeight",    N64_SCREEN_Y * SCREEN_SCALE)
    PARAM("NumWorkers",      0)
    PARAM("ViMode",          0)
    PARAM("ViInterpolation", 0)
    PARAM("DpCompat",        0)


    logfatal("Unknown int param: %s", param)
}

EXPORT float CALL ConfigGetParamFloat(m64p_handle handle, const char* param) {
    logfatal("Unknown float param: %s", param)
}

EXPORT int CALL ConfigGetParamBool(m64p_handle handle, const char* param) {
    PARAM("Fullscreen",     false)
    PARAM("Parallel",       true)
    PARAM("ViWidescreen",   false)
    PARAM("ViHideOverscan", false)


    logfatal("Unknown bool param: %s", param)
}

EXPORT const char * CALL ConfigGetParamString(m64p_handle handle, const char* param) {
    logfatal("Unknown string param: %s", param)
}

EXPORT m64p_error CALL VidExt_Quit(void) {
    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL VidExt_GL_SwapBuffers(void) {
    render_screen(mupen_interface_global_system);
    return M64ERR_SUCCESS;
}
