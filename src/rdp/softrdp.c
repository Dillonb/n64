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
    memcpy(&rdp->rdram[address ^ 2], &value, sizeof(half));
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

typedef struct edge_coefficients {
    bool dir;
    word level;
    word tile;

    // Y position where the triangle starts
    word yh;
    // Y position where the second minor line starts
    word ym;
    // Y position where the triangle ends.
    word yl;

    // X position where the major line starts
    word xh_i;
    word xh_f;
    // X position where the first minor line starts.
    word xm_i;
    word xm_f;
    // X position where the second minor line starts
    word xl_i;
    word xl_f;

    // Change in X per Y along the major line
    shalf dxhdy_i;
    half  dxhdy_f;
    // Change in X per Y along the first minor line
    shalf dxmdy_i;
    half  dxmdy_f;
    // Change in X per Y along the second minor line
    shalf dxldy_i;
    half  dxldy_f;
} edge_coefficients_t;

typedef struct span {
    int start;
    int end;
} span_t;

typedef struct spans {
    int start_y;
    int num_spans;
    span_t spans[1024];
} spans_t;

INLINE void get_edge_coefficients(const word* buffer, word command_length, edge_coefficients_t* coefficients) {
    coefficients->dir   = BIT(55 + 64 * 3);
    coefficients->level = BITS(53 + 64 * 3, 51 + 64 * 3);
    coefficients->tile  = BITS(50 + 64 * 3, 48 + 64 * 3);

    // Y position where the triangle starts
    coefficients->yh = BITS(13 + 64 * 3, 0  + 64 * 3);
    // Y position where the second minor line starts
    coefficients->ym = BITS(29 + 64 * 3, 16 + 64 * 3);
    // Y position where the triangle ends.
    coefficients->yl = BITS(45 + 64 * 3, 32 + 64 * 3);

    // X position where the major line starts
    coefficients->xh_i    = BITS(63 + 64 * 1, 48 + 64 * 1);
    coefficients->xh_f    = BITS(47 + 64 * 1, 32 + 64 * 1);
    // X position where the first minor line starts.
    coefficients->xm_i    = BITS(63 + 64 * 0, 48 + 64 * 0);
    coefficients->xm_f    = BITS(47 + 64 * 0, 32 + 64 * 0);
    // X position where the second minor line starts
    coefficients->xl_i = BITS(63 + 64 * 2, 48 + 64 * 2);
    coefficients->xl_f = BITS(47 + 64 * 2, 32 + 64 * 2);


    // Change in X per Y along the major line
    coefficients->dxhdy_i = BITS(31 + 64 * 1, 16 + 64 * 1);
    coefficients->dxhdy_f = BITS(15 + 64 * 1, 0  + 64 * 1);
    // Change in X per Y along the first minor line
    coefficients->dxmdy_i = BITS(31 + 64 * 0, 16 + 64 * 0);
    coefficients->dxmdy_f = BITS(15 + 64 * 0, 0  + 64 * 0);
    // Change in X per Y along the second minor line
    coefficients->dxldy_i = BITS(31 + 64 * 2, 16 + 64 * 2);
    coefficients->dxldy_f = BITS(15 + 64 * 2, 0  + 64 * 2);
}

INLINE void triangle_edgewalker(softrdp_state_t* rdp, edge_coefficients_t* ec, spans_t* spans) {
    word xstart;
    word xend;

    sword dxstart;
    sword dxend;

    if (ec->dir) {
        xstart = ec->xm_i << 16 | ec->xm_f;
        xend = ec->xh_i << 16 | ec->xh_f;

        dxstart = ec->dxmdy_i << 16 | ec->dxmdy_f;
        dxend = ec->dxhdy_i << 16 | ec->dxhdy_f;
    } else {
        xstart = ec->xh_i << 16 | ec->xh_f;
        xend = ec->xm_i << 16 | ec->xm_f;

        dxstart = ec->dxhdy_i << 16 | ec->dxhdy_f;
        dxend = ec->dxmdy_i << 16 | ec->dxmdy_f;
    }

    int bytes_per_pixel = get_bytes_per_pixel(rdp);

    int yh = ec->yh / 4;
    int ym = ec->ym / 4;
    int yl = ec->yl / 4;

    logalways("Edgewalking triangle yh %d ym %d yl %d", yh, ym, yl);

    spans->start_y = yh;

    int span_index = 0;

    for (int y = yh; y < ym; y += 1) {
        span_t* s = &spans->spans[span_index++];

        s->start = ((ec->dir ? xend : xstart) >> 16);
        s->end = ((ec->dir ? xstart : xend) >> 16);

        xstart += dxstart;
        xend += dxend;
    }

    if (ec->dir) {
        xstart = ec->xl_i << 16 | ec->xl_f;
        dxstart = ec->dxldy_i << 16 | ec->dxldy_f;
    } else {
        xend = ec->xl_i << 16 | ec->xl_f;
        dxend = ec->dxldy_i << 16 | ec->dxldy_f;
    }

    for (int y = ym; y < yl; y += 1) {
        span_t* s = &spans->spans[span_index++];

        s->start = ((ec->dir ? xend : xstart) >> 16);
        s->end = ((ec->dir ? xstart : xend) >> 16);

        xstart += dxstart;
        xend += dxend;
    }

    spans->num_spans = span_index;
}

