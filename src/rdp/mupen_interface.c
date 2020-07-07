#include "mupen_interface.h"
#include "../common/log.h"
#include "../frontend/render.h"

#include <stdbool.h>
#include <stdio.h>
#include <SDL_video.h>

#define PARAM(name, value) if (strcmp(param, name) == 0) { return value; }

void init_mupen_interface() {
    printf("Initialized Mupen64Plus plugin interface\n");
}

EXPORT m64p_error CALL ConfigOpenSection(const char* section, m64p_handle* handle) {
    printf("ConfigOpen %s\n", section);
    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL ConfigSaveSection(const char* section) {
    printf("ConfigSaveSection\n");
    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL ConfigSetDefaultInt(m64p_handle handle, const char* param, int value, const char* help) {
    printf("ConfigSetDefaultInt: %s = %d. Help: %s\n", param, value, help);
    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL ConfigSetDefaultFloat(m64p_handle handle, const char* param, float value, const char* help) {
    printf("ConfigSetDefaultFloat: %s = %f. Help: %s\n", param, value, help);
    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL ConfigSetDefaultBool(m64p_handle handle, const char* param, int value, const char* help) {
    printf("ConfigSetDefaultBool: %s = %d. Help: %s\n", param, value, help);
    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL ConfigSetDefaultString(m64p_handle handle, const char* param, const char* value, const char* help) {
    printf("ConfigSetDefaultString: %s = %s. Help: %s\n", param, value, help);
    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL VidExt_Init() {
    printf("VidExt_Init\n");
    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL VidExt_GL_SetAttribute(m64p_GLattr attr, int arg1) {
    printf("VidExt_GL_SetAttribute\n");
    return M64ERR_SUCCESS;
}

EXPORT m64p_error CALL VidExt_SetVideoMode(int arg1, int arg2, int arg3, m64p_video_mode mode, m64p_video_flags flags) {
    printf("VidExt_SetVideoMode\n");
    return M64ERR_SUCCESS;
}

EXPORT void* CALL VidExt_GL_GetProcAddress(const char* proc) {
    printf("VidExt_GL_GetProcAddress: %s\n", proc);
    return SDL_GL_GetProcAddress(proc);
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
    printf("VidExt_Quit\n");
    return M64ERR_SUCCESS;
}
