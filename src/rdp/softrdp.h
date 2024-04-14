#ifndef SOFTRDP_H
#define SOFTRDP_H
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct softrdp_tile {
    uint8_t format;
    uint8_t size;
    uint8_t line;
    uint8_t tmem_adrs;
    uint8_t palette;
    bool ct;
    bool mt;
    uint8_t mask_t;
    uint8_t shift_t;
    bool cs;
    bool ms;
    uint8_t mask_s;
    uint8_t shift_s;
} softrdp_tile_t;

typedef enum blender_source {
    // colors
    BLENDER_PIXEL_COLOR,
    BLENDER_MEMORY_COLOR,
    BLENDER_BLEND_COLOR,
    BLENDER_FOG_COLOR,

    // Alphas
    BLENDER_PIXEL_ALPHA,
    BLENDER_PRIMITIVE_ALPHA,
    BLENDER_SHADE_ALPHA,
    BLENDER_ONE_MINUS_ALPHA,
    BLENDER_MEMORY_ALPHA,
    BLENDER_ONE,
    BLENDER_ZERO
} blender_source_t;

typedef struct blender_config {
    blender_source_t source_1a;
    blender_source_t source_1b;
    blender_source_t source_2a;
    blender_source_t source_2b;
} blender_config_t;

typedef union color_32bpp {
    uint32_t raw;
    struct {
        uint8_t a;
        uint8_t b;
        uint8_t g;
        uint8_t r;
    };
} __attribute__((__packed__)) color_32bpp_t;

typedef union color_16bpp {
    uint16_t raw;
    struct {
        uint16_t a:1;
        uint16_t b:5;
        uint16_t g:5;
        uint16_t r:5;
    };
} __attribute__((__packed__)) color_16bpp_t;

typedef struct softrdp_state {
    uint8_t* rdram;

    struct {
        uint16_t xl;
        uint16_t yl;
        uint16_t xh;
        uint16_t yh;
        bool f;
        bool o;
    } scissor;

    uint16_t primitive_z;
    uint16_t primitive_delta_z;
    uint32_t fill_color;

    struct {
        uint8_t format;
        uint8_t size;
        uint16_t width;
        uint32_t dram_addr;
    } color_image;

    color_32bpp_t blend_color;

    struct {
        uint8_t format;
        uint8_t size;
        uint16_t width;
        uint32_t dram_addr;
    } texture_image;

    struct {
        bool atomic_prim;
        uint8_t cycle_type;
        bool persp_tex_en;
        bool detail_tex_en;
        bool sharpen_tex_en;
        bool tex_lod_en;
        bool en_tlut;
        bool tlut_type;
        bool sample_type;
        bool mid_texel;
        bool bi_lerp_0;
        bool bi_lerp_1;
        bool convert_one;
        bool key_en;
        uint8_t rgb_dither_sel;
        uint8_t alpha_dither_sel;

        blender_config_t blender_config[2]; // one config for each cycle

        bool force_blend;
        bool alpha_cvg_select;
        bool cvg_times_alpha;

        uint8_t z_mode;
        uint8_t cvg_dest;

        bool color_on_cvg;
        bool image_read_en;
        bool z_update_en;
        bool z_compare_en;
        bool antialias_en;
        bool z_source_sel;
        bool dither_alpha_en;
        bool alpha_compare_en;
    } other_modes;

    struct {
        uint8_t sub_a_R_0;
        uint8_t mul_R_0;
        uint8_t sub_a_A_0;
        uint8_t mul_A_0;
        uint8_t sub_a_R_1;
        uint8_t mul_R_1;
        uint8_t sub_b_R_0;
        uint8_t sub_b_R_1;
        uint8_t sub_a_A_1;
        uint8_t mul_A_1;
        uint8_t add_R_0;
        uint8_t sub_b_A_0;
        uint8_t add_A_0;
        uint8_t add_R_1;
        uint8_t sub_b_A_1;
        uint8_t add_A_1;
    } combine;

    softrdp_tile_t tiles[8];

    u8 tmem[0x1000];

    uint32_t z_image;
} softrdp_state_t;

void softrdp_init(softrdp_state_t* state, uint8_t* rdramptr);
#define full_sync_softrdp() do {} while(0)
void softrdp_enqueue_command(softrdp_state_t* rdp, int command_length, uint64_t* buffer);

#ifdef __cplusplus
}
#endif
#endif