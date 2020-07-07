#include "render.h"

#include <SDL.h>
#include <glad/glad.h>

#include "../mem/pif.h"

int SCREEN_SCALE = 2;
static SDL_GLContext gl_context;
static SDL_Window* window = NULL;
static uint32_t window_id;
static SDL_Renderer* renderer = NULL;
static SDL_Texture* argb32buffer = NULL;
static SDL_Texture* rgb16buffer = NULL;
static half* rgb16_host_endianness = NULL;
static int rgb16buffer_width;
static int rgb16buffer_height;

#define AUDIO_SAMPLE_RATE 48000
static SDL_AudioStream* audio_stream = NULL;
SDL_AudioSpec audio_spec;
SDL_AudioSpec request;
SDL_AudioDeviceID audio_dev;

word fps_interval = 1000; // 1000ms = 1 second
word sdl_lastframe = 0;
word sdl_numframes = 0;
word sdl_fps = 0;
char sdl_wintitle[100] = "dgb n64 00 FPS";

void update_rgb16_buffer(int width, int height) {
    if (rgb16buffer != NULL) {
        SDL_DestroyTexture(rgb16buffer);
    }
    if (rgb16_host_endianness != NULL) {
        free(rgb16_host_endianness);
    }

    rgb16buffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA5551, SDL_TEXTUREACCESS_STREAMING, width, height);
    rgb16_host_endianness = malloc(width * height * sizeof(half));
    rgb16buffer_width = width;
    rgb16buffer_height = height;
}

void audio_callback(void* userdata, Uint8* stream, int length) {
    int gotten = 0;
    if (SDL_AudioStreamAvailable(audio_stream) > 0) {
        gotten = SDL_AudioStreamGet(audio_stream, stream, length);
    }

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
    unimplemented(request.format != audio_spec.format, "Request != got")

    if (audio_dev == 0) {
        logfatal("Failed to initialize SDL audio: %s", SDL_GetError())
    }

    SDL_PauseAudioDevice(audio_dev, false);
}

void video_init() {
    window = SDL_CreateWindow("dgb n64",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              N64_SCREEN_X * SCREEN_SCALE,
                              N64_SCREEN_Y * SCREEN_SCALE,
                              SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
    gl_context = SDL_GL_CreateContext(window);
    if (gl_context == NULL) {
        logfatal("SDL couldn't create OpenGL context! %s", SDL_GetError())
    }

    int gl_version = gladLoadGLLoader(SDL_GL_GetProcAddress);



    if (gl_version == 0) {
        logfatal("Failed to initialize Glad context")
    }

    printf("OpenGL initialized.\n");
    printf("Vendor:   %s\n", glGetString(GL_VENDOR));
    printf("Renderer: %s\n", glGetString(GL_RENDERER));
    printf("Version:  %s\n", glGetString(GL_VERSION));

    window_id = SDL_GetWindowID(window);

    glViewport(0, 0, N64_SCREEN_X * SCREEN_SCALE, N64_SCREEN_Y * SCREEN_SCALE);
    glClearColor(0.0f, 0.5f, 1.0f, 0.0f);

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    argb32buffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, N64_SCREEN_X, N64_SCREEN_Y);
    update_rgb16_buffer(N64_SCREEN_X, N64_SCREEN_Y);

    if (renderer == NULL) {
        logfatal("SDL couldn't create a renderer! %s", SDL_GetError())
    }

    SDL_RenderSetScale(renderer, SCREEN_SCALE, SCREEN_SCALE);
}

void render_init(n64_system_t* system) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        logfatal("SDL couldn't initialize! %s", SDL_GetError());
    }
    video_init();
    audio_init(system);
}

