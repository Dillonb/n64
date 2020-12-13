#include <SDL_audio.h>
#include <pthread.h>
#include "audio.h"

#define AUDIO_SAMPLE_RATE 48000
static SDL_AudioStream* audio_stream = NULL;
static pthread_mutex_t audio_stream_mutex;
SDL_AudioSpec audio_spec;
SDL_AudioSpec request;
SDL_AudioDeviceID audio_dev;

INLINE void acquire_audiostream_mutex() {
    pthread_mutex_lock(&audio_stream_mutex);
}

INLINE void release_audiostream_mutex() {
    pthread_mutex_unlock(&audio_stream_mutex);
}

void audio_callback(void* userdata, Uint8* stream, int length) {
    int gotten = 0;
    acquire_audiostream_mutex();
    if (SDL_AudioStreamAvailable(audio_stream) > 0) {
        gotten = SDL_AudioStreamGet(audio_stream, stream, length);
    }
    release_audiostream_mutex();

    if (gotten < length) {
        int gotten_samples = gotten / sizeof(float);
        float* out = (float*)stream;
        out += gotten_samples;

        for (int i = gotten_samples; i < length / sizeof(float); i++) {
            float sample = 0;
            *out++ = sample;
        }
    }
}

void audio_init(n64_system_t* system) {
    adjust_audio_sample_rate(AUDIO_SAMPLE_RATE);
    memset(&request, 0, sizeof(request));

    request.freq = AUDIO_SAMPLE_RATE;
    request.format = AUDIO_F32SYS;
    request.channels = 2;
    request.samples = 1024;
    request.callback = audio_callback;
    request.userdata = NULL;

    audio_dev = SDL_OpenAudioDevice(NULL, 0, &request, &audio_spec, 0);

    audio_dev = SDL_OpenAudioDevice(NULL, 0, &request, &audio_spec, 0);
    unimplemented(request.format != audio_spec.format, "Request != got");

    if (audio_dev == 0) {
        logfatal("Failed to initialize SDL audio: %s", SDL_GetError());
    }

    SDL_PauseAudioDevice(audio_dev, false);

    if (pthread_mutex_init(&audio_stream_mutex, NULL) != 0) {
        logfatal("Unable to initialize mutex");
    }
}

void adjust_audio_sample_rate(int sample_rate) {
    logwarn("Adjusting audio sample rate, locking mutex!");
    acquire_audiostream_mutex();
    if (audio_stream != NULL) {
        SDL_FreeAudioStream(audio_stream);
    }

    audio_stream = SDL_NewAudioStream(AUDIO_S16SYS, 2, sample_rate, AUDIO_F32SYS, 2, AUDIO_SAMPLE_RATE);
    release_audiostream_mutex();
}

void audio_push_sample(shalf left, shalf right) {
    if (SDL_AudioStreamAvailable(audio_stream) < (AUDIO_SAMPLE_RATE * 2 * 1)) {
        shalf samples[2] = {
                left,
                right
        };

        SDL_AudioStreamPut(audio_stream, samples, 2 * sizeof(shalf));
    }
}
