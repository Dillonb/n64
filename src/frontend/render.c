#include "render.h"
#include "SDL_error.h"
#include "audio.h"
#include "gamepad.h"
#include "frontend.h"
#include "log.h"

#include <SDL.h>
#include <SDL_vulkan.h>

#include <volk.h>
#include <rdp/parallel_rdp_wrapper.h>
#include <settings.h>

// prior to 2.0.10, this was anonymous enum
#if SDL_COMPILEDVERSION <  SDL_VERSIONNUM(2, 0, 10)
    typedef int SDL_PixelFormatEnum;
#endif

int SCREEN_SCALE = 2;
static SDL_GLContext gl_context;
SDL_Window* window = NULL;
static SDL_Renderer* renderer = NULL;
static SDL_Texture* texture = NULL;
static u8 pixel_buffer[640 * 480 * 4]; // should be the largest needed
static n64_video_type_t n64_video_type = UNKNOWN_VIDEO_TYPE;

u32 fps_interval = 1000; // 1000ms = 1 second
u32 sdl_lastframe = 0;
u32 sdl_numframes = 0;
u32 sdl_fps = 0;
u32 game_fps = 0;
char sdl_wintitle[100] = N64_APP_NAME " 00 FPS";

SDL_Window* get_window_handle() {
    return window;
}

