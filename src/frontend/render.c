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
static SDL_Texture* texture = NULL;
static byte pixel_buffer[640 * 480 * 4]; // should be the largest needed
static n64_video_type_t n64_video_type = UNKNOWN_VIDEO_TYPE;

word fps_interval = 1000; // 1000ms = 1 second
word sdl_lastframe = 0;
word sdl_numframes = 0;
word sdl_fps = 0;
char sdl_wintitle[100] = N64_APP_NAME " 00 FPS";

SDL_Window* get_window_handle() {
    return window;
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
    if (volkInitialize() != VK_SUCCESS) {
        logfatal("Failed to load Volk");
    }

}

void video_init_software() {
    window = SDL_CreateWindow(N64_APP_NAME,
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              N64_SCREEN_X * SCREEN_SCALE,
                              N64_SCREEN_Y * SCREEN_SCALE,
                              SDL_WINDOW_SHOWN);
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
}

void render_init(n64_system_t* system, n64_video_type_t video_type) {
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        logfatal("SDL couldn't initialize! %s", SDL_GetError());
    }
    switch (video_type) {
        case OPENGL_VIDEO_TYPE:
            video_init_opengl();
            break;
        case VULKAN_VIDEO_TYPE:
            video_init_vulkan();
            break;
        case SOFTWARE_VIDEO_TYPE:
            video_init_software();
            break;

        case UNKNOWN_VIDEO_TYPE:
        default:
            logwarn("Unknown video type, not initializing video!");
            break;
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

#define yscale_to_height(yscale) ((15 * (yscale)) / 64)

static word last_vi_type = 0;
static word last_yscale = 0;
static word vi_height = 0;
static word vi_width = 0;

INLINE void pre_scanout(n64_system_t* system, SDL_PixelFormatEnum pixel_format) {
    vi_width = system->vi.vi_width;
    if (system->vi.yscale != last_yscale) {
        last_yscale = system->vi.yscale;
        vi_height = yscale_to_height(last_yscale);
    }

    if (last_vi_type != system->vi.status.type) {
        last_vi_type = system->vi.status.type;
        if (texture != NULL) {
            SDL_DestroyTexture(texture);
        }
        texture = SDL_CreateTexture(renderer, pixel_format, SDL_TEXTUREACCESS_STREAMING, vi_width, vi_height);
    }

}

static void vi_scanout_16bit(n64_system_t* system) {
    pre_scanout(system, SDL_PIXELFORMAT_RGBA5551);
    int rdram_offset = system->vi.vi_origin & (N64_RDRAM_SIZE - 1);
    for (int y = 0; y < vi_height; y++) {
        int yofs = (y * vi_width * 2);
        for (int x = 0; x < vi_width; x += 2) {
            memcpy(&pixel_buffer[yofs + x * 2 + 2], &system->mem.rdram[rdram_offset + yofs + x * 2 + 0], sizeof(half));
            memcpy(&pixel_buffer[yofs + x * 2 + 0], &system->mem.rdram[rdram_offset + yofs + x * 2 + 2], sizeof(half));
        }
    }
    SDL_UpdateTexture(texture, NULL, &pixel_buffer, vi_width * 2);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
}

static void vi_scanout_32bit(n64_system_t* system) {
    pre_scanout(system, SDL_PIXELFORMAT_RGBA8888);
    int rdram_offset = system->vi.vi_origin & (N64_RDRAM_SIZE - 1);
    SDL_UpdateTexture(texture, NULL, &system->mem.rdram[rdram_offset], vi_width * 4);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
}

void render_screen_software(n64_system_t* system) {
    n64_poll_input(system);

    switch (system->vi.status.type) {
        case VI_TYPE_BLANK:
            SDL_RenderClear(renderer);
            break;
        case VI_TYPE_RESERVED:
            logfatal("VI_TYPE_RESERVED");
        case VI_TYPE_16BIT:
            vi_scanout_16bit(system);
            break;
        case VI_TYPE_32BIT:
            vi_scanout_32bit(system);
            break;
        default:
            logfatal("Unknown VI type: %d", system->vi.status.type);
    }

    SDL_RenderPresent(renderer);
}

void n64_render_screen(n64_system_t* system) {
    switch (n64_video_type) {
        case OPENGL_VIDEO_TYPE:
            SDL_RenderPresent(renderer);
            break;
        case VULKAN_VIDEO_TYPE: // frame pushing handled elsewhere
            break;
        case SOFTWARE_VIDEO_TYPE:
            render_screen_software(system);
            break;
        case UNKNOWN_VIDEO_TYPE:
        default:
            logfatal("Unknown video type!");
    }

    sdl_numframes++;
    uint32_t ticks = SDL_GetTicks();
    if (sdl_lastframe < ticks - fps_interval) {
        sdl_lastframe = ticks;
        sdl_fps = sdl_numframes;
        sdl_numframes = 0;
        const char* game_name = system->mem.rom.game_name_db != NULL ? system->mem.rom.game_name_db : system->mem.rom.game_name_cartridge;
        if (game_name == NULL || strcmp(game_name, "") == 0) {
            snprintf(sdl_wintitle, sizeof(sdl_wintitle), N64_APP_NAME " %02d FPS", sdl_fps);
        } else {
            snprintf(sdl_wintitle, sizeof(sdl_wintitle), N64_APP_NAME " [%s] %02d FPS", game_name, sdl_fps);
        }
        SDL_SetWindowTitle(window, sdl_wintitle);
    }
}
