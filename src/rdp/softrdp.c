#include <stdio.h>
#include <log.h>
#include <string.h>
#include "softrdp.h"

typedef uint8_t byte;
typedef uint16_t half;
typedef uint32_t word;
typedef uint64_t dword;

typedef int8_t sbyte;
typedef int16_t shalf;
typedef int32_t sword;
typedef int64_t sdword;

void init_softrdp(softrdp_state_t* state, byte* rdramptr) {
    state->rdram = rdramptr;
}

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
#define MASK_LEN(n) ((1 << (n)) - 1)
#define BITS(hi, lo) ((buffer[((command_length) - 1) - ((lo) / 32)] >> ((lo) & 0b11111)) & MASK_LEN((hi) - (lo) + 1))
#define BIT(index) (((buffer[((command_length) - 1) - ((index) / 32)] >> ((index) & 0b11111)) & 1) != 0)
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#define MAX(x, y) ((x) > (y) ? (x) : (y))

INLINE void rdram_write32(softrdp_state_t* rdp, word address, word value) {
    memcpy(&rdp->rdram[address], &value, sizeof(word));
}

INLINE void rdram_write16(softrdp_state_t* rdp, word address, half value) {
    memcpy(&rdp->rdram[address], &value, sizeof(half));
}

INLINE bool check_scissor(softrdp_state_t* rdp, int x, int y) {
    //return (x >= rdp->scissor.xl && x <= rdp->scissor.xh && y >= rdp->scissor.yl && y <= rdp->scissor.yh);
    return true;
}

INLINE int get_bytes_per_pixel(softrdp_state_t* rdp) {
    switch (rdp->color_image.size) {
        case 2: return 2;
        case 3: return 4;
        default:
            logfatal("unknown color image size %d", rdp->color_image.size);
    }
}

