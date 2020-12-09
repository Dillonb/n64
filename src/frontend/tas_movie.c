#include <string.h>
#include <stdio.h>
#include <log.h>
#include <system/n64system.h>
#include "tas_movie.h"

typedef struct m64_movie_controller_flags {
    bool controller_1_present:1;
    bool controller_2_present:1;
    bool controller_3_present:1;
    bool controller_4_present:1;

    bool controller_1_mempak:1;
    bool controller_2_mempak:1;
    bool controller_3_mempak:1;
    bool controller_4_mempak:1;

    bool controller_1_rumblepak:1;
    bool controller_2_rumblepak:1;
    bool controller_3_rumblepak:1;
    bool controller_4_rumblepak:1;

    unsigned:20;
} m64_movie_controller_flags_t;

//static_assert(sizeof(m64_movie_controller_flags_t) == 4, "Incorrect size!");

typedef struct m64_movie_header {
    byte signature[4];
    uint32_t version;
    uint32_t uid;
    uint32_t num_frames;
    uint32_t rerecord_count;
    byte fps;
    byte num_controllers;
    byte reserved1;
    byte reserved2;
    uint32_t num_input_samples;
    /*
     value 1: movie begins from snapshot (the snapshot will be loaded from an externalfile
                                          with the movie filename and a .st extension)
     value 2: movie begins from power-on
     other values: invalid movie
     */
    uint16_t start_type;
    byte reserved3;
    byte reserved4;
    m64_movie_controller_flags_t controller_flags;
    byte reserved5[160];
    char rom_name[32];
    uint32_t rom_crc32;
    uint16_t rom_country_code;
    byte reserved6[56];
    // 122 64-byte ASCII string: name of video plugin used when recording, directly from plugin
    char video_plugin_name[64];
    // 162 64-byte ASCII string: name of sound plugin used when recording, directly from plugin
    char audio_plugin_name[64];
    // 1A2 64-byte ASCII string: name of input plugin used when recording, directly from plugin
    char input_plugin_name[64];
    // 1E2 64-byte ASCII string: name of rsp plugin used when recording, directly from plugin
    char rsp_plugin_name[64];
    // 222 222-byte UTF-8 string: author name info
    char author_name[222];
    // 300 256-byte UTF-8 string: author movie description info
    char movie_description[256];
} m64_movie_header_t;

typedef union tas_movie_controller_data {
    struct {
        bool dpad_right: 1;
        bool dpad_left: 1;
        bool dpad_down: 1;
        bool dpad_up: 1;
        bool start: 1;
        bool z: 1;
        bool b: 1;
        bool a: 1;
        bool c_right: 1;
        bool c_left: 1;
        bool c_down: 1;
        bool c_up: 1;
        bool r: 1;
        bool l: 1;
        byte: 2;
        sbyte analog_x: 8;
        sbyte analog_y: 8;
    };
    word raw;
} __attribute__((__packed__)) tas_movie_controller_data_t;

//static_assert(sizeof(tas_movie_controller_data_t) == 4, "Incorrect size for tas_movie_controller_data_t!");


//static_assert(sizeof(m64_movie_header_t) == 1024, "Incorrect size!");

static byte* loaded_tas_movie = NULL;
static size_t loaded_tas_movie_size = 0;
m64_movie_header_t loaded_tas_movie_header;
uint32_t loaded_tas_movie_index = 0;

void load_tas_movie(const char* filename) {
    FILE *fp = fopen(filename, "rb");

    if (fp == NULL) {
        logfatal("Error opening the movie file! Are you sure it's a valid movie and that it exists?");
    }

    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);

    fseek(fp, 0, SEEK_SET);
    byte *buf = malloc(size);
    fread(buf, size, 1, fp);

    loaded_tas_movie = buf;
    loaded_tas_movie_size = size;

    if (loaded_tas_movie == NULL) {
        logfatal("Error loading movie!");
    }

    if (loaded_tas_movie_size < 1024) {
        logfatal("This file looks too small to be a valid movie!");
    }

    memcpy(&loaded_tas_movie_header, buf, sizeof(m64_movie_header_t));

    if (loaded_tas_movie_header.signature[0] != 0x4D || loaded_tas_movie_header.signature[1] != 0x36 || loaded_tas_movie_header.signature[2] != 0x34 || loaded_tas_movie_header.signature[3] != 0x1A) {
        logfatal("Failed to load movie: incorrect signature. Are you sure this is a valid movie?");
    }

    if (loaded_tas_movie_header.version != 3) {
        logfatal("This movie is version %d: only version 3 is supported.", loaded_tas_movie_header.version);
    }

    if (loaded_tas_movie_header.start_type != 2) {
        logfatal("Movie start type is %d - only movies with a start type of 2 are supported (start at power on)", loaded_tas_movie_header.start_type);
    }

    // TODO: check ROM CRC32 here

    logalways("Loaded movie %s", loaded_tas_movie_header.movie_description);
    logalways("By %s", loaded_tas_movie_header.author_name);
    logalways("%d controller(s) connected\n", loaded_tas_movie_header.num_controllers);

    if (loaded_tas_movie_header.num_controllers != 1) {
        logfatal("Currently, only movies with 1 controller connected are supported.\n");
    }

    loaded_tas_movie_index = sizeof(m64_movie_header_t) - 4; // skip header
}

bool tas_movie_loaded() {
    return loaded_tas_movie != NULL;
}

n64_controller_t tas_next_inputs() {
    if (loaded_tas_movie_index + sizeof(tas_movie_controller_data_t) > loaded_tas_movie_size) {
        loaded_tas_movie = NULL;
        n64_controller_t empty_controller;
        memset(&empty_controller, 0, sizeof(n64_controller_t));
        return empty_controller;
    }

    tas_movie_controller_data_t movie_cdata;
    memcpy(&movie_cdata, loaded_tas_movie + loaded_tas_movie_index, sizeof(tas_movie_controller_data_t));

    loaded_tas_movie_index += sizeof(tas_movie_controller_data_t);

    n64_controller_t controller;
    memset(&controller, 0, sizeof(n64_controller_t));

    controller.c_right = movie_cdata.c_right;
    controller.c_left = movie_cdata.c_left;
    controller.c_down = movie_cdata.c_down;
    controller.c_up = movie_cdata.c_up;
    controller.r = movie_cdata.r;
    controller.l = movie_cdata.l;

    controller.dp_right = movie_cdata.dpad_right;
    controller.dp_left = movie_cdata.dpad_left;
    controller.dp_down = movie_cdata.dpad_down;
    controller.dp_up = movie_cdata.dpad_up;

    controller.z = movie_cdata.z;
    controller.b = movie_cdata.b;
    controller.a = movie_cdata.a;
    controller.start = movie_cdata.start;

    controller.joy_x = movie_cdata.analog_x;
    controller.joy_y = movie_cdata.analog_y;

    return controller;
}
