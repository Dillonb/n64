#ifndef SOFTRDP_H
#define SOFTRDP_H
#include <stdint.h>
#include <stdbool.h>
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

        uint8_t b_m1a_0;
        uint8_t b_m1a_1;

        uint8_t b_m1b_0;
        uint8_t b_m1b_1;

        uint8_t b_m2a_0;
        uint8_t b_m2a_1;

        uint8_t b_m2b_0;
        uint8_t b_m2b_1;

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

} softrdp_state_t;

void init_softrdp(softrdp_state_t* state, uint8_t* rdramptr);
#define full_sync_softrdp() do {} while(0)
void enqueue_command_softrdp(softrdp_state_t* state, int command_length, uint32_t* buffer);
#endif