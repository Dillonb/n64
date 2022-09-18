#include <cstdio>
#include <log.h>
#include <cstring>
#include <util.h>
#include <mem/mem_util.h>
#include "softrdp.h"

#ifndef INLINE
#define INLINE static inline __attribute__((always_inline))
#endif

#define EXEC_RDP_COMMAND(name) rdp_command_##name(rdp, command_length, buffer); break
#define EXEC_RDP_COMMAND_TEMPLATE(name, tmpl) rdp_command_##name<tmpl>(rdp, command_length, buffer); break
#define DEF_RDP_COMMAND(name) INLINE void rdp_command_##name(softrdp_state_t* rdp, int command_length, const uint64_t* buffer)

const int TEXEL_SIZE_8  = 1;
const int TEXEL_SIZE_16 = 2;
const int TEXEL_SIZE_32 = 3;

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
    // Word 0
    // ======
    // Y position of the highest point on the triangle
    u64 yh:14;
    u64:2;

    // Y position of the middle point on the triangle, where the second minor line starts
    u64 ym:14;
    u64:2;

    // Y position of the lowest point on the triangle
    u64 yl:14;
    u64:2;

    u64 tile:3;
    u64 level:3;

    u64:1;

    // 0 = left major 1 = right major
    u64 right_major:1;

    // Command identifier
    u64 cmd:6;
    u64:2;

    // Word 1
    // ======

    // Change in X per Y along the second minor line
    u16 dxldy_f;
    s16 dxldy;

    // X position of the lowest point on the triangle
    u16 xl_f;
    u16 xl;

    // Word 2
    // ======

    // Change in X per Y along the major line
    u16 dxhdy_f;
    s16 dxhdy;

    // X position of the highest point on the triangle
    u16 xh_f;
    u16 xh;

    // Word 3
    // ======

    // Change in X per Y along the first minor line
    u16 dxmdy_f;
    s16 dxmdy;

    // X position of the middle point of the triangle
    u16 xm_f;
    u16 xm;
} PACKED edge_coefficients_t;

static_assert(sizeof(edge_coefficients_t) == 4 * sizeof(u64), "Edge coefficients must be 4 u64s");

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

template<int int_part, int frac_part>
union fixed_point_16 {
    static_assert(int_part + frac_part + 1 == 16, "int part + frac part + 1 != 16");
    s16 raw;
    struct {
        u16 frac:frac_part;
        s16 integer:(int_part + 1);
    };

    template<int other_int_part, int other_frac_part>
    fixed_point_16<int_part, frac_part> operator+(fixed_point_16<other_int_part, other_frac_part> other) {
        static_assert(sizeof(*this) == sizeof(raw));

        fixed_point_16<int_part, frac_part> result;

        int this_shift = 0;
        int other_shift = 0;
        int after_shift = 0;

        if constexpr (frac_part < other_frac_part) {
            // this:  iiiiiiiiii.fffff
            // other: iiiii.ffffffffff

            // Shift only this
            // Don't shift other
            // Becomes:
            // this:  iiiiiiiiii.fffffxxxxx
            // other:      iiiii.ffffffffff

            this_shift = other_frac_part - frac_part;
            other_shift = 0;
            after_shift = this_shift;
        } else if constexpr (frac_part > other_frac_part) {
            // this:  iiiii.ffffffffff
            // other: iiiiiiiiii.fffff

            // Shift only other
            // Don't shift this
            // Becomes:
            // this:       iiiii.ffffffffff
            // other: iiiiiiiiii.fffffxxxxx

            this_shift = 0;
            other_shift = frac_part - other_frac_part;
            after_shift = other_shift;
        }
        // If the sizes are equal, no need for any shifting

        s32 a = (s32)raw << this_shift;
        s32 b = (s32)other.raw << other_shift;
        result.raw = (a + b) >> after_shift;
        return result;
    }

    template<int other_int_part, int other_frac_part>
    void operator+=(fixed_point_16<other_int_part, other_frac_part> other) {
        *this = *this + other;
    }
};

