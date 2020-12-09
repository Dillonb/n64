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
    if (SDL_GameControllerAddMappingsFromFile("gamecontrollerdb.txt") == -1) {
        logalways("Failed to load game controller DB!");
    } else {
        logalways("Loaded game controller DB!");
    }
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
        case SDL_CONTROLLER_BUTTON_START:
            update_button(system, 0, N64_BUTTON_START, state);
            break;

    }
}

shalf right_joyx, right_joyy;

#define SLICE_OFFSET 67.5
#define CHECKSLICE(degrees, angle) ((degrees) > ((angle) - SLICE_OFFSET) && (degrees) < ((angle) + SLICE_OFFSET))

void update_right_joyaxis(n64_system_t* system, byte axis, shalf value) {
    switch (axis) {
        case SDL_CONTROLLER_AXIS_RIGHTX:
            right_joyx = value;
            break;
        case SDL_CONTROLLER_AXIS_RIGHTY:
            right_joyy = value;
            break;
        default:
            break;
    }

    if (abs(right_joyx) < 8000 && abs(right_joyy) < 8000) {
        update_button(system, 0, N64_BUTTON_C_LEFT, false);
        update_button(system, 0, N64_BUTTON_C_RIGHT, false);
        update_button(system, 0, N64_BUTTON_C_UP, false);
        update_button(system, 0, N64_BUTTON_C_DOWN, false);
    } else {

        // normalize to unit circle
        double adjusted_joyx = (double)right_joyx / 32768;
        double adjusted_joyy = (double)right_joyy / -32768; // y axis is reversed from what you'd expect, so flip it back with this division

        // what direction are we pointing?
        double degrees = atan2(adjusted_joyy, adjusted_joyx) * 180 / 3.141592;

        // atan2 returns negative numbers for values > 180
        if (degrees < 0) {
            degrees = 360 + degrees;
        }

        // 135 degree slices, overlapping.
        update_button(system, 0, N64_BUTTON_C_UP, CHECKSLICE(degrees, 90));
        update_button(system, 0, N64_BUTTON_C_DOWN, CHECKSLICE(degrees, 270));
        update_button(system, 0, N64_BUTTON_C_LEFT, CHECKSLICE(degrees, 180));

        // Slightly different since it's around the 0 angle
        update_button(system, 0, N64_BUTTON_C_RIGHT, degrees < SLICE_OFFSET || degrees > (360 - SLICE_OFFSET));
    }
}

void gamepad_update_axis(n64_system_t* system, byte axis, shalf value) {
    shalf trimmed = value >> 8;
    shalf inverted = -(trimmed + 1);
    switch (axis) {
        case SDL_CONTROLLER_AXIS_LEFTX:
            update_joyaxis_x(system, 0, trimmed);
            break;
        case SDL_CONTROLLER_AXIS_LEFTY:
            update_joyaxis_y(system, 0, inverted);
            break;
        case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
        case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
            update_button(system, 0, N64_BUTTON_Z, value == INT16_MAX);
            break;
        case SDL_CONTROLLER_AXIS_RIGHTX:
        case SDL_CONTROLLER_AXIS_RIGHTY:
            update_right_joyaxis(system, axis, value);
            break;
        default:
            printf("axis %d %d\n", axis, value);
    }
}
