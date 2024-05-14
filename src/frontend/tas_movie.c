#include <string.h>
#include <stdio.h>
#include <log.h>
#include <system/n64system.h>
#include <stddef.h>
#include "tas_movie.h"

typedef struct m64_movie_controller_flags {
    unsigned controller_1_present:1;
    unsigned controller_2_present:1;
    unsigned controller_3_present:1;
    unsigned controller_4_present:1;

    unsigned controller_1_mempak:1;
    unsigned controller_2_mempak:1;
    unsigned controller_3_mempak:1;
    unsigned controller_4_mempak:1;

    unsigned controller_1_rumblepak:1;
    unsigned controller_2_rumblepak:1;
    unsigned controller_3_rumblepak:1;
    unsigned controller_4_rumblepak:1;

    unsigned:20;
} m64_movie_controller_flags_t;

_Static_assert(sizeof(m64_movie_controller_flags_t) == 4, "Incorrect size!");

typedef struct m64_movie_header {
    u8 signature[4];
    u32 version;
    u32 uid;
    u32 num_frames;
    u32 rerecord_count;
    u8 fps;
    u8 num_controllers;
    u8 reserved1;
    u8 reserved2;
    u32 num_input_samples;
    /*
     value 1: movie begins from snapshot (the snapshot will be loaded from an externalfile
                                          with the movie filename and a .st extension)
     value 2: movie begins from power-on
     other values: invalid movie
     */
    u16 start_type;
    u8 reserved3;
    u8 reserved4;
    m64_movie_controller_flags_t controller_flags;
    u8 reserved5[160];
    char rom_name[32];
    u32 rom_crc32;
    u16 rom_country_code;
    u8 reserved6[56];
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
} PACKED m64_movie_header_t;

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
        u8: 2;
        s8 analog_x: 8;
        s8 analog_y: 8;
    };
    u32 raw;
} PACKED tas_movie_controller_data_t;

_Static_assert(sizeof(tas_movie_controller_data_t) == 4, "Incorrect size for tas_movie_controller_data_t!");


_Static_assert(sizeof(m64_movie_header_t) == 1024, "Incorrect size!");

static u8* loaded_tas_movie = NULL;
static size_t loaded_tas_movie_size = 0;
m64_movie_header_t loaded_tas_movie_header;
uint32_t loaded_tas_movie_index = 0;

static u32 num_inputs_recorded = 0;
static FILE* recording_tas_movie = NULL;

void load_tas_movie(const char* filename) {
    FILE *fp = fopen(filename, "rb");

    if (fp == NULL) {
        logfatal("Error opening the movie file! Are you sure it's a valid movie and that it exists?");
    }

    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);

    fseek(fp, 0, SEEK_SET);
    u8 *buf = malloc(size);
    checked_fread(buf, size, 1, fp);

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

void start_tas_recording(const char* movie_path) {
    if (n64sys.mem.rom.rom == NULL) {
        logdie("Must load a ROM at launch when recording a TAS movie.");
    }

    {
        FILE* fp = fopen(movie_path, "rb");
        if (fp != NULL) {
            fclose(fp);
            logfatal("The movie file already exists, refusing to overwrite!");
        }
    }

    recording_tas_movie = fopen(movie_path, "wb");
    num_inputs_recorded = 0;
    if (recording_tas_movie == NULL) {
        logfatal("Error opening the movie file for writing!");
    }
    {
        m64_movie_header_t header;
        memset(&header, 0, sizeof(m64_movie_header_t));

        header.signature[0] = 0x4D;
        header.signature[1] = 0x36;
        header.signature[2] = 0x34;
        header.signature[3] = 0x1A;

        header.version = 3;
        header.uid = 0;
        header.num_frames = 0;
        header.rerecord_count = 0;
        header.fps = 30;
        header.num_controllers = 1;
        header.reserved1 = 0;
        header.reserved2 = 0;
        header.num_input_samples = num_inputs_recorded; // We'll fill this in later
        header.start_type = 2; // Begins from power-on
        header.reserved3 = 0;
        header.reserved4 = 0;
        header.controller_flags.controller_1_present = true;
        header.controller_flags.controller_1_mempak = true;
        strcpy((char*)header.reserved5, "");
        strcpy((char*)header.rom_name, n64sys.mem.rom.header.image_name);
        header.rom_crc32 = n64sys.mem.rom.header.crc1;
        header.rom_country_code = n64sys.mem.rom.header.country_code_int;
        strcpy((char*)header.reserved6, "");
        strcpy((char*)header.video_plugin_name, "dgb-n64");
        strcpy((char*)header.audio_plugin_name, "dgb-n64");
        strcpy((char*)header.input_plugin_name, "dgb-n64");
        strcpy((char*)header.rsp_plugin_name, "dgb-n64");
        strcpy((char*)header.author_name, "dgb-n64");
        strcpy((char*)header.movie_description, "dgb-n64 test demo");

        fwrite(&header, sizeof(m64_movie_header_t), 1, recording_tas_movie);
    }
}

bool tas_movie_recording() {
    return recording_tas_movie != NULL;
}

void tas_record_inputs(n64_controller_t* inputs) {
    tas_movie_controller_data_t controller_data;

    controller_data.c_right = inputs->c_right;
    controller_data.c_left = inputs->c_left;
    controller_data.c_down = inputs->c_down;
    controller_data.c_up = inputs->c_up;
    controller_data.r = inputs->r;
    controller_data.l = inputs->l;

    controller_data.dpad_right = inputs->dp_right;
    controller_data.dpad_left = inputs->dp_left;
    controller_data.dpad_down = inputs->dp_down;
    controller_data.dpad_up = inputs->dp_up;

    controller_data.z = inputs->z;
    controller_data.b = inputs->b;
    controller_data.a = inputs->a;
    controller_data.start = inputs->start;

    controller_data.analog_x = inputs->joy_x;
    controller_data.analog_y = inputs->joy_y;

    fseek(recording_tas_movie, 0, SEEK_END);
    fwrite(&controller_data, sizeof(tas_movie_controller_data_t), 1, recording_tas_movie);

    num_inputs_recorded++;

    fseek(recording_tas_movie, offsetof(m64_movie_header_t, num_input_samples), SEEK_SET);
    fwrite(&num_inputs_recorded, sizeof(num_inputs_recorded), 1, recording_tas_movie);

    fseek(recording_tas_movie, 0, SEEK_END);
}
