#ifndef N64_AUDIO_H
#define N64_AUDIO_H
#include <system/n64system.h>
void adjust_audio_sample_rate(int sample_rate);
void audio_push_sample(s16 left, s16 right);
void audio_init();
#endif //N64_AUDIO_H