void handle_event(n64_system_t* system, SDL_Event* event) {
    switch (event->type) {
        case SDL_QUIT:
            logwarn("User requested quit")
            n64_request_quit();
            break;
        case SDL_KEYDOWN: {
            switch (event->key.keysym.sym) {
                case SDLK_ESCAPE:
                    n64_request_quit();
                    break;

                case SDLK_j:
                    update_button(system, 0, A, true);
                    break;

                case SDLK_k:
                    update_button(system, 0, B, true);
                    break;

                case SDLK_UP:
                case SDLK_w:
                    update_button(system, 0, DPAD_UP, true);
                    break;

                case SDLK_DOWN:
                case SDLK_s:
                    update_button(system, 0, DPAD_DOWN, true);
                    break;

                case SDLK_LEFT:
                case SDLK_a:
                    update_button(system, 0, DPAD_LEFT, true);
                    break;

                case SDLK_RIGHT:
                case SDLK_d:
                    update_button(system, 0, DPAD_RIGHT, true);
                    break;

                case SDLK_RETURN:
                    update_button(system, 0, START, true);
                    break;
            }
            break;
        }
        case SDL_KEYUP: {
            switch (event->key.keysym.sym) {
                case SDLK_j:
                    update_button(system, 0, A, false);
                    break;

                case SDLK_k:
                    update_button(system, 0, B, false);
                    break;

                case SDLK_UP:
                case SDLK_w:
                    update_button(system, 0, DPAD_UP, false);
                    break;

                case SDLK_DOWN:
                case SDLK_s:
                    update_button(system, 0, DPAD_DOWN, false);
                    break;

                case SDLK_LEFT:
                case SDLK_a:
                    update_button(system, 0, DPAD_LEFT, false);
                    break;

                case SDLK_RIGHT:
                case SDLK_d:
                    update_button(system, 0, DPAD_RIGHT, false);
                    break;

                case SDLK_RETURN:
                    update_button(system, 0, START, false);
                    break;
            }
            break;
        }

    }
}

void render_screen(n64_system_t* system) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        handle_event(system, &event);
    }

    unimplemented(system->vi.status.type == VI_TYPE_RESERVED, "VI_TYPE_RESERVED unimplemented!")

    if (system->vi.status.type == VI_TYPE_32BIT) {
        SDL_UpdateTexture(argb32buffer, NULL, &system->mem.rdram[system->vi.vi_origin], system->vi.vi_width * 4);
        SDL_RenderCopy(renderer, argb32buffer, NULL, NULL);
    } else if (system->vi.status.type == VI_TYPE_16BIT) {
        if (system->vi.vi_width != rgb16buffer_width || system->vi.calculated_height != rgb16buffer_height) {
            update_rgb16_buffer(system->vi.vi_width, system->vi.calculated_height);
        }
        half* harr = (half*)system->mem.rdram;
        for (int i = 0; i < system->vi.vi_width * system->vi.calculated_height; i++) {
            half converted = be16toh(harr[system->vi.vi_origin / 2 + i]);
            rgb16_host_endianness[i] = converted;
        }
        SDL_UpdateTexture(rgb16buffer, NULL, rgb16_host_endianness, system->vi.vi_width * 2);
        SDL_RenderCopy(renderer, rgb16buffer, NULL, NULL);
    }

    SDL_RenderPresent(renderer);

    sdl_numframes++;
    uint32_t ticks = SDL_GetTicks();
    if (sdl_lastframe < ticks - fps_interval) {
        sdl_lastframe = ticks;
        sdl_fps = sdl_numframes;
        sdl_numframes = 0;
        snprintf(sdl_wintitle, sizeof(sdl_wintitle), "dgb n64 [%s] %02d FPS", system->mem.rom.header.image_name, sdl_fps);
        SDL_SetWindowTitle(window, sdl_wintitle);
    }
}

void adjust_audio_sample_rate(int sample_rate) {
    if (audio_stream != NULL) {
        SDL_FreeAudioStream(audio_stream);
    }

    audio_stream = SDL_NewAudioStream(AUDIO_S16SYS, 2, sample_rate, AUDIO_F32SYS, 2, AUDIO_SAMPLE_RATE);
}

void audio_push_sample(shalf left, shalf right) {
    shalf samples[2] = {
            left,
            right
    };

    SDL_AudioStreamPut(audio_stream, samples, 2 * sizeof(shalf));
}
