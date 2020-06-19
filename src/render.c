#include <SDL.h>
#include "render.h"

#define N64_SCREEN_X 640
#define N64_SCREEN_Y 480

static int SCREEN_SCALE = 2;

static SDL_Window* window = NULL;
static uint32_t window_id;
static SDL_Renderer* renderer = NULL;
static SDL_Texture* argb32buffer = NULL;

word fps_interval = 1000; // 1000ms = 1 second
word sdl_lastframe = 0;
word sdl_numframes = 0;
word sdl_fps = 0;
char sdl_wintitle[100] = "dgb n64 00 FPS";

void render_init() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        logfatal("SDL couldn't initialize! %s", SDL_GetError());
    }

    window = SDL_CreateWindow("dgb n64",
                              SDL_WINDOWPOS_UNDEFINED,
                              SDL_WINDOWPOS_UNDEFINED,
                              N64_SCREEN_X * SCREEN_SCALE,
                              N64_SCREEN_Y * SCREEN_SCALE,
                              SDL_WINDOW_SHOWN);
    window_id = SDL_GetWindowID(window);

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    argb32buffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_STREAMING, N64_SCREEN_X, N64_SCREEN_Y);

    if (renderer == NULL) {
        logfatal("SDL couldn't create a renderer! %s", SDL_GetError());
    }

    SDL_RenderSetScale(renderer, SCREEN_SCALE, SCREEN_SCALE);
}

void handle_event(SDL_Event* event) {
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
            }
        }
    }
}

void render_screen(n64_system_t* system) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        handle_event(&event);
    }

    unimplemented(system->vi.status.type == VI_TYPE_RESERVED, "VI_TYPE_RESERVED unimplemented!")
    unimplemented(system->vi.status.type == VI_TYPE_16BIT, "VI_TYPE_16BIT unimplemented!")

    if (system->vi.status.type == VI_TYPE_32BIT) {
        SDL_UpdateTexture(argb32buffer, NULL, &system->mem.rdram[system->vi.vi_origin], system->vi.vi_width * 4);
        SDL_RenderCopy(renderer, argb32buffer, NULL, NULL);
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
