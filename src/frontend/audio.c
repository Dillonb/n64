#include "audio.h"
#include <SDL_audio.h>
#include <metrics.h>
#include <samplerate.h>
#include <fifo.h>

static_assert(sizeof(float) == 4, "float must be 32 bits");
#define S16_TO_F32(x) ((float)(x) / (float)32768)

#define HOST_SAMPLE_RATE 48000
#define HOST_SAMPLE_FORMAT AUDIO_F32SYS
#define HOST_SAMPLE_SIZE sizeof(float)
//#define HOST_BUFFER_SIZE ((HOST_SAMPLE_RATE) / 2)
#define HOST_BUFFER_SIZE 32768

#define GUEST_SAMPLE_SIZE sizeof(float)

// Variable, controlled by the game
unsigned guest_sample_rate = HOST_SAMPLE_RATE;

// output_sample_rate / input_sample_rate
double resample_ratio = 1;


#define FRAMES_PER_REQUEST 1024
#define AUDIO_CHANNELS 2
#define GUEST_BUFFER_SIZE ((FRAMES_PER_REQUEST) * (AUDIO_CHANNELS))

SDL_AudioSpec audio_spec;
SDL_AudioSpec request;
SDL_AudioDeviceID audio_dev;

float guest_sample_buffer[GUEST_BUFFER_SIZE];

// Much larger than needed
#define TEMP_RESAMPLED_BUFFER_SIZE HOST_SAMPLE_RATE
#define TEMP_RESAMPLED_BUFFER_FRAMES (HOST_SAMPLE_RATE / AUDIO_CHANNELS)
float temp_resampled_buffer[TEMP_RESAMPLED_BUFFER_SIZE];
unsigned idx_guest_sample_buffer = 0;
struct fifo* host_sample_buffer;

SRC_STATE* resampler;

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
void audio_callback(void* userdata, Uint8* stream, int length) {
    int avail = fifo_read_available(host_sample_buffer);
    set_metric(METRIC_AUDIOSTREAM_AVAILABLE, avail);

    int to_read = MIN(length, avail);
    fifo_read(host_sample_buffer, stream, to_read);

    if (avail < length) {
        memset(stream + avail, 0, length - avail);
    }
}

void audio_init() {
    memset(temp_resampled_buffer, 0, TEMP_RESAMPLED_BUFFER_SIZE * HOST_SAMPLE_SIZE);
    host_sample_buffer = fifo_create(HOST_BUFFER_SIZE);
    adjust_audio_sample_rate(HOST_SAMPLE_RATE);
    memset(&request, 0, sizeof(request));

    request.freq = HOST_SAMPLE_RATE;
    request.format = HOST_SAMPLE_FORMAT;
    request.channels = AUDIO_CHANNELS;
    request.samples = FRAMES_PER_REQUEST;
    request.callback = audio_callback;
    request.userdata = NULL;

    audio_dev = SDL_OpenAudioDevice(NULL, 0, &request, &audio_spec, 0);
    unimplemented(request.format != audio_spec.format, "Request format != got");
    unimplemented(request.freq != audio_spec.freq, "Request freq %d != got freq %d", request.freq, audio_spec.freq);

    if (audio_dev == 0) {
        logfatal("Failed to initialize SDL audio: %s", SDL_GetError());
    }

    SDL_PauseAudioDevice(audio_dev, false);

    int src_error = 0;
    resampler = src_new(SRC_SINC_BEST_QUALITY, AUDIO_CHANNELS, &src_error);
    if (resampler == NULL) {
        logfatal("Failed to initialize libsamplerate! Error: %d", src_error);
    }
}

void flush_guest_buffer() {
    if (idx_guest_sample_buffer == 0) {
        return;
    }

    SRC_DATA resampler_data = { 0 };
    resampler_data.data_in = guest_sample_buffer;
    resampler_data.input_frames = idx_guest_sample_buffer / AUDIO_CHANNELS;
    resampler_data.data_out = temp_resampled_buffer;
    resampler_data.output_frames = TEMP_RESAMPLED_BUFFER_FRAMES;
    resampler_data.src_ratio = resample_ratio;
    resampler_data.end_of_input = false;

    int error = src_process(resampler, &resampler_data);
    if (error != 0) {
        logalways("Error resampling! %s", src_strerror(error));
    } else {
        loginfo("Input frames: %ld Input frames used: %ld Output frames: %ld", resampler_data.input_frames, resampler_data.input_frames_used, resampler_data.output_frames_gen);

        long buf_size = resampler_data.output_frames_gen * AUDIO_CHANNELS * HOST_SAMPLE_SIZE;
        // Bytes remaining to be written to the FIFO
        long bytes_remaining = buf_size;
        long buf_idx = 0;
        while (bytes_remaining > 0) {
            long chunk_size = MIN(1024l * AUDIO_CHANNELS * HOST_SAMPLE_SIZE, bytes_remaining);
            int fifo_write_avail = fifo_write_remaining(host_sample_buffer);
            if (fifo_write_avail < chunk_size) {
                continue;
            }
            fifo_write(host_sample_buffer, ((u8*)temp_resampled_buffer) + buf_idx, chunk_size);
            buf_idx += chunk_size;
            bytes_remaining -= chunk_size;
        }
    }

    idx_guest_sample_buffer = 0;
}

void adjust_audio_sample_rate(int sample_rate) {
    // Flush first so all samples already pushed are resampled at the old rate
    flush_guest_buffer();

    guest_sample_rate = sample_rate;
    resample_ratio = ((double)HOST_SAMPLE_RATE) / ((double)guest_sample_rate);
    logalways("Adjusting guest sample rate. Host rate: %d Guest rate: %d Ratio: %f", HOST_SAMPLE_RATE, guest_sample_rate, resample_ratio);
}

void audio_push_sample(s16 left, s16 right) {
    if (idx_guest_sample_buffer + 2 > GUEST_BUFFER_SIZE) {
        // resample and push to host buffer
        flush_guest_buffer();
    }

    guest_sample_buffer[idx_guest_sample_buffer++] = S16_TO_F32(left);
    guest_sample_buffer[idx_guest_sample_buffer++] = S16_TO_F32(right);
}
