#ifndef N64_RENDER_H
#define N64_RENDER_H

#include "../system/n64system.h"
void render_init(n64_system_t* system);
void render_screen(n64_system_t* system);
void adjust_audio_sample_rate(int sample_rate);
void audio_push_sample(shalf left, shalf right);

#endif //N64_RENDER_H