typedef struct texture_rectangle {
    uint16_t yh:12;
    uint16_t xh:12;
    uint16_t tile:3;
    uint16_t:5;
    uint16_t yl:12;
    uint16_t xl:12;
    uint16_t cmd:6;
    uint16_t:2;

    // Change in texture T coordinate per Y coordinate of rectangle
    fixed_point_16<5, 10> dtdy;
    // Change in texture S coordinate per X coordinate of rectangle
    fixed_point_16<5, 10> dsdx; // 5.10 fixed point format

    // Initial S and T values (top left of rectangle)
    fixed_point_16<10, 5> t;
    fixed_point_16<10, 5> s;
} PACKED texture_rectangle_t;

static_assert(sizeof(texture_rectangle_t) == 2 * sizeof(u64), "Texture rectangle command must be 2 u64s");

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

INLINE void triangle_edgewalker(softrdp_state_t* rdp, const edge_coefficients_t* ec, spans_t* spans) {
    uint32_t xstart;
    uint32_t xend;

    int32_t dxstart;
    int32_t dxend;

    if (ec->right_major) {
        xstart = ec->xm << 16 | ec->xm_f;
        xend = ec->xh << 16 | ec->xh_f;

        dxstart = ec->dxmdy << 16 | ec->dxmdy_f;
        dxend = ec->dxhdy << 16 | ec->dxhdy_f;
    } else {
        xstart = ec->xh << 16 | ec->xh_f;
        xend = ec->xm << 16 | ec->xm_f;

        dxstart = ec->dxhdy << 16 | ec->dxhdy_f;
        dxend = ec->dxmdy << 16 | ec->dxmdy_f;
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

        s->start = ((ec->right_major ? xend : xstart) >> 16);
        s->end = ((ec->right_major ? xstart : xend) >> 16);

        xstart += dxstart;
        xend += dxend;
    }

    if (ec->right_major) {
        xstart = ec->xl << 16 | ec->xl_f;
        dxstart = ec->dxldy << 16 | ec->dxldy_f;
    } else {
        xend = ec->xl << 16 | ec->xl_f;
        dxend = ec->dxldy << 16 | ec->dxldy_f;
    }

    for (int y = ym; y < yl; y += 1) {
        span_t* s = &spans->spans[span_index++];

        s->start = ((ec->right_major ? xend : xstart) >> 16);
        s->end = ((ec->right_major ? xstart : xend) >> 16);

        xstart += dxstart;
        xend += dxend;
    }

    spans->num_spans = span_index;
}

INLINE blender_source_t from_1a(int value) {
    switch (value) {
        case 0: return BLENDER_PIXEL_COLOR;
        case 1: return BLENDER_MEMORY_COLOR;
        case 2: return BLENDER_BLEND_COLOR;
        case 3: return BLENDER_FOG_COLOR;
        default: logfatal("Unknown 1a blender source: %d", value);
    }
}

INLINE blender_source_t from_1b(int value) {
    switch(value) {
        case 0: return BLENDER_PIXEL_ALPHA;
        case 1: return BLENDER_PRIMITIVE_ALPHA;
        case 2: return BLENDER_SHADE_ALPHA;
        case 3: return BLENDER_ZERO;
        default: logfatal("Unknown 1b blender source: %d", value);
    }
}

INLINE blender_source_t from_2a(int value) {
    switch(value) {
        case 0: return BLENDER_PIXEL_COLOR;
        case 1: return BLENDER_MEMORY_COLOR;
        case 2: return BLENDER_BLEND_COLOR;
        case 3: return BLENDER_FOG_COLOR;
        default: logfatal("Unknown 2a blender source: %d", value);
    }
}

INLINE blender_source_t from_2b(int value) {
    switch (value) {
        case 0: return BLENDER_ONE_MINUS_ALPHA;
        case 1: return BLENDER_MEMORY_ALPHA;
        case 2: return BLENDER_ONE;
        case 3: return BLENDER_ZERO;
        default: logfatal("Unknown 2b blender source: %d", value);
    }
}

