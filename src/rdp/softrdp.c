#include <stdio.h>
#include <log.h>
#include "softrdp.h"

typedef uint8_t byte;
typedef uint16_t half;
typedef uint32_t word;
typedef uint64_t dword;

typedef int8_t sbyte;
typedef int16_t shalf;
typedef int32_t sword;
typedef int64_t sdword;

void init_softrdp(softrdp_state_t* state) {}

typedef enum rdp_command {
    RDP_COMMAND_FILL_TRIANGLE = 0x08,
    RDP_COMMAND_FILL_ZBUFFER_TRIANGLE = 0x09,
    RDP_COMMAND_TEXTURE_TRIANGLE = 0x0a,
    RDP_COMMAND_TEXTURE_ZBUFFER_TRIANGLE = 0x0b,
    RDP_COMMAND_SHADE_TRIANGLE = 0x0c,
    RDP_COMMAND_SHADE_ZBUFFER_TRIANGLE = 0x0d,
    RDP_COMMAND_SHADE_TEXTURE_TRIANGLE = 0x0e,
    RDP_COMMAND_SHADE_TEXTURE_ZBUFFER_TRIANGLE = 0x0f,
    RDP_COMMAND_TEXTURE_RECTANGLE = 0x24,
    RDP_COMMAND_TEXTURE_RECTANGLE_FLIP = 0x25,
    RDP_COMMAND_SYNC_LOAD = 0x26,
    RDP_COMMAND_SYNC_PIPE = 0x27,
    RDP_COMMAND_SYNC_TILE = 0x28,
    RDP_COMMAND_SYNC_FULL = 0x29,
    RDP_COMMAND_SET_KEY_GB = 0x2a,
    RDP_COMMAND_SET_KEY_R = 0x2b,
    RDP_COMMAND_SET_CONVERT = 0x2c,
    RDP_COMMAND_SET_SCISSOR = 0x2d,
    RDP_COMMAND_SET_PRIM_DEPTH = 0x2e,
    RDP_COMMAND_SET_OTHER_MODES = 0x2f,
    RDP_COMMAND_LOAD_TLUT = 0x30,
    RDP_COMMAND_SET_TILE_SIZE = 0x32,
    RDP_COMMAND_LOAD_BLOCK = 0x33,
    RDP_COMMAND_LOAD_TILE = 0x34,
    RDP_COMMAND_SET_TILE = 0x35,
    RDP_COMMAND_FILL_RECTANGLE = 0x36,
    RDP_COMMAND_SET_FILL_COLOR = 0x37,
    RDP_COMMAND_SET_FOG_COLOR = 0x38,
    RDP_COMMAND_SET_BLEND_COLOR = 0x39,
    RDP_COMMAND_SET_PRIM_COLOR = 0x3a,
    RDP_COMMAND_SET_ENV_COLOR = 0x3b,
    RDP_COMMAND_SET_COMBINE = 0x3c,
    RDP_COMMAND_SET_TEXTURE_IMAGE = 0x3d,
    RDP_COMMAND_SET_MASK_IMAGE = 0x3e,
    RDP_COMMAND_SET_COLOR_IMAGE = 0x3f
} rdp_command_t;

#ifndef INLINE
#define INLINE static inline __attribute__((always_inline))
#endif

#define DEF_RDP_COMMAND(name) INLINE void rdp_command_##name(softrdp_state_t* rdp, int command_length, const word* buffer)
#define EXEC_RDP_COMMAND(name) rdp_command_##name(rdp, command_length, buffer); break


DEF_RDP_COMMAND(fill_triangle) {
    logfatal("rdp_fill_triangle unimplemented");
}

DEF_RDP_COMMAND(fill_zbuffer_triangle) {
    logfatal("rdp_fill_zbuffer_triangle unimplemented");
}

DEF_RDP_COMMAND(texture_triangle) {
    logfatal("rdp_texture_triangle unimplemented");
}

DEF_RDP_COMMAND(texture_zbuffer_triangle) {
    logfatal("rdp_texture_zbuffer_triangle unimplemented");
}

DEF_RDP_COMMAND(shade_triangle) {
    logfatal("rdp_shade_triangle unimplemented");
}

DEF_RDP_COMMAND(shade_zbuffer_triangle) {
    logfatal("rdp_shade_zbuffer_triangle unimplemented");
}

