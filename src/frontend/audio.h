#ifndef N64_AUDIO_H
#define N64_AUDIO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <system/n64system.h>
void adjust_audio_sample_rate(int sample_rate);
void audio_push_sample(s16 left, s16 right);
void audio_init();

// Volume, in the range [0.0, 1.0]
float n64_get_volume();
void n64_set_volume(float volume);

#ifdef __cplusplus
}
#endif

#endif //N64_AUDIO_H
