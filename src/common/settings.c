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
#include <SDL_keyboard.h>

#define GETCWD getcwd
const char PATH_DELIMITER = '/';
#endif

void n64_settings_load_defaults() {
    n64_settings.controller_port[0] = JOYBUS_CONTROLLER;
    n64_settings.controller_port[1] = JOYBUS_NONE;
    n64_settings.controller_port[2] = JOYBUS_NONE;
    n64_settings.controller_port[3] = JOYBUS_NONE;

    n64_settings.controller[0].keyboard_enabled = true;
    n64_settings.controller[1].keyboard_enabled = false;
    n64_settings.controller[2].keyboard_enabled = false;
    n64_settings.controller[3].keyboard_enabled = false;

    // Controller 0 mapping
    n64_settings.controller[0].keyboard_a[0] = SDLK_j;
    n64_settings.controller[0].keyboard_a[1] = SDLK_UNKNOWN;
    n64_settings.controller[0].keyboard_b[0] = SDLK_k;
    n64_settings.controller[0].keyboard_b[1] = SDLK_UNKNOWN;
    n64_settings.controller[0].keyboard_start[0] = SDLK_RETURN;
    n64_settings.controller[0].keyboard_start[1] = SDLK_UNKNOWN;

    n64_settings.controller[0].keyboard_dpad_up[0] = SDLK_UNKNOWN;
    n64_settings.controller[0].keyboard_dpad_up[1] = SDLK_UNKNOWN;
    n64_settings.controller[0].keyboard_dpad_down[0] = SDLK_UNKNOWN;
    n64_settings.controller[0].keyboard_dpad_down[1] = SDLK_UNKNOWN;
    n64_settings.controller[0].keyboard_dpad_left[0] = SDLK_UNKNOWN;
    n64_settings.controller[0].keyboard_dpad_left[1] = SDLK_UNKNOWN;
    n64_settings.controller[0].keyboard_dpad_right[0] = SDLK_UNKNOWN;
    n64_settings.controller[0].keyboard_dpad_right[1] = SDLK_UNKNOWN;

    n64_settings.controller[0].keyboard_c_up[0] = SDLK_UNKNOWN;
    n64_settings.controller[0].keyboard_c_up[1] = SDLK_UNKNOWN;
    n64_settings.controller[0].keyboard_c_down[0] = SDLK_UNKNOWN;
    n64_settings.controller[0].keyboard_c_down[1] = SDLK_UNKNOWN;
    n64_settings.controller[0].keyboard_c_left[0] = SDLK_UNKNOWN;
    n64_settings.controller[0].keyboard_c_left[1] = SDLK_UNKNOWN;
    n64_settings.controller[0].keyboard_c_right[0] = SDLK_UNKNOWN;
    n64_settings.controller[0].keyboard_c_right[1] = SDLK_UNKNOWN;

    n64_settings.controller[0].keyboard_joy_up[0] = SDLK_UP;
    n64_settings.controller[0].keyboard_joy_up[1] = SDLK_w;
    n64_settings.controller[0].keyboard_joy_down[0] = SDLK_DOWN;
    n64_settings.controller[0].keyboard_joy_down[1] = SDLK_s;
    n64_settings.controller[0].keyboard_joy_left[0] = SDLK_LEFT;
    n64_settings.controller[0].keyboard_joy_left[1] = SDLK_a;
    n64_settings.controller[0].keyboard_joy_right[0] = SDLK_RIGHT;
    n64_settings.controller[0].keyboard_joy_right[1] = SDLK_d;

    n64_settings.controller[0].keyboard_rb[0] = SDLK_UNKNOWN;
    n64_settings.controller[0].keyboard_rb[1] = SDLK_UNKNOWN;
    n64_settings.controller[0].keyboard_lb[0] = SDLK_UNKNOWN;
    n64_settings.controller[0].keyboard_lb[1] = SDLK_UNKNOWN;
    n64_settings.controller[0].keyboard_z[0] = SDLK_q;
    n64_settings.controller[0].keyboard_z[1] = SDLK_UNKNOWN;

    // Controller 1 mapping
    n64_settings.controller[1].keyboard_a[0] = SDLK_UNKNOWN;
    n64_settings.controller[1].keyboard_a[1] = SDLK_UNKNOWN;
    n64_settings.controller[1].keyboard_b[0] = SDLK_UNKNOWN;
    n64_settings.controller[1].keyboard_b[1] = SDLK_UNKNOWN;
    n64_settings.controller[1].keyboard_start[0] = SDLK_UNKNOWN;
    n64_settings.controller[1].keyboard_start[1] = SDLK_UNKNOWN;

    n64_settings.controller[1].keyboard_dpad_up[0] = SDLK_UNKNOWN;
    n64_settings.controller[1].keyboard_dpad_up[1] = SDLK_UNKNOWN;
    n64_settings.controller[1].keyboard_dpad_down[0] = SDLK_UNKNOWN;
    n64_settings.controller[1].keyboard_dpad_down[1] = SDLK_UNKNOWN;
    n64_settings.controller[1].keyboard_dpad_left[0] = SDLK_UNKNOWN;
    n64_settings.controller[1].keyboard_dpad_left[1] = SDLK_UNKNOWN;
    n64_settings.controller[1].keyboard_dpad_right[0] = SDLK_UNKNOWN;
    n64_settings.controller[1].keyboard_dpad_right[1] = SDLK_UNKNOWN;

    n64_settings.controller[1].keyboard_c_up[0] = SDLK_UNKNOWN;
    n64_settings.controller[1].keyboard_c_up[1] = SDLK_UNKNOWN;
    n64_settings.controller[1].keyboard_c_down[0] = SDLK_UNKNOWN;
    n64_settings.controller[1].keyboard_c_down[1] = SDLK_UNKNOWN;
    n64_settings.controller[1].keyboard_c_left[0] = SDLK_UNKNOWN;
    n64_settings.controller[1].keyboard_c_left[1] = SDLK_UNKNOWN;
    n64_settings.controller[1].keyboard_c_right[0] = SDLK_UNKNOWN;
    n64_settings.controller[1].keyboard_c_right[1] = SDLK_UNKNOWN;

    n64_settings.controller[1].keyboard_joy_up[0] = SDLK_UNKNOWN;
    n64_settings.controller[1].keyboard_joy_up[1] = SDLK_UNKNOWN;
    n64_settings.controller[1].keyboard_joy_down[0] = SDLK_UNKNOWN;
    n64_settings.controller[1].keyboard_joy_down[1] = SDLK_UNKNOWN;
    n64_settings.controller[1].keyboard_joy_left[0] = SDLK_UNKNOWN;
    n64_settings.controller[1].keyboard_joy_left[1] = SDLK_UNKNOWN;
    n64_settings.controller[1].keyboard_joy_right[0] = SDLK_UNKNOWN;
    n64_settings.controller[1].keyboard_joy_right[1] = SDLK_UNKNOWN;

    n64_settings.controller[1].keyboard_rb[0] = SDLK_UNKNOWN;
    n64_settings.controller[1].keyboard_rb[1] = SDLK_UNKNOWN;
    n64_settings.controller[1].keyboard_lb[0] = SDLK_UNKNOWN;
    n64_settings.controller[1].keyboard_lb[1] = SDLK_UNKNOWN;
    n64_settings.controller[1].keyboard_z[0] = SDLK_UNKNOWN;
    n64_settings.controller[1].keyboard_z[1] = SDLK_UNKNOWN;

    // Controller 2 mapping
    n64_settings.controller[2].keyboard_a[0] = SDLK_UNKNOWN;
    n64_settings.controller[2].keyboard_a[1] = SDLK_UNKNOWN;
    n64_settings.controller[2].keyboard_b[0] = SDLK_UNKNOWN;
    n64_settings.controller[2].keyboard_b[1] = SDLK_UNKNOWN;
    n64_settings.controller[2].keyboard_start[0] = SDLK_UNKNOWN;
    n64_settings.controller[2].keyboard_start[1] = SDLK_UNKNOWN;

    n64_settings.controller[2].keyboard_dpad_up[0] = SDLK_UNKNOWN;
    n64_settings.controller[2].keyboard_dpad_up[1] = SDLK_UNKNOWN;
    n64_settings.controller[2].keyboard_dpad_down[0] = SDLK_UNKNOWN;
    n64_settings.controller[2].keyboard_dpad_down[1] = SDLK_UNKNOWN;
    n64_settings.controller[2].keyboard_dpad_left[0] = SDLK_UNKNOWN;
    n64_settings.controller[2].keyboard_dpad_left[1] = SDLK_UNKNOWN;
    n64_settings.controller[2].keyboard_dpad_right[0] = SDLK_UNKNOWN;
    n64_settings.controller[2].keyboard_dpad_right[1] = SDLK_UNKNOWN;

    n64_settings.controller[2].keyboard_c_up[0] = SDLK_UNKNOWN;
    n64_settings.controller[2].keyboard_c_up[1] = SDLK_UNKNOWN;
    n64_settings.controller[2].keyboard_c_down[0] = SDLK_UNKNOWN;
    n64_settings.controller[2].keyboard_c_down[1] = SDLK_UNKNOWN;
    n64_settings.controller[2].keyboard_c_left[0] = SDLK_UNKNOWN;
    n64_settings.controller[2].keyboard_c_left[1] = SDLK_UNKNOWN;
    n64_settings.controller[2].keyboard_c_right[0] = SDLK_UNKNOWN;
    n64_settings.controller[2].keyboard_c_right[1] = SDLK_UNKNOWN;

    n64_settings.controller[2].keyboard_joy_up[0] = SDLK_UNKNOWN;
    n64_settings.controller[2].keyboard_joy_up[1] = SDLK_UNKNOWN;
    n64_settings.controller[2].keyboard_joy_down[0] = SDLK_UNKNOWN;
    n64_settings.controller[2].keyboard_joy_down[1] = SDLK_UNKNOWN;
    n64_settings.controller[2].keyboard_joy_left[0] = SDLK_UNKNOWN;
    n64_settings.controller[2].keyboard_joy_left[1] = SDLK_UNKNOWN;
    n64_settings.controller[2].keyboard_joy_right[0] = SDLK_UNKNOWN;
    n64_settings.controller[2].keyboard_joy_right[1] = SDLK_UNKNOWN;

    n64_settings.controller[2].keyboard_rb[0] = SDLK_UNKNOWN;
    n64_settings.controller[2].keyboard_rb[1] = SDLK_UNKNOWN;
    n64_settings.controller[2].keyboard_lb[0] = SDLK_UNKNOWN;
    n64_settings.controller[2].keyboard_lb[1] = SDLK_UNKNOWN;
    n64_settings.controller[2].keyboard_z[0] = SDLK_UNKNOWN;
    n64_settings.controller[2].keyboard_z[1] = SDLK_UNKNOWN;

    // Controller 3 mapping
    n64_settings.controller[3].keyboard_a[0] = SDLK_UNKNOWN;
    n64_settings.controller[3].keyboard_a[1] = SDLK_UNKNOWN;
    n64_settings.controller[3].keyboard_b[0] = SDLK_UNKNOWN;
    n64_settings.controller[3].keyboard_b[1] = SDLK_UNKNOWN;
    n64_settings.controller[3].keyboard_start[0] = SDLK_UNKNOWN;
    n64_settings.controller[3].keyboard_start[1] = SDLK_UNKNOWN;

    n64_settings.controller[3].keyboard_dpad_up[0] = SDLK_UNKNOWN;
    n64_settings.controller[3].keyboard_dpad_up[1] = SDLK_UNKNOWN;
    n64_settings.controller[3].keyboard_dpad_down[0] = SDLK_UNKNOWN;
    n64_settings.controller[3].keyboard_dpad_down[1] = SDLK_UNKNOWN;
    n64_settings.controller[3].keyboard_dpad_left[0] = SDLK_UNKNOWN;
    n64_settings.controller[3].keyboard_dpad_left[1] = SDLK_UNKNOWN;
    n64_settings.controller[3].keyboard_dpad_right[0] = SDLK_UNKNOWN;
    n64_settings.controller[3].keyboard_dpad_right[1] = SDLK_UNKNOWN;

    n64_settings.controller[3].keyboard_c_up[0] = SDLK_UNKNOWN;
    n64_settings.controller[3].keyboard_c_up[1] = SDLK_UNKNOWN;
    n64_settings.controller[3].keyboard_c_down[0] = SDLK_UNKNOWN;
    n64_settings.controller[3].keyboard_c_down[1] = SDLK_UNKNOWN;
    n64_settings.controller[3].keyboard_c_left[0] = SDLK_UNKNOWN;
    n64_settings.controller[3].keyboard_c_left[1] = SDLK_UNKNOWN;
    n64_settings.controller[3].keyboard_c_right[0] = SDLK_UNKNOWN;
    n64_settings.controller[3].keyboard_c_right[1] = SDLK_UNKNOWN;

    n64_settings.controller[3].keyboard_joy_up[0] = SDLK_UNKNOWN;
    n64_settings.controller[3].keyboard_joy_up[1] = SDLK_UNKNOWN;
    n64_settings.controller[3].keyboard_joy_down[0] = SDLK_UNKNOWN;
    n64_settings.controller[3].keyboard_joy_down[1] = SDLK_UNKNOWN;
    n64_settings.controller[3].keyboard_joy_left[0] = SDLK_UNKNOWN;
    n64_settings.controller[3].keyboard_joy_left[1] = SDLK_UNKNOWN;
    n64_settings.controller[3].keyboard_joy_right[0] = SDLK_UNKNOWN;
    n64_settings.controller[3].keyboard_joy_right[1] = SDLK_UNKNOWN;

    n64_settings.controller[3].keyboard_rb[0] = SDLK_UNKNOWN;
    n64_settings.controller[3].keyboard_rb[1] = SDLK_UNKNOWN;
    n64_settings.controller[3].keyboard_lb[0] = SDLK_UNKNOWN;
    n64_settings.controller[3].keyboard_lb[1] = SDLK_UNKNOWN;
    n64_settings.controller[3].keyboard_z[0] = SDLK_UNKNOWN;
    n64_settings.controller[3].keyboard_z[1] = SDLK_UNKNOWN;

    n64_settings.controller[0].gamepad_enabled = true;
    n64_settings.controller[1].gamepad_enabled = false;
    n64_settings.controller[2].gamepad_enabled = false;
    n64_settings.controller[3].gamepad_enabled = false;


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

#define CONFIG_TEXT(l, ...) do { if (fprintf(f, l, ##__VA_ARGS__) < 0) { return -1; }} while(0)
#define CONFIG_LINE(l, ...) CONFIG_TEXT(l "\n", ##__VA_ARGS__)
#define BOOL_TO_TEXT(x) ((x) ? "true" : "false")

int write_key_bindings(FILE* f, SDL_KeyCode bindings[2]) {
    if (bindings[0] == SDLK_UNKNOWN && bindings[1] == SDLK_UNKNOWN) {
        CONFIG_LINE("");
    } else if (bindings[0] == SDLK_UNKNOWN) {
        CONFIG_LINE("%s", SDL_GetKeyName(bindings[1]));
    } else if (bindings[1] == SDLK_UNKNOWN) {
        CONFIG_LINE("%s", SDL_GetKeyName(bindings[0]));
    } else {
        CONFIG_TEXT("%s,", SDL_GetKeyName(bindings[0]));
        CONFIG_LINE("%s", SDL_GetKeyName(bindings[1]));
    }
    return 0;
}

int n64_settings_write(const char* path) {
    FILE* f = fopen(path, "w");
    CONFIG_LINE("; WARNING: this file will be overwritten automatically.");
    CONFIG_LINE("; Settings can be modified manually, but only when the emulator is closed.");
    CONFIG_LINE("; All formatting changes will be lost when the emulator rewrites this file.");
    CONFIG_LINE("");


    CONFIG_LINE("; Joybus devices/Controller ports. Configure what type of device is plugged in.");
    CONFIG_LINE("; Valid values: 'NONE', 'CONTROLLER', 'DANCEPAD', 'VRU', 'MOUSE', 'KEYBOARD', 'DENSHA'");
    CONFIG_LINE("; WARNING: Not all are implemented yet.");
    CONFIG_LINE("[joybus]");
    for (int i = 0; i < 4; i++) {
        CONFIG_LINE("port%d=%s", i + 1, joybus_to_str(n64_settings.controller_port[i]));
    }

    CONFIG_LINE("; Controllers. These settings are only applied if the matching joybus port is set to CONTROLLER.");
    for (int i = 0; i < 4; i++) {
        CONFIG_LINE("[controller%d]", i + 1);
        CONFIG_LINE("keyboard_enabled=%s", BOOL_TO_TEXT(n64_settings.controller[i].keyboard_enabled));
        CONFIG_LINE();

        CONFIG_TEXT("keyboard_a=");
        write_key_bindings(f, n64_settings.controller[i].keyboard_a);
        CONFIG_TEXT("keyboard_b=");
        write_key_bindings(f, n64_settings.controller[i].keyboard_b);
        CONFIG_TEXT("keyboard_start=");
        write_key_bindings(f, n64_settings.controller[i].keyboard_start);
        CONFIG_LINE();

        CONFIG_TEXT("keyboard_dpad_up=");
        write_key_bindings(f, n64_settings.controller[i].keyboard_dpad_up);
        CONFIG_TEXT("keyboard_dpad_down=");
        write_key_bindings(f, n64_settings.controller[i].keyboard_dpad_down);
        CONFIG_TEXT("keyboard_dpad_left=");
        write_key_bindings(f, n64_settings.controller[i].keyboard_dpad_left);
        CONFIG_TEXT("keyboard_dpad_right=");
        write_key_bindings(f, n64_settings.controller[i].keyboard_dpad_right);
        CONFIG_LINE();

        CONFIG_TEXT("keyboard_c_up=");
        write_key_bindings(f, n64_settings.controller[i].keyboard_c_up);
        CONFIG_TEXT("keyboard_c_down=");
        write_key_bindings(f, n64_settings.controller[i].keyboard_c_down);
        CONFIG_TEXT("keyboard_c_left=");
        write_key_bindings(f, n64_settings.controller[i].keyboard_c_left);
        CONFIG_TEXT("keyboard_c_right=");
        write_key_bindings(f, n64_settings.controller[i].keyboard_c_right);
        CONFIG_LINE();

        CONFIG_TEXT("keyboard_joy_up=");
        write_key_bindings(f, n64_settings.controller[i].keyboard_joy_up);
        CONFIG_TEXT("keyboard_joy_down=");
        write_key_bindings(f, n64_settings.controller[i].keyboard_joy_down);
        CONFIG_TEXT("keyboard_joy_left=");
        write_key_bindings(f, n64_settings.controller[i].keyboard_joy_left);
        CONFIG_TEXT("keyboard_joy_right=");
        write_key_bindings(f, n64_settings.controller[i].keyboard_joy_right);
        CONFIG_LINE();

        CONFIG_TEXT("keyboard_rb=");
        write_key_bindings(f, n64_settings.controller[i].keyboard_rb);
        CONFIG_TEXT("keyboard_lb=");
        write_key_bindings(f, n64_settings.controller[i].keyboard_lb);
        CONFIG_TEXT("keyboard_z=");
        write_key_bindings(f, n64_settings.controller[i].keyboard_z);
        CONFIG_LINE();


        CONFIG_LINE();
        CONFIG_LINE("gamepad_enabled=%s", BOOL_TO_TEXT(n64_settings.controller[i].gamepad_enabled));
    }

    if (fclose(f) != 0) {
        return -1;
    }

    return 0;
}

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
#define SECTION_IS(sec) (strcmp(section, sec) == 0)
#define NAME_IS(n) (strcmp(name, n) == 0)

void load_controller_mapping_entry(SDL_KeyCode bindings[2], const char* entry) {
    char* save;
    char* first = strtok_r((char*)entry, ",", &save);
    char* second = NULL;
    if (first != NULL) {
        second = strtok_r(NULL, ",", &save);
    }

    if (first == NULL) {
        first = "";
    }

    if (second == NULL) {
        second = "";
    }

    bindings[0] = SDL_GetKeyFromName(first);
    bindings[1] = SDL_GetKeyFromName(second);
}

void load_controller_mapping(int index, const char* name, const char* value) {
    if (NAME_IS("keyboard_a")) {
        load_controller_mapping_entry(n64_settings.controller[index].keyboard_a, value);
    } else if (NAME_IS("keyboard_b")) {
        load_controller_mapping_entry(n64_settings.controller[index].keyboard_b, value);
    } else if (NAME_IS("keyboard_start")) {
        load_controller_mapping_entry(n64_settings.controller[index].keyboard_start, value);
    } else if (NAME_IS("keyboard_dpad_up")) {
        load_controller_mapping_entry(n64_settings.controller[index].keyboard_dpad_up, value);
    } else if (NAME_IS("keyboard_dpad_down")) {
        load_controller_mapping_entry(n64_settings.controller[index].keyboard_dpad_down, value);
    } else if (NAME_IS("keyboard_dpad_left")) {
        load_controller_mapping_entry(n64_settings.controller[index].keyboard_dpad_left, value);
    } else if (NAME_IS("keyboard_dpad_right")) {
        load_controller_mapping_entry(n64_settings.controller[index].keyboard_dpad_right, value);
    } else if (NAME_IS("keyboard_c_up")) {
        load_controller_mapping_entry(n64_settings.controller[index].keyboard_c_up, value);
    } else if (NAME_IS("keyboard_c_down")) {
        load_controller_mapping_entry(n64_settings.controller[index].keyboard_c_down, value);
    } else if (NAME_IS("keyboard_c_left")) {
        load_controller_mapping_entry(n64_settings.controller[index].keyboard_c_left, value);
    } else if (NAME_IS("keyboard_c_right")) {
        load_controller_mapping_entry(n64_settings.controller[index].keyboard_c_right, value);
    } else if (NAME_IS("keyboard_joy_up")) {
        load_controller_mapping_entry(n64_settings.controller[index].keyboard_joy_up, value);
    } else if (NAME_IS("keyboard_joy_down")) {
        load_controller_mapping_entry(n64_settings.controller[index].keyboard_joy_down, value);
    } else if (NAME_IS("keyboard_joy_left")) {
        load_controller_mapping_entry(n64_settings.controller[index].keyboard_joy_left, value);
    } else if (NAME_IS("keyboard_joy_right")) {
        load_controller_mapping_entry(n64_settings.controller[index].keyboard_joy_right, value);
    } else if (NAME_IS("keyboard_rb")) {
        load_controller_mapping_entry(n64_settings.controller[index].keyboard_rb, value);
    } else if (NAME_IS("keyboard_lb")) {
        load_controller_mapping_entry(n64_settings.controller[index].keyboard_lb, value);
    } else if (NAME_IS("keyboard_z")) {
        load_controller_mapping_entry(n64_settings.controller[index].keyboard_z, value);
    }
}

int handler(void* user, const char* section, const char* name, const char* value) {
    if (SECTION_IS("joybus")) {
        if (NAME_IS("port1")) {
            n64_settings.controller_port[0] = str_to_joybus(value);
        } else if(NAME_IS("port2")) {
            n64_settings.controller_port[1] = str_to_joybus(value);
        } else if(NAME_IS("port3")) {
            n64_settings.controller_port[2] = str_to_joybus(value);
        } else if(NAME_IS("port4")) {
            n64_settings.controller_port[3] = str_to_joybus(value);
        } else {
            return 0;
        }
    } else if (SECTION_IS("controller1")) {
        load_controller_mapping(0, name, value);
    } else if (SECTION_IS("controller2")) {
        load_controller_mapping(1, name, value);
    } else if (SECTION_IS("controller3")) {
        load_controller_mapping(2, name, value);
    } else if (SECTION_IS("controller4")) {
        load_controller_mapping(3, name, value);
    }

    return 1;
}

int n64_settings_load(const char* path) {
    int result = ini_parse(path, handler, NULL);
    if (result != 0) {
        return result;
    }
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
    if (file_exists(config_file_path)) {
        int err = n64_settings_load(config_file_path);
        if (err != 0) {
            logwarn("Failed to load settings file!");
        }
    }
    // Rewrite settings file always
    n64_settings_write(config_file_path);
}