DEF_RDP_COMMAND(fill_triangle) {
    bool dir   = BIT(55 + 64 * 3);
    //word level = BITS(53 + 64 * 3, 51 + 64 * 3);
    //word tile  = BITS(50 + 64 * 3, 48 + 64 * 3);

    // Y position where the triangle starts
    word yh = BITS(13 + 64 * 3, 0  + 64 * 3);
    // Y position where the second minor line starts
    word ym = BITS(29 + 64 * 3, 16 + 64 * 3);
    // Y position where the triangle ends.
    word yl = BITS(45 + 64 * 3, 32 + 64 * 3);

    // X position where the major line starts
    word xh_i    = BITS(63 + 64 * 1, 48 + 64 * 1);
    word xh_f    = BITS(47 + 64 * 1, 32 + 64 * 1);
    // X position where the first minor line starts.
    word xm_i    = BITS(63 + 64 * 0, 48 + 64 * 0);
    word xm_f    = BITS(47 + 64 * 0, 32 + 64 * 0);
    // X position where the second minor line starts
    word xl_i = BITS(63 + 64 * 2, 48 + 64 * 2);
    word xl_f = BITS(47 + 64 * 2, 32 + 64 * 2);

    logalways("Filling triangle starting at %d, %d", yh, xh_i);

    // Change in X per Y along the major line
    shalf dxhdy_i = BITS(31 + 64 * 1, 16 + 64 * 1);
    half  dxhdy_f = BITS(15 + 64 * 1, 0  + 64 * 1);
    // Change in X per Y along the first minor line
    shalf dxmdy_i = BITS(31 + 64 * 0, 16 + 64 * 0);
    half  dxmdy_f = BITS(15 + 64 * 0, 0  + 64 * 0);
    // Change in X per Y along the second minor line
    shalf dxldy_i = BITS(31 + 64 * 2, 16 + 64 * 2);
    half  dxldy_f = BITS(15 + 64 * 2, 0  + 64 * 2);

    word xstart;
    word xend;

    sword dxstart;
    sword dxend;

    if (dir) {
        xstart = xm_i << 16 | xm_f;
        xend = xh_i << 16 | xh_f;

        dxstart = dxmdy_i << 16 | dxmdy_f;
        dxend = dxhdy_i << 16 | dxhdy_f;
    } else {
        xstart = xh_i << 16 | xh_f;
        xend = xm_i << 16 | xm_f;

        dxstart = dxhdy_i << 16 | dxhdy_f;
        dxend = dxmdy_i << 16 | dxmdy_f;
    }

    int bytes_per_pixel = get_bytes_per_pixel(rdp);

    if (bytes_per_pixel == 2) {
        yh /= 2;
        ym /= 2;
        yl /= 2;
    }

    // Top half of triangle (between YH and YM)
    for (int y = yh; y < ym; y += bytes_per_pixel) {
        int yofs = (y * rdp->color_image.width);

        int xmin = ((dir ? xend : xstart) >> 16) * bytes_per_pixel;
        int xmax = ((dir ? xstart : xend) >> 16) * bytes_per_pixel;

        for (int x = xmin; x < xmax; x += bytes_per_pixel) {
            word address = rdp->color_image.dram_addr + yofs + x;
            if (bytes_per_pixel == 4) {
                rdram_write32(rdp, address, rdp->fill_color);
            } else if (bytes_per_pixel == 2) {
                rdram_write16(rdp, address ^ 2, rdp->fill_color);
            }
        }
        xstart += dxstart;
        xend += dxend;
    }

    if (dir) {
        xstart = xl_i << 16 | xl_f;
        dxstart = dxldy_i << 16 | dxldy_f;
    } else {
        xend = xl_i << 16 | xl_f;
        dxend = dxldy_i << 16 | dxldy_f;
    }

    // Bottom half of triangle (between YM and YL)
    for (int y = ym; y < yl; y += bytes_per_pixel) {
        int yofs = (y * rdp->color_image.width);

        int xmin = ((dir ? xend : xstart) >> 16) * bytes_per_pixel;
        int xmax = ((dir ? xstart : xend) >> 16) * bytes_per_pixel;

        for (int x = xmin; x < xmax; x += bytes_per_pixel) {
            word address = rdp->color_image.dram_addr + yofs + x;
            if (bytes_per_pixel == 4) {
                unimplemented(rdp->other_modes.cycle_type != 3, "Fill triangle 32bpp not in fill mode");
                rdram_write32(rdp, address, rdp->fill_color);
            } else if (bytes_per_pixel == 2) {
                unimplemented(rdp->other_modes.cycle_type != 3, "Fill triangle 16bpp not in fill mode");
                rdram_write16(rdp, address ^ 2, rdp->fill_color);
            }
        }
        xstart += dxstart;
        xend += dxend;
    }
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
    logwarn("rdp_sync_pipe unimplemented");
}

DEF_RDP_COMMAND(sync_tile) {
    logfatal("rdp_sync_tile unimplemented");
}

DEF_RDP_COMMAND(sync_full) {
    logwarn("rdp_sync_full unimplemented");
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
    rdp->scissor.yl = BITS(11, 0);
    rdp->scissor.xl = BITS(23, 12);

    rdp->scissor.yh = BITS(43, 32);
    rdp->scissor.xh = BITS(55, 44);

    rdp->scissor.f = BIT(25);
    rdp->scissor.o = BIT(24);
}

DEF_RDP_COMMAND(set_prim_depth) {
    rdp->primitive_z       = BITS(31, 16);
    rdp->primitive_delta_z = BITS(15, 0);
}

