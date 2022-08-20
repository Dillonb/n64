#include <frontend/device.h>

typedef struct n64_settings {
    n64_joybus_device_type_t controller_ports[4];
} n64_settings_t;

extern n64_settings_t n64_settings;

void n64_settings_init();