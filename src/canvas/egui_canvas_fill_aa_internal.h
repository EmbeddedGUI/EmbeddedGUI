#ifndef _EGUI_CANVAS_FILL_AA_INTERNAL_H_
#define _EGUI_CANVAS_FILL_AA_INTERNAL_H_

#include "canvas/egui_canvas.h"

enum
{
    EGUI_CANVAS_SDF_AA_HALF_Q8 = 192,
    EGUI_CANVAS_SDF_AA_SCALE_Q8 = 170,
};

__EGUI_STATIC_INLINE__ egui_alpha_t egui_canvas_sdf_alpha_from_q8(int32_t d_q8)
{
    if (d_q8 <= -EGUI_CANVAS_SDF_AA_HALF_Q8)
    {
        return EGUI_ALPHA_0;
    }
    if (d_q8 >= EGUI_CANVAS_SDF_AA_HALF_Q8)
    {
        return EGUI_ALPHA_100;
    }

    d_q8 = ((d_q8 + EGUI_CANVAS_SDF_AA_HALF_Q8) * EGUI_CANVAS_SDF_AA_SCALE_Q8 + 128) >> 8;
    if (d_q8 < 0)
    {
        d_q8 = 0;
    }
    if (d_q8 > (int32_t)EGUI_ALPHA_100)
    {
        d_q8 = (int32_t)EGUI_ALPHA_100;
    }
    return (egui_alpha_t)d_q8;
}

__EGUI_STATIC_INLINE__ void egui_canvas_blend_direct_pixel(egui_color_t *dst, egui_color_t color, egui_alpha_t alpha)
{
    if (alpha == EGUI_ALPHA_0)
    {
        return;
    }

    if (alpha == EGUI_ALPHA_100)
    {
        *dst = color;
    }
    else
    {
        egui_rgb_mix_ptr(dst, &color, dst, alpha);
    }
}

__EGUI_STATIC_INLINE__ void egui_canvas_fill_direct_span(egui_color_t *dst_row, egui_dim_t pfb_ofs_x, egui_dim_t x_start, egui_dim_t x_end, egui_color_t color,
                                                         egui_alpha_t alpha)
{
    egui_color_int_t *dst;
    uint32_t count;

    if (alpha == EGUI_ALPHA_0 || x_end < x_start)
    {
        return;
    }

    dst = (egui_color_int_t *)&dst_row[x_start - pfb_ofs_x];
    count = (uint32_t)(x_end - x_start + 1);

    if (alpha == EGUI_ALPHA_100)
    {
        egui_canvas_fill_color_buffer(dst, count, color);
    }
    else
    {
        egui_canvas_blend_color_buffer_alpha(dst, count, color, alpha);
    }
}

#endif
