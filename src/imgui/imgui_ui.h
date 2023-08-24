#ifndef N64_IMGUI_UI_H
#define N64_IMGUI_UI_H


#ifdef __cplusplus
#include <imgui.h>
ImDrawData* imgui_frame();
extern "C" {
#else
#include <stdbool.h>
#endif

#include <SDL_events.h>

void load_imgui_ui();
bool imgui_wants_mouse();
bool imgui_wants_keyboard();
bool imgui_handle_event(SDL_Event* event);

#ifdef __cplusplus
}
#endif
#endif //N64_IMGUI_UI_H
