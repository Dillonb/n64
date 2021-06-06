#include <cstdio>
#include <log.h>
#include <cstring>
#include "softrdp.h"

#ifndef INLINE
#define INLINE static inline __attribute__((always_inline))
#endif

#define EXEC_RDP_COMMAND(name) rdp_command_##name(rdp, command_length, buffer); break
#define DEF_RDP_COMMAND(name) INLINE void rdp_command_##name(softrdp_state_t* rdp, int command_length, const uint64_t* buffer)

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

typedef struct edge_coefficients {
    bool dir;
    uint32_t level;
    uint32_t tile;

    // Y position where the triangle starts
    uint32_t yh;
    // Y position where the second minor line starts
    uint32_t ym;
    // Y position where the triangle ends.
    uint32_t yl;

    // X position where the major line starts
    uint32_t xh_i;
    uint32_t xh_f;
    // X position where the first minor line starts.
    uint32_t xm_i;
    uint32_t xm_f;
    // X position where the second minor line starts
    uint32_t xl_i;
    uint32_t xl_f;

    // Change in X per Y along the major line
    int16_t dxhdy_i;
    uint16_t  dxhdy_f;
    // Change in X per Y along the first minor line
    int16_t dxmdy_i;
    uint16_t  dxmdy_f;
    // Change in X per Y along the second minor line
    int16_t dxldy_i;
    uint16_t  dxldy_f;
} edge_coefficients_t;

typedef struct z_coefficients {
    // Inverse depth
    int16_t z;
    uint16_t z_f;

    // Change in Z per change in X coordinate
    int16_t dzdx;
    uint16_t dzdx_f;

    // Change in Z along major edge
    int16_t dzde;
    uint16_t dzde_f;

    // Change in Z per change in Y coordinate
    int16_t dzdy;
    uint16_t dzdy_f;
} z_coefficients_t;

typedef struct span {
    int start;
    int end;
} span_t;

typedef struct spans {
    int start_y;
    int num_spans;
    span_t spans[1024];
} spans_t;

constexpr bool get_bit(uint64_t cmd, int bit) {
    return (cmd >> bit) & 1;
}

constexpr uint64_t get_mask(int len) {
    return (1ULL << (len + 1)) - 1;
}

constexpr uint64_t get_bits(uint64_t cmd, int hi, int lo) {
    return (cmd >> lo) & get_mask(hi - lo);
}

INLINE int get_bytes_per_pixel(softrdp_state_t* rdp) {
    switch (rdp->color_image.size) {
        case 2: return 2;
        case 3: return 4;
        default:
            logfatal("unknown color image size %d", rdp->color_image.size);
    }
}

INLINE void get_edge_coefficients(const uint64_t* buffer, edge_coefficients_t* coefficients) {
    coefficients->dir   = get_bit(buffer[0], 55);
    coefficients->level = get_bits(buffer[0], 53, 51);
    coefficients->tile  = get_bits(buffer[0], 50, 48);

    // Y position where the triangle starts
    coefficients->yh = get_bits(buffer[0], 13, 0);
    // Y position where the second minor line starts
    coefficients->ym = get_bits(buffer[0], 29, 16);
    // Y position where the triangle ends.
    coefficients->yl = get_bits(buffer[0], 45, 32);

    // X position where the major line starts
    coefficients->xh_i    = get_bits(buffer[2], 63, 48);
    coefficients->xh_f    = get_bits(buffer[2], 47, 32);
    // X position where the first minor line starts.
    coefficients->xm_i    = get_bits(buffer[3], 63, 48);
    coefficients->xm_f    = get_bits(buffer[3], 47, 32);
    // X position where the second minor line starts
    coefficients->xl_i = get_bits(buffer[1], 63, 48);
    coefficients->xl_f = get_bits(buffer[1], 47, 32);


    // Change in X per Y along the major line
    coefficients->dxhdy_i = get_bits(buffer[2], 31, 16);
    coefficients->dxhdy_f = get_bits(buffer[2], 15, 0);
    // Change in X per Y along the first minor line
    coefficients->dxmdy_i = get_bits(buffer[3], 31, 16);
    coefficients->dxmdy_f = get_bits(buffer[3], 15, 0);
    // Change in X per Y along the second minor line
    coefficients->dxldy_i = get_bits(buffer[1], 31, 16);
    coefficients->dxldy_f = get_bits(buffer[1], 15, 0);
}


