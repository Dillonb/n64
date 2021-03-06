#ifndef N64_DEVICE_H
#define N64_DEVICE_H

#include <util.h>
#include <stdbool.h>

#include <mem/n64mem.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum n64_button {
    N64_BUTTON_A,
    N64_BUTTON_B,
    N64_BUTTON_Z,
    N64_BUTTON_START,
    N64_BUTTON_DPAD_UP,
    N64_BUTTON_DPAD_DOWN,
    N64_BUTTON_DPAD_LEFT,
    N64_BUTTON_DPAD_RIGHT,
    N64_BUTTON_L,
    N64_BUTTON_R,
    N64_BUTTON_C_UP,
    N64_BUTTON_C_DOWN,
    N64_BUTTON_C_LEFT,
    N64_BUTTON_C_RIGHT,
} n64_button_t;

void update_button(int controller, n64_button_t button, bool held);
void update_joyaxis_x(int controller, sbyte x);
void update_joyaxis_y(int controller, sbyte y);
void devices_init(n64_save_type_t save_type);
void device_id_for_pif(int pif_channel, byte* res);
bool device_read_buttons_for_pif(int pif_channel, byte* res);

typedef struct n64_controller {
    bool plugged_in;
    union {
        byte byte1;
        struct {
            bool dp_right:1;
            bool dp_left:1;
            bool dp_down:1;
            bool dp_up:1;
            bool start:1;
            bool z:1;
            bool b:1;
            bool a:1;
        };
    };
    union {
        byte byte2;
        struct {
            bool c_right:1;
            bool c_left:1;
            bool c_down:1;
            bool c_up:1;
            bool r:1;
            bool l:1;
            bool zero:1;
            bool joy_reset:1;
        };
    };
    sbyte joy_x;
    sbyte joy_y;
} n64_controller_t;

typedef enum n64_joybus_device_type {
    JOYBUS_NONE,
    JOYBUS_CONTROLLER,
    JOYBUS_DANCEPAD,
    JOYBUS_VRU,
    JOYBUS_MOUSE,
    JOYBUS_RANDNET_KEYBOARD,
    JOYBUS_DENSHA_DE_GO,
    JOYBUS_4KB_EEPROM,
    JOYBUS_16KB_EEPROM
} n64_joybus_device_type_t;

typedef struct n64_joybus_device {
    n64_joybus_device_type_t type;
    union {
        n64_controller_t controller;
    };
} n64_joybus_device_t;

#ifdef __cplusplus
}
#endif

#endif //N64_DEVICE_H