uint8_t get_blender_alpha(softrdp_state_t* rdp, blender_source_t source) {
    switch (source) {
        case BLENDER_PIXEL_ALPHA:
            // TODO
            return 0xFF;
        case BLENDER_PRIMITIVE_ALPHA:
            logfatal("BLENDER_PRIMITIVE_ALPHA");
        case BLENDER_SHADE_ALPHA:
            logfatal("BLENDER_SHADE_ALPHA");
        case BLENDER_ONE_MINUS_ALPHA:
            // TODO
            return 0xFF;
        case BLENDER_MEMORY_ALPHA:
            logfatal("BLENDER_MEMORY_ALPHA");
        case BLENDER_ONE:
            logfatal("BLENDER_ONE");
        case BLENDER_ZERO:
            logfatal("BLENDER_ZERO");

        case BLENDER_PIXEL_COLOR:
        case BLENDER_MEMORY_COLOR:
        case BLENDER_BLEND_COLOR:
        case BLENDER_FOG_COLOR:
            logfatal("Getting color value from get_blender_alpha function!");
        default:
            logfatal("get_blender_alpha(): unknown source!");
    }
}

color_32bpp_t get_blender_color(softrdp_state_t* rdp, blender_source_t source) {
    switch(source) {
        case BLENDER_PIXEL_COLOR:
            // TODO: Output of color combiner
            return {.raw = 0xFFFFFFFF };
        case BLENDER_MEMORY_COLOR:
            logfatal("BLENDER_MEMORY_COLOR");
            break;
        case BLENDER_BLEND_COLOR:
            return rdp->blend_color;
        case BLENDER_FOG_COLOR:
            logfatal("BLENDER_FOG_COLOR");
            break;
        case BLENDER_PIXEL_ALPHA:
        case BLENDER_PRIMITIVE_ALPHA:
        case BLENDER_SHADE_ALPHA:
        case BLENDER_ONE_MINUS_ALPHA:
        case BLENDER_MEMORY_ALPHA:
        case BLENDER_ONE:
        case BLENDER_ZERO:
            logfatal("Getting alpha value from get_blender_color function!");
        default:
            logfatal("get_blender_color(): unknown source!");
    }
}

color_32bpp_t blender(softrdp_state_t* rdp, int cycle) {
    color_32bpp_t _1a = get_blender_color(rdp, rdp->other_modes.blender_config[cycle].source_1a);
    uint8_t _1b       = get_blender_alpha(rdp, rdp->other_modes.blender_config[cycle].source_1b);
    color_32bpp_t _2a = get_blender_color(rdp, rdp->other_modes.blender_config[cycle].source_2a);
    uint8_t _2b       = get_blender_alpha(rdp, rdp->other_modes.blender_config[cycle].source_2b);

    // TODO: (1a * 1b + 2a * 2b) / (1b + 2b)
    return _1a;
}

color_16bpp_t convert_32bpp_to_16bpp(color_32bpp_t color) {
    color_16bpp_t converted;
    static_assert(sizeof(color_16bpp_t) == 2, "16bpp color should be 16 bits");
    converted.a = 1;
    converted.r = color.r >> 3;
    converted.g = color.g >> 3;
    converted.b = color.b >> 3;
    return converted;
}

uint32_t convert_32bpp_to_packed_16bpp(color_32bpp_t color) {
    color_16bpp_t converted = convert_32bpp_to_16bpp(color);
    return ((uint32_t)converted.raw << 16) | converted.raw;
}

INLINE uint16_t fill_for_addr(softrdp_state_t* rdp, uint32_t addr) {
    uint32_t color = 0;
    color_32bpp_t blender_color;
    switch (rdp->other_modes.cycle_type) {
        case 0: // 1-cycle mode: run entire pipeline

            // color combiner: cycle 1 value
            // blender: cycle 0 value

            blender_color = blender(rdp, 0);
            if (get_bytes_per_pixel(rdp) == 2) {
                color = convert_32bpp_to_packed_16bpp(blender_color);
            } else {
                color = blender_color.raw;
            }
            break;
        case 1: // 2-cycle mode: runs the pipeline twice
            logfatal("2-cycle mode");
            break;
        case 2: // Copy mode: copies from TMEM to framebuffer
            logfatal("Copy mode");
            break;
        case 3: // Fill mode: just runs the rasterizer
            color = rdp->fill_color;
            break;
        default:
            logfatal("fill_for_addr(): unknown cycle type %d", rdp->other_modes.cycle_type);
    }

    // Code below is optimized to remove the conditional.
    // Essentially, the behavior is to use the upper bits of fill_color if we're on an even column,
    // and the lower bits if we're on an odd column.

    // return rdp->fill_color >> (addr % 4 == 0 ? 16 : 0);
    return color >> (16 - ((addr % 4) * 8));
}