DEF_RDP_COMMAND(set_other_modes) {
    rdp->other_modes.atomic_prim = BIT(55);
    rdp->other_modes.cycle_type = BITS(53, 52);
    rdp->other_modes.persp_tex_en = BIT(51);
    rdp->other_modes.detail_tex_en = BIT(50);
    rdp->other_modes.sharpen_tex_en = BIT(49);
    rdp->other_modes.tex_lod_en = BIT(48);
    rdp->other_modes.en_tlut = BIT(47);
    rdp->other_modes.tlut_type = BIT(46);
    rdp->other_modes.sample_type = BIT(45);
    rdp->other_modes.mid_texel = BIT(44);
    rdp->other_modes.bi_lerp_0 = BIT(43);
    rdp->other_modes.bi_lerp_1 = BIT(42);
    rdp->other_modes.convert_one = BIT(41);
    rdp->other_modes.key_en = BIT(40);
    rdp->other_modes.rgb_dither_sel = BITS(39, 38);
    rdp->other_modes.alpha_dither_sel = BITS(37, 36);

    rdp->other_modes.b_m1a_0 = BITS(31, 30);
    rdp->other_modes.b_m1a_1 = BITS(29, 28);

    rdp->other_modes.b_m1b_0 = BITS(27, 26);
    rdp->other_modes.b_m1b_1 = BITS(25, 24);

    rdp->other_modes.b_m2a_0 = BITS(23, 22);
    rdp->other_modes.b_m2a_1 = BITS(21, 20);

    rdp->other_modes.b_m2b_0 = BITS(19, 18);
    rdp->other_modes.b_m2b_1 = BITS(17, 16);

    rdp->other_modes.force_blend = BIT(14);
    rdp->other_modes.alpha_cvg_select = BIT(13);
    rdp->other_modes.cvg_times_alpha = BIT(12);

    rdp->other_modes.z_mode = BITS(11, 10);
    rdp->other_modes.cvg_dest = BITS(8, 9);

    rdp->other_modes.color_on_cvg = BIT(7);
    rdp->other_modes.image_read_en = BIT(6);
    rdp->other_modes.z_update_en = BIT(5);
    rdp->other_modes.z_compare_en = BIT(4);
    rdp->other_modes.antialias_en = BIT(3);
    rdp->other_modes.z_source_sel = BIT(2);
    rdp->other_modes.dither_alpha_en = BIT(1);
    rdp->other_modes.alpha_compare_en = BIT(0);
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
    int tile_index = BITS(26, 24);

    half sl   = BITS(55, 44);
    half tl   = BITS(43, 32);
    half sh   = BITS(23, 12);
    half th   = BITS(11, 0);

    logfatal("rdp_load_tile unimplemented sl: %d tl: %d, tile: %d, sh: %d, th: %d", sl, tl, tile_index, sh, th);
}

DEF_RDP_COMMAND(set_tile) {
    int tile_index = BITS(26, 24);
    rdp->tiles[tile_index].format    = BITS(55, 53);
    rdp->tiles[tile_index].size      = BITS(52, 51);
    rdp->tiles[tile_index].line      = BITS(49, 41);
    rdp->tiles[tile_index].tmem_adrs = BITS(40, 32);
    rdp->tiles[tile_index].palette   = BITS(23, 20);
    rdp->tiles[tile_index].mt        = BIT(18);
    rdp->tiles[tile_index].mask_t    = BITS(17, 14);
    rdp->tiles[tile_index].shift_t   = BITS(13, 10);
    rdp->tiles[tile_index].cs        = BIT(9);
    rdp->tiles[tile_index].ms        = BIT(8);
    rdp->tiles[tile_index].mask_s    = BITS(7, 4);
    rdp->tiles[tile_index].shift_s   = BITS(3, 0);
}

