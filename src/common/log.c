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
        const char* filename = n64_save_system_state();
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

void handle_logfatal(const char* message, const char* file, int line, ...) {
    size_t needed = snprintf(NULL, 0, "[FATAL] at %s:%d ", file, line);
    char* prefix_buf = malloc(needed + 1);
    snprintf(prefix_buf, needed + 1, "[FATAL] at %s:%d ", file, line);

    va_list vargs;
    va_start(vargs, line);

    needed = vsnprintf(NULL, 0, message, vargs);
    char* message_buf = malloc(needed + 1);
    vsnprintf(message_buf, needed + 1, message, vargs);

    needed = snprintf(NULL, 0, "%s%s", prefix_buf, message_buf);
    char* everything_buf = malloc(needed + 1);
    snprintf(everything_buf, needed + 1, "%s%s", prefix_buf, message_buf);

    free(prefix_buf);
    prefix_buf = NULL;
    free(message_buf);
    message_buf = NULL;

    fprintf(stderr, "%s%s%s\n", COLOR_RED, everything_buf, COLOR_END);

    n64_error_messagebox(everything_buf);

    free(everything_buf);
    everything_buf = NULL;
}