INLINE void rdram_write16(softrdp_state_t* rdp, u32 address, u16 value) {
    memcpy(&rdp->rdram[HALF_ADDRESS(address)], &value, sizeof(u16));
}

INLINE void rdram_write32(softrdp_state_t* rdp, u32 address, u32 value) {
    memcpy(&rdp->rdram[WORD_ADDRESS(address)], &value, sizeof(u32));
}

INLINE u16 rdram_read16(softrdp_state_t* rdp, u32 address) {
    u16 value;
    memcpy(&value, &rdp->rdram[HALF_ADDRESS(address)], sizeof(u16));
    return value;
}

INLINE u32 rdram_read32(softrdp_state_t* rdp, u32 address) {
    u32 value;
    memcpy(&value, &rdp->rdram[WORD_ADDRESS(address)], sizeof(u32));
    return value;
}

INLINE u16 tmem_read16(softrdp_state_t* rdp, u16 address) {
    u16 value;
    memcpy(&value, &rdp->tmem[HALF_ADDRESS(address)], sizeof(u16));
    return value;
}

INLINE void tmem_write16(softrdp_state_t* rdp, u16 address, u16 value) {
    memcpy(&rdp->tmem[HALF_ADDRESS(address)], &value, sizeof(u16));
}

void softrdp_init(softrdp_state_t* state, uint8_t* rdramptr) {
    state->rdram = rdramptr;
}

DEF_RDP_COMMAND(fill_triangle) {
    const auto* ec = reinterpret_cast<const edge_coefficients_t*>(buffer);

    static spans_t spans;
    triangle_edgewalker(rdp, ec, &spans);

    int bytes_per_pixel = get_bytes_per_pixel(rdp);

    for (int i = 0; i < spans.num_spans; i++) {
        int y = spans.start_y + i;
        span_t* s = &spans.spans[i];

        uint32_t yofs = rdp->color_image.dram_addr + y * rdp->color_image.width * bytes_per_pixel;

        int x_start = s->start < s->end ? s->start : s->end;
        int x_end = s->end > s->start ? s->end : s->start;

        for (int x = x_start * bytes_per_pixel; x < x_end * bytes_per_pixel; x += 2) {
            uint32_t addr = yofs + x;
            rdram_write16(rdp, addr, fill_for_addr(rdp, addr));
        }
    }
}

DEF_RDP_COMMAND(fill_zbuffer_triangle) {
    const auto* ec = reinterpret_cast<const edge_coefficients_t*>(buffer);

    static z_coefficients_t zc;
    get_zbuffer_coefficients(&buffer[4], &zc);

    static spans_t spans;
    triangle_edgewalker(rdp, ec, &spans);
    logfatal("fill_zbuffer_triangle unimplemented");
}

DEF_RDP_COMMAND(texture_triangle) {
    const auto* ec = reinterpret_cast<const edge_coefficients_t*>(buffer);
    logfatal("texture_triangle unimplemented");
}

DEF_RDP_COMMAND(texture_zbuffer_triangle) {
    const auto* ec = reinterpret_cast<const edge_coefficients_t*>(buffer);
    logfatal("texture_zbuffer_triangle unimplemented");
}

DEF_RDP_COMMAND(shade_triangle) {
    const auto* ec = reinterpret_cast<const edge_coefficients_t*>(buffer);
    logfatal("shade_triangle unimplemented");
}

DEF_RDP_COMMAND(shade_zbuffer_triangle) {
    const auto* ec = reinterpret_cast<const edge_coefficients_t*>(buffer);
    logfatal("shade_zbuffer_triangle unimplemented");
}

