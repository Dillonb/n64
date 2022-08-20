#include "settings.h"
#include "log.h"
#include <string.h>
#include <util.h>
#include <stdio.h>
#include <ini.h>

n64_settings_t n64_settings;

#define CONFIG_FILENAME "dgb-n64.ini"

#ifdef N64_WIN
#include <direct.h>
const char PATH_DELIMITER = '\\';
#define GETCWD _getcwd
#else
#include <unistd.h>
#define GETCWD getcwd
const char PATH_DELIMITER = '/';
#endif

void n64_settings_load_defaults() {
    n64_settings.controller_ports[0] = JOYBUS_CONTROLLER;
    n64_settings.controller_ports[1] = JOYBUS_NONE;
    n64_settings.controller_ports[2] = JOYBUS_NONE;
    n64_settings.controller_ports[3] = JOYBUS_NONE;
}

const char* joybus_to_str(n64_joybus_device_type_t joybus) {
    switch (joybus) {
        case JOYBUS_NONE:             return "NONE";
        case JOYBUS_CONTROLLER:       return "CONTROLLER";
        case JOYBUS_DANCEPAD:         return "DANCEPAD";
        case JOYBUS_VRU:              return "VRU";
        case JOYBUS_MOUSE:            return "MOUSE";
        case JOYBUS_RANDNET_KEYBOARD: return "KEYBOARD";
        case JOYBUS_DENSHA_DE_GO:     return "DENSHA";
        case JOYBUS_4KB_EEPROM:       return "EEPROM_4K";
        case JOYBUS_16KB_EEPROM:      return "EEPROM_16K";
    }
}

n64_joybus_device_type_t str_to_joybus(const char* joybus) {
    if (strcmp("NONE",       joybus) == 0) return JOYBUS_NONE;
    if (strcmp("CONTROLLER", joybus) == 0) return JOYBUS_CONTROLLER;
    if (strcmp("DANCEPAD",   joybus) == 0) return JOYBUS_DANCEPAD;
    if (strcmp("VRU",        joybus) == 0) return JOYBUS_VRU;
    if (strcmp("MOUSE",      joybus) == 0) return JOYBUS_MOUSE;
    if (strcmp("KEYBOARD",   joybus) == 0) return JOYBUS_RANDNET_KEYBOARD;
    if (strcmp("DENSHA",     joybus) == 0) return JOYBUS_DENSHA_DE_GO;
    if (strcmp("EEPROM_4K",  joybus) == 0) return JOYBUS_4KB_EEPROM;
    if (strcmp("EEPROM_16K", joybus) == 0) return JOYBUS_16KB_EEPROM;
    return JOYBUS_NONE;
}

void n64_settings_write(const char* path) {
    FILE* f = fopen(path, "w");
    fprintf(f, "; WARNING: this file will be overwritten automatically.\n");
    fprintf(f, "; Settings can be modified manually, but only when the emulator is closed.\n");
    fprintf(f, "; All formatting changes will be lost when the emulator rewrites this file.\n");


    fprintf(f, "\n; Joybus devices/Controller ports. Configure what type of device is plugged in.\n");
    fprintf(f, "; Valid values: 'NONE', 'CONTROLLER', 'DANCEPAD', 'VRU', 'MOUSE', 'KEYBOARD', 'DENSHA'\n");
    fprintf(f, "; WARNING: Not all are implemented yet.\n");
    fprintf(f, "[joybus]\n");
    for (int i = 0; i < 4; i++) {
        fprintf(f, "port%d=%s\n", i + 1, joybus_to_str(n64_settings.controller_ports[i]));
    }
}

int handler(void* user, const char* section, const char* name, const char* value) {
#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
#define SECTION_IS(sec) (strcmp(section, sec) == 0)
#define NAME_IS(n) (strcmp(name, n) == 0)
    if (SECTION_IS("joybus")) {
        if (NAME_IS("port1")) {
            n64_settings.controller_ports[0] = str_to_joybus(value);
        } else if(NAME_IS("port2")) {
            n64_settings.controller_ports[1] = str_to_joybus(value);
        } else if(NAME_IS("port3")) {
            n64_settings.controller_ports[2] = str_to_joybus(value);
        } else if(NAME_IS("port4")) {
            n64_settings.controller_ports[3] = str_to_joybus(value);
        } else {
            return 0;
        }
    }

    return 1;
}

void n64_settings_load(const char* path) {
    ini_parse(path, handler, NULL);
}

void n64_settings_init() {
    n64_settings_load_defaults();
    char cwd[PATH_MAX];
    char config_file_path[PATH_MAX];
    GETCWD(cwd, PATH_MAX);
    int result = snprintf(config_file_path, PATH_MAX, "%s%c" CONFIG_FILENAME, cwd, PATH_DELIMITER);
    if (result < 0) {
        logfatal("Unable to build path to config file.");
    }
    if (!file_exists(config_file_path)) {
        n64_settings_write(config_file_path);
    }
    n64_settings_load(config_file_path);
}