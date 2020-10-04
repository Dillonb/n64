#include "render.h"

#include <SDL.h>
#include <SDL_vulkan.h>

#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#define GLAD_VULKAN_IMPLEMENTATION
#include <glad/vulkan.h>

#include <mem/pif.h>

int SCREEN_SCALE = 2;
static SDL_GLContext gl_context;
static SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;
static n64_video_type_t n64_video_type = UNKNOWN;

// Vulkan stuff
VkInstance vk_instance;
VkSurfaceKHR vk_surface;
VkPhysicalDevice vk_physical_device;
const char* required_device_extensions[64];
uint32_t num_required_extensions = sizeof(required_device_extensions) / sizeof(required_device_extensions[0]);

const char* required_device_layers[64];
uint32_t num_required_device_layers = sizeof(required_device_layers) / sizeof(required_device_layers[0]);

const VkPhysicalDeviceFeatures* required_features;

#define AUDIO_SAMPLE_RATE 48000
static SDL_AudioStream* audio_stream = NULL;
SDL_AudioSpec audio_spec;
SDL_AudioSpec request;
SDL_AudioDeviceID audio_dev;

word fps_interval = 1000; // 1000ms = 1 second
word sdl_lastframe = 0;
word sdl_numframes = 0;
word sdl_fps = 0;
char sdl_wintitle[100] = N64_APP_NAME " 00 FPS";

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
    unimplemented(request.format != audio_spec.format, "Request != got");

    if (audio_dev == 0) {
        logfatal("Failed to initialize SDL audio: %s", SDL_GetError());
    }

    SDL_PauseAudioDevice(audio_dev, false);
}

void video_init_opengl() {
    window = SDL_CreateWindow(N64_APP_NAME,
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              N64_SCREEN_X * SCREEN_SCALE,
                              N64_SCREEN_Y * SCREEN_SCALE,
                              SDL_WINDOW_SHOWN | SDL_WINDOW_OPENGL);
    gl_context = SDL_GL_CreateContext(window);
    if (gl_context == NULL) {
        logfatal("SDL couldn't create OpenGL context! %s", SDL_GetError());
    }

    int gl_version = gladLoadGL((GLADloadfunc)SDL_GL_GetProcAddress);

    if (gl_version == 0) {
        logfatal("Failed to initialize Glad context");
    }

    printf("OpenGL initialized.\n");
    printf("Vendor:   %s\n", glGetString(GL_VENDOR));
    printf("Renderer: %s\n", glGetString(GL_RENDERER));
    printf("Version:  %s\n", glGetString(GL_VERSION));

    glViewport(0, 0, N64_SCREEN_X * SCREEN_SCALE, N64_SCREEN_Y * SCREEN_SCALE);
    glClearColor(0.0f, 0.5f, 1.0f, 0.0f);

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (renderer == NULL) {
        logfatal("SDL couldn't create a renderer! %s", SDL_GetError());
    }
}

void video_init_vulkan() {
    window = SDL_CreateWindow(N64_APP_NAME,
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              N64_SCREEN_X * SCREEN_SCALE,
                              N64_SCREEN_Y * SCREEN_SCALE,
                              SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN);

    int vk_version = gladLoaderLoadVulkan(NULL, NULL, NULL);

    if (!vk_version) {
        logfatal("Failed to load Vulkan! Does your GPU and driver support Vulkan 1.1?");
    }

    if (!SDL_Vulkan_GetInstanceExtensions(window, &num_required_extensions, required_device_extensions)) {
        logfatal("SDL_Vulkan_GetInstanceExtensions failed: %s", SDL_GetError());
    }

    for (int i = 0; i < num_required_extensions; i++) {
        printf("Extension: %s\n", required_device_extensions[i]);
    }

    printf("Loaded Vulkan %d.%d\n", GLAD_VERSION_MAJOR(vk_version), GLAD_VERSION_MINOR(vk_version));

    VkApplicationInfo app_info = {};
    app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    app_info.pEngineName = N64_APP_NAME;
    app_info.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.pApplicationName = N64_APP_NAME;
    app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    app_info.apiVersion = VK_API_VERSION_1_1;

    VkInstanceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &app_info;
    createInfo.enabledExtensionCount = num_required_extensions;
    createInfo.ppEnabledExtensionNames = required_device_extensions;

    if (vkCreateInstance(&createInfo, NULL, &vk_instance) != VK_SUCCESS) {
        logfatal("Failed to create Vulkan instance.");
    }

    if (!SDL_Vulkan_CreateSurface(window, vk_instance, &vk_surface)) {
        logfatal("Failed to create Vulkan window surface: %s", SDL_GetError());
    }

    logfatal("Need to initialize required_device_layers and num_required_device_layers");
    logfatal("Need to initialize required_features");
    logfatal("Need a vulkan physical device here");
}

void render_init(n64_system_t* system, n64_video_type_t video_type) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        logfatal("SDL couldn't initialize! %s", SDL_GetError());
    }
    if (video_type == OPENGL) {
        video_init_opengl();
    } else if (video_type == VULKAN) {
        video_init_vulkan();
    }
    n64_video_type = video_type;
    audio_init(system);
}

void handle_event(n64_system_t* system, SDL_Event* event) {
    switch (event->type) {
        case SDL_QUIT:
            logwarn("User requested quit");
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
                    update_joyaxis_y(system, 0, INT8_MAX);
                    //update_button(system, 0, DPAD_UP, true);
                    break;

                case SDLK_DOWN:
                case SDLK_s:
                    update_joyaxis_y(system, 0, INT8_MIN);
                    //update_button(system, 0, DPAD_DOWN, true);
                    break;

                case SDLK_LEFT:
                case SDLK_a:
                    update_joyaxis_x(system, 0, INT8_MIN);
                    //update_button(system, 0, DPAD_LEFT, true);
                    break;

                case SDLK_RIGHT:
                case SDLK_d:
                    update_joyaxis_x(system, 0, INT8_MAX);
                    //update_button(system, 0, DPAD_RIGHT, true);
                    break;

                case SDLK_q:
                    update_button(system, 0, Z, true);
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
                    update_joyaxis_y(system, 0, 0);
                    //update_button(system, 0, DPAD_UP, false);
                    break;

                case SDLK_DOWN:
                case SDLK_s:
                    update_joyaxis_y(system, 0, 0);
                    //update_button(system, 0, DPAD_DOWN, false);
                    break;

                case SDLK_LEFT:
                case SDLK_a:
                    update_joyaxis_x(system, 0, 0);
                    //update_button(system, 0, DPAD_LEFT, false);
                    break;

                case SDLK_RIGHT:
                case SDLK_d:
                    update_joyaxis_x(system, 0, 0);
                    //update_button(system, 0, DPAD_RIGHT, false);
                    break;

                case SDLK_q:
                    update_button(system, 0, Z, false);
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

    switch (n64_video_type) {
        case OPENGL:
            SDL_RenderPresent(renderer);
            break;
        case VULKAN:
            logfatal("Unsupported video type VULKAN!");
        case UNKNOWN:
            logfatal("Unknown video type!");
    }

    sdl_numframes++;
    uint32_t ticks = SDL_GetTicks();
    if (sdl_lastframe < ticks - fps_interval) {
        sdl_lastframe = ticks;
        sdl_fps = sdl_numframes;
        sdl_numframes = 0;
        snprintf(sdl_wintitle, sizeof(sdl_wintitle), N64_APP_NAME " [%s] %02d FPS", system->mem.rom.header.image_name, sdl_fps);
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