DEF_RDP_COMMAND(shade_texture_triangle) {
    const auto* ec = reinterpret_cast<const edge_coefficients_t*>(buffer);
    logfatal("shade_texture_triangle unimplemented");
}

DEF_RDP_COMMAND(shade_texture_zbuffer_triangle) {
    const auto* ec = reinterpret_cast<const edge_coefficients_t*>(buffer);
    logfatal("shade_texture_zbuffer_triangle unimplemented");
}

INLINE fixed_point_16<10, 5> process_st(fixed_point_16<10, 5> val, bool clamp_enable, bool mirror_enable, u16 mask, u16 shift) {
    u16 mask_value = (1 << mask) - 1;

    // shift, clamp, wrap, mirror

    // Shift
    switch (shift) {
        case 0 ... 10:
            val.raw >>= shift;
            break;
        case 11 ... 15:
            val.raw <<= (16 - shift);
            break;
        default:
            logfatal("Invalid shift value: %d", shift);
    }

    // TODO: take the min of the val and the mask?
    unimplemented(clamp_enable, "Clamp enabled");

    // Grab the mirror enable bit before masking it out below
    int mirror_bit_num = mask;
    bool mirror_bit_set = (val.integer >> mirror_bit_num) & 1;


    // Wrap
    if (mask > 0) {
        val.integer &= mask_value;
    }

    // Mirror
    if (mirror_enable && mirror_bit_set) {
        val.integer = mask_value - val.integer;
    }

    return val;
}

template<bool flip>
DEF_RDP_COMMAND(texture_rectangle) {
    const auto* cmd = reinterpret_cast<const texture_rectangle_t*>(buffer);
    const auto* descriptor = &rdp->tiles[cmd->tile];
    int tmem_base = descriptor->tmem_adrs * sizeof(u64); // tmem address in descriptor is in multiples of 64 bits

    // TODO Coordinates are in a 10.2 fixed point format, just discard the decimal places
    int xl = cmd->xl >> 2;
    int yl = cmd->yl >> 2;

    int xh = cmd->xh >> 2;
    int yh = cmd->yh >> 2;
    logalways("Texture rectangle%s (%d, %d) (%d, %d) with tile %d starting at s,t %d.%d, %d.%d.", flip ? " flip" : "", xh, yh, xl, yl, cmd->tile, cmd->s.integer, cmd->s.frac, cmd->t.integer, cmd->t.frac);
    logalways("dsdx: %s%d.%d", cmd->dsdx.integer < 0 ? "-" : "", cmd->dsdx.integer, cmd->dsdx.frac);
    logalways("dtdy: %s%d.%d", cmd->dtdy.integer < 0 ? "-" : "", cmd->dtdy.integer, cmd->dtdy.frac);

    const auto orig_s = cmd->s;
    const auto orig_t = cmd->t;

    const auto dsdx = cmd->dsdx;
    const auto dtdy = cmd->dtdy;


    unimplemented(descriptor->size != 3, "texture rectangle: tile descriptor pixel size != 3");
    int bytes_per_texel = 4;

    int bytes_per_pixel = get_bytes_per_pixel(rdp);


    u32 bytes_per_screen_line = rdp->color_image.width * bytes_per_pixel;
    u32 bytes_per_tile_line = descriptor->line * sizeof(u64);

    auto s = orig_s;
    auto t = orig_t;
    for (int y = yh; y < yl; y++) {
        u32 screen_line = rdp->color_image.dram_addr + y * bytes_per_screen_line;
        for (int x = xh; x < xl; x++) {
            // TODO: for non-flipped rects, this can go in the body of the outer loop, before this inner loop
            const auto processed_t = process_st(flip ? s : t, descriptor->ct, descriptor->mt, descriptor->mask_t, descriptor->shift_t);
            const u32 tmem_xor = (processed_t.integer & 1) << 2; // Xor the address by 4 for odd lines
            const u16 tmem_line = tmem_base + processed_t.integer * bytes_per_tile_line;
            // TODO: end of block referenced above

            const auto processed_s = process_st(flip ? t : s, descriptor->cs, descriptor->ms, descriptor->mask_s, descriptor->shift_s);
            const u16 tmem_addr_rg = ((tmem_line + processed_s.integer * 2) & 0X7FF) ^ tmem_xor;
            const u16 tmem_addr_ba = tmem_addr_rg | 0x800;

            const u16 rg = tmem_read16(rdp, tmem_addr_rg);
            const u16 ba = tmem_read16(rdp, tmem_addr_ba);
            const u32 pixel = (u32)rg << 16 | ba;

            // TODO: implement a write_pixel(x, y, color) function - texels are processed internally as 32bpp always and written out to the framebuffer in the correct format
            // TODO: real transparency support
            if ((pixel & 0xFF) > 0) {
                rdram_write32(rdp, screen_line + x * bytes_per_pixel, pixel);
            }
            s += dsdx;
        }
        t += dtdy;
        s = orig_s;
    }
}

