#ifndef N64_DEVICE_H
#define N64_DEVICE_H

#include <util.h>
#include <stdbool.h>

#include <mem/n64mem.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum n64_button {
    N64_BUTTON_NONE,

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

    // Fake buttons: set the joystick to max in each direction
    N64_BUTTON_JOY_UP,
    N64_BUTTON_JOY_DOWN,
    N64_BUTTON_JOY_LEFT,
    N64_BUTTON_JOY_RIGHT,
} n64_button_t;

typedef enum n64_controller_accessory_type {
    CONTROLLER_ACCESSORY_NONE,
    CONTROLLER_ACCESSORY_MEMPACK,
    CONTROLLER_ACCESSORY_RUMBLE_PACK
} n64_controller_accessory_type_t;

#define JOYAXIS_MIN INT16_MIN
#define JOYAXIS_MAX INT16_MAX

typedef struct n64_controller {
    n64_controller_accessory_type_t accessory_type;
    union {
        u8 byte1;
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
        u8 byte2;
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
    s8 joy_x;
    s8 joy_y;

    s16 raw_x;
    s16 raw_y;
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

void update_button(int controller, n64_button_t button, bool held);
void update_joyaxis_x(int controller, s16 x);
void update_joyaxis_y(int controller, s16 y);
void devices_init(n64_save_type_t save_type);
void device_id_for_pif(int pif_channel, u8* res);
bool device_read_buttons_for_pif(int pif_channel, u8* res);
n64_controller_accessory_type_t get_controller_accessory_type(int pif_channel);

// For testing
void override_joybus_devices_ptr(n64_joybus_device_t* override);

// Exposed for testing

// Trim input
double trim_gamepad_axis(s16 raw);
// Apply deadzone and scale according to response curve
double deadzone_corrected_response(double peak_cardinal, double peak_diagonal, double axial_deadzone, double outer_radius, double length, double current_axis_input);
// Do all requisite clamping for a controller
void clamp_gamepad(n64_controller_t* controller);

#ifdef __cplusplus
}
#endif

#endif //N64_DEVICE_H
