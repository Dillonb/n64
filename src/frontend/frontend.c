#include "frontend.h"
#include "device.h"
#include "gamepad.h"
#include "render.h"

#include <log.h>

static event_handler_t imgui_event_handler = NULL;

typedef struct n64_keyboard_mapping {
    n64_button_t button;
    int player;
} n64_keyboard_mapping_t;

//n64_button_t keyboard_mappings[4][SDL_NUM_SCANCODES] = { 0 };
n64_keyboard_mapping_t keyboard_mappings[SDL_NUM_SCANCODES] = { 0 };

void register_sdl_keyboard_bindings(int player, SDL_KeyCode bindings[2], n64_button_t button) {
    for (int i = 0; i < 2; i++) {
        if (bindings[i] != SDLK_UNKNOWN) {
            SDL_Scancode scancode = SDL_GetScancodeFromKey(bindings[i]);
            if (scancode == SDL_SCANCODE_UNKNOWN) {
                logwarn("non-unknown keycode mapped to unknown scancode! %s\n", SDL_GetError());
            } else {
                keyboard_mappings[scancode].button = button;
                keyboard_mappings[scancode].player = player;
            }
        }
    }
}

void handle_event(SDL_Event* event) {
    switch (event->type) {
        case SDL_QUIT:
            logwarn("User requested quit");
            n64_request_quit();
            break;
        case SDL_KEYDOWN: {
            n64_keyboard_mapping_t* mapping = &keyboard_mappings[event->key.keysym.scancode];
            if (mapping->button != N64_BUTTON_NONE) {
                update_button(mapping->player, mapping->button, true);
            }

            switch (event->key.keysym.sym) {
                case SDLK_ESCAPE: // TODO: shift-esc
                    n64_request_quit();
                    break;
                case SDLK_u:
                    set_framerate_unlocked(!is_framerate_unlocked());
                    break;
            }
            break;
        }
        case SDL_KEYUP: {
            n64_keyboard_mapping_t* mapping = &keyboard_mappings[event->key.keysym.scancode];
            if (mapping->button != N64_BUTTON_NONE) {
                update_button(mapping->player, mapping->button, false);
            }
            break;
        }

        case SDL_CONTROLLERDEVICEADDED:
        case SDL_CONTROLLERDEVICEREMOVED:
        case SDL_CONTROLLERDEVICEREMAPPED:
            gamepad_refresh();
            break;
        case SDL_CONTROLLERBUTTONDOWN:
            gamepad_update_button(event->cbutton.button, true);
            break;
        case SDL_CONTROLLERBUTTONUP:
            gamepad_update_button(event->cbutton.button, false);
            break;
        case SDL_CONTROLLERAXISMOTION:
            gamepad_update_axis(event->caxis.axis, event->caxis.value);
            break;
    }
}

void n64_poll_input() {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        if (imgui_event_handler == NULL || !imgui_event_handler(&event)) {
            handle_event(&event);
        }
    }
}

void register_imgui_event_handler(event_handler_t handler) {
    imgui_event_handler = handler;
}