DEF_RDP_COMMAND(sync_load) {
    logfatal("sync_load unimplemented");
}

DEF_RDP_COMMAND(sync_pipe) {
    //logfatal("sync_pipe unimplemented");
}

DEF_RDP_COMMAND(sync_tile) {
    //logfatal("sync_tile unimplemented");
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

    rdp->other_modes.blender_config[0].source_1a = from_1a(get_bits(buffer[0], 31, 30));
    rdp->other_modes.blender_config[1].source_1a = from_1a(get_bits(buffer[0], 29, 28));

    rdp->other_modes.blender_config[0].source_1b = from_1b(get_bits(buffer[0], 27, 26));
    rdp->other_modes.blender_config[1].source_1b = from_1b(get_bits(buffer[0], 25, 24));

    rdp->other_modes.blender_config[0].source_2a = from_2a(get_bits(buffer[0], 23, 22));
    rdp->other_modes.blender_config[1].source_2a = from_2a(get_bits(buffer[0], 21, 20));

    rdp->other_modes.blender_config[0].source_2b = from_2b(get_bits(buffer[0], 19, 18));
    rdp->other_modes.blender_config[1].source_2b = from_2b(get_bits(buffer[0], 17, 16));

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
    softrdp_tile_t* descriptor = &rdp->tiles[tile_index];

    unimplemented(descriptor->format != rdp->texture_image.format, "load tile: descriptor format (%d) != texture image format (%d)", descriptor->format, rdp->texture_image.format);
    unimplemented(descriptor->format != 0, "Load tile format other than rgba");
    unimplemented(descriptor->size != 3, "load tile: descriptor pixel size != 3");
    unimplemented(rdp->texture_image.size != 3, "load tile: texture image pixel size != 3");

    // Ignore fractional parts for now (TODO)
    const u16 sl = get_bits(buffer[0], 55, 44) >> 2;
    const u16 tl = get_bits(buffer[0], 43, 32) >> 2;
    const u16 sh = get_bits(buffer[0], 23, 12) >> 2;
    const u16 th = get_bits(buffer[0], 11, 0) >> 2;

    const int bytes_per_texel = 4; // TODO calc from texture_image.size
    const int bytes_per_texture_line = bytes_per_texel * rdp->texture_image.width;
    const int bytes_per_tile_line = descriptor->line * sizeof(u64);

    const u32 tmem_base = descriptor->tmem_adrs * sizeof(u64); // tmem address in descriptor is in multiples of 64 bits
    const u32 dram_base = rdp->texture_image.dram_addr;

    unimplemented(tmem_base != 0, "load_tile not to start of tmem");

    int bytes_copied = 0;
    switch (rdp->texture_image.size) {
        case TEXEL_SIZE_32:
            for (int t = 0; t <= (th - tl); t++) {
                const u32 tile_line = tmem_base + bytes_per_tile_line * t;
                const u32 dram_line = dram_base + bytes_per_texture_line * (t + tl) + sl * bytes_per_texel;

                // For odd lines: xor the tmem index with 4
                const u32 tmem_xor = t & 1 ? 4 : 0;

                for (int s = 0; s <= (sh - sl); s++) {
                    u32 dram_texel_address = dram_line + s * bytes_per_texel;
                    u32 texel = rdram_read32(rdp, dram_texel_address);

                    u16 tmem_texel_address = tile_line + (s * 2);
                    tmem_texel_address ^= tmem_xor; // For odd lines
                    tmem_texel_address &= 0X7FF; // Mask to lower half of TMEM

                    u16 rg = (texel >> 16) & 0xFFFF;
                    u16 ba = (texel >>  0) & 0xFFFF;

                    tmem_write16(rdp, tmem_texel_address | 0x000, rg); // RG component goes to lower half of TMEM
                    tmem_write16(rdp, tmem_texel_address | 0x800, ba); // BA component goes to upper half of TMEM
                }
            }

            break;
        default:
            logfatal("Load tile: Unknown texel size: %d", rdp->texture_image.size);
    }

    logalways("rdp_load_tile: copied %d bytes.", bytes_copied);
}