DEF_RDP_COMMAND(shade_texture_triangle) {
    logfatal("rdp_shade_texture_triangle unimplemented");
}

DEF_RDP_COMMAND(shade_texture_zbuffer_triangle) {
    logfatal("rdp_shade_texture_zbuffer_triangle unimplemented");
}

DEF_RDP_COMMAND(texture_rectangle) {
    logfatal("rdp_texture_rectangle unimplemented");
}

DEF_RDP_COMMAND(texture_rectangle_flip) {
    logfatal("rdp_texture_rectangle_flip unimplemented");
}

DEF_RDP_COMMAND(sync_load) {
    logfatal("rdp_sync_load unimplemented");
}

DEF_RDP_COMMAND(sync_pipe) {
    logfatal("rdp_sync_pipe unimplemented");
}

DEF_RDP_COMMAND(sync_tile) {
    logfatal("rdp_sync_tile unimplemented");
}

DEF_RDP_COMMAND(sync_full) {
    logfatal("rdp_sync_full unimplemented");
}

DEF_RDP_COMMAND(set_key_gb) {
    logfatal("rdp_set_key_gb unimplemented");
}

DEF_RDP_COMMAND(set_key_r) {
    logfatal("rdp_set_key_r unimplemented");
}

DEF_RDP_COMMAND(set_convert) {
    logfatal("rdp_set_convert unimplemented");
}

DEF_RDP_COMMAND(set_scissor) {
    rdp->scissor.yl = (buffer[1] >>  0) & 0xFFF;
    rdp->scissor.xl = (buffer[1] >> 12) & 0xFFF;

    rdp->scissor.yh = (buffer[0] >>  0) & 0xFFF;
    rdp->scissor.xh = (buffer[0] >> 12) & 0xFFF;

    rdp->scissor.f = (buffer[1] & (1 << 25)) != 0;
    rdp->scissor.o = (buffer[1] & (1 << 24)) != 0;
}

DEF_RDP_COMMAND(set_prim_depth) {
    rdp->primitive_delta_z = (buffer[1] >>  0) & 0xFFFF;
    rdp->primitive_z       = (buffer[1] >> 16) & 0xFFFF;
}

DEF_RDP_COMMAND(set_other_modes) {
    logwarn("rdp_set_other_modes unimplemented");
}

DEF_RDP_COMMAND(load_tlut) {
    logfatal("rdp_load_tlut unimplemented");
}

DEF_RDP_COMMAND(set_tile_size) {
    logfatal("rdp_set_tile_size unimplemented");
}

DEF_RDP_COMMAND(load_block) {
    logfatal("rdp_load_block unimplemented");
}

DEF_RDP_COMMAND(load_tile) {
    logfatal("rdp_load_tile unimplemented");
}

DEF_RDP_COMMAND(set_tile) {
    logfatal("rdp_set_tile unimplemented");
}

DEF_RDP_COMMAND(fill_rectangle) {
    logfatal("rdp_fill_rectangle unimplemented");
}

DEF_RDP_COMMAND(set_fill_color) {
    logfatal("rdp_set_fill_color unimplemented");
}

DEF_RDP_COMMAND(set_fog_color) {
    logfatal("rdp_set_fog_color unimplemented");
}

DEF_RDP_COMMAND(set_blend_color) {
    logfatal("rdp_set_blend_color unimplemented");
}

DEF_RDP_COMMAND(set_prim_color) {
    logfatal("rdp_set_prim_color unimplemented");
}

DEF_RDP_COMMAND(set_env_color) {
    logfatal("rdp_set_env_color unimplemented");
}

DEF_RDP_COMMAND(set_combine) {
    logfatal("rdp_set_combine unimplemented");
}

DEF_RDP_COMMAND(set_texture_image) {
    logfatal("rdp_set_texture_image unimplemented");
}

DEF_RDP_COMMAND(set_mask_image) {
    logfatal("rdp_set_mask_image unimplemented");
}

DEF_RDP_COMMAND(set_color_image) {
    logfatal("rdp_set_color_image unimplemented");
}

