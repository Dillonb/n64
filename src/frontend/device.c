#include "device.h"
#include "tas_movie.h"

#include <stdbool.h>
#include <math.h>
#include <system/n64system.h>
#include <settings.h>

static n64_joybus_device_t joybus_devices[6];

void update_button(int controller, n64_button_t button, bool held) {
    switch(button) {
        case N64_BUTTON_NONE:
            break;

        case N64_BUTTON_A:
            joybus_devices[controller].controller.a = held;
            break;

        case N64_BUTTON_B:
            joybus_devices[controller].controller.b = held;
            break;

        case N64_BUTTON_Z:
            joybus_devices[controller].controller.z = held;
            break;

        case N64_BUTTON_DPAD_UP:
            joybus_devices[controller].controller.dp_up = held;
            break;

        case N64_BUTTON_DPAD_DOWN:
            joybus_devices[controller].controller.dp_down = held;
            break;

        case N64_BUTTON_DPAD_LEFT:
            joybus_devices[controller].controller.dp_left = held;
            break;

        case N64_BUTTON_DPAD_RIGHT:
            joybus_devices[controller].controller.dp_right = held;
            break;

        case N64_BUTTON_START:
            joybus_devices[controller].controller.start = held;
            break;

        case N64_BUTTON_L:
            joybus_devices[controller].controller.l = held;
            break;

        case N64_BUTTON_R:
            joybus_devices[controller].controller.r = held;
            break;

        case N64_BUTTON_C_UP:
            joybus_devices[controller].controller.c_up = held;
            break;

        case N64_BUTTON_C_DOWN:
            joybus_devices[controller].controller.c_down = held;
            break;

        case N64_BUTTON_C_LEFT:
            joybus_devices[controller].controller.c_left = held;
            break;

        case N64_BUTTON_C_RIGHT:
            joybus_devices[controller].controller.c_right = held;
            break;

        case N64_BUTTON_JOY_UP:
            update_joyaxis_y(controller, held ? JOYAXIS_MIN : 0);
            break;

        case N64_BUTTON_JOY_DOWN:
            update_joyaxis_y(controller, held ? JOYAXIS_MAX : 0);
            break;

        case N64_BUTTON_JOY_LEFT:
            update_joyaxis_x(controller, held ? JOYAXIS_MIN : 0);
            break;

        case N64_BUTTON_JOY_RIGHT:
            update_joyaxis_x(controller, held ? JOYAXIS_MAX : 0);
            break;
    }
}

double trim_gamepad_axis(s16 raw) {
    // INT16_MIN through INT16_MAX to -1 through +1
    if(raw < -32767.0) raw = -32767.0;
    return (s16)raw / 32767.0;
}

double deadzone_corrected_response(double peak_cardinal, double peak_diagonal, double axial_deadzone, double outer_radius, double length, double current_axis_input) {
    if(length <= outer_radius) {
        double length_absolute = fabs(current_axis_input);
        if(length_absolute <= axial_deadzone) {
            length_absolute = 0.0;
        } else {
            length_absolute = (length_absolute - axial_deadzone) * peak_cardinal / (peak_cardinal - axial_deadzone) / length_absolute;
        }
        current_axis_input *= length_absolute;
    } else {
        length = outer_radius / length;
        current_axis_input *= length;
    }
    
    return current_axis_input;
}

void clamp_gamepad(n64_controller_t* controller) {
    double max_cardinal   = 85.0;
    double max_diagonal   = 69.0;
    double inner_deadzone =  7.0;
    double max_outer_deadzone_radius = 2.0 / sqrt(2.0) * (max_diagonal / max_cardinal * (max_cardinal - inner_deadzone) + inner_deadzone);

    double ax = trim_gamepad_axis(controller->raw_x);
    double ay = -trim_gamepad_axis(controller->raw_y);

    ax *= max_outer_deadzone_radius;
    ay *= max_outer_deadzone_radius;
    double len = sqrt(ax*ax+ay*ay);
    ax = deadzone_corrected_response(max_cardinal, max_diagonal, inner_deadzone, max_outer_deadzone_radius, len, ax);
    ay = deadzone_corrected_response(max_cardinal, max_diagonal, inner_deadzone, max_outer_deadzone_radius, len, ay);

    //bound diagonals to an octagonal range {-max_diagonal ... +max_diagonal} - Thanks merrymage
    if(ax != 0.0f && ay != 0.0f) {
        double slope = ay / ax;
        double edgex = copysign(max_cardinal / (fabs(slope) + (max_cardinal - max_diagonal) / max_diagonal), ax);
        double edgey = copysign(fmin(fabs(edgex * slope), max_cardinal / (1.0f / fabs(slope) + (max_cardinal - max_diagonal) / max_diagonal)), ay);
        edgex = edgey / slope;

        double scale = sqrt(edgex*edgex+edgey*edgey) / max_outer_deadzone_radius;
        ax *= scale;
        ay *= scale;
    }
    
    //clamp excess between max_cardinal and max_outer_deadzone_radius
    if(fabs(ax) > max_cardinal) ax = copysign(max_cardinal, ax);
    if(fabs(ay) > max_cardinal) ay = copysign(max_cardinal, ay);
    
    //add epsilon to counteract floating point precision error
    ax = copysign(fabs(ax) + 1e-09, ax);
    ay = copysign(fabs(ay) + 1e-09, ay);

    controller->joy_x = ax;
    controller->joy_y = ay;
}

