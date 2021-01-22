#ifndef N64_IMGUI_UI_H
#define N64_IMGUI_UI_H


#ifdef __cplusplus
#include <imgui.h>
ImDrawData* imgui_frame();
extern "C" {
#endif

void load_imgui_ui();

#ifdef __cplusplus
}
#endif
#endif //N64_IMGUI_UI_H