void enqueue_command_softrdp(softrdp_state_t* rdp, int command_length, word* buffer) {
    rdp_command_t command = (buffer[0] >> 24) & 0x3F;
    switch (command) {
        case RDP_COMMAND_FILL_TRIANGLE:                  EXEC_RDP_COMMAND(fill_triangle);
        case RDP_COMMAND_FILL_ZBUFFER_TRIANGLE:          EXEC_RDP_COMMAND(fill_zbuffer_triangle);
        case RDP_COMMAND_TEXTURE_TRIANGLE:               EXEC_RDP_COMMAND(texture_triangle);
        case RDP_COMMAND_TEXTURE_ZBUFFER_TRIANGLE:       EXEC_RDP_COMMAND(texture_zbuffer_triangle);
        case RDP_COMMAND_SHADE_TRIANGLE:                 EXEC_RDP_COMMAND(shade_triangle);
        case RDP_COMMAND_SHADE_ZBUFFER_TRIANGLE:         EXEC_RDP_COMMAND(shade_zbuffer_triangle);
        case RDP_COMMAND_SHADE_TEXTURE_TRIANGLE:         EXEC_RDP_COMMAND(shade_texture_triangle);
        case RDP_COMMAND_SHADE_TEXTURE_ZBUFFER_TRIANGLE: EXEC_RDP_COMMAND(shade_texture_zbuffer_triangle);
        case RDP_COMMAND_TEXTURE_RECTANGLE:              EXEC_RDP_COMMAND(texture_rectangle);
        case RDP_COMMAND_TEXTURE_RECTANGLE_FLIP:         EXEC_RDP_COMMAND(texture_rectangle_flip);
        case RDP_COMMAND_SYNC_LOAD:                      EXEC_RDP_COMMAND(sync_load);
        case RDP_COMMAND_SYNC_PIPE:                      EXEC_RDP_COMMAND(sync_pipe);
        case RDP_COMMAND_SYNC_TILE:                      EXEC_RDP_COMMAND(sync_tile);
        case RDP_COMMAND_SYNC_FULL:                      EXEC_RDP_COMMAND(sync_full);
        case RDP_COMMAND_SET_KEY_GB:                     EXEC_RDP_COMMAND(set_key_gb);
        case RDP_COMMAND_SET_KEY_R:                      EXEC_RDP_COMMAND(set_key_r);
        case RDP_COMMAND_SET_CONVERT:                    EXEC_RDP_COMMAND(set_convert);
        case RDP_COMMAND_SET_SCISSOR:                    EXEC_RDP_COMMAND(set_scissor);
        case RDP_COMMAND_SET_PRIM_DEPTH:                 EXEC_RDP_COMMAND(set_prim_depth);
        case RDP_COMMAND_SET_OTHER_MODES:                EXEC_RDP_COMMAND(set_other_modes);
        case RDP_COMMAND_LOAD_TLUT:                      EXEC_RDP_COMMAND(load_tlut);
        case RDP_COMMAND_SET_TILE_SIZE:                  EXEC_RDP_COMMAND(set_tile_size);
        case RDP_COMMAND_LOAD_BLOCK:                     EXEC_RDP_COMMAND(load_block);
        case RDP_COMMAND_LOAD_TILE:                      EXEC_RDP_COMMAND(load_tile);
        case RDP_COMMAND_SET_TILE:                       EXEC_RDP_COMMAND(set_tile);
        case RDP_COMMAND_FILL_RECTANGLE:                 EXEC_RDP_COMMAND(fill_rectangle);
        case RDP_COMMAND_SET_FILL_COLOR:                 EXEC_RDP_COMMAND(set_fill_color);
        case RDP_COMMAND_SET_FOG_COLOR:                  EXEC_RDP_COMMAND(set_fog_color);
        case RDP_COMMAND_SET_BLEND_COLOR:                EXEC_RDP_COMMAND(set_blend_color);
        case RDP_COMMAND_SET_PRIM_COLOR:                 EXEC_RDP_COMMAND(set_prim_color);
        case RDP_COMMAND_SET_ENV_COLOR:                  EXEC_RDP_COMMAND(set_env_color);
        case RDP_COMMAND_SET_COMBINE:                    EXEC_RDP_COMMAND(set_combine);
        case RDP_COMMAND_SET_TEXTURE_IMAGE:              EXEC_RDP_COMMAND(set_texture_image);
        case RDP_COMMAND_SET_MASK_IMAGE:                 EXEC_RDP_COMMAND(set_mask_image);
        case RDP_COMMAND_SET_COLOR_IMAGE:                EXEC_RDP_COMMAND(set_color_image);

        default: logfatal("Unknown RDP command: %02X", command);
    }
}
