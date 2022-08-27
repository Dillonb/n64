#include <frontend/device.h>
#include <stdbool.h>
#include <SDL_keycode.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct n64_controller_mapping {
    bool keyboard_enabled;
    bool gamepad_enabled;

    SDL_KeyCode keyboard_a[2];
    SDL_KeyCode keyboard_b[2];
    SDL_KeyCode keyboard_start[2];

    SDL_KeyCode keyboard_dpad_up[2];
    SDL_KeyCode keyboard_dpad_down[2];
    SDL_KeyCode keyboard_dpad_left[2];
    SDL_KeyCode keyboard_dpad_right[2];

    SDL_KeyCode keyboard_c_up[2];
    SDL_KeyCode keyboard_c_down[2];
    SDL_KeyCode keyboard_c_left[2];
    SDL_KeyCode keyboard_c_right[2];

    SDL_KeyCode keyboard_joy_up[2];
    SDL_KeyCode keyboard_joy_down[2];
    SDL_KeyCode keyboard_joy_left[2];
    SDL_KeyCode keyboard_joy_right[2];

    SDL_KeyCode keyboard_rb[2];
    SDL_KeyCode keyboard_lb[2];
    SDL_KeyCode keyboard_z[2];
} n64_controller_mapping_t;

typedef struct n64_settings {
    n64_joybus_device_type_t controller_port[4];
    n64_controller_mapping_t controller[4];
    int scaling; // valid values: 0, 2, 4, 8
} n64_settings_t;

extern n64_settings_t n64_settings;

void n64_settings_init();

#ifdef __cplusplus
}
#endif