void video_init_vulkan() {
    window = SDL_CreateWindow(N64_APP_NAME,
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              N64_SCREEN_X * SCREEN_SCALE,
                              N64_SCREEN_Y * SCREEN_SCALE,
                              SDL_WINDOW_SHOWN | SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
    if (!window) {
        logfatal("Failed to initialize SDL window: %s", SDL_GetError());
    }
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
    if (!window) {
        logfatal("Failed to create SDL window: %s", SDL_GetError());
    }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (!renderer) {
        logfatal("Failed to create SDL renderer: %s", SDL_GetError());
    }
}

void render_init(n64_video_type_t video_type) {
    uint32_t flags = SDL_INIT_AUDIO;
    if (video_type != QT_VULKAN_VIDEO_TYPE) {
        flags |= SDL_INIT_VIDEO;
    }

    if (SDL_Init(flags) < 0) {
        logfatal("SDL couldn't initialize! %s", SDL_GetError());
    }
    switch (video_type) {
        case VULKAN_VIDEO_TYPE:
            video_init_vulkan();
            break;
        case QT_VULKAN_VIDEO_TYPE:
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
    audio_init();
    gamepad_init();

    for (int i = 0; i < 4; i++) {
        register_sdl_keyboard_bindings(i, n64_settings.controller[i].keyboard_a, N64_BUTTON_A);
        register_sdl_keyboard_bindings(i, n64_settings.controller[i].keyboard_b, N64_BUTTON_B);
        register_sdl_keyboard_bindings(i, n64_settings.controller[i].keyboard_start, N64_BUTTON_START);

        register_sdl_keyboard_bindings(i, n64_settings.controller[i].keyboard_dpad_up, N64_BUTTON_DPAD_UP);
        register_sdl_keyboard_bindings(i, n64_settings.controller[i].keyboard_dpad_down, N64_BUTTON_DPAD_DOWN);
        register_sdl_keyboard_bindings(i, n64_settings.controller[i].keyboard_dpad_left, N64_BUTTON_DPAD_LEFT);
        register_sdl_keyboard_bindings(i, n64_settings.controller[i].keyboard_dpad_right, N64_BUTTON_DPAD_RIGHT);

        register_sdl_keyboard_bindings(i, n64_settings.controller[i].keyboard_c_up, N64_BUTTON_C_UP);
        register_sdl_keyboard_bindings(i, n64_settings.controller[i].keyboard_c_down, N64_BUTTON_C_DOWN);
        register_sdl_keyboard_bindings(i, n64_settings.controller[i].keyboard_c_left, N64_BUTTON_C_LEFT);
        register_sdl_keyboard_bindings(i, n64_settings.controller[i].keyboard_c_right, N64_BUTTON_C_RIGHT);

        register_sdl_keyboard_bindings(i, n64_settings.controller[i].keyboard_joy_up, N64_BUTTON_JOY_UP);
        register_sdl_keyboard_bindings(i, n64_settings.controller[i].keyboard_joy_down, N64_BUTTON_JOY_DOWN);
        register_sdl_keyboard_bindings(i, n64_settings.controller[i].keyboard_joy_left, N64_BUTTON_JOY_LEFT);
        register_sdl_keyboard_bindings(i, n64_settings.controller[i].keyboard_joy_right, N64_BUTTON_JOY_RIGHT);

        register_sdl_keyboard_bindings(i, n64_settings.controller[i].keyboard_rb, N64_BUTTON_R);
        register_sdl_keyboard_bindings(i, n64_settings.controller[i].keyboard_lb, N64_BUTTON_L);
        register_sdl_keyboard_bindings(i, n64_settings.controller[i].keyboard_z, N64_BUTTON_Z);
    }
}

static u32 last_vi_type = 0;
static u32 vi_height = 0;
static u32 vi_width = 0;

INLINE void pre_scanout(SDL_PixelFormatEnum pixel_format) {
    float y_scale = (float)n64sys.vi.yscale.scale / 1024.0;
    float x_scale = (float)n64sys.vi.xscale.scale / 1024.0;

    int new_height = ceilf((float)((n64sys.vi.vstart.end - n64sys.vi.vstart.start) >> 1) * y_scale);
    int new_width  = ceilf((float)((n64sys.vi.hstart.end - n64sys.vi.hstart.start) >> 0) * x_scale);

    bool should_recreate_texture = false;

    should_recreate_texture |= new_height != vi_height;
    should_recreate_texture |= new_width != vi_width;

    should_recreate_texture |= last_vi_type != n64sys.vi.status.type;

    if (should_recreate_texture) {
        last_vi_type = n64sys.vi.status.type;
        vi_height = new_height;
        vi_width = new_width;
        if (texture != NULL) {
            SDL_DestroyTexture(texture);
        }
        texture = SDL_CreateTexture(renderer, pixel_format, SDL_TEXTUREACCESS_STREAMING, vi_width, vi_height);
    }

}

static void vi_scanout_16bit() {
    pre_scanout(SDL_PIXELFORMAT_RGBA5551);
    const int rdram_offset = n64sys.vi.vi_origin & (N64_RDRAM_SIZE - 1);
    for (int y = 0; y < vi_height; y++) {
        int yofs = (y * vi_width * 2);
        for (int x = 0; x < vi_width; x += 2) {
            memcpy(&pixel_buffer[yofs + x * 2 + 2], &n64sys.mem.rdram[rdram_offset + yofs + x * 2 + 0], sizeof(u16));
            memcpy(&pixel_buffer[yofs + x * 2 + 0], &n64sys.mem.rdram[rdram_offset + yofs + x * 2 + 2], sizeof(u16));
        }
    }
    SDL_UpdateTexture(texture, NULL, &pixel_buffer, vi_width * 2);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
}

static void vi_scanout_32bit() {
    pre_scanout(SDL_PIXELFORMAT_RGBA8888);
    int rdram_offset = n64sys.vi.vi_origin & (N64_RDRAM_SIZE - 1);
    SDL_UpdateTexture(texture, NULL, &n64sys.mem.rdram[rdram_offset], vi_width * 4);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
}

void render_screen_software() {
    n64_poll_input();

    switch (n64sys.vi.status.type) {
        case VI_TYPE_BLANK:
            SDL_RenderClear(renderer);
            break;
        case VI_TYPE_RESERVED:
            logfatal("VI_TYPE_RESERVED");
        case VI_TYPE_16BIT:
            vi_scanout_16bit();
            break;
        case VI_TYPE_32BIT:
            vi_scanout_32bit();
            break;
        default:
            logfatal("Unknown VI type: %d", n64sys.vi.status.type);
    }

    SDL_RenderPresent(renderer);
}

void n64_render_screen() {
    switch (n64_video_type) {
        case VULKAN_VIDEO_TYPE: // frame pushing handled elsewhere
        case QT_VULKAN_VIDEO_TYPE:
            break;
        case SOFTWARE_VIDEO_TYPE:
            render_screen_software();
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
        game_fps = n64sys.vi.swaps;
        n64sys.vi.swaps = 0;
        const char* game_name = n64sys.mem.rom.game_name_db != NULL ? n64sys.mem.rom.game_name_db : n64sys.mem.rom.game_name_cartridge;
        if (game_name == NULL || strcmp(game_name, "") == 0) {
            snprintf(sdl_wintitle, sizeof(sdl_wintitle), N64_APP_NAME " %02d emulator FPS / %02d game FPS", sdl_fps, game_fps);
        } else {
            snprintf(sdl_wintitle, sizeof(sdl_wintitle), N64_APP_NAME " [%s] %02d emulator FPS / %02d game FPS", game_name, sdl_fps, game_fps);
        }
        SDL_SetWindowTitle(window, sdl_wintitle);
    }
}

bool is_framerate_unlocked() {
    switch (n64_video_type) {
        case VULKAN_VIDEO_TYPE:
        case QT_VULKAN_VIDEO_TYPE:
            return prdp_is_framerate_unlocked();

        case UNKNOWN_VIDEO_TYPE:
        case SOFTWARE_VIDEO_TYPE:
            return false;
    }
}

void set_framerate_unlocked(bool unlocked) {
    switch (n64_video_type) {
        case VULKAN_VIDEO_TYPE:
        case QT_VULKAN_VIDEO_TYPE:
            prdp_set_framerate_unlocked(unlocked);
            break;

        case UNKNOWN_VIDEO_TYPE:
        case SOFTWARE_VIDEO_TYPE:
            break;
    }
}