void update_joyaxis_x(int controller, s16 x) {
    joybus_devices[controller].controller.raw_x = x;
    clamp_gamepad(&joybus_devices[controller].controller);
}

void update_joyaxis_y(int controller, s16 y) {
    joybus_devices[controller].controller.raw_y = y;
    clamp_gamepad(&joybus_devices[controller].controller);
}

void devices_init(n64_save_type_t save_type) {
    for (int i = 0; i < 4; i++) {
        joybus_devices[i].type = n64_settings.controller_port[i];
        if (joybus_devices[i].type) {
            // TODO: make this configurable
            joybus_devices[i].controller.accessory_type = CONTROLLER_ACCESSORY_MEMPACK;
        }
    }

    if (save_type == SAVE_EEPROM_4k) {
        joybus_devices[4].type = JOYBUS_4KB_EEPROM;
    } else if (save_type == SAVE_EEPROM_16k) {
        joybus_devices[4].type = JOYBUS_16KB_EEPROM;
    } else {
        joybus_devices[4].type = JOYBUS_NONE;
    }
    joybus_devices[5].type = JOYBUS_NONE;
}

void device_id_for_pif(int pif_channel, u8* res) {
    if (pif_channel < 6) {
        switch (joybus_devices[pif_channel].type) {
            case JOYBUS_NONE:
                res[0] = 0x00;
                res[1] = 0x00;
                res[2] = 0x00;
                break;
            case JOYBUS_CONTROLLER:
                res[0] = 0x05;
                res[1] = 0x00;
                res[2] = joybus_devices[pif_channel].controller.accessory_type != CONTROLLER_ACCESSORY_NONE ? 0x01 : 0x02;
                break;
            case JOYBUS_DANCEPAD:
                res[0] = 0x05;
                res[1] = 0x00;
                res[2] = 0x00;
                break;
            case JOYBUS_VRU:
                res[0] = 0x00;
                res[1] = 0x01;
                res[2] = 0x00;
                break;
            case JOYBUS_MOUSE:
                res[0] = 0x02;
                res[1] = 0x00;
                res[2] = 0x00;
                break;
            case JOYBUS_RANDNET_KEYBOARD:
                res[0] = 0x00;
                res[1] = 0x02;
                res[2] = 0x00;
                break;
            case JOYBUS_DENSHA_DE_GO:
                res[0] = 0x20;
                res[1] = 0x04;
                res[2] = 0x00;
                break;
            case JOYBUS_4KB_EEPROM:
                res[0] = 0x00;
                res[1] = 0x80;
                res[2] = 0x00;
                break;
            case JOYBUS_16KB_EEPROM:
                res[0] = 0x00;
                res[1] = 0xC0;
                res[2] = 0x00;
                break;
        }
    } else {
        logfatal("Device ID on unknown channel %d", pif_channel);
    }
}

bool device_read_buttons_for_pif(int pif_channel, u8* res) {
    if (pif_channel >= 6) {
        res[0]  = 0x00;
        res[1]  = 0x00;
        res[2]  = 0x00;
        res[3]  = 0x00;
        return false; // Device not present
    }

    switch (joybus_devices[pif_channel].type) {
        case JOYBUS_NONE:
            res[0]  = 0x00;
            res[1]  = 0x00;
            res[2]  = 0x00;
            res[3]  = 0x00;
            return false; // Device not present
        case JOYBUS_CONTROLLER:
            if (tas_movie_loaded()) {
                // Load inputs from TAS movie
                n64_controller_t controller = tas_next_inputs();
                res[0] = controller.byte1;
                res[1] = controller.byte2;
                res[2] = controller.joy_x;
                res[3] = controller.joy_y;
            } else {
                // Load inputs normally
                res[0] = joybus_devices[pif_channel].controller.byte1;
                res[1] = joybus_devices[pif_channel].controller.byte2;
                res[2] = joybus_devices[pif_channel].controller.joy_x;
                res[3] = joybus_devices[pif_channel].controller.joy_y;

                if (tas_movie_recording()) {
                    tas_record_inputs(&joybus_devices[pif_channel].controller);
                }
            }
            break;
        case JOYBUS_DANCEPAD:
            logfatal("read buttons from JOYBUS_DANCEPAD");
        case JOYBUS_VRU:
            logfatal("read buttons from JOYBUS_VRU");
        case JOYBUS_MOUSE:
            logfatal("read buttons from JOYBUS_MOUSE");
        case JOYBUS_RANDNET_KEYBOARD:
            logfatal("read buttons from JOYBUS_RANDNET_KEYBOARD");
        case JOYBUS_DENSHA_DE_GO:
            logfatal("read buttons from JOYBUS_DENSHA_DE_GO");
        case JOYBUS_4KB_EEPROM:
            logfatal("read buttons from JOYBUS_4KB_EEPROM");
        case JOYBUS_16KB_EEPROM:
            logfatal("read buttons from JOYBUS_16KB_EEPROM");
    }

    return true; // Success!
}

n64_controller_accessory_type_t get_controller_accessory_type(int pif_channel) {
    if (pif_channel >= 4) {
        logfatal("Accessing controller accessory of out of range channel id %d", pif_channel);
    } else if (joybus_devices[pif_channel].type != JOYBUS_CONTROLLER) {
        return CONTROLLER_ACCESSORY_NONE;
    } else {
        return joybus_devices[pif_channel].controller.accessory_type;
    }
}
