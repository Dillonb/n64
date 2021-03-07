#include "device.h"
#include "tas_movie.h"

#include <stdbool.h>
#include <system/n64system.h>

static n64_joybus_device_t joybus_devices[6];

void update_button(int controller, n64_button_t button, bool held) {
    switch(button) {
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
    }
}

void update_joyaxis_x(int controller, sbyte x) {
    joybus_devices[controller].controller.joy_x = x;
}

void update_joyaxis_y(int controller, sbyte y) {
    joybus_devices[controller].controller.joy_y = y;
}

void devices_init(n64_save_type_t save_type) {
    joybus_devices[0].type = JOYBUS_CONTROLLER;
    // TODO: make this configurable
    joybus_devices[0].controller.accessory_type = CONTROLLER_ACCESSORY_MEMPACK;

    joybus_devices[1].type = JOYBUS_NONE;
    joybus_devices[2].type = JOYBUS_NONE;
    joybus_devices[3].type = JOYBUS_NONE;
    if (save_type == SAVE_EEPROM_4k) {
        joybus_devices[4].type = JOYBUS_4KB_EEPROM;
    } else if (save_type == SAVE_EEPROM_16k) {
        joybus_devices[4].type = JOYBUS_16KB_EEPROM;
    } else {
        joybus_devices[4].type = JOYBUS_NONE;
    }
    joybus_devices[5].type = JOYBUS_NONE;
}

void device_id_for_pif(int pif_channel, byte* res) {
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

bool device_read_buttons_for_pif(int pif_channel, byte* res) {
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
