#ifndef N64_PIF_H
#define N64_PIF_H

#include <system/n64system.h>

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

void pif_rom_execute(n64_system_t* system);
void process_pif_command(n64_system_t* system);
void update_button(n64_system_t* system, int controller, n64_button_t button, bool held);
void update_joyaxis(n64_system_t* system, int controller, sbyte x, sbyte y);
void update_joyaxis_x(n64_system_t* system, int controller, sbyte x);
void update_joyaxis_y(n64_system_t* system, int controller, sbyte y);
void load_pif_rom(n64_system_t* system, const char* pif_rom_path);

#ifdef __cplusplus
}
#endif
#endif //N64_PIF_H
