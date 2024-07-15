#include "log.h"

#include <SDL.h>
#include <SDL_syswm.h>
#include <stdarg.h>

#include <system/crashdump.h>
#include <generated/version.h>

unsigned int n64_log_verbosity = 0;
unsigned int next_n64_log_verbosity = 0;

#ifdef N64_WIN
#include <windows.h>
SDL_Window* get_window_handle();
void n64_error_messagebox(const char* message) {
    SDL_Window* sdl_window = get_window_handle();
    HWND handle = NULL;
    if (sdl_window) {
        SDL_SysWMinfo wmInfo;
        SDL_VERSION(&wmInfo.version);
        SDL_GetWindowWMInfo(sdl_window, &wmInfo);
        handle = wmInfo.info.win.window;
    }

    size_t needed = snprintf(NULL, 0, "%s\n\nWould you like to save a crash dump?", message);
    char* full_message = malloc(needed + 1);
    snprintf(full_message, needed + 1, "%s\n\nWould you like to save a crash dump?", message);
    int chosen = MessageBox(handle, full_message, "Fatal emulation error", MB_YESNO | MB_ICONERROR);
    free(full_message);
    full_message = NULL;

    if (chosen == IDYES) {
        const char* filename = n64_save_system_state(message);
        const char* saved_fmt = "Saved to %s. Please zip this file and send it to me privately (I can be reached through Discord @dgb) or through GitHub Issues.";
        needed = snprintf(NULL, 0, saved_fmt, filename);
        char* saved_message = malloc(needed + 1);
        snprintf(saved_message, needed + 1, saved_fmt, filename);
        MessageBox(handle, saved_message, "Crash dump saved", MB_OK | MB_ICONINFORMATION);
        free(saved_message);
        saved_message = NULL;
    }
}
#else
#define n64_error_messagebox(msg)
#endif

char logfatal_buf[LOGFATAL_BUF_SIZE];
void handle_logfatal(const char* buf) {
    n64_error_messagebox(buf);
}
