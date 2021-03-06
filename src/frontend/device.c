#include "device.h"
#include "tas_movie.h"

#include <stdbool.h>
#include <system/n64system.h>

static n64_controller_t controllers[4];

void update_button(int controller, n64_button_t button, bool held) {
    switch(button) {
        case N64_BUTTON_A:
            controllers[controller].a = held;
            break;

        case N64_BUTTON_B:
            controllers[controller].b = held;
            break;

        case N64_BUTTON_Z:
            controllers[controller].z = held;
            break;

        case N64_BUTTON_DPAD_UP:
            controllers[controller].dp_up = held;
            break;

        case N64_BUTTON_DPAD_DOWN:
            controllers[controller].dp_down = held;
            break;

        case N64_BUTTON_DPAD_LEFT:
            controllers[controller].dp_left = held;
            break;

        case N64_BUTTON_DPAD_RIGHT:
            controllers[controller].dp_right = held;
            break;

        case N64_BUTTON_START:
            controllers[controller].start = held;
            break;

        case N64_BUTTON_L:
            controllers[controller].l = held;
            break;

        case N64_BUTTON_R:
            controllers[controller].r = held;
            break;

        case N64_BUTTON_C_UP:
            controllers[controller].c_up = held;
            break;

        case N64_BUTTON_C_DOWN:
            controllers[controller].c_down = held;
            break;

        case N64_BUTTON_C_LEFT:
            controllers[controller].c_left = held;
            break;

        case N64_BUTTON_C_RIGHT:
            controllers[controller].c_right = held;
            break;
    }
}

void update_joyaxis_x(int controller, sbyte x) {
    controllers[controller].joy_x = x;
}

void update_joyaxis_y(int controller, sbyte y) {
    controllers[controller].joy_y = y;
}

void devices_init() {
    controllers[0].plugged_in = true;
    controllers[1].plugged_in = false;
    controllers[2].plugged_in = false;
    controllers[3].plugged_in = false;
}

void device_id_for_pif(int pif_channel, byte* res) {
    if (pif_channel < 4) {
        if (controllers[pif_channel].plugged_in) {
            res[0] = 0x05;
            res[1] = 0x00;
            res[2] = 0x01; // Controller pak plugged in.
        } else {
            res[0] = 0x05;
            res[1] = 0x00;
            res[2] = 0x02;
        }
    } else if (pif_channel == 4) { // EEPROM is on channel 4, and sometimes 5.
        res[0] = 0x00;
        res[1] = 0x80;
        res[2] = 0x00;
    } else {
        logfatal("Controller ID on unknown channel %d", pif_channel);
    }
}

bool device_read_buttons_for_pif(int pif_channel, byte* res) {
    if (pif_channel < 4 && controllers[pif_channel].plugged_in) {
        if (tas_movie_loaded()) {
            // Load inputs from TAS movie
            n64_controller_t controller = tas_next_inputs();
            res[0] = controller.byte1;
            res[1] = controller.byte2;
            res[2] = controller.joy_x;
            res[3] = controller.joy_y;
        } else {
            // Load inputs normally
            res[0] = controllers[pif_channel].byte1;
            res[1] = controllers[pif_channel].byte2;
            res[2] = controllers[pif_channel].joy_x;
            res[3] = controllers[pif_channel].joy_y;
        }
        return true; // Success!
    } else {
        res[0]  = 0x00;
        res[1]  = 0x00;
        res[2]  = 0x00;
        res[3]  = 0x00;
        return false; // Device not present
    }
}
