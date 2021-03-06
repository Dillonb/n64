#include "frontend.h"
#include "device.h"
#include "gamepad.h"

#include <log.h>

static event_handler_t imgui_event_handler = NULL;

void handle_event(SDL_Event* event) {
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
                    update_button(0, N64_BUTTON_A, true);
                    break;

                case SDLK_k:
                    update_button(0, N64_BUTTON_B, true);
                    break;

                case SDLK_UP:
                case SDLK_w:
                    update_joyaxis_y(0, INT8_MAX);
                    //update_button(0, DPAD_UP, true);
                    break;

                case SDLK_DOWN:
                case SDLK_s:
                    update_joyaxis_y(0, INT8_MIN);
                    //update_button(0, DPAD_DOWN, true);
                    break;

                case SDLK_LEFT:
                case SDLK_a:
                    update_joyaxis_x(0, INT8_MIN);
                    //update_button(0, DPAD_LEFT, true);
                    break;

                case SDLK_RIGHT:
                case SDLK_d:
                    update_joyaxis_x(0, INT8_MAX);
                    //update_button(0, DPAD_RIGHT, true);
                    break;

                case SDLK_q:
                    update_button(0, N64_BUTTON_Z, true);
                    break;

                case SDLK_RETURN:
                    update_button(0, N64_BUTTON_START, true);
                    break;
            }
            break;
        }
        case SDL_KEYUP: {
            switch (event->key.keysym.sym) {
                case SDLK_j:
                    update_button(0, N64_BUTTON_A, false);
                    break;

                case SDLK_k:
                    update_button(0, N64_BUTTON_B, false);
                    break;

                case SDLK_UP:
                case SDLK_w:
                    update_joyaxis_y(0, 0);
                    //update_button(0, DPAD_UP, false);
                    break;

                case SDLK_DOWN:
                case SDLK_s:
                    update_joyaxis_y(0, 0);
                    //update_button(0, DPAD_DOWN, false);
                    break;

                case SDLK_LEFT:
                case SDLK_a:
                    update_joyaxis_x(0, 0);
                    //update_button(0, DPAD_LEFT, false);
                    break;

                case SDLK_RIGHT:
                case SDLK_d:
                    update_joyaxis_x(0, 0);
                    //update_button(0, DPAD_RIGHT, false);
                    break;

                case SDLK_q:
                    update_button(0, N64_BUTTON_Z, false);
                    break;

                case SDLK_RETURN:
                    update_button(0, N64_BUTTON_START, false);
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
