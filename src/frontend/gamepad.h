#ifndef N64_GAMEPAD_H
#define N64_GAMEPAD_H

#include <system/n64system.h>

void gamepad_init(n64_system_t* system);
void gamepad_refresh();
void gamepad_update_button(n64_system_t* system, byte button, bool state);
void gamepad_update_axis(n64_system_t* system, byte axis, shalf value);

#endif //N64_GAMEPAD_H
