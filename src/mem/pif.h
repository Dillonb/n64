#ifndef N64_PIF_H
#define N64_PIF_H

#include <system/n64system.h>

typedef enum n64_button {
    A,
    B,
    Z,
    START,
    DPAD_UP,
    DPAD_DOWN,
    DPAD_LEFT,
    DPAD_RIGHT,
} n64_button_t;

void pif_rom_execute(n64_system_t* system);
void process_pif_command(n64_system_t* system);
void update_button(n64_system_t* system, int controller, n64_button_t button, bool held);
void update_joyaxis(n64_system_t* system, int controller, sbyte x, sbyte y);
void update_joyaxis_x(n64_system_t* system, int controller, sbyte x);
void update_joyaxis_y(n64_system_t* system, int controller, sbyte y);
void load_pif_rom(n64_system_t* system, const char* pif_rom_path);
#endif //N64_PIF_H