DEF_RDP_COMMAND(fill_rectangle) {
    int xl = BITS(55, 44) / 4;
    int yl = BITS(43, 32) / 4;

    int xh = BITS(23, 12) / 4;
    int yh = BITS(11, 0) / 4;

    logalways("Fill rectangle (%d, %d) (%d, %d)", xh, yh, xl, yl);

    unimplemented(rdp->color_image.format != 0, "Fill rect when color image format not RGBA");

    int bytes_per_pixel = get_bytes_per_pixel(rdp);

    int y_range = (yl - yh) + 1;
    int x_range = (xl - xh) + 1;

    logalways("y range: %d x range: %d", y_range, x_range);

    for (int y = 0; y < y_range; y++) {
        int y_pixel = y + yh;
        int yofs = (y_pixel * bytes_per_pixel * rdp->color_image.width);
        for (int x = 0; x < x_range; x++) {
            int x_pixel = x + xh;
            int xofs = xh * bytes_per_pixel + (x * bytes_per_pixel);
            word addr = rdp->color_image.dram_addr + yofs + xofs;
            if (check_scissor(rdp, x_pixel, y_pixel)) {
                if (bytes_per_pixel == 4) {
                    unimplemented(rdp->other_modes.cycle_type != 3, "Fill rectangle 32bpp not in fill mode");
                    rdram_write32(rdp, addr, rdp->fill_color);
                } else if (bytes_per_pixel == 2) {
                    unimplemented(rdp->other_modes.cycle_type != 3, "Fill rectangle 16bpp not in fill mode");
                    rdram_write16(rdp, addr ^ 2, rdp->fill_color);
                }
            }
        }
    }

}

DEF_RDP_COMMAND(set_fill_color) {
    rdp->fill_color = buffer[1];
    logalways("Fill color: 0x%08X", buffer[1]);
}

DEF_RDP_COMMAND(set_fog_color) {
    logfatal("rdp_set_fog_color unimplemented");
}

DEF_RDP_COMMAND(set_blend_color) {
    rdp->blend_color.r = BITS(61, 56);
    rdp->blend_color.g = BITS(31, 24);
    rdp->blend_color.b = BITS(23, 16);
    rdp->blend_color.a = BITS(7, 0);

    logalways("Blend color: #%02X%02X%02X alpha %02X", rdp->blend_color.r, rdp->blend_color.g, rdp->blend_color.b, rdp->blend_color.a);
}

DEF_RDP_COMMAND(set_prim_color) {
    logfatal("rdp_set_prim_color unimplemented");
}

DEF_RDP_COMMAND(set_env_color) {
    logfatal("rdp_set_env_color unimplemented");
}

DEF_RDP_COMMAND(set_combine) {
    rdp->combine.sub_a_R_0 = BITS(55, 52);
    rdp->combine.mul_R_0   = BITS(51, 47);
    rdp->combine.sub_a_A_0 = BITS(46, 44);
    rdp->combine.mul_A_0   = BITS(43, 41);
    rdp->combine.sub_a_R_1 = BITS(40, 37);
    rdp->combine.mul_R_1   = BITS(36, 32);
    rdp->combine.sub_b_R_0 = BITS(31, 28);
    rdp->combine.sub_b_R_1 = BITS(27, 24);
    rdp->combine.sub_a_A_1 = BITS(23, 21);
    rdp->combine.mul_A_1   = BITS(20, 18);
    rdp->combine.add_R_0   = BITS(17, 15);
    rdp->combine.sub_b_A_0 = BITS(14, 12);
    rdp->combine.add_A_0   = BITS(11,  9);
    rdp->combine.add_R_1   = BITS(8,   6);
    rdp->combine.sub_b_A_1 = BITS(5,   3);
    rdp->combine.add_A_1   = BITS(2,   0);
}

DEF_RDP_COMMAND(set_texture_image) {
    rdp->texture_image.format = BITS(55, 53);
    rdp->texture_image.size   = BITS(52, 51);

    rdp->texture_image.width     = BITS(41, 32) + 1;
    rdp->texture_image.dram_addr = BITS(25, 0);
}

DEF_RDP_COMMAND(set_mask_image) {
    logfatal("rdp_set_mask_image unimplemented");
}

DEF_RDP_COMMAND(set_color_image) {
    logalways("Set color image:");
    rdp->color_image.format    = BITS(55, 53);
    rdp->color_image.size      = BITS(52, 51);
    rdp->color_image.width     = BITS(41, 32) + 1;
    rdp->color_image.dram_addr = BITS(25, 0);
    logalways("Format: %d", rdp->color_image.format);
    logalways("Size: %d",   rdp->color_image.size);
    logalways("Width: %d",  rdp->color_image.width);
    logalways("DRAM addr: 0x%08X", rdp->color_image.dram_addr);
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