DEF_RDP_COMMAND(set_tile) {
    int tile_index = get_bits(buffer[0], 26, 24);
    rdp->tiles[tile_index].format    = get_bits(buffer[0], 55, 53);
    rdp->tiles[tile_index].size      = get_bits(buffer[0], 52, 51);
    rdp->tiles[tile_index].line      = get_bits(buffer[0], 49, 41);
    rdp->tiles[tile_index].tmem_adrs = get_bits(buffer[0], 40, 32);
    rdp->tiles[tile_index].palette   = get_bits(buffer[0], 23, 20);
    rdp->tiles[tile_index].ct        = get_bit(buffer[0], 19);
    rdp->tiles[tile_index].mt        = get_bit(buffer[0], 18);
    rdp->tiles[tile_index].mask_t    = get_bits(buffer[0], 17, 14);
    rdp->tiles[tile_index].shift_t   = get_bits(buffer[0], 13, 10);
    rdp->tiles[tile_index].cs        = get_bit(buffer[0], 9);
    rdp->tiles[tile_index].ms        = get_bit(buffer[0], 8);
    rdp->tiles[tile_index].mask_s    = get_bits(buffer[0], 7, 4);
    rdp->tiles[tile_index].shift_s   = get_bits(buffer[0], 3, 0);

    logalways("Set tile");
    logalways("format:    %d", rdp->tiles[tile_index].format);
    logalways("size:      %d", rdp->tiles[tile_index].size);
    logalways("line:      %d", rdp->tiles[tile_index].line);
    logalways("tmem_adrs: %d", rdp->tiles[tile_index].tmem_adrs);
    logalways("palette:   %d", rdp->tiles[tile_index].palette);
    logalways("ct:        %d", rdp->tiles[tile_index].ct);
    logalways("mt:        %d", rdp->tiles[tile_index].mt);
    logalways("mask_t:    %d", rdp->tiles[tile_index].mask_t);
    logalways("shift_t:   %d", rdp->tiles[tile_index].shift_t);
    logalways("cs:        %d", rdp->tiles[tile_index].cs);
    logalways("ms:        %d", rdp->tiles[tile_index].ms);
    logalways("mask_s:    %d", rdp->tiles[tile_index].mask_s);
    logalways("shift_s:   %d", rdp->tiles[tile_index].shift_s);
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
    rdp->blend_color.r = get_bits(buffer[0], 31, 24);
    rdp->blend_color.g = get_bits(buffer[0], 23, 16);
    rdp->blend_color.b = get_bits(buffer[0], 15, 8);
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
    logalways("Set combine: %016lX", buffer[0]);
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
    logalways("Set texture image:");
    logalways("format: %d", rdp->texture_image.format);
    logalways("size: %d", rdp->texture_image.size);

    logalways("width: %d", rdp->texture_image.width);
    logalways("dram_addr: %08X", rdp->texture_image.dram_addr);
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


void softrdp_enqueue_command(softrdp_state_t* rdp, int command_length, uint64_t* buffer) {
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
        case RDP_COMMAND_TEXTURE_RECTANGLE:              EXEC_RDP_COMMAND_TEMPLATE(texture_rectangle, false);
        case RDP_COMMAND_TEXTURE_RECTANGLE_FLIP:         EXEC_RDP_COMMAND_TEMPLATE(texture_rectangle, true);
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