typedef struct z_coefficients {
    // Inverse depth
    shalf z;
    half z_f;

    // Change in Z per change in X coordinate
    shalf dzdx;
    half dzdx_f;

    // Change in Z along major edge
    shalf dzde;
    half dzde_f;

    // Change in Z per change in Y coordinate
    shalf dzdy;
    half dzdy_f;
} z_coefficients_t;

INLINE void get_zbuffer_coefficients(const word* buffer, word command_length, z_coefficients_t* coefficients) {
    coefficients->z   = BITS(63 + 64, 48 + 64);
    coefficients->z_f = BITS(47 + 64, 32 + 64);

    coefficients->dzdx   = BITS(31 + 64, 16 + 64);
    coefficients->dzdx_f = BITS(15 + 64,  0 + 64);

    coefficients->dzde   = BITS(63 +  0, 48 +  0);
    coefficients->dzde_f = BITS(47 +  0, 32 +  0);

    coefficients->dzdy   = BITS(31 +  0, 16 +  0);
    coefficients->dzdy_f = BITS(15 +  0,  0 +  0);


    logalways("Z coefficients: z: %d.%d, dzdx: %d.%d, dzde: %d.%d, dzdy: %d.%d",
              coefficients->z, coefficients->z_f,
              coefficients->dzdx, coefficients->dzdx_f,
              coefficients->dzde, coefficients->dzde_f,
              coefficients->dzdy, coefficients->dzdy_f);
}


DEF_RDP_COMMAND(fill_triangle) {
    static edge_coefficients_t ec;
    get_edge_coefficients(buffer, command_length, &ec);
    static spans_t spans;
    triangle_edgewalker(rdp, &ec, &spans);

    logalways("Filling triangle starting at %d, %d. from y=%d to y=%d (pixel %d to %d?)", ec.yh, ec.xh_i, ec.yh, ec.yl, ec.yh / 2, ec.yl / 2);

    int bytes_per_pixel = get_bytes_per_pixel(rdp);

    switch (bytes_per_pixel) {
        case 2: {
            // Select color
            half color;
            switch (rdp->other_modes.cycle_type) {
                case 0:
                    // hack, this is incorrect. Just use the blend color
                    color = (rdp->blend_color.r >> 3) << 11 | (rdp->blend_color.g >> 3) << 6 | (rdp->blend_color.b >> 3) << 1 | 1;
                    break;
                case 3:
                    color = rdp->fill_color;
                    break;
            }

            // Fill in each span
            for (int i = 0; i < spans.num_spans; i++) {
                int y = spans.start_y + i;
                span_t* s = &spans.spans[i];

                word yofs = rdp->color_image.dram_addr + y * rdp->color_image.width * bytes_per_pixel;

                for (int pixel = s->start; pixel < s->end; pixel++) {
                    word addr = yofs + pixel * bytes_per_pixel;
                    rdram_write16(rdp, addr, color);
                }
            }
            break;
        }
        case 4: {
            for (int i = 0; i < spans.num_spans; i++) {
                int y = spans.start_y + i;
                span_t* s = &spans.spans[i];

                word yofs = rdp->color_image.dram_addr + y * rdp->color_image.width * bytes_per_pixel;

                for (int pixel = s->start; pixel < s->end; pixel++) {
                    word addr = yofs + pixel * bytes_per_pixel;
                    rdram_write32(rdp, addr, rdp->fill_color);
                }
            }
            break;
        }
        default:
            logfatal("Unsupported fill triangle bpp %d", bytes_per_pixel);
    }
}

DEF_RDP_COMMAND(fill_zbuffer_triangle) {
    static edge_coefficients_t ec;
    get_edge_coefficients(buffer, 8, &ec);

    static z_coefficients_t zc;
    get_zbuffer_coefficients(buffer + 8, 4, &zc);

    static spans_t spans;
    triangle_edgewalker(rdp, &ec, &spans);
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
    // Coordinates are in a 10.2 fixed point format, just discard the decimal places
    int xl = BITS(55, 44) >> 2;
    int yl = BITS(43, 32) >> 2;

    int xh = BITS(23, 12) >> 2;
    int yh = BITS(11, 0) >> 2;

    logalways("Fill rectangle (%d, %d) (%d, %d) with color %08X", xh, yh, xl, yl, rdp->fill_color);

    unimplemented(rdp->color_image.format != 0, "Fill rect when color image format not RGBA (this may just work?)");

    int bytes_per_pixel = get_bytes_per_pixel(rdp);

    int x_start = xh * bytes_per_pixel;
    int x_end = (xl + 1) * bytes_per_pixel;

    int stride = rdp->color_image.width * bytes_per_pixel;

    for (int y = yh; y < yl; y++) {
        int yofs = y * stride;
        for (int x = x_start; x < x_end; x += 4) {
            word addr = rdp->color_image.dram_addr + yofs + x;
            // Endianness means we need to do this as two separate writes to support both 32bpp and 16bpp
            rdram_write16(rdp, addr + 0, rdp->fill_color >> 16);
            rdram_write16(rdp, addr + 2, rdp->fill_color >> 0);
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
    rdp->z_image = BITS(25, 0);
    logalways("Setting Zbuffer image to 0x%08X", rdp->z_image);
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

    logalways("Command %02X", command);

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