INLINE void get_zbuffer_coefficients(const uint64_t* buffer, z_coefficients_t* coefficients) {
    coefficients->z   = get_bits(buffer[0], 63, 48);
    coefficients->z_f = get_bits(buffer[0], 47, 32);

    coefficients->dzdx   = get_bits(buffer[0], 31, 16);
    coefficients->dzdx_f = get_bits(buffer[0], 15, 0);

    coefficients->dzde   = get_bits(buffer[1], 63, 48);
    coefficients->dzde_f = get_bits(buffer[1], 47, 32);

    coefficients->dzdy   = get_bits(buffer[1], 31, 16);
    coefficients->dzdy_f = get_bits(buffer[1], 15, 0);


    logalways("Z coefficients: z: %d.%d, dzdx: %d.%d, dzde: %d.%d, dzdy: %d.%d",
              coefficients->z, coefficients->z_f,
              coefficients->dzdx, coefficients->dzdx_f,
              coefficients->dzde, coefficients->dzde_f,
              coefficients->dzdy, coefficients->dzdy_f);
}

INLINE void triangle_edgewalker(softrdp_state_t* rdp, edge_coefficients_t* ec, spans_t* spans) {
    uint32_t xstart;
    uint32_t xend;

    int32_t dxstart;
    int32_t dxend;

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

INLINE uint16_t fill_for_addr(softrdp_state_t* rdp, uint32_t addr) {
    // Code below is optimized to remove the conditional.
    // Essentially, the behavior is to use the upper bits of fill_color if we're on an even column,
    // and the lower bits if we're on an odd column.

    // return rdp->fill_color >> (addr % 4 == 0 ? 16 : 0);
    switch (rdp->other_modes.cycle_type) {
        case 3:
            return rdp->fill_color >> (16 - ((addr % 4) * 8));
        default:
            logfatal("fill_for_addr(): unknown cycle type %d", rdp->other_modes.cycle_type);
    }
}

INLINE void rdram_write16(softrdp_state_t* rdp, uint32_t address, uint16_t value) {
    memcpy(&rdp->rdram[address ^ 2], &value, sizeof(uint16_t));
}

void init_softrdp(softrdp_state_t* state, uint8_t* rdramptr) {
    state->rdram = rdramptr;
}

DEF_RDP_COMMAND(fill_triangle) {
    static edge_coefficients_t ec;
    get_edge_coefficients(buffer, &ec);
    static spans_t spans;
    triangle_edgewalker(rdp, &ec, &spans);

    int bytes_per_pixel = get_bytes_per_pixel(rdp);

    for (int i = 0; i < spans.num_spans; i++) {
        int y = spans.start_y + i;
        span_t* s = &spans.spans[i];

        uint32_t yofs = rdp->color_image.dram_addr + y * rdp->color_image.width * bytes_per_pixel;

        for (int x = s->start * bytes_per_pixel; x < s->end * bytes_per_pixel; x += 2) {
            uint32_t addr = yofs + x;
            rdram_write16(rdp, addr, fill_for_addr(rdp, addr));
        }
    }
}

DEF_RDP_COMMAND(fill_zbuffer_triangle) {
    static edge_coefficients_t ec;
    get_edge_coefficients(buffer, &ec);

    static z_coefficients_t zc;
    get_zbuffer_coefficients(&buffer[4], &zc);

    static spans_t spans;
    triangle_edgewalker(rdp, &ec, &spans);
    logfatal("fill_zbuffer_triangle unimplemented");
}

DEF_RDP_COMMAND(texture_triangle) {
    static edge_coefficients_t ec;
    get_edge_coefficients(buffer, &ec);
    logfatal("texture_triangle unimplemented");
}

DEF_RDP_COMMAND(texture_zbuffer_triangle) {
    static edge_coefficients_t ec;
    get_edge_coefficients(buffer, &ec);
    logfatal("texture_zbuffer_triangle unimplemented");
}

DEF_RDP_COMMAND(shade_triangle) {
    static edge_coefficients_t ec;
    get_edge_coefficients(buffer, &ec);
    logfatal("shade_triangle unimplemented");
}

DEF_RDP_COMMAND(shade_zbuffer_triangle) {
    static edge_coefficients_t ec;
    get_edge_coefficients(buffer, &ec);
    logfatal("shade_zbuffer_triangle unimplemented");
}

DEF_RDP_COMMAND(shade_texture_triangle) {
    static edge_coefficients_t ec;
    get_edge_coefficients(buffer, &ec);
    logfatal("shade_texture_triangle unimplemented");
}

DEF_RDP_COMMAND(shade_texture_zbuffer_triangle) {
    static edge_coefficients_t ec;
    get_edge_coefficients(buffer, &ec);
    logfatal("shade_texture_zbuffer_triangle unimplemented");
}

DEF_RDP_COMMAND(texture_rectangle) {
    logfatal("texture_rectangle unimplemented");
}

DEF_RDP_COMMAND(texture_rectangle_flip) {
    logfatal("texture_rectangle_flip unimplemented");
}

DEF_RDP_COMMAND(sync_load) {
    logfatal("sync_load unimplemented");
}

DEF_RDP_COMMAND(sync_pipe) {
    //logfatal("sync_pipe unimplemented");
}

DEF_RDP_COMMAND(sync_tile) {
    logfatal("sync_tile unimplemented");
}

DEF_RDP_COMMAND(sync_full) {
    //logfatal("sync_full unimplemented");
}

DEF_RDP_COMMAND(set_key_gb) {
    logfatal("set_key_gb unimplemented");
}

DEF_RDP_COMMAND(set_key_r) {
    logfatal("set_key_r unimplemented");
}

DEF_RDP_COMMAND(set_convert) {
    logfatal("set_convert unimplemented");
}

DEF_RDP_COMMAND(set_scissor) {
    rdp->scissor.yl = get_bits(buffer[0], 11, 0);
    rdp->scissor.xl = get_bits(buffer[0], 23, 12);

    rdp->scissor.yh = get_bits(buffer[0], 43, 32);
    rdp->scissor.xh = get_bits(buffer[0], 55, 44);

    rdp->scissor.f = get_bit(buffer[0], 25);
    rdp->scissor.o = get_bit(buffer[0], 24);
}

DEF_RDP_COMMAND(set_prim_depth) {
    rdp->primitive_z       = get_bits(buffer[0], 31, 16);
    rdp->primitive_delta_z = get_bits(buffer[0], 15, 0);
}

DEF_RDP_COMMAND(set_other_modes) {
    rdp->other_modes.atomic_prim      = get_bit(buffer[0], 55);
    rdp->other_modes.cycle_type       = get_bits(buffer[0], 53, 52);
    rdp->other_modes.persp_tex_en     = get_bit(buffer[0], 51);
    rdp->other_modes.detail_tex_en    = get_bit(buffer[0], 50);
    rdp->other_modes.sharpen_tex_en   = get_bit(buffer[0], 49);
    rdp->other_modes.tex_lod_en       = get_bit(buffer[0], 48);
    rdp->other_modes.en_tlut          = get_bit(buffer[0], 47);
    rdp->other_modes.tlut_type        = get_bit(buffer[0], 46);
    rdp->other_modes.sample_type      = get_bit(buffer[0], 45);
    rdp->other_modes.mid_texel        = get_bit(buffer[0], 44);
    rdp->other_modes.bi_lerp_0        = get_bit(buffer[0], 43);
    rdp->other_modes.bi_lerp_1        = get_bit(buffer[0], 42);
    rdp->other_modes.convert_one      = get_bit(buffer[0], 41);
    rdp->other_modes.key_en           = get_bit(buffer[0], 40);
    rdp->other_modes.rgb_dither_sel   = get_bits(buffer[0], 39, 38);
    rdp->other_modes.alpha_dither_sel = get_bits(buffer[0], 37, 36);

    rdp->other_modes.b_m1a_0 = get_bits(buffer[0], 31, 30);
    rdp->other_modes.b_m1a_1 = get_bits(buffer[0], 29, 28);

    rdp->other_modes.b_m1b_0 = get_bits(buffer[0], 27, 26);
    rdp->other_modes.b_m1b_1 = get_bits(buffer[0], 25, 24);

    rdp->other_modes.b_m2a_0 = get_bits(buffer[0], 23, 22);
    rdp->other_modes.b_m2a_1 = get_bits(buffer[0], 21, 20);

    rdp->other_modes.b_m2b_0 = get_bits(buffer[0], 19, 18);
    rdp->other_modes.b_m2b_1 = get_bits(buffer[0], 17, 16);

    rdp->other_modes.force_blend      = get_bit(buffer[0], 14);
    rdp->other_modes.alpha_cvg_select = get_bit(buffer[0], 13);
    rdp->other_modes.cvg_times_alpha  = get_bit(buffer[0], 12);

    rdp->other_modes.z_mode   = get_bits(buffer[0], 11, 10);
    rdp->other_modes.cvg_dest = get_bits(buffer[0], 8, 9);

    rdp->other_modes.color_on_cvg     = get_bit(buffer[0], 7);
    rdp->other_modes.image_read_en    = get_bit(buffer[0], 6);
    rdp->other_modes.z_update_en      = get_bit(buffer[0], 5);
    rdp->other_modes.z_compare_en     = get_bit(buffer[0], 4);
    rdp->other_modes.antialias_en     = get_bit(buffer[0], 3);
    rdp->other_modes.z_source_sel     = get_bit(buffer[0], 2);
    rdp->other_modes.dither_alpha_en  = get_bit(buffer[0], 1);
    rdp->other_modes.alpha_compare_en = get_bit(buffer[0], 0);
}

DEF_RDP_COMMAND(load_tlut) {
    logfatal("load_tlut unimplemented");
}

DEF_RDP_COMMAND(set_tile_size) {
    logfatal("set_tile_size unimplemented");
}

DEF_RDP_COMMAND(load_block) {
    logfatal("load_block unimplemented");
}

DEF_RDP_COMMAND(load_tile) {
    int tile_index = get_bits(buffer[0], 26, 24);

    uint16_t sl   = get_bits(buffer[0], 55, 44);
    uint16_t tl   = get_bits(buffer[0], 43, 32);
    uint16_t sh   = get_bits(buffer[0], 23, 12);
    uint16_t th   = get_bits(buffer[0], 11, 0);

    logfatal("rdp_load_tile unimplemented sl: %d tl: %d, tile: %d, sh: %d, th: %d", sl, tl, tile_index, sh, th);
}

DEF_RDP_COMMAND(set_tile) {
    int tile_index = get_bits(buffer[0], 26, 24);
    rdp->tiles[tile_index].format    = get_bits(buffer[0], 55, 53);
    rdp->tiles[tile_index].size      = get_bits(buffer[0], 52, 51);
    rdp->tiles[tile_index].line      = get_bits(buffer[0], 49, 41);
    rdp->tiles[tile_index].tmem_adrs = get_bits(buffer[0], 40, 32);
    rdp->tiles[tile_index].palette   = get_bits(buffer[0], 23, 20);
    rdp->tiles[tile_index].mt        = get_bit(buffer[0], 18);
    rdp->tiles[tile_index].mask_t    = get_bits(buffer[0], 17, 14);
    rdp->tiles[tile_index].shift_t   = get_bits(buffer[0], 13, 10);
    rdp->tiles[tile_index].cs        = get_bit(buffer[0], 9);
    rdp->tiles[tile_index].ms        = get_bit(buffer[0], 8);
    rdp->tiles[tile_index].mask_s    = get_bits(buffer[0], 7, 4);
    rdp->tiles[tile_index].shift_s   = get_bits(buffer[0], 3, 0);
}

DEF_RDP_COMMAND(fill_rectangle) {
    // Coordinates are in a 10.2 fixed point format, just discard the decimal places
    int xl = get_bits(buffer[0], 55, 44) >> 2;
    int yl = get_bits(buffer[0], 43, 32) >> 2;

    int xh = get_bits(buffer[0], 23, 12) >> 2;
    int yh = get_bits(buffer[0], 11, 0) >> 2;
    logalways("Fill rectangle (%d, %d) (%d, %d) with color %08X", xh, yh, xl, yl, rdp->fill_color);

    int bytes_per_pixel = get_bytes_per_pixel(rdp);

    int x_start = xh * bytes_per_pixel;
    int x_end = (xl + 1) * bytes_per_pixel;

    int stride = rdp->color_image.width * bytes_per_pixel;

    for (int y = yh; y < yl; y++) {
        int yofs = y * stride;
        for (int x = x_start; x < x_end; x += 2) {
            uint32_t addr = rdp->color_image.dram_addr + yofs + x;
            rdram_write16(rdp, addr, fill_for_addr(rdp, addr));
        }
    }
}

DEF_RDP_COMMAND(set_fill_color) {
    rdp->fill_color = get_bits(buffer[0], 31, 0);
    logalways("Fill color cmd word: %016lX", buffer[0]);
    logalways("Fill color: 0x%08X", rdp->fill_color);
}

DEF_RDP_COMMAND(set_fog_color) {
    logfatal("set_fog_color unimplemented");
}

DEF_RDP_COMMAND(set_blend_color) {
    rdp->blend_color.r = get_bits(buffer[0], 61, 56);
    rdp->blend_color.g = get_bits(buffer[0], 31, 24);
    rdp->blend_color.b = get_bits(buffer[0], 23, 16);
    rdp->blend_color.a = get_bits(buffer[0], 7, 0);

    logalways("Blend color: #%02X%02X%02X alpha %02X", rdp->blend_color.r, rdp->blend_color.g, rdp->blend_color.b, rdp->blend_color.a);
}

DEF_RDP_COMMAND(set_prim_color) {
    logfatal("set_prim_color unimplemented");
}

DEF_RDP_COMMAND(set_env_color) {
    logfatal("set_env_color unimplemented");
}

DEF_RDP_COMMAND(set_combine) {
    rdp->combine.sub_a_R_0 = get_bits(buffer[0], 55, 52);
    rdp->combine.mul_R_0   = get_bits(buffer[0], 51, 47);
    rdp->combine.sub_a_A_0 = get_bits(buffer[0], 46, 44);
    rdp->combine.mul_A_0   = get_bits(buffer[0], 43, 41);
    rdp->combine.sub_a_R_1 = get_bits(buffer[0], 40, 37);
    rdp->combine.mul_R_1   = get_bits(buffer[0], 36, 32);
    rdp->combine.sub_b_R_0 = get_bits(buffer[0], 31, 28);
    rdp->combine.sub_b_R_1 = get_bits(buffer[0], 27, 24);
    rdp->combine.sub_a_A_1 = get_bits(buffer[0], 23, 21);
    rdp->combine.mul_A_1   = get_bits(buffer[0], 20, 18);
    rdp->combine.add_R_0   = get_bits(buffer[0], 17, 15);
    rdp->combine.sub_b_A_0 = get_bits(buffer[0], 14, 12);
    rdp->combine.add_A_0   = get_bits(buffer[0], 11,  9);
    rdp->combine.add_R_1   = get_bits(buffer[0], 8,   6);
    rdp->combine.sub_b_A_1 = get_bits(buffer[0], 5,   3);
    rdp->combine.add_A_1   = get_bits(buffer[0], 2,   0);
}

DEF_RDP_COMMAND(set_texture_image) {
    rdp->texture_image.format = get_bits(buffer[0], 55, 53);
    rdp->texture_image.size   = get_bits(buffer[0], 52, 51);

    rdp->texture_image.width     = get_bits(buffer[0], 41, 32) + 1;
    rdp->texture_image.dram_addr = get_bits(buffer[0], 25, 0);
}

DEF_RDP_COMMAND(set_mask_image) {
    rdp->z_image = get_bits(buffer[0], 25, 0);
    logalways("Setting Zbuffer image to 0x%08X", rdp->z_image);
}

DEF_RDP_COMMAND(set_color_image) {
    logalways("Set color image %016lX:", buffer[0]);
    rdp->color_image.format    = get_bits(buffer[0], 55, 53);
    rdp->color_image.size      = get_bits(buffer[0], 52, 51);
    rdp->color_image.width     = get_bits(buffer[0], 41, 32) + 1;
    rdp->color_image.dram_addr = get_bits(buffer[0], 25, 0);
    logalways("Format: %d", rdp->color_image.format);
    logalways("Size: %d",   rdp->color_image.size);
    logalways("Width: %d",  rdp->color_image.width);
    logalways("DRAM addr: 0x%08X", rdp->color_image.dram_addr);
}


void enqueue_command_softrdp(softrdp_state_t* rdp, int command_length, uint64_t* buffer) {
    for (int i = 0; i < (command_length >> 1); i++) {
        uint64_t lo = (buffer[i] >>  0) & 0xFFFFFFFF;
        uint64_t hi = (buffer[i] >> 32) & 0xFFFFFFFF;
        buffer[i] = (lo << 32) | hi;
    }

    auto command = static_cast<rdp_command_t>(get_bits(buffer[0], 61, 56));

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
