#ifndef N64_FRONTEND_H
#define N64_FRONTEND_H

#include <stdbool.h>
#include <util.h>
#ifdef __cplusplus
extern "C" {
#endif

#include <SDL_events.h>
#include <frontend/device.h>

typedef bool(*event_handler_t)(SDL_Event*);

void n64_poll_input();
void register_imgui_event_handler(event_handler_t handler);
void register_sdl_keyboard_bindings(int player, SDL_KeyCode bindings[2], n64_button_t button);

#ifdef __cplusplus
}
#endif

#endif //N64_FRONTEND_H
