#ifndef N64_RENDER_H
#define N64_RENDER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <system/n64system.h>

#define N64_SCREEN_X 640
#define N64_SCREEN_Y 480
extern int SCREEN_SCALE;

void render_init(n64_video_type_t video_type);
void n64_render_screen();
void n64_poll_input();

#ifdef __cplusplus
}
#endif

#endif //N64_RENDER_H
