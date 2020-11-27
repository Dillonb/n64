#include "render.h"
#include "audio.h"
#include "gamepad.h"

#include <SDL.h>
#include <SDL_vulkan.h>

#define GLAD_GL_IMPLEMENTATION
#include <glad/gl.h>
#include <volk.h>

#include <mem/pif.h>

int SCREEN_SCALE = 2;
static SDL_GLContext gl_context;
SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;
static n64_video_type_t n64_video_type = UNKNOWN_VIDEO_TYPE;

word fps_interval = 1000; // 1000ms = 1 second
word sdl_lastframe = 0;
word sdl_numframes = 0;
word sdl_fps = 0;
char sdl_wintitle[100] = N64_APP_NAME " 00 FPS";

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
    if (volkInitialize() != VK_SUCCESS) {
        logfatal("Failed to load Volk");
    }

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
    gamepad_init(system);
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
                    update_button(system, 0, N64_BUTTON_A, true);
                    break;

                case SDLK_k:
                    update_button(system, 0, N64_BUTTON_B, true);
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
                    update_button(system, 0, N64_BUTTON_Z, true);
                    break;

                case SDLK_RETURN:
                    update_button(system, 0, N64_BUTTON_START, true);
                    break;
            }
            break;
        }
        case SDL_KEYUP: {
            switch (event->key.keysym.sym) {
                case SDLK_j:
                    update_button(system, 0, N64_BUTTON_A, false);
                    break;

                case SDLK_k:
                    update_button(system, 0, N64_BUTTON_B, false);
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
                    update_button(system, 0, N64_BUTTON_Z, false);
                    break;

                case SDLK_RETURN:
                    update_button(system, 0, N64_BUTTON_START, false);
                    break;
            }
            break;
        }

        case SDL_CONTROLLERDEVICEADDED:
        case SDL_CONTROLLERDEVICEREMOVED:
        case SDL_CONTROLLERDEVICEREMAPPED:
            gamepad_refresh();
            break;
        case SDL_CONTROLLERBUTTONDOWN:
            gamepad_update_button(system, event->cbutton.button, true);
            break;
        case SDL_CONTROLLERBUTTONUP:
            gamepad_update_button(system, event->cbutton.button, false);
            break;
        case SDL_CONTROLLERAXISMOTION:
            gamepad_update_axis(system, event->caxis.axis, event->caxis.value);
            break;
    }
}

void n64_poll_input(n64_system_t* system) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        handle_event(system, &event);
    }
}

void n64_render_screen(n64_system_t* system) {
    switch (n64_video_type) {
        case OPENGL:
            SDL_RenderPresent(renderer);
            break;
        case VULKAN: // frame pushing handled elsewhere
            break;
        case UNKNOWN_VIDEO_TYPE:
            logfatal("Unknown video type!");
    }

    sdl_numframes++;
    uint32_t ticks = SDL_GetTicks();
    if (sdl_lastframe < ticks - fps_interval) {
        sdl_lastframe = ticks;
        sdl_fps = sdl_numframes;
        sdl_numframes = 0;
        snprintf(sdl_wintitle, sizeof(sdl_wintitle), N64_APP_NAME " [%s] %02d FPS", system->mem.rom.game_name, sdl_fps);
        SDL_SetWindowTitle(window, sdl_wintitle);
    }
}
