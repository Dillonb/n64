#include <SDL.h>
#include <mem/pif.h>
#include "gamepad.h"

// TODO: support multiple controllers
static SDL_GameController* controller = NULL;
static SDL_Joystick* joystick = NULL;

void gamepad_refresh() {
    if (controller != NULL) {
        SDL_GameControllerClose(controller);
        controller = NULL;
        joystick = NULL;
    }

    bool found_one = false;

    for (int i = 0; i < SDL_NumJoysticks(); i++) {
        if (SDL_IsGameController(i)) {
            if (!found_one) {
                logalways("Detected game controller!");
                found_one = true;
                controller = SDL_GameControllerOpen(i);
                if (controller) {
                    joystick = SDL_GameControllerGetJoystick(controller);
                }
            } else {
                logalways("Found more than one game controller, using the first one detected!");
            }
        }
    }

    if (!found_one) {
        logalways("Didn't detect any game controllers.");
    }
}

void gamepad_init(n64_system_t* system) {
    if (SDL_Init(SDL_INIT_GAMECONTROLLER) < 0) {
        logfatal("Failed to initialize SDL Gamecontroller!");
    }
    gamepad_refresh();
}

void gamepad_update_button(n64_system_t* system, byte button, bool state) {
    switch (button) {
        case SDL_CONTROLLER_BUTTON_A:
            update_button(system, 0, N64_BUTTON_A, state);
            break;

        case SDL_CONTROLLER_BUTTON_B:
        case SDL_CONTROLLER_BUTTON_X:
            update_button(system, 0, N64_BUTTON_B, state);
            break;

        case SDL_CONTROLLER_BUTTON_DPAD_UP:
            update_button(system, 0, N64_BUTTON_DPAD_UP, state);
            break;

        case SDL_CONTROLLER_BUTTON_DPAD_DOWN:
            update_button(system, 0, N64_BUTTON_DPAD_DOWN, state);
            break;

        case SDL_CONTROLLER_BUTTON_DPAD_LEFT:
            update_button(system, 0, N64_BUTTON_DPAD_LEFT, state);
            break;

        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT:
            update_button(system, 0, N64_BUTTON_DPAD_RIGHT, state);
            break;

        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER:
            update_button(system, 0, N64_BUTTON_L, state);
            break;

        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER:
            update_button(system, 0, N64_BUTTON_R, state);
            break;

        case SDL_CONTROLLER_BUTTON_GUIDE:
        case SDL_CONTROLLER_BUTTON_BACK:
            update_button(system, 0, N64_BUTTON_START, state);
            break;

    }
    printf("Button %d %s\n", button, state ? "down" : "up");
}

void gamepad_update_axis(n64_system_t* system, byte axis, shalf value) {
    sbyte trimmed = value >> 8;
    switch (axis) {
        case SDL_CONTROLLER_AXIS_LEFTX:
            update_joyaxis_x(system, 0, trimmed);
            break;
        case SDL_CONTROLLER_AXIS_LEFTY:
            update_joyaxis_y(system, 0, -trimmed);
            break;
        default:
            printf("axis %d %d\n", axis, value);
    }
}
