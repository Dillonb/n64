#ifndef N64_AUDIO_H
#define N64_AUDIO_H
#include <system/n64system.h>
void adjust_audio_sample_rate(int sample_rate);
void audio_push_sample(shalf left, shalf right);
void audio_init(n64_system_t* system);
#endif //N64_AUDIO_H
