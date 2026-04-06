#include "egui_canvas.h"
#include "egui_canvas_gradient.h"
#include "egui_api.h"
#include "utils/egui_fixmath.h"
#include "image/egui_image.h"
#if EGUI_CONFIG_FUNCTION_SUPPORT_MASK
#include "mask/egui_mask_gradient.h"
#endif

/**
 * @brief Canvas gradient fill support.
 *
 * Provides gradient-aware fill functions for all shapes.
 * Supports linear (vertical/horizontal) and radial gradients with multi-stop colors.
 */

/* ========================== Core Utilities ========================== */

/* Sub-pixel sampling for round-rectangle corner AA (matches circle_hq quality).
 * dx, dy: pixel offset from corner circle center (dx < 0 = toward corner, dx >= 0 = toward interior).
 * r: corner radius.
 * is_left: 1 for left corner (interior is dx >= 0), 0 for right corner (interior is dx <= 0).
 * Returns coverage alpha 0..255.
 *
 * A sub-pixel sample is "inside" the rounded rectangle if:
 *   - It's inside the corner circle (dx² + dy² < r²), OR
 *   - It's in the straight section (past the corner center toward interior) AND within vertical bounds
 */
#if EGUI_CONFIG_CIRCLE_HQ_SAMPLE_2X2
#define GRAD_RR_SCALE    4
#define GRAD_RR_SCALE_SQ 16
static const egui_alpha_t grad_rr_alpha_table[] = {0, 64, 128, 191, 255};
#else
#define GRAD_RR_SCALE    8
#define GRAD_RR_SCALE_SQ 64
static const egui_alpha_t grad_rr_alpha_table[] = {0, 16, 32, 48, 64, 80, 96, 112, 128, 143, 159, 175, 191, 207, 223, 239, 255};
#endif

__EGUI_STATIC_INLINE__ egui_alpha_t gradient_round_rect_corner_alpha(int32_t dx, int32_t dy, int32_t r, uint8_t is_left)
{
    int32_t r_scaled_sq = r * r * GRAD_RR_SCALE_SQ;
    int32_t r_scaled = r * GRAD_RR_SCALE;
    int32_t dx_s = dx * GRAD_RR_SCALE;
    int32_t dy_s = dy * GRAD_RR_SCALE;
    uint8_t count = 0;

#if EGUI_CONFIG_CIRCLE_HQ_SAMPLE_2X2
    for (int32_t sy = -1; sy <= 1; sy += 2)
    {
        int32_t py = dy_s + sy;
        int32_t py_sq = py * py;
        int32_t py_abs = (py >= 0) ? py : -py;
        for (int32_t sx = -1; sx <= 1; sx += 2)
        {
            int32_t px = dx_s + sx;
            /* Inside corner circle? */
            if (px * px + py_sq < r_scaled_sq)
            {
                count++;
            }
            /* Or in the straight section (past corner center, within vertical bounds)? */
            else if (py_abs < r_scaled && ((is_left && px >= 0) || (!is_left && px <= 0)))
            {
                count++;
            }
        }
    }
#else
    for (int32_t sy = -3; sy <= 3; sy += 2)
    {
        int32_t py = dy_s + sy;
        int32_t py_sq = py * py;
        int32_t py_abs = (py >= 0) ? py : -py;
        for (int32_t sx = -3; sx <= 3; sx += 2)
        {
            int32_t px = dx_s + sx;
            if (px * px + py_sq < r_scaled_sq)
            {
                count++;
            }
            else if (py_abs < r_scaled && ((is_left && px >= 0) || (!is_left && px <= 0)))
            {
                count++;
            }
        }
    }
#endif
    return grad_rr_alpha_table[count];
}

/* Vertical edge alpha for interior pixels at boundary rows.
 * Counts how many sub-pixel y-samples are within the corner circle's vertical bounds.
 * At the top/bottom row of the rounded rect, this gives ~128 (half coverage).
 * At interior rows, this gives 255 (full coverage).
 * dy: pixel offset from corner circle center (same as corner_alpha dy).
 * r: corner radius.
 */
__EGUI_STATIC_INLINE__ egui_alpha_t gradient_round_rect_vert_edge_alpha(int32_t dy, int32_t r)
{
    int32_t r_scaled = r * GRAD_RR_SCALE;
    int32_t dy_s = dy * GRAD_RR_SCALE;
    uint8_t count = 0;

#if EGUI_CONFIG_CIRCLE_HQ_SAMPLE_2X2
    for (int32_t sy = -1; sy <= 1; sy += 2)
    {
        int32_t py = dy_s + sy;
        int32_t py_abs = (py >= 0) ? py : -py;
        if (py_abs < r_scaled)
            count += 2; /* 2 x-samples per y-level in 2x2 grid */
    }
#else
    for (int32_t sy = -3; sy <= 3; sy += 2)
    {
        int32_t py = dy_s + sy;
        int32_t py_abs = (py >= 0) ? py : -py;
        if (py_abs < r_scaled)
            count += 4; /* 4 x-samples per y-level in 4x4 grid */
    }
#endif
    return grad_rr_alpha_table[count];
}

/* Division-free integer square root (bit-by-bit method) for radial gradient distance. */
static uint32_t gradient_isqrt(uint32_t n)
{
    if (n == 0)
    {
        return 0;
    }

    uint32_t root = 0;
    uint32_t bit = 1UL << 30;

    while (bit > n)
    {
        bit >>= 2;
    }

    while (bit != 0)
    {
        if (n >= root + bit)
        {
            n -= root + bit;
            root = (root >> 1) + bit;
        }
        else
        {
            root >>= 1;
        }
        bit >>= 2;
    }

    return root;
}

/* Compute gradient color at position t (0-255) given a stop array. */
egui_color_t egui_gradient_get_color(const egui_gradient_stop_t *stops, uint8_t stop_count, uint8_t t)
{
    /* Clamp to first/last stop */
    if (t <= stops[0].position)
    {
        return stops[0].color;
    }
    if (t >= stops[stop_count - 1].position)
    {
        return stops[stop_count - 1].color;
    }

    /* Find the two surrounding stops */
    uint8_t i;
    for (i = 1; i < stop_count; i++)
    {
        if (t <= stops[i].position)
        {
            break;
        }
    }

    /* Interpolate between stops[i-1] and stops[i] */
    uint8_t range = stops[i].position - stops[i - 1].position;
    if (range == 0)
    {
        return stops[i].color;
    }

    egui_alpha_t mix = (egui_alpha_t)((uint32_t)(t - stops[i - 1].position) * 255 / range);
    egui_color_t result;
    egui_rgb_mix_ptr((egui_color_t *)&stops[i - 1].color, (egui_color_t *)&stops[i].color, &result, mix);
    return result;
}

#if EGUI_CONFIG_FUNCTION_GRADIENT_DITHERING

// Bayer 4x4 ordered dithering matrix (values 0-15)
// clang-format off
static const uint8_t egui_gradient_bayer_4x4[16] = {
     0,  8,  2, 10,
    12,  4, 14,  6,
     3, 11,  1,  9,
    15,  7, 13,  5,
};
// clang-format on

egui_color_t egui_gradient_get_color_dithered(const egui_gradient_stop_t *stops, uint8_t stop_count, uint8_t t, egui_dim_t px, egui_dim_t py)
{
    /* Apply dithering offset to the gradient position (t).
     * The Bayer threshold (0-15) is centered around 8, then scaled to a small
     * offset in the 0-255 t-range. This spatially distributes quantization
     * boundaries, breaking up visible color banding on RGB565 displays. */
    uint8_t threshold = egui_gradient_bayer_4x4[(py & 3) * 4 + (px & 3)];
    int16_t t_adj = (int16_t)t + (int16_t)threshold - 8;
    if (t_adj < 0)
    {
        t_adj = 0;
    }
    if (t_adj > 255)
    {
        t_adj = 255;
    }
    return egui_gradient_get_color(stops, stop_count, (uint8_t)t_adj);
}

#endif /* EGUI_CONFIG_FUNCTION_GRADIENT_DITHERING */

/* Compute linear gradient t value (0-255) for a given position within a dimension. */
__EGUI_STATIC_INLINE__ uint8_t gradient_linear_t(egui_dim_t pos, egui_dim_t size)
{
    if (size <= 1)
    {
        return 0;
    }
    return (uint8_t)((uint32_t)pos * 255 / (size - 1));
}

/* Compute radial gradient t value (0-255) for a given pixel offset from center. */
__EGUI_STATIC_INLINE__ uint8_t gradient_radial_t(egui_dim_t dx, egui_dim_t dy, egui_dim_t radius)
{
    if (radius <= 0)
    {
        return 255;
    }
    uint32_t dist = gradient_isqrt((uint32_t)(dx * dx) + (uint32_t)(dy * dy));
    uint32_t t = dist * 255 / radius;
    return (t > 255) ? 255 : (uint8_t)t;
}

/* ---- Integer atan2 for angular gradient ---- */

/* First-octant LUT: atan2_octant_lut[i] = round(atan(i/32) / (2*PI) * 256).
 * Maps ratio min/max in Q5 (0..32) to angle 0..32 in the 256-value space (0°..45°). */
// clang-format off
static const uint8_t atan2_octant_lut[33] = {
     0,  1,  3,  4,  5,  6,  8,  9, 10, 11, 12, 13, 15, 16, 17, 18,
    19, 20, 21, 22, 23, 24, 25, 25, 26, 27, 28, 29, 29, 30, 31, 31, 32
};
// clang-format on

/* Integer atan2 returning angle in [0, 255] where 0=0°(right), 64=90°(down),
 * 128=180°(left), 192=270°(up). Clockwise screen convention (y-down). */
static uint8_t gradient_atan2_256(int32_t dy, int32_t dx)
{
    if (dx == 0 && dy == 0)
    {
        return 0;
    }

    int32_t adx = (dx >= 0) ? dx : -dx;
    int32_t ady = (dy >= 0) ? dy : -dy;

    /* First-quadrant angle (0..63) via octant decomposition */
    uint8_t q_angle;
    if (adx >= ady)
    {
        q_angle = atan2_octant_lut[(uint32_t)(ady * 32 + (adx >> 1)) / (uint32_t)adx];
    }
    else
    {
        q_angle = 64 - atan2_octant_lut[(uint32_t)(adx * 32 + (ady >> 1)) / (uint32_t)ady];
    }

    /* Map to full 0-255 range based on quadrant */
    if (dx >= 0)
    {
        return (dy >= 0) ? q_angle : (uint8_t)(256 - q_angle);
    }
    else
    {
        return (dy >= 0) ? (uint8_t)(128 - q_angle) : (uint8_t)(128 + q_angle);
    }
}

/* Compute angular gradient t value (0-255) for a pixel at (dx, dy) from center.
 * Full 360° sweep: 0°(right) → 255 → 0°(right). */
__EGUI_STATIC_INLINE__ uint8_t gradient_angular_t(egui_dim_t dx, egui_dim_t dy)
{
    return gradient_atan2_256((int32_t)dy, (int32_t)dx);
}

/* Compute angular gradient t value for an arc span.
 * Maps pixel angle to [0, 255] within [start_angle_deg, start_angle_deg + span_deg]. */
__EGUI_STATIC_INLINE__ uint8_t gradient_angular_arc_t(int32_t dy, int32_t dx, int32_t start_angle_deg, int32_t span_deg)
{
    uint8_t pixel_angle = gradient_atan2_256(dy, dx);
    /* Convert start angle to 256-space: start_256 = start_angle_deg * 256 / 360 */
    uint8_t start_256 = (uint8_t)((uint32_t)((start_angle_deg % 360 + 360) % 360) * 256 / 360);
    /* Angular offset from start (uint8_t wraps naturally for circular arithmetic) */
    uint8_t angle_offset = pixel_angle - start_256;
    /* Convert span to 256-space */
    uint32_t span_256 = (uint32_t)span_deg * 256 / 360;
    if (span_256 == 0)
    {
        return 0;
    }
    /* Map offset to 0..255 within the span */
    uint32_t t = (uint32_t)angle_offset * 255 / span_256;
    return (t > 255) ? 255 : (uint8_t)t;
}

/* Get gradient color for a pixel at (px, py) relative to shape origin (0,0) with shape size (w, h). */
__EGUI_STATIC_INLINE__ egui_color_t gradient_color_at(const egui_gradient_t *gradient, egui_dim_t px, egui_dim_t py, egui_dim_t w, egui_dim_t h)
{
    uint8_t t;
    if (gradient->type == EGUI_GRADIENT_TYPE_LINEAR_VERTICAL)
    {
        t = gradient_linear_t(py, h);
    }
    else if (gradient->type == EGUI_GRADIENT_TYPE_LINEAR_HORIZONTAL)
    {
        t = gradient_linear_t(px, w);
    }
    else if (gradient->type == EGUI_GRADIENT_TYPE_ANGULAR)
    {
        egui_dim_t dx = px - gradient->center_x;
        egui_dim_t dy = py - gradient->center_y;
        t = gradient_angular_t(dx, dy);
    }
    else /* RADIAL */
    {
        egui_dim_t dx = px - gradient->center_x;
        egui_dim_t dy = py - gradient->center_y;
        t = gradient_radial_t(dx, dy, gradient->radius);
    }
    return egui_gradient_get_color(gradient->stops, gradient->stop_count, t);
}

#if EGUI_CONFIG_FUNCTION_GRADIENT_DITHERING
__EGUI_STATIC_INLINE__ egui_color_t gradient_color_at_dithered(const egui_gradient_t *gradient, egui_dim_t px, egui_dim_t py, egui_dim_t w, egui_dim_t h,
                                                               egui_dim_t screen_x, egui_dim_t screen_y)
{
    uint8_t t;
    if (gradient->type == EGUI_GRADIENT_TYPE_LINEAR_VERTICAL)
    {
        t = gradient_linear_t(py, h);
    }
    else if (gradient->type == EGUI_GRADIENT_TYPE_LINEAR_HORIZONTAL)
    {
        t = gradient_linear_t(px, w);
    }
    else if (gradient->type == EGUI_GRADIENT_TYPE_ANGULAR)
    {
        egui_dim_t dx = px - gradient->center_x;
        egui_dim_t dy = py - gradient->center_y;
        t = gradient_angular_t(dx, dy);
    }
    else /* RADIAL */
    {
        egui_dim_t dx = px - gradient->center_x;
        egui_dim_t dy = py - gradient->center_y;
        t = gradient_radial_t(dx, dy, gradient->radius);
    }
    return egui_gradient_get_color_dithered(gradient->stops, gradient->stop_count, t, screen_x, screen_y);
}
#endif

/* Unified gradient color lookup: applies dithering when enabled, falls back to plain color_at otherwise.
 * screen_x/screen_y are absolute pixel coordinates for the Bayer matrix. */
__EGUI_STATIC_INLINE__ egui_color_t gradient_color_at_pixel(const egui_gradient_t *gradient, egui_dim_t rel_x, egui_dim_t rel_y, egui_dim_t w, egui_dim_t h,
                                                            egui_dim_t screen_x, egui_dim_t screen_y)
{
#if EGUI_CONFIG_FUNCTION_GRADIENT_DITHERING
    return gradient_color_at_dithered(gradient, rel_x, rel_y, w, h, screen_x, screen_y);
#else
    (void)screen_x;
    (void)screen_y;
    return gradient_color_at(gradient, rel_x, rel_y, w, h);
#endif
}

/* Unified single-stop gradient color: applies dithering when enabled. */
__EGUI_STATIC_INLINE__ egui_color_t gradient_get_color_pixel(const egui_gradient_stop_t *stops, uint8_t stop_count, uint8_t t, egui_dim_t screen_x,
                                                             egui_dim_t screen_y)
{
#if EGUI_CONFIG_FUNCTION_GRADIENT_DITHERING
    return egui_gradient_get_color_dithered(stops, stop_count, t, screen_x, screen_y);
#else
    (void)screen_x;
    (void)screen_y;
    return egui_gradient_get_color(stops, stop_count, t);
#endif
}

/* 129-entry cache stores t = {0, 2, ..., 254, 255}.
 * This keeps lookup O(1) while cutting the old 256-entry LUT stack roughly in half. */
__EGUI_STATIC_INLINE__ uint8_t gradient_compact_cache_index(uint8_t t)
{
    return (t == 255) ? 128 : (t >> 1);
}

/* ========================== Rectangle Fill Gradient ========================== */

void egui_canvas_draw_rectangle_fill_gradient(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, const egui_gradient_t *gradient)
{
    egui_canvas_t *self = &canvas_data;

    EGUI_REGION_DEFINE(region, x, y, width, height);
    if (!egui_region_is_intersect(&region, &self->base_view_work_region))
    {
        return;
    }

    egui_alpha_t base_alpha = egui_color_alpha_mix(self->alpha, gradient->alpha);
    if (base_alpha == 0)
    {
        return;
    }

    /* Clip to PFB work region to avoid iterating invisible rows/columns */
    egui_dim_t vis_y_start = EGUI_MAX(0, (egui_dim_t)(self->base_view_work_region.location.y - y));
    egui_dim_t vis_y_end = EGUI_MIN(height, (egui_dim_t)(self->base_view_work_region.location.y + self->base_view_work_region.size.height - y));
    egui_dim_t vis_x_start = EGUI_MAX(0, (egui_dim_t)(self->base_view_work_region.location.x - x));
    egui_dim_t vis_x_end = EGUI_MIN(width, (egui_dim_t)(self->base_view_work_region.location.x + self->base_view_work_region.size.width - x));

    if (gradient->type == EGUI_GRADIENT_TYPE_LINEAR_VERTICAL)
    {
#if EGUI_CONFIG_FUNCTION_GRADIENT_DITHERING
        /* Dithered path: 4 color variants per row based on Bayer 4x4 column phase */
        for (egui_dim_t row = vis_y_start; row < vis_y_end; row++)
        {
            uint8_t base_t = gradient_linear_t(row, height);
            egui_dim_t screen_y = y + row;
            /* Pre-compute 4 dithered colors for this row (one per column phase) */
            egui_color_t phase_colors[4];
            for (int phase = 0; phase < 4; phase++)
            {
                phase_colors[phase] = egui_gradient_get_color_dithered(gradient->stops, gradient->stop_count, base_t, phase, screen_y);
            }
            for (egui_dim_t col = vis_x_start; col < vis_x_end; col++)
            {
                egui_canvas_draw_point_limit(x + col, screen_y, phase_colors[(x + col) & 3], base_alpha);
            }
        }
#else
        /* Fast path: one hline per row, all pixels in a row share the same color */
        for (egui_dim_t row = vis_y_start; row < vis_y_end; row++)
        {
            uint8_t t = gradient_linear_t(row, height);
            egui_color_t color = egui_gradient_get_color(gradient->stops, gradient->stop_count, t);
            egui_canvas_draw_fillrect(x, y + row, width, 1, color, base_alpha);
        }
#endif
    }
    else if (gradient->type == EGUI_GRADIENT_TYPE_LINEAR_HORIZONTAL)
    {
#if EGUI_CONFIG_FUNCTION_GRADIENT_DITHERING
        /* Dithered path: 4 color variants per column based on Bayer 4x4 row phase */
        for (egui_dim_t col = vis_x_start; col < vis_x_end; col++)
        {
            uint8_t base_t = gradient_linear_t(col, width);
            egui_dim_t screen_x = x + col;
            egui_color_t phase_colors[4];
            for (int phase = 0; phase < 4; phase++)
            {
                phase_colors[phase] = egui_gradient_get_color_dithered(gradient->stops, gradient->stop_count, base_t, screen_x, phase);
            }
            for (egui_dim_t row = vis_y_start; row < vis_y_end; row++)
            {
                egui_canvas_draw_point_limit(screen_x, y + row, phase_colors[(y + row) & 3], base_alpha);
            }
        }
#else
        /* Fast path: one vline per column */
        for (egui_dim_t col = vis_x_start; col < vis_x_end; col++)
        {
            uint8_t t = gradient_linear_t(col, width);
            egui_color_t color = egui_gradient_get_color(gradient->stops, gradient->stop_count, t);
            egui_canvas_draw_fillrect(x + col, y, 1, height, color, base_alpha);
        }
#endif
    }
    else if (gradient->type == EGUI_GRADIENT_TYPE_ANGULAR)
    {
        /* Angular gradient: per-pixel atan2 with direct PFB write.
         * Per-row optimization: precompute reciprocal of |dy| to replace divisions
         * when
         * |dy| > |dx| (~50% of pixels). */
        egui_color_t color_cache[129];
        for (uint16_t i = 0; i < 128; i++)
        {
            color_cache[i] = egui_gradient_get_color(gradient->stops, gradient->stop_count, (uint8_t)(i << 1));
        }
        color_cache[128] = egui_gradient_get_color(gradient->stops, gradient->stop_count, 255);

        int use_direct = (self->mask == NULL) ? 1 : 0;
        egui_dim_t pfb_w = self->pfb_region.size.width;
        egui_dim_t pfb_ox = self->pfb_location_in_base_view.x;
        egui_dim_t pfb_oy = self->pfb_location_in_base_view.y;
        egui_alpha_t eff_alpha = egui_color_alpha_mix(self->alpha, base_alpha);
        egui_dim_t cx = gradient->center_x;
        egui_dim_t cy = gradient->center_y;

        for (egui_dim_t row = vis_y_start; row < vis_y_end; row++)
        {
            int32_t dy = (int32_t)(row - cy);
            int32_t ady = (dy >= 0) ? dy : -dy;
            /* Precompute reciprocal for this row's |dy| */
            uint32_t inv_ady_q16 = (ady > 0) ? ((1U << 16) / (uint32_t)ady) : 0;
            int32_t ady_half = ady >> 1;

            if (use_direct)
            {
                egui_color_t *dst_row = (egui_color_t *)&self->pfb[(y + row - pfb_oy) * pfb_w + (x - pfb_ox)];
                for (egui_dim_t col = vis_x_start; col < vis_x_end; col++)
                {
                    int32_t dx = (int32_t)(col - cx);
                    int32_t adx = (dx >= 0) ? dx : -dx;
                    uint8_t q_angle;

                    if (dx == 0 && dy == 0)
                    {
                        dst_row[col] = color_cache[0];
                        continue;
                    }

                    if (adx >= ady)
                    {
                        q_angle = atan2_octant_lut[(uint32_t)(ady * 32 + (adx >> 1)) / (uint32_t)adx];
                    }
                    else
                    {
                        /* Use per-row reciprocal: multiply + shift instead of division */
                        q_angle = 64 - atan2_octant_lut[((uint32_t)(adx * 32 + ady_half) * inv_ady_q16) >> 16];
                    }

                    uint8_t t;
                    if (dx >= 0)
                    {
                        t = (dy >= 0) ? q_angle : (uint8_t)(256 - q_angle);
                    }
                    else
                    {
                        t = (dy >= 0) ? (uint8_t)(128 - q_angle) : (uint8_t)(128 + q_angle);
                    }

                    egui_color_t color = color_cache[gradient_compact_cache_index(t)];
                    if (eff_alpha == EGUI_ALPHA_100)
                    {
                        dst_row[col] = color;
                    }
                    else
                    {
                        egui_rgb_mix_ptr(&dst_row[col], &color, &dst_row[col], eff_alpha);
                    }
                }
            }
            else
            {
                for (egui_dim_t col = vis_x_start; col < vis_x_end; col++)
                {
                    int32_t dx = (int32_t)(col - cx);
                    int32_t adx = (dx >= 0) ? dx : -dx;
                    uint8_t q_angle;

                    if (dx == 0 && dy == 0)
                    {
                        egui_canvas_draw_point_limit(x + col, y + row, color_cache[0], base_alpha);
                        continue;
                    }

                    if (adx >= ady)
                    {
                        q_angle = atan2_octant_lut[(uint32_t)(ady * 32 + (adx >> 1)) / (uint32_t)adx];
                    }
                    else
                    {
                        q_angle = 64 - atan2_octant_lut[((uint32_t)(adx * 32 + ady_half) * inv_ady_q16) >> 16];
                    }

                    uint8_t t;
                    if (dx >= 0)
                    {
                        t = (dy >= 0) ? q_angle : (uint8_t)(256 - q_angle);
                    }
                    else
                    {
                        t = (dy >= 0) ? (uint8_t)(128 - q_angle) : (uint8_t)(128 + q_angle);
                    }

                    egui_color_t color = color_cache[gradient_compact_cache_index(t)];
                    egui_canvas_draw_point_limit(x + col, y + row, color, base_alpha);
                }
            }
        }
    }
    else /* RADIAL */
    {
        egui_color_t color_cache[129];
        for (uint16_t i = 0; i < 128; i++)
        {
            color_cache[i] = egui_gradient_get_color(gradient->stops, gradient->stop_count, (uint8_t)(i << 1));
        }
        color_cache[128] = egui_gradient_get_color(gradient->stops, gradient->stop_count, 255);

        if (gradient->radius <= 0)
        {
            egui_color_t solid = color_cache[128];
            for (egui_dim_t row = vis_y_start; row < vis_y_end; row++)
            {
                for (egui_dim_t col = vis_x_start; col < vis_x_end; col++)
                {
                    egui_canvas_draw_point_limit(x + col, y + row, solid, base_alpha);
                }
            }
        }
        else
        {
            /* Incremental sqrt + multiply-shift for division-free radial computation.
             * Sweep each row from center outward in both directions so distance is
             * monotonically increasing, replacing per-pixel isqrt with ~1-2 comparisons. */
            uint32_t t_mul = ((uint32_t)255 << 12) / (uint32_t)gradient->radius;
            egui_dim_t cx = gradient->center_x;
            egui_dim_t cy = gradient->center_y;

            int use_direct = (self->mask == NULL) ? 1 : 0;
            egui_dim_t pfb_w = self->pfb_region.size.width;
            egui_dim_t pfb_ox = self->pfb_location_in_base_view.x;
            egui_dim_t pfb_oy = self->pfb_location_in_base_view.y;
            egui_alpha_t eff_alpha = egui_color_alpha_mix(self->alpha, base_alpha);

/* Emit one radial pixel using current dist value */
#define RADIAL_RECT_PIXEL(col_val)                                                                                                                             \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        uint32_t _t = (dist * t_mul) >> 12;                                                                                                                    \
        if (_t > 255)                                                                                                                                          \
            _t = 255;                                                                                                                                          \
        egui_color_t _color = color_cache[gradient_compact_cache_index((uint8_t)_t)];                                                                          \
        if (use_direct)                                                                                                                                        \
        {                                                                                                                                                      \
            if (eff_alpha == EGUI_ALPHA_100)                                                                                                                   \
            {                                                                                                                                                  \
                dst_row[(col_val)] = _color;                                                                                                                   \
            }                                                                                                                                                  \
            else                                                                                                                                               \
            {                                                                                                                                                  \
                egui_rgb_mix_ptr(&dst_row[(col_val)], &_color, &dst_row[(col_val)], eff_alpha);                                                                \
            }                                                                                                                                                  \
        }                                                                                                                                                      \
        else                                                                                                                                                   \
        {                                                                                                                                                      \
            egui_canvas_draw_point_limit(x + (col_val), y + row, _color, base_alpha);                                                                          \
        }                                                                                                                                                      \
    } while (0)

            for (egui_dim_t row = vis_y_start; row < vis_y_end; row++)
            {
                int32_t dy = (int32_t)(row - cy);
                uint32_t dy_sq = (uint32_t)((int32_t)dy * dy);

                egui_color_t *dst_row = NULL;
                if (use_direct)
                {
                    dst_row = (egui_color_t *)&self->pfb[(y + row - pfb_oy) * pfb_w + (x - pfb_ox)];
                }

                /* Right sweep: from max(cx, vis_x_start) rightward to vis_x_end */
                {
                    egui_dim_t rs = (cx >= vis_x_start) ? cx : vis_x_start;
                    if (rs < vis_x_end)
                    {
                        int32_t sdx = (int32_t)(rs - cx);
                        uint32_t d_sq = (uint32_t)(sdx * sdx) + dy_sq;
                        uint32_t dist = gradient_isqrt(d_sq);
                        uint32_t next_sq = (dist + 1) * (dist + 1);
                        uint32_t next_sq_step = (dist << 1) + 3;
                        uint32_t d_sq_step = (uint32_t)(sdx * 2 + 1);

                        for (egui_dim_t col = rs; col < vis_x_end; col++)
                        {
                            while (d_sq >= next_sq)
                            {
                                dist++;
                                next_sq += next_sq_step;
                                next_sq_step += 2;
                            }
                            RADIAL_RECT_PIXEL(col);
                            d_sq += d_sq_step;
                            d_sq_step += 2;
                        }
                    }
                }

                /* Left sweep: from min(cx-1, vis_x_end-1) leftward to vis_x_start */
                {
                    egui_dim_t le = (cx - 1 < vis_x_end) ? (cx - 1) : (vis_x_end - 1);
                    if (le >= vis_x_start)
                    {
                        int32_t sdx = (int32_t)(le - cx);
                        if (sdx < 0)
                            sdx = -sdx;
                        uint32_t d_sq = (uint32_t)(sdx * sdx) + dy_sq;
                        uint32_t dist = gradient_isqrt(d_sq);
                        uint32_t next_sq = (dist + 1) * (dist + 1);
                        uint32_t next_sq_step = (dist << 1) + 3;
                        uint32_t d_sq_step = (uint32_t)(sdx * 2 + 1);

                        for (egui_dim_t col = le; col >= vis_x_start; col--)
                        {
                            while (d_sq >= next_sq)
                            {
                                dist++;
                                next_sq += next_sq_step;
                                next_sq_step += 2;
                            }
                            RADIAL_RECT_PIXEL(col);
                            d_sq += d_sq_step;
                            d_sq_step += 2;
                        }
                    }
                }
            }

#undef RADIAL_RECT_PIXEL
        }
    }
}

/* ========================== Circle Fill Gradient ========================== */

/* Reciprocal-based SDF edge alpha for a circle pixel.
 * d_sq = dx*dx + dy*dy (distance squared from center).
 * Linearly maps [r_outer_sq .. r_inner_sq] → [0 .. 255].
 * inv_r = (255 << 8) / (r_outer_sq - r_inner_sq), pre-computed once per draw call.
 * Matches SCGUI sc_arc.c reciprocal technique: 1 multiply + 1 shift per pixel. */
__EGUI_STATIC_INLINE__ egui_alpha_t gradient_circle_edge_alpha_sdf(int32_t d_sq, int32_t r_inner_sq, int32_t r_outer_sq, uint32_t inv_r)
{
    if (d_sq >= r_outer_sq)
    {
        return 0;
    }
    if (d_sq <= r_inner_sq)
    {
        return EGUI_ALPHA_100;
    }
    uint32_t cov = ((uint32_t)(r_outer_sq - d_sq) * inv_r) >> 8;
    return (egui_alpha_t)(cov > 255 ? 255 : cov);
}

void egui_canvas_draw_circle_fill_gradient(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, const egui_gradient_t *gradient)
{
    egui_canvas_t *self = &canvas_data;

    if (radius <= 0)
    {
        if (radius == 0)
        {
            egui_color_t color = gradient_color_at(gradient, 0, 0, 1, 1);
            egui_canvas_draw_point(center_x, center_y, color, gradient->alpha);
        }
        return;
    }

    EGUI_REGION_DEFINE(region, center_x - radius, center_y - radius, (radius << 1) + 1, (radius << 1) + 1);
    egui_region_t ri;
    egui_region_intersect(&region, &self->base_view_work_region, &ri);
    if (egui_region_is_empty(&ri))
    {
        return;
    }

    egui_alpha_t base_alpha = egui_color_alpha_mix(self->alpha, gradient->alpha);
    if (base_alpha == 0)
    {
        return;
    }

    egui_dim_t bbox_size = (radius << 1) + 1;
    egui_dim_t bbox_x = center_x - radius;
    egui_dim_t bbox_y = center_y - radius;
    egui_dim_t y_start = ri.location.y, y_end = ri.location.y + ri.size.height;
    egui_dim_t x_start = ri.location.x, x_end = ri.location.x + ri.size.width;

    egui_color_t color_cache[129];
    if (gradient->type != EGUI_GRADIENT_TYPE_LINEAR_VERTICAL)
    {
        for (uint16_t i = 0; i < 128; i++)
        {
            color_cache[i] = egui_gradient_get_color(gradient->stops, gradient->stop_count, (uint8_t)(i << 1));
        }
        color_cache[128] = egui_gradient_get_color(gradient->stops, gradient->stop_count, 255);
    }

    /* SDF edge alpha boundaries */
    int32_t r_inner_sq = (int32_t)(radius - 1) * (radius - 1);
    int32_t r_outer_sq = (int32_t)(radius + 1) * (radius + 1);
    uint32_t inv_r = (r_outer_sq > r_inner_sq) ? ((255U << 8) / (uint32_t)(r_outer_sq - r_inner_sq)) : 0;

    /* Direct PFB write setup for no-mask fast path */
    int use_direct_pfb = (self->mask == NULL) ? 1 : 0;
    egui_dim_t pfb_width = self->pfb_region.size.width;
    egui_dim_t pfb_ofs_x = self->pfb_location_in_base_view.x;
    egui_dim_t pfb_ofs_y = self->pfb_location_in_base_view.y;

    for (egui_dim_t y = y_start; y < y_end; y++)
    {
        int32_t dy = (int32_t)(y - center_y);
        int32_t dy_sq = dy * dy;
        if (dy_sq > r_outer_sq)
        {
            continue;
        }

        /* Compute scan boundaries using outer radius */
        int32_t x_inner_half = (dy_sq < r_inner_sq) ? (int32_t)gradient_isqrt((uint32_t)(r_inner_sq - dy_sq)) : 0;
        int32_t x_outer_half = (int32_t)gradient_isqrt((uint32_t)(r_outer_sq - dy_sq));

        egui_dim_t ol = EGUI_MAX(center_x - (egui_dim_t)x_outer_half, x_start);
        egui_dim_t or_ = EGUI_MIN(center_x + (egui_dim_t)x_outer_half, x_end - 1);

        if (use_direct_pfb)
        {
            egui_color_t *dst_base = (egui_color_t *)&self->pfb[(y - pfb_ofs_y) * pfb_width - pfb_ofs_x];
            egui_alpha_t eff_alpha = egui_color_alpha_mix(self->alpha, base_alpha);

            if (gradient->type == EGUI_GRADIENT_TYPE_LINEAR_VERTICAL)
            {
                uint8_t t = gradient_linear_t(y - bbox_y, bbox_size);
                egui_color_t color = egui_gradient_get_color(gradient->stops, gradient->stop_count, t);

                if (x_inner_half > 0)
                {
                    egui_dim_t il = center_x - (egui_dim_t)x_inner_half + 1;
                    egui_dim_t ir = center_x + (egui_dim_t)x_inner_half - 1;

                    /* Left edge pixels */
                    for (egui_dim_t x = ol; x < il && x <= or_; x++)
                    {
                        int32_t edge_dx = (int32_t)(x - center_x);
                        egui_alpha_t cov = gradient_circle_edge_alpha_sdf(edge_dx * edge_dx + dy_sq, r_inner_sq, r_outer_sq, inv_r);
                        if (cov > 0)
                        {
                            egui_alpha_t m = egui_color_alpha_mix(eff_alpha, cov);
                            if (m > 0)
                            {
                                egui_color_t *back_color = &dst_base[x];
                                if (m == EGUI_ALPHA_100)
                                {
                                    *back_color = color;
                                }
                                else
                                {
                                    egui_rgb_mix_ptr(back_color, &color, back_color, m);
                                }
                            }
                        }
                    }

                    /* Interior hline */
                    egui_dim_t bl = EGUI_MAX(il, x_start);
                    egui_dim_t br = EGUI_MIN(ir, x_end - 1);
                    if (bl <= br)
                    {
                        egui_dim_t count = br - bl + 1;
                        egui_color_t *dst = &dst_base[bl];
                        if (eff_alpha == EGUI_ALPHA_100)
                        {
                            for (egui_dim_t i = 0; i < count; i++)
                            {
                                dst[i] = color;
                            }
                        }
                        else
                        {
                            for (egui_dim_t i = 0; i < count; i++)
                            {
                                egui_rgb_mix_ptr(&dst[i], &color, &dst[i], eff_alpha);
                            }
                        }
                    }

                    /* Right edge pixels */
                    egui_dim_t rs = EGUI_MAX(ir + 1, x_start);
                    for (egui_dim_t x = rs; x <= or_; x++)
                    {
                        int32_t edge_dx = (int32_t)(x - center_x);
                        egui_alpha_t cov = gradient_circle_edge_alpha_sdf(edge_dx * edge_dx + dy_sq, r_inner_sq, r_outer_sq, inv_r);
                        if (cov > 0)
                        {
                            egui_alpha_t m = egui_color_alpha_mix(eff_alpha, cov);
                            if (m > 0)
                            {
                                egui_color_t *back_color = &dst_base[x];
                                if (m == EGUI_ALPHA_100)
                                {
                                    *back_color = color;
                                }
                                else
                                {
                                    egui_rgb_mix_ptr(back_color, &color, back_color, m);
                                }
                            }
                        }
                    }
                }
                else
                {
                    for (egui_dim_t x = ol; x <= or_; x++)
                    {
                        int32_t dx = (int32_t)(x - center_x);
                        int32_t d_sq = dx * dx + dy_sq;
                        if (d_sq > r_outer_sq)
                        {
                            continue;
                        }
                        egui_alpha_t cov;
                        if (d_sq < r_inner_sq)
                        {
                            cov = EGUI_ALPHA_100;
                        }
                        else
                        {
                            cov = gradient_circle_edge_alpha_sdf(d_sq, r_inner_sq, r_outer_sq, inv_r);
                        }
                        if (cov > 0)
                        {
                            egui_alpha_t m = egui_color_alpha_mix(eff_alpha, cov);
                            if (m > 0)
                            {
                                egui_color_t *back_color = &dst_base[x];
                                if (m == EGUI_ALPHA_100)
                                {
                                    *back_color = color;
                                }
                                else
                                {
                                    egui_rgb_mix_ptr(back_color, &color, back_color, m);
                                }
                            }
                        }
                    }
                }
            }
            else if (gradient->type == EGUI_GRADIENT_TYPE_LINEAR_HORIZONTAL)
            {
                for (egui_dim_t x = ol; x <= or_; x++)
                {
                    int32_t dx = (int32_t)(x - center_x);
                    int32_t d_sq = dx * dx + dy_sq;
                    if (d_sq > r_outer_sq)
                    {
                        continue;
                    }

                    egui_alpha_t cov = (d_sq < r_inner_sq) ? EGUI_ALPHA_100 : gradient_circle_edge_alpha_sdf(d_sq, r_inner_sq, r_outer_sq, inv_r);
                    if (cov > 0)
                    {
                        uint8_t t = gradient_linear_t(x - bbox_x, bbox_size);
                        egui_color_t color = color_cache[gradient_compact_cache_index(t)];
                        egui_alpha_t m = egui_color_alpha_mix(eff_alpha, cov);
                        if (m > 0)
                        {
                            egui_color_t *back_color = &dst_base[x];
                            if (m == EGUI_ALPHA_100)
                            {
                                *back_color = color;
                            }
                            else
                            {
                                egui_rgb_mix_ptr(back_color, &color, back_color, m);
                            }
                        }
                    }
                }
            }
            else
            {
                /* Radial: two-pass incremental sqrt (center→right, center→left) */
                uint32_t t_mul = 0;
                if (gradient->radius > 0)
                {
                    t_mul = (((uint32_t)255) << 12) / (uint32_t)gradient->radius;
                }

                /* Macro: emit one radial pixel using pre-computed d_sq and tracked dist */
#define RADIAL_PIXEL_DIRECT(px, d_sq_val)                                                                                                                      \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        if ((int32_t)(d_sq_val) <= r_outer_sq)                                                                                                                 \
        {                                                                                                                                                      \
            egui_alpha_t _cov =                                                                                                                                \
                    ((int32_t)(d_sq_val) < r_inner_sq) ? EGUI_ALPHA_100 : gradient_circle_edge_alpha_sdf((int32_t)(d_sq_val), r_inner_sq, r_outer_sq, inv_r);  \
            if (_cov > 0)                                                                                                                                      \
            {                                                                                                                                                  \
                egui_color_t _color;                                                                                                                           \
                if (gradient->radius <= 0)                                                                                                                     \
                {                                                                                                                                              \
                    _color = color_cache[128];                                                                                                                 \
                }                                                                                                                                              \
                else                                                                                                                                           \
                {                                                                                                                                              \
                    uint32_t _t = (dist * t_mul) >> 12;                                                                                                        \
                    if (_t > 255)                                                                                                                              \
                        _t = 255;                                                                                                                              \
                    _color = color_cache[gradient_compact_cache_index((uint8_t)_t)];                                                                           \
                }                                                                                                                                              \
                egui_alpha_t _m = egui_color_alpha_mix(eff_alpha, _cov);                                                                                       \
                if (_m > 0)                                                                                                                                    \
                {                                                                                                                                              \
                    egui_color_t *_back = &dst_base[(px)];                                                                                                     \
                    if (_m == EGUI_ALPHA_100)                                                                                                                  \
                    {                                                                                                                                          \
                        *_back = _color;                                                                                                                       \
                    }                                                                                                                                          \
                    else                                                                                                                                       \
                    {                                                                                                                                          \
                        egui_rgb_mix_ptr(_back, &_color, _back, _m);                                                                                           \
                    }                                                                                                                                          \
                }                                                                                                                                              \
            }                                                                                                                                                  \
        }                                                                                                                                                      \
    } while (0)

                /* Right half: ol → or_, incremental d_sq (eliminates dx*dx per pixel) */
                {
                    egui_dim_t rs = EGUI_MAX(center_x, ol);
                    if (rs <= or_)
                    {
                        uint32_t sdx = (uint32_t)(rs - center_x);
                        uint32_t d_sq = sdx * sdx + (uint32_t)dy_sq;
                        uint32_t dist = gradient_isqrt(d_sq);
                        uint32_t next_sq = (dist + 1) * (dist + 1);
                        uint32_t next_sq_step = (dist << 1) + 3;
                        uint32_t d_sq_step = (sdx << 1) + 1;

                        for (egui_dim_t x = rs; x <= or_; x++)
                        {
                            while (d_sq >= next_sq)
                            {
                                dist++;
                                next_sq += next_sq_step;
                                next_sq_step += 2;
                            }
                            RADIAL_PIXEL_DIRECT(x, d_sq);
                            d_sq += d_sq_step;
                            d_sq_step += 2;
                        }
                    }
                }

                /* Left half: or_ → ol, incremental d_sq */
                {
                    egui_dim_t le = EGUI_MIN(center_x - 1, or_);
                    if (le >= ol)
                    {
                        uint32_t sdx = (uint32_t)(center_x - le);
                        uint32_t d_sq = sdx * sdx + (uint32_t)dy_sq;
                        uint32_t dist = gradient_isqrt(d_sq);
                        uint32_t next_sq = (dist + 1) * (dist + 1);
                        uint32_t next_sq_step = (dist << 1) + 3;
                        uint32_t d_sq_step = (sdx << 1) + 1;

                        for (egui_dim_t x = le; x >= ol; x--)
                        {
                            while (d_sq >= next_sq)
                            {
                                dist++;
                                next_sq += next_sq_step;
                                next_sq_step += 2;
                            }
                            RADIAL_PIXEL_DIRECT(x, d_sq);
                            d_sq += d_sq_step;
                            d_sq_step += 2;
                        }
                    }
                }

#undef RADIAL_PIXEL_DIRECT
            }
        }
        else
        {
            /* Fallback: with mask, use original draw_point_limit path */
            if (gradient->type == EGUI_GRADIENT_TYPE_LINEAR_VERTICAL)
            {
                uint8_t t = gradient_linear_t(y - bbox_y, bbox_size);
                egui_color_t color = egui_gradient_get_color(gradient->stops, gradient->stop_count, t);

                if (x_inner_half > 0)
                {
                    egui_dim_t il = center_x - (egui_dim_t)x_inner_half + 1;
                    egui_dim_t ir = center_x + (egui_dim_t)x_inner_half - 1;

                    for (egui_dim_t x = ol; x < il && x <= or_; x++)
                    {
                        int32_t edge_dx = (int32_t)(x - center_x);
                        egui_alpha_t cov = gradient_circle_edge_alpha_sdf(edge_dx * edge_dx + dy_sq, r_inner_sq, r_outer_sq, inv_r);
                        if (cov > 0)
                        {
                            egui_alpha_t m = egui_color_alpha_mix(base_alpha, cov);
                            if (m > 0)
                            {
                                egui_canvas_draw_point_limit(x, y, color, m);
                            }
                        }
                    }

                    egui_dim_t bl = EGUI_MAX(il, x_start);
                    egui_dim_t br = EGUI_MIN(ir, x_end - 1);
                    if (bl <= br)
                    {
                        egui_canvas_draw_fillrect(bl, y, br - bl + 1, 1, color, base_alpha);
                    }

                    egui_dim_t rs = EGUI_MAX(ir + 1, x_start);
                    for (egui_dim_t x = rs; x <= or_; x++)
                    {
                        int32_t edge_dx = (int32_t)(x - center_x);
                        egui_alpha_t cov = gradient_circle_edge_alpha_sdf(edge_dx * edge_dx + dy_sq, r_inner_sq, r_outer_sq, inv_r);
                        if (cov > 0)
                        {
                            egui_alpha_t m = egui_color_alpha_mix(base_alpha, cov);
                            if (m > 0)
                            {
                                egui_canvas_draw_point_limit(x, y, color, m);
                            }
                        }
                    }
                }
                else
                {
                    for (egui_dim_t x = ol; x <= or_; x++)
                    {
                        int32_t dx = (int32_t)(x - center_x);
                        int32_t d_sq = dx * dx + dy_sq;
                        if (d_sq > r_outer_sq)
                        {
                            continue;
                        }
                        if (d_sq < r_inner_sq)
                        {
                            egui_canvas_draw_point_limit(x, y, color, base_alpha);
                            continue;
                        }
                        egui_alpha_t cov = gradient_circle_edge_alpha_sdf(d_sq, r_inner_sq, r_outer_sq, inv_r);
                        if (cov > 0)
                        {
                            egui_alpha_t m = egui_color_alpha_mix(base_alpha, cov);
                            if (m > 0)
                            {
                                egui_canvas_draw_point_limit(x, y, color, m);
                            }
                        }
                    }
                }
            }
            else if (gradient->type == EGUI_GRADIENT_TYPE_LINEAR_HORIZONTAL)
            {
                for (egui_dim_t x = ol; x <= or_; x++)
                {
                    int32_t dx = (int32_t)(x - center_x);
                    int32_t d_sq = dx * dx + dy_sq;
                    if (d_sq > r_outer_sq)
                    {
                        continue;
                    }

                    egui_alpha_t cov = (d_sq < r_inner_sq) ? EGUI_ALPHA_100 : gradient_circle_edge_alpha_sdf(d_sq, r_inner_sq, r_outer_sq, inv_r);
                    if (cov > 0)
                    {
                        uint8_t t = gradient_linear_t(x - bbox_x, bbox_size);
                        egui_color_t color = color_cache[gradient_compact_cache_index(t)];
                        egui_alpha_t m = egui_color_alpha_mix(base_alpha, cov);
                        if (m > 0)
                        {
                            egui_canvas_draw_point_limit(x, y, color, m);
                        }
                    }
                }
            }
            else
            {
                /* Radial: two-pass incremental sqrt (center→right, center→left) */
                uint32_t t_mul = 0;
                if (gradient->radius > 0)
                {
                    t_mul = (((uint32_t)255) << 12) / (uint32_t)gradient->radius;
                }

                /* Macro: emit one radial pixel using pre-computed d_sq (fallback path) */
#define RADIAL_PIXEL_FALLBACK(px, d_sq_val)                                                                                                                    \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        if ((int32_t)(d_sq_val) <= r_outer_sq)                                                                                                                 \
        {                                                                                                                                                      \
            egui_alpha_t _cov =                                                                                                                                \
                    ((int32_t)(d_sq_val) < r_inner_sq) ? EGUI_ALPHA_100 : gradient_circle_edge_alpha_sdf((int32_t)(d_sq_val), r_inner_sq, r_outer_sq, inv_r);  \
            if (_cov > 0)                                                                                                                                      \
            {                                                                                                                                                  \
                egui_color_t _color;                                                                                                                           \
                if (gradient->radius <= 0)                                                                                                                     \
                {                                                                                                                                              \
                    _color = color_cache[128];                                                                                                                 \
                }                                                                                                                                              \
                else                                                                                                                                           \
                {                                                                                                                                              \
                    uint32_t _t = (dist * t_mul) >> 12;                                                                                                        \
                    if (_t > 255)                                                                                                                              \
                        _t = 255;                                                                                                                              \
                    _color = color_cache[gradient_compact_cache_index((uint8_t)_t)];                                                                           \
                }                                                                                                                                              \
                egui_alpha_t _m = egui_color_alpha_mix(base_alpha, _cov);                                                                                      \
                if (_m > 0)                                                                                                                                    \
                {                                                                                                                                              \
                    egui_canvas_draw_point_limit((px), y, _color, _m);                                                                                         \
                }                                                                                                                                              \
            }                                                                                                                                                  \
        }                                                                                                                                                      \
    } while (0)

                /* Right half: ol → or_, incremental d_sq (fallback) */
                {
                    egui_dim_t rs = EGUI_MAX(center_x, ol);
                    if (rs <= or_)
                    {
                        uint32_t sdx = (uint32_t)(rs - center_x);
                        uint32_t d_sq = sdx * sdx + (uint32_t)dy_sq;
                        uint32_t dist = gradient_isqrt(d_sq);
                        uint32_t next_sq = (dist + 1) * (dist + 1);
                        uint32_t next_sq_step = (dist << 1) + 3;
                        uint32_t d_sq_step = (sdx << 1) + 1;

                        for (egui_dim_t x = rs; x <= or_; x++)
                        {
                            while (d_sq >= next_sq)
                            {
                                dist++;
                                next_sq += next_sq_step;
                                next_sq_step += 2;
                            }
                            RADIAL_PIXEL_FALLBACK(x, d_sq);
                            d_sq += d_sq_step;
                            d_sq_step += 2;
                        }
                    }
                }

                /* Left half: or_ → ol, incremental d_sq (fallback) */
                {
                    egui_dim_t le = EGUI_MIN(center_x - 1, or_);
                    if (le >= ol)
                    {
                        uint32_t sdx = (uint32_t)(center_x - le);
                        uint32_t d_sq = sdx * sdx + (uint32_t)dy_sq;
                        uint32_t dist = gradient_isqrt(d_sq);
                        uint32_t next_sq = (dist + 1) * (dist + 1);
                        uint32_t next_sq_step = (dist << 1) + 3;
                        uint32_t d_sq_step = (sdx << 1) + 1;

                        for (egui_dim_t x = le; x >= ol; x--)
                        {
                            while (d_sq >= next_sq)
                            {
                                dist++;
                                next_sq += next_sq_step;
                                next_sq_step += 2;
                            }
                            RADIAL_PIXEL_FALLBACK(x, d_sq);
                            d_sq += d_sq_step;
                            d_sq_step += 2;
                        }
                    }
                }

#undef RADIAL_PIXEL_FALLBACK
            }
        }
    }
}

/* ========================== Round Rectangle Fill Gradient ========================== */

/* Helper: compute the x-inset at a given dy from a corner center with given radius.
 * Returns the number of pixels from the corner edge to the circle boundary.
 * If dy >= radius, returns 0 (no inset). */
__EGUI_STATIC_INLINE__ egui_dim_t round_rect_corner_inset(egui_dim_t dy, egui_dim_t corner_radius)
{
    if (corner_radius <= 0 || dy >= corner_radius)
    {
        return 0;
    }
    int32_t r = corner_radius;
    int32_t d = dy;
    int32_t x_boundary = (int32_t)gradient_isqrt((uint32_t)(r * r - d * d));
    return (egui_dim_t)(r - x_boundary);
}

void egui_canvas_draw_round_rectangle_corners_fill_gradient(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_dim_t radius_left_top,
                                                            egui_dim_t radius_left_bottom, egui_dim_t radius_right_top, egui_dim_t radius_right_bottom,
                                                            const egui_gradient_t *gradient)
{
    egui_canvas_t *self = &canvas_data;

    if (width <= 0 || height <= 0)
    {
        return;
    }

    EGUI_REGION_DEFINE(region, x, y, width, height);
    if (!egui_region_is_intersect(&region, &self->base_view_work_region))
    {
        return;
    }

    egui_alpha_t base_alpha = egui_color_alpha_mix(self->alpha, gradient->alpha);
    if (base_alpha == 0)
    {
        return;
    }

    /* Clamp radii: allow radius == half (pill/stadium shape) but prevent overlap */
    egui_dim_t half_h = height >> 1;
    egui_dim_t half_w = width >> 1;
    if (radius_left_top > half_h)
    {
        radius_left_top = half_h;
    }
    if (radius_left_top > half_w)
    {
        radius_left_top = half_w;
    }
    if (radius_left_bottom > half_h)
    {
        radius_left_bottom = half_h;
    }
    if (radius_left_bottom > half_w)
    {
        radius_left_bottom = half_w;
    }
    if (radius_right_top > half_h)
    {
        radius_right_top = half_h;
    }
    if (radius_right_top > half_w)
    {
        radius_right_top = half_w;
    }
    if (radius_right_bottom > half_h)
    {
        radius_right_bottom = half_h;
    }
    if (radius_right_bottom > half_w)
    {
        radius_right_bottom = half_w;
    }

    /* Lookup precomputed circle AA tables for each corner radius.
     * These provide much smoother anti-aliasing than runtime sub-pixel sampling. */
    const egui_circle_info_t *lut_lt = (radius_left_top > 0) ? egui_canvas_get_circle_item(radius_left_top) : NULL;
    const egui_circle_info_t *lut_lb = (radius_left_bottom > 0) ? egui_canvas_get_circle_item(radius_left_bottom) : NULL;
    const egui_circle_info_t *lut_rt = (radius_right_top > 0) ? egui_canvas_get_circle_item(radius_right_top) : NULL;
    const egui_circle_info_t *lut_rb = (radius_right_bottom > 0) ? egui_canvas_get_circle_item(radius_right_bottom) : NULL;

    /* Scanline approach: clip to PFB work region to avoid iterating invisible rows */
    egui_dim_t vis_y_start = EGUI_MAX(y, self->base_view_work_region.location.y);
    egui_dim_t vis_y_end = EGUI_MIN(y + height, self->base_view_work_region.location.y + self->base_view_work_region.size.height);

    /* Direct PFB write setup for no-mask fast path */
    int use_direct_pfb = (self->mask == NULL) ? 1 : 0;
    egui_dim_t pfb_width = self->pfb_region.size.width;
    egui_dim_t pfb_ofs_x = self->pfb_location_in_base_view.x;
    egui_dim_t pfb_ofs_y = self->pfb_location_in_base_view.y;

    /* Middle band: rows between all corners — no SDF, full width, full alpha */
    egui_dim_t corner_band_top = EGUI_MAX(radius_left_top, radius_right_top);
    egui_dim_t corner_band_bottom = height - EGUI_MAX(radius_left_bottom, radius_right_bottom);

    for (egui_dim_t abs_row = vis_y_start; abs_row < vis_y_end; abs_row++)
    {
        egui_dim_t row = abs_row - y;

        /* ---- Middle-band fast path: skip all corner logic ---- */
        if (row >= corner_band_top && row < corner_band_bottom)
        {
            if (gradient->type == EGUI_GRADIENT_TYPE_LINEAR_VERTICAL)
            {
                uint8_t t = gradient_linear_t(row, height);
                egui_color_t color = egui_gradient_get_color(gradient->stops, gradient->stop_count, t);

                if (use_direct_pfb)
                {
                    egui_alpha_t effective_alpha = egui_color_alpha_mix(self->alpha, base_alpha);
                    egui_color_t *dst_base = (egui_color_t *)&self->pfb[(abs_row - pfb_ofs_y) * pfb_width - pfb_ofs_x];
                    egui_dim_t fill_left = EGUI_MAX(x, self->base_view_work_region.location.x);
                    egui_dim_t fill_right = EGUI_MIN(x + width - 1, self->base_view_work_region.location.x + self->base_view_work_region.size.width - 1);
                    if (fill_left <= fill_right)
                    {
                        egui_dim_t count = fill_right - fill_left + 1;
                        if (effective_alpha >= EGUI_ALPHA_100)
                        {
                            egui_color_t *dst = &dst_base[fill_left];
                            for (egui_dim_t i = 0; i < count; i++)
                            {
                                dst[i] = color;
                            }
                        }
                        else if (effective_alpha > 0)
                        {
                            egui_color_t *dst = &dst_base[fill_left];
                            for (egui_dim_t i = 0; i < count; i++)
                            {
                                egui_rgb_mix_ptr(&dst[i], &color, &dst[i], effective_alpha);
                            }
                        }
                    }
                }
                else
                {
#if EGUI_CONFIG_FUNCTION_GRADIENT_DITHERING
                    egui_color_t phase_colors[4];
                    for (int phase = 0; phase < 4; phase++)
                    {
                        phase_colors[phase] = egui_gradient_get_color_dithered(gradient->stops, gradient->stop_count, t, phase, abs_row);
                    }
                    egui_dim_t fill_left = EGUI_MAX(x, self->base_view_work_region.location.x);
                    egui_dim_t fill_right = EGUI_MIN(x + width - 1, self->base_view_work_region.location.x + self->base_view_work_region.size.width - 1);
                    for (egui_dim_t col = fill_left; col <= fill_right; col++)
                    {
                        egui_canvas_draw_point_limit(col, abs_row, phase_colors[col & 3], base_alpha);
                    }
#else
                    egui_canvas_draw_fillrect(x, abs_row, width, 1, color, base_alpha);
#endif
                }
            }
            else if (gradient->type == EGUI_GRADIENT_TYPE_LINEAR_HORIZONTAL)
            {
                egui_dim_t fill_left = EGUI_MAX(x, self->base_view_work_region.location.x);
                egui_dim_t fill_right = EGUI_MIN(x + width - 1, self->base_view_work_region.location.x + self->base_view_work_region.size.width - 1);
                for (egui_dim_t col = fill_left; col <= fill_right; col++)
                {
                    uint8_t ct = gradient_linear_t(col - x, width);
                    egui_color_t color = egui_gradient_get_color(gradient->stops, gradient->stop_count, ct);
                    egui_canvas_draw_point_limit(col, abs_row, color, base_alpha);
                }
            }
            else
            {
                egui_dim_t fill_left = EGUI_MAX(x, self->base_view_work_region.location.x);
                egui_dim_t fill_right = EGUI_MIN(x + width - 1, self->base_view_work_region.location.x + self->base_view_work_region.size.width - 1);
                for (egui_dim_t col = fill_left; col <= fill_right; col++)
                {
                    egui_color_t color = gradient_color_at_pixel(gradient, col - x, row, width, height, col, abs_row);
                    egui_canvas_draw_point_limit(col, abs_row, color, base_alpha);
                }
            }
            continue;
        }

        /* Determine which corner affects each side */
        egui_dim_t left_corner_r = 0, left_pos_row = 0;
        egui_dim_t right_corner_r = 0, right_pos_row = 0;
        const egui_circle_info_t *left_lut = NULL;
        const egui_circle_info_t *right_lut = NULL;

        if (row < radius_left_top)
        {
            left_corner_r = radius_left_top;
            left_pos_row = row;
            left_lut = lut_lt;
        }
        else if (row >= height - radius_left_bottom)
        {
            left_corner_r = radius_left_bottom;
            left_pos_row = height - 1 - row;
            left_lut = lut_lb;
        }

        if (row < radius_right_top)
        {
            right_corner_r = radius_right_top;
            right_pos_row = row;
            right_lut = lut_rt;
        }
        else if (row >= height - radius_right_bottom)
        {
            right_corner_r = radius_right_bottom;
            right_pos_row = height - 1 - row;
            right_lut = lut_rb;
        }

        /* Compute hard boundary using isqrt, then extend for AA fringe (capped at 2px) */
        egui_dim_t left_inset = 0;
        egui_dim_t right_inset = 0;
        egui_dim_t left_inset_aa = 0;
        egui_dim_t right_inset_aa = 0;
        if (left_corner_r > 0)
        {
            egui_dim_t dy = left_corner_r - left_pos_row;
            left_inset = round_rect_corner_inset(dy, left_corner_r);
            if (left_lut)
            {
                /* Use lookup table: iterate from column 0 of the corner zone.
                 * The table returns 0 for outside pixels, so no wasted draws. */
                left_inset_aa = 0;
            }
            else
            {
                /* Compute outer boundary using (r+1), then cap extension to 2 pixels */
                int32_t r_outer = left_corner_r + 1;
                int32_t d = dy;
                if (d < r_outer)
                {
                    int32_t x_outer = (int32_t)gradient_isqrt((uint32_t)(r_outer * r_outer - d * d));
                    left_inset_aa = (egui_dim_t)(left_corner_r - x_outer);
                    if (left_inset_aa < 0)
                        left_inset_aa = 0;
                    /* Cap: at most 2 pixels wider than inner boundary */
                    if (left_inset - left_inset_aa > 2)
                        left_inset_aa = left_inset - 2;
                }
                else
                {
                    left_inset_aa = (left_inset > 0) ? (left_inset - 1) : left_inset;
                }
            }
        }
        if (right_corner_r > 0)
        {
            egui_dim_t dy = right_corner_r - right_pos_row;
            right_inset = round_rect_corner_inset(dy, right_corner_r);
            if (right_lut)
            {
                right_inset_aa = 0;
            }
            else
            {
                int32_t r_outer = right_corner_r + 1;
                int32_t d = dy;
                if (d < r_outer)
                {
                    int32_t x_outer = (int32_t)gradient_isqrt((uint32_t)(r_outer * r_outer - d * d));
                    right_inset_aa = (egui_dim_t)(right_corner_r - x_outer);
                    if (right_inset_aa < 0)
                        right_inset_aa = 0;
                    if (right_inset - right_inset_aa > 2)
                        right_inset_aa = right_inset - 2;
                }
                else
                {
                    right_inset_aa = (right_inset > 0) ? (right_inset - 1) : right_inset;
                }
            }
        }

        /* Use AA boundary for row extent */
        egui_dim_t row_left = x + left_inset_aa;
        egui_dim_t row_right = x + width - 1 - right_inset_aa;

        egui_dim_t row_width = row_right - row_left + 1;
        if (row_width <= 0)
        {
            continue;
        }

        /* Determine corner/interior boundaries.
         * When using lookup table: keep corner zone at exactly R pixels (no +1 extension)
         * since the table handles the junction pixel correctly.
         * When using sub-pixel sampling: extend corner zone 1 pixel into interior so the
         * junction pixel (at dx=0 from corner center) gets circle-based AA. */
        egui_dim_t left_corner_end = row_left;
        if (left_corner_r > 0)
        {
            left_corner_end = left_lut ? (x + left_corner_r) : (x + left_corner_r + 1);
        }
        egui_dim_t right_corner_start = row_right + 1;
        if (right_corner_r > 0)
        {
            right_corner_start = right_lut ? (x + width - right_corner_r) : (x + width - 1 - right_corner_r);
        }
        egui_dim_t interior_left = EGUI_MAX(left_corner_end, row_left);
        egui_dim_t interior_right = EGUI_MIN(right_corner_start - 1, row_right);

        /* Compute vertical edge alpha for interior pixels at boundary rows.
         * When using lookup table: interior pixels get full alpha (matches non-gradient behavior).
         * When using sub-pixel sampling: reduce alpha at top/bottom edge rows. */
        egui_alpha_t vert_edge_alpha = EGUI_ALPHA_100;
        if (left_corner_r > 0 && !left_lut)
        {
            int32_t dy_left = (int32_t)left_pos_row - (int32_t)left_corner_r;
            egui_alpha_t va = gradient_round_rect_vert_edge_alpha(dy_left, left_corner_r);
            if (va < vert_edge_alpha)
                vert_edge_alpha = va;
        }
        if (right_corner_r > 0 && !right_lut)
        {
            int32_t dy_right = (int32_t)right_pos_row - (int32_t)right_corner_r;
            egui_alpha_t va = gradient_round_rect_vert_edge_alpha(dy_right, right_corner_r);
            if (va < vert_edge_alpha)
                vert_edge_alpha = va;
        }

        if (gradient->type == EGUI_GRADIENT_TYPE_LINEAR_VERTICAL)
        {
            uint8_t t = gradient_linear_t(row, height);
            egui_color_t color = egui_gradient_get_color(gradient->stops, gradient->stop_count, t);

            /* Pre-mix alpha with base for the row constant color */
            egui_alpha_t interior_alpha = (vert_edge_alpha < EGUI_ALPHA_100) ? egui_color_alpha_mix(base_alpha, vert_edge_alpha) : base_alpha;

            if (use_direct_pfb)
            {
                /* Direct PFB write fast path (no mask) */
                egui_alpha_t effective_alpha = egui_color_alpha_mix(self->alpha, base_alpha);
                egui_color_t *dst_base = (egui_color_t *)&self->pfb[(abs_row - pfb_ofs_y) * pfb_width - pfb_ofs_x];

                /* Left corner edge pixels */
                if (left_corner_r > 0)
                {
                    egui_dim_t le = EGUI_MIN(left_corner_end - 1, row_right);
                    egui_dim_t start_col = EGUI_MAX(row_left, self->base_view_work_region.location.x);
                    egui_dim_t end_col = EGUI_MIN(le, self->base_view_work_region.location.x + self->base_view_work_region.size.width - 1);
                    if (left_lut)
                    {
                        for (egui_dim_t col = start_col; col <= end_col; col++)
                        {
                            egui_alpha_t cov = egui_canvas_get_circle_corner_value(left_pos_row, col - x, left_lut);
                            if (cov > 0)
                            {
                                egui_alpha_t m = egui_color_alpha_mix(effective_alpha, cov);
                                if (m > 0)
                                {
                                    egui_color_t *back_color = &dst_base[col];
                                    if (m == EGUI_ALPHA_100)
                                    {
                                        *back_color = color;
                                    }
                                    else
                                    {
                                        egui_rgb_mix_ptr(back_color, &color, back_color, m);
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        int32_t cy_off = (int32_t)left_pos_row - (int32_t)left_corner_r;
                        for (egui_dim_t col = start_col; col <= end_col; col++)
                        {
                            int32_t cx_off = (int32_t)(col - x) - (int32_t)left_corner_r;
                            egui_alpha_t cov = gradient_round_rect_corner_alpha(cx_off, cy_off, left_corner_r, 1);
                            if (cov > 0)
                            {
                                egui_alpha_t m = egui_color_alpha_mix(effective_alpha, cov);
                                if (m > 0)
                                {
                                    egui_color_t *back_color = &dst_base[col];
                                    if (m == EGUI_ALPHA_100)
                                    {
                                        *back_color = color;
                                    }
                                    else
                                    {
                                        egui_rgb_mix_ptr(back_color, &color, back_color, m);
                                    }
                                }
                            }
                        }
                    }
                }

                /* Interior pixels - direct PFB fill */
                if (interior_left <= interior_right)
                {
                    egui_alpha_t eff_interior = egui_color_alpha_mix(self->alpha, interior_alpha);
#if EGUI_CONFIG_FUNCTION_GRADIENT_DITHERING
                    egui_dim_t screen_y = y + row;
                    egui_color_t phase_colors[4];
                    for (int phase = 0; phase < 4; phase++)
                    {
                        phase_colors[phase] = egui_gradient_get_color_dithered(gradient->stops, gradient->stop_count, t, phase, screen_y);
                    }
                    egui_dim_t dith_left = EGUI_MAX(interior_left, self->base_view_work_region.location.x);
                    egui_dim_t dith_right = EGUI_MIN(interior_right, self->base_view_work_region.location.x + self->base_view_work_region.size.width - 1);
                    for (egui_dim_t col = dith_left; col <= dith_right; col++)
                    {
                        egui_color_t *back_color = &dst_base[col];
                        if (eff_interior == EGUI_ALPHA_100)
                        {
                            *back_color = phase_colors[col & 3];
                        }
                        else
                        {
                            egui_color_t pc = phase_colors[col & 3];
                            egui_rgb_mix_ptr(back_color, &pc, back_color, eff_interior);
                        }
                    }
#else
                    egui_dim_t fill_left = EGUI_MAX(interior_left, self->base_view_work_region.location.x);
                    egui_dim_t fill_right = EGUI_MIN(interior_right, self->base_view_work_region.location.x + self->base_view_work_region.size.width - 1);
                    if (fill_left <= fill_right)
                    {
                        egui_dim_t count = fill_right - fill_left + 1;
                        egui_color_t *dst = &dst_base[fill_left];
                        if (eff_interior == EGUI_ALPHA_100)
                        {
                            for (egui_dim_t i = 0; i < count; i++)
                            {
                                dst[i] = color;
                            }
                        }
                        else if (eff_interior > 0)
                        {
                            for (egui_dim_t i = 0; i < count; i++)
                            {
                                egui_rgb_mix_ptr((egui_color_t *)&dst[i], &color, (egui_color_t *)&dst[i], eff_interior);
                            }
                        }
                    }
#endif
                }

                /* Right corner edge pixels */
                if (right_corner_r > 0)
                {
                    egui_dim_t rs = EGUI_MAX(right_corner_start, row_left);
                    egui_dim_t start_col = EGUI_MAX(rs, self->base_view_work_region.location.x);
                    egui_dim_t end_col = EGUI_MIN(row_right, self->base_view_work_region.location.x + self->base_view_work_region.size.width - 1);
                    if (right_lut)
                    {
                        for (egui_dim_t col = start_col; col <= end_col; col++)
                        {
                            egui_dim_t pos_col = x + width - 1 - col;
                            egui_alpha_t cov = egui_canvas_get_circle_corner_value(right_pos_row, pos_col, right_lut);
                            if (cov > 0)
                            {
                                egui_alpha_t m = egui_color_alpha_mix(effective_alpha, cov);
                                if (m > 0)
                                {
                                    egui_color_t *back_color = &dst_base[col];
                                    if (m == EGUI_ALPHA_100)
                                    {
                                        *back_color = color;
                                    }
                                    else
                                    {
                                        egui_rgb_mix_ptr(back_color, &color, back_color, m);
                                    }
                                }
                            }
                        }
                    }
                    else
                    {
                        int32_t cy_off = (int32_t)right_pos_row - (int32_t)right_corner_r;
                        for (egui_dim_t col = start_col; col <= end_col; col++)
                        {
                            int32_t cx_off = (int32_t)(col - x) - (int32_t)(width - 1 - right_corner_r);
                            egui_alpha_t cov = gradient_round_rect_corner_alpha(cx_off, cy_off, right_corner_r, 0);
                            if (cov > 0)
                            {
                                egui_alpha_t m = egui_color_alpha_mix(effective_alpha, cov);
                                if (m > 0)
                                {
                                    egui_color_t *back_color = &dst_base[col];
                                    if (m == EGUI_ALPHA_100)
                                    {
                                        *back_color = color;
                                    }
                                    else
                                    {
                                        egui_rgb_mix_ptr(back_color, &color, back_color, m);
                                    }
                                }
                            }
                        }
                    }
                }
            }
            else
            {
                /* Fallback path: with mask or non-direct PFB */
                /* Left corner edge pixels with HQ AA */
                if (left_corner_r > 0)
                {
                    egui_dim_t le = EGUI_MIN(left_corner_end - 1, row_right);
                    egui_dim_t start_col = EGUI_MAX(row_left, self->base_view_work_region.location.x);
                    egui_dim_t end_col = EGUI_MIN(le, self->base_view_work_region.location.x + self->base_view_work_region.size.width - 1);
                    if (left_lut)
                    {
                        for (egui_dim_t col = start_col; col <= end_col; col++)
                        {
                            egui_alpha_t cov = egui_canvas_get_circle_corner_value(left_pos_row, col - x, left_lut);
                            if (cov > 0)
                            {
                                egui_alpha_t m = egui_color_alpha_mix(base_alpha, cov);
                                if (m > 0)
                                {
                                    egui_canvas_draw_point_limit(col, y + row, color, m);
                                }
                            }
                        }
                    }
                    else
                    {
                        int32_t cy_off = (int32_t)left_pos_row - (int32_t)left_corner_r;
                        for (egui_dim_t col = start_col; col <= end_col; col++)
                        {
                            int32_t cx_off = (int32_t)(col - x) - (int32_t)left_corner_r;
                            egui_alpha_t cov = gradient_round_rect_corner_alpha(cx_off, cy_off, left_corner_r, 1);
                            if (cov > 0)
                            {
                                egui_alpha_t m = egui_color_alpha_mix(base_alpha, cov);
                                if (m > 0)
                                {
                                    egui_canvas_draw_point_limit(col, y + row, color, m);
                                }
                            }
                        }
                    }
                }

                /* Interior pixels with hline */
                if (interior_left <= interior_right)
                {
#if EGUI_CONFIG_FUNCTION_GRADIENT_DITHERING
                    egui_dim_t screen_y = y + row;
                    egui_color_t phase_colors[4];
                    for (int phase = 0; phase < 4; phase++)
                    {
                        phase_colors[phase] = egui_gradient_get_color_dithered(gradient->stops, gradient->stop_count, t, phase, screen_y);
                    }
                    egui_dim_t dith_left = EGUI_MAX(interior_left, self->base_view_work_region.location.x);
                    egui_dim_t dith_right = EGUI_MIN(interior_right, self->base_view_work_region.location.x + self->base_view_work_region.size.width - 1);
                    for (egui_dim_t col = dith_left; col <= dith_right; col++)
                    {
                        egui_canvas_draw_point_limit(col, screen_y, phase_colors[col & 3], interior_alpha);
                    }
#else
                    egui_canvas_draw_fillrect(interior_left, y + row, interior_right - interior_left + 1, 1, color, interior_alpha);
#endif
                }

                /* Right corner edge pixels with HQ AA */
                if (right_corner_r > 0)
                {
                    egui_dim_t rs = EGUI_MAX(right_corner_start, row_left);
                    egui_dim_t start_col = EGUI_MAX(rs, self->base_view_work_region.location.x);
                    egui_dim_t end_col = EGUI_MIN(row_right, self->base_view_work_region.location.x + self->base_view_work_region.size.width - 1);
                    if (right_lut)
                    {
                        for (egui_dim_t col = start_col; col <= end_col; col++)
                        {
                            egui_dim_t pos_col = x + width - 1 - col;
                            egui_alpha_t cov = egui_canvas_get_circle_corner_value(right_pos_row, pos_col, right_lut);
                            if (cov > 0)
                            {
                                egui_alpha_t m = egui_color_alpha_mix(base_alpha, cov);
                                if (m > 0)
                                {
                                    egui_canvas_draw_point_limit(col, y + row, color, m);
                                }
                            }
                        }
                    }
                    else
                    {
                        int32_t cy_off = (int32_t)right_pos_row - (int32_t)right_corner_r;
                        for (egui_dim_t col = start_col; col <= end_col; col++)
                        {
                            int32_t cx_off = (int32_t)(col - x) - (int32_t)(width - 1 - right_corner_r);
                            egui_alpha_t cov = gradient_round_rect_corner_alpha(cx_off, cy_off, right_corner_r, 0);
                            if (cov > 0)
                            {
                                egui_alpha_t m = egui_color_alpha_mix(base_alpha, cov);
                                if (m > 0)
                                {
                                    egui_canvas_draw_point_limit(col, y + row, color, m);
                                }
                            }
                        }
                    }
                }
            }
        }
        else if (gradient->type == EGUI_GRADIENT_TYPE_LINEAR_HORIZONTAL)
        {
            /* Horizontal: per-pixel AA with cached horizontal color */
            egui_dim_t start_col = EGUI_MAX(row_left, self->base_view_work_region.location.x);
            egui_dim_t end_col = EGUI_MIN(row_right, self->base_view_work_region.location.x + self->base_view_work_region.size.width - 1);
            for (egui_dim_t col = start_col; col <= end_col; col++)
            {
                egui_alpha_t cov = vert_edge_alpha;

                if (left_corner_r > 0 && col < left_corner_end)
                {
                    if (left_lut)
                    {
                        cov = egui_canvas_get_circle_corner_value(left_pos_row, col - x, left_lut);
                    }
                    else
                    {
                        int32_t cx_off = (int32_t)(col - x) - (int32_t)left_corner_r;
                        int32_t cy_off = (int32_t)left_pos_row - (int32_t)left_corner_r;
                        cov = gradient_round_rect_corner_alpha(cx_off, cy_off, left_corner_r, 1);
                    }
                }
                else if (right_corner_r > 0 && col >= right_corner_start)
                {
                    if (right_lut)
                    {
                        egui_dim_t pos_col = x + width - 1 - col;
                        cov = egui_canvas_get_circle_corner_value(right_pos_row, pos_col, right_lut);
                    }
                    else
                    {
                        int32_t cx_off = (int32_t)(col - x) - (int32_t)(width - 1 - right_corner_r);
                        int32_t cy_off = (int32_t)right_pos_row - (int32_t)right_corner_r;
                        cov = gradient_round_rect_corner_alpha(cx_off, cy_off, right_corner_r, 0);
                    }
                }

                if (cov > 0)
                {
                    uint8_t ct = gradient_linear_t(col - x, width);
                    egui_color_t color = egui_gradient_get_color(gradient->stops, gradient->stop_count, ct);
                    egui_alpha_t m = egui_color_alpha_mix(base_alpha, cov);
                    if (m > 0)
                    {
                        egui_canvas_draw_point_limit(col, y + row, color, m);
                    }
                }
            }
        }
        else
        {
            /* Radial: per-pixel AA with radial distance color */
            egui_dim_t start_col = EGUI_MAX(row_left, self->base_view_work_region.location.x);
            egui_dim_t end_col = EGUI_MIN(row_right, self->base_view_work_region.location.x + self->base_view_work_region.size.width - 1);
            for (egui_dim_t col = start_col; col <= end_col; col++)
            {
                egui_alpha_t cov = vert_edge_alpha;

                if (left_corner_r > 0 && col < left_corner_end)
                {
                    if (left_lut)
                    {
                        cov = egui_canvas_get_circle_corner_value(left_pos_row, col - x, left_lut);
                    }
                    else
                    {
                        int32_t cx_off = (int32_t)(col - x) - (int32_t)left_corner_r;
                        int32_t cy_off = (int32_t)left_pos_row - (int32_t)left_corner_r;
                        cov = gradient_round_rect_corner_alpha(cx_off, cy_off, left_corner_r, 1);
                    }
                }
                else if (right_corner_r > 0 && col >= right_corner_start)
                {
                    if (right_lut)
                    {
                        egui_dim_t pos_col = x + width - 1 - col;
                        cov = egui_canvas_get_circle_corner_value(right_pos_row, pos_col, right_lut);
                    }
                    else
                    {
                        int32_t cx_off = (int32_t)(col - x) - (int32_t)(width - 1 - right_corner_r);
                        int32_t cy_off = (int32_t)right_pos_row - (int32_t)right_corner_r;
                        cov = gradient_round_rect_corner_alpha(cx_off, cy_off, right_corner_r, 0);
                    }
                }

                if (cov > 0)
                {
                    egui_dim_t dx = col - x - gradient->center_x;
                    egui_dim_t dy = row - gradient->center_y;
                    egui_color_t color;
                    if (gradient->radius <= 0)
                    {
                        color = egui_gradient_get_color(gradient->stops, gradient->stop_count, 255);
                    }
                    else
                    {
                        uint32_t dist = gradient_isqrt((uint32_t)(dx * dx) + (uint32_t)(dy * dy));
                        uint32_t t = dist * 255 / gradient->radius;
                        if (t > 255)
                            t = 255;
                        color = egui_gradient_get_color(gradient->stops, gradient->stop_count, (uint8_t)t);
                    }
                    egui_alpha_t m = egui_color_alpha_mix(base_alpha, cov);
                    if (m > 0)
                    {
                        egui_canvas_draw_point_limit(col, y + row, color, m);
                    }
                }
            }
        }
    }
}

void egui_canvas_draw_round_rectangle_fill_gradient(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_dim_t radius,
                                                    const egui_gradient_t *gradient)
{
    egui_canvas_draw_round_rectangle_corners_fill_gradient(x, y, width, height, radius, radius, radius, radius, gradient);
}

/* ========================== Triangle Fill Gradient ========================== */

/**
 * @brief Compute SDF-based alpha for a pixel relative to one triangle edge.
 *
 * cross = edx*(py-ey) - edy*(px-ex), sign ensures positive=inside.
 * Maps signed distance d in [-0.5, +0.5] to alpha linearly.
 */
__EGUI_STATIC_INLINE__ egui_alpha_t gradient_triangle_edge_alpha(int32_t cross, int32_t sign, uint32_t len)
{
    int32_t d_q8 = (int32_t)(sign * ((int64_t)cross * 256 / (int32_t)len));

    if (d_q8 <= -128)
    {
        return EGUI_ALPHA_0;
    }
    if (d_q8 >= 128)
    {
        return EGUI_ALPHA_100;
    }

    int32_t alpha_val = ((d_q8 + 128) * (int32_t)EGUI_ALPHA_100 + 128) >> 8;
    if (alpha_val < 0)
    {
        alpha_val = 0;
    }
    if (alpha_val > (int32_t)EGUI_ALPHA_100)
    {
        alpha_val = (int32_t)EGUI_ALPHA_100;
    }
    return (egui_alpha_t)alpha_val;
}

void egui_canvas_draw_triangle_fill_gradient(egui_dim_t x1, egui_dim_t y1, egui_dim_t x2, egui_dim_t y2, egui_dim_t x3, egui_dim_t y3,
                                             const egui_gradient_t *gradient)
{
    egui_canvas_t *self = &canvas_data;

    /* Bounding box */
    egui_dim_t min_x = EGUI_MIN(EGUI_MIN(x1, x2), x3);
    egui_dim_t min_y = EGUI_MIN(EGUI_MIN(y1, y2), y3);
    egui_dim_t max_x = EGUI_MAX(EGUI_MAX(x1, x2), x3);
    egui_dim_t max_y = EGUI_MAX(EGUI_MAX(y1, y2), y3);

    EGUI_REGION_DEFINE(region, min_x, min_y, max_x - min_x + 1, max_y - min_y + 1);
    if (!egui_region_is_intersect(&region, &self->base_view_work_region))
    {
        return;
    }

    egui_alpha_t base_alpha = egui_color_alpha_mix(self->alpha, gradient->alpha);
    if (base_alpha == 0)
    {
        return;
    }

    egui_dim_t bbox_w = max_x - min_x + 1;
    egui_dim_t bbox_h = max_y - min_y + 1;

    /* Sort vertices by y: v0.y <= v1.y <= v2.y */
    egui_dim_t vx0 = x1, vy0 = y1;
    egui_dim_t vx1 = x2, vy1 = y2;
    egui_dim_t vx2 = x3, vy2 = y3;

    if (vy0 > vy1)
    {
        EGUI_SWAP(vx0, vx1);
        EGUI_SWAP(vy0, vy1);
    }
    if (vy1 > vy2)
    {
        EGUI_SWAP(vx1, vx2);
        EGUI_SWAP(vy1, vy2);
    }
    if (vy0 > vy1)
    {
        EGUI_SWAP(vx0, vx1);
        EGUI_SWAP(vy0, vy1);
    }

    if (vy0 == vy2)
    {
        /* Degenerate: horizontal line */
        egui_dim_t left = EGUI_MIN(EGUI_MIN(vx0, vx1), vx2);
        egui_dim_t right = EGUI_MAX(EGUI_MAX(vx0, vx1), vx2);
        uint8_t t = gradient_linear_t(vy0 - min_y, bbox_h);
        egui_color_t color = egui_gradient_get_color(gradient->stops, gradient->stop_count, t);
        egui_canvas_draw_hline(left, vy0, right - left + 1, color, base_alpha);
        return;
    }

    /* Precompute edge vectors, lengths, and inside-direction signs */
    int32_t eLdx = vx2 - vx0, eLdy = vy2 - vy0;
    uint32_t lenL = gradient_isqrt((uint32_t)(eLdx * eLdx + eLdy * eLdy));
    if (lenL == 0)
    {
        lenL = 1;
    }

    int32_t eAdx = vx1 - vx0, eAdy = vy1 - vy0;
    uint32_t lenA = gradient_isqrt((uint32_t)(eAdx * eAdx + eAdy * eAdy));
    if (lenA == 0)
    {
        lenA = 1;
    }

    int32_t eBdx = vx2 - vx1, eBdy = vy2 - vy1;
    uint32_t lenB = gradient_isqrt((uint32_t)(eBdx * eBdx + eBdy * eBdy));
    if (lenB == 0)
    {
        lenB = 1;
    }

    /* Inside signs via centroid cross-product test */
    int32_t cx3 = vx0 + vx1 + vx2;
    int32_t cy3 = vy0 + vy1 + vy2;
    int32_t sL = (eLdx * (cy3 - 3 * vy0) - eLdy * (cx3 - 3 * vx0) >= 0) ? 1 : -1;
    int32_t sA = (eAdx * (cy3 - 3 * vy0) - eAdy * (cx3 - 3 * vx0) >= 0) ? 1 : -1;
    int32_t sB = (eBdx * (cy3 - 3 * vy1) - eBdy * (cx3 - 3 * vx1) >= 0) ? 1 : -1;

    /* Clamp scanline range */
    egui_dim_t work_y_start = self->base_view_work_region.location.y;
    egui_dim_t work_y_end = work_y_start + self->base_view_work_region.size.height;
    egui_dim_t work_x_start = self->base_view_work_region.location.x;
    egui_dim_t work_x_end = work_x_start + self->base_view_work_region.size.width - 1;
    egui_dim_t scan_y_start = EGUI_MAX(vy0, work_y_start);
    egui_dim_t scan_y_end = EGUI_MIN(vy2, work_y_end - 1);

#define GRAD_TRI_AA_MARGIN 2

    for (egui_dim_t y = scan_y_start; y <= scan_y_end; y++)
    {
        egui_float_t left_x, right_x;

        /* Long edge: v0 -> v2 */
        if (vy2 != vy0)
        {
            left_x = EGUI_FLOAT_VALUE_INT(vx0) +
                     EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(vx2 - vx0), EGUI_FLOAT_DIV(EGUI_FLOAT_VALUE_INT(y - vy0), EGUI_FLOAT_VALUE_INT(vy2 - vy0)));
        }
        else
        {
            left_x = EGUI_FLOAT_VALUE_INT(vx0);
        }

        /* Short edges - also track which edge is active for SDF */
        int32_t act_edx, act_edy, act_sign;
        uint32_t act_len;
        egui_dim_t act_ox, act_oy;

        if (y <= vy1)
        {
            if (vy1 != vy0)
            {
                right_x = EGUI_FLOAT_VALUE_INT(vx0) +
                          EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(vx1 - vx0), EGUI_FLOAT_DIV(EGUI_FLOAT_VALUE_INT(y - vy0), EGUI_FLOAT_VALUE_INT(vy1 - vy0)));
            }
            else
            {
                right_x = EGUI_FLOAT_VALUE_INT(vx1);
            }
            act_edx = eAdx;
            act_edy = eAdy;
            act_len = lenA;
            act_sign = sA;
            act_ox = vx0;
            act_oy = vy0;
        }
        else
        {
            if (vy2 != vy1)
            {
                right_x = EGUI_FLOAT_VALUE_INT(vx1) +
                          EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(vx2 - vx1), EGUI_FLOAT_DIV(EGUI_FLOAT_VALUE_INT(y - vy1), EGUI_FLOAT_VALUE_INT(vy2 - vy1)));
            }
            else
            {
                right_x = EGUI_FLOAT_VALUE_INT(vx2);
            }
            act_edx = eBdx;
            act_edy = eBdy;
            act_len = lenB;
            act_sign = sB;
            act_ox = vx1;
            act_oy = vy1;
        }

        /* Assign left/right edge SDF parameters */
        int32_t left_edx, left_edy, left_sign;
        uint32_t left_len;
        egui_dim_t left_ox, left_oy;
        int32_t right_edx, right_edy, right_sign;
        uint32_t right_len;
        egui_dim_t right_ox, right_oy;

        if (left_x <= right_x)
        {
            left_edx = eLdx;
            left_edy = eLdy;
            left_len = lenL;
            left_sign = sL;
            left_ox = vx0;
            left_oy = vy0;
            right_edx = act_edx;
            right_edy = act_edy;
            right_len = act_len;
            right_sign = act_sign;
            right_ox = act_ox;
            right_oy = act_oy;
        }
        else
        {
            egui_float_t tmp = left_x;
            left_x = right_x;
            right_x = tmp;
            left_edx = act_edx;
            left_edy = act_edy;
            left_len = act_len;
            left_sign = act_sign;
            left_ox = act_ox;
            left_oy = act_oy;
            right_edx = eLdx;
            right_edy = eLdy;
            right_len = lenL;
            right_sign = sL;
            right_ox = vx0;
            right_oy = vy0;
        }

        egui_dim_t ix_left = EGUI_FLOAT_INT_PART(left_x);
        egui_dim_t ix_right = EGUI_FLOAT_INT_PART(right_x);

        if (ix_right < work_x_start || ix_left > work_x_end)
        {
            continue;
        }

        /* AA zones */
        egui_dim_t aa_left_start = EGUI_MAX(ix_left - 1, work_x_start);
        egui_dim_t aa_left_end = EGUI_MIN(ix_left + GRAD_TRI_AA_MARGIN - 1, work_x_end);
        egui_dim_t aa_right_start = EGUI_MAX(ix_right - GRAD_TRI_AA_MARGIN + 1, work_x_start);
        egui_dim_t aa_right_end = EGUI_MIN(ix_right + 1, work_x_end);

        egui_dim_t interior_left = aa_left_end + 1;
        egui_dim_t interior_right = aa_right_start - 1;

        /* Thin triangle: AA zones overlap, per-pixel SDF for entire span */
        if (interior_left > interior_right)
        {
            egui_dim_t span_start = EGUI_MAX(ix_left - 1, work_x_start);
            egui_dim_t span_end = EGUI_MIN(ix_right + 1, work_x_end);

            for (egui_dim_t px = span_start; px <= span_end; px++)
            {
                int32_t crossL = left_edx * (y - left_oy) - left_edy * (px - left_ox);
                egui_alpha_t aL = gradient_triangle_edge_alpha(crossL, left_sign, left_len);

                int32_t crossR = right_edx * (y - right_oy) - right_edy * (px - right_ox);
                egui_alpha_t aR = gradient_triangle_edge_alpha(crossR, right_sign, right_len);

                egui_alpha_t cov = (aL < aR) ? aL : aR;
                if (cov > 0)
                {
                    egui_alpha_t mix = egui_color_alpha_mix(base_alpha, cov);
                    if (mix > 0)
                    {
                        egui_color_t color = gradient_color_at_pixel(gradient, px - min_x, y - min_y, bbox_w, bbox_h, px, y);
                        egui_canvas_draw_point_limit(px, y, color, mix);
                    }
                }
            }
            continue;
        }

        /* Interior pixels: full coverage */
        if (interior_left <= interior_right)
        {
            if (gradient->type == EGUI_GRADIENT_TYPE_LINEAR_VERTICAL)
            {
                uint8_t t = gradient_linear_t(y - min_y, bbox_h);
                egui_color_t color = egui_gradient_get_color(gradient->stops, gradient->stop_count, t);
                egui_canvas_draw_hline(interior_left, y, interior_right - interior_left + 1, color, base_alpha);
            }
            else
            {
                egui_dim_t draw_start = EGUI_MAX(interior_left, work_x_start);
                egui_dim_t draw_end = EGUI_MIN(interior_right, work_x_end);
                for (egui_dim_t px = draw_start; px <= draw_end; px++)
                {
                    egui_color_t color = gradient_color_at_pixel(gradient, px - min_x, y - min_y, bbox_w, bbox_h, px, y);
                    egui_canvas_draw_point_limit(px, y, color, base_alpha);
                }
            }
        }

        /* Left AA zone */
        for (egui_dim_t px = aa_left_start; px <= aa_left_end; px++)
        {
            int32_t crossL = left_edx * (y - left_oy) - left_edy * (px - left_ox);
            egui_alpha_t aL = gradient_triangle_edge_alpha(crossL, left_sign, left_len);

            int32_t crossR = right_edx * (y - right_oy) - right_edy * (px - right_ox);
            egui_alpha_t aR = gradient_triangle_edge_alpha(crossR, right_sign, right_len);

            egui_alpha_t cov = (aL < aR) ? aL : aR;
            if (cov > 0)
            {
                egui_alpha_t mix = egui_color_alpha_mix(base_alpha, cov);
                if (mix > 0)
                {
                    egui_color_t color = gradient_color_at_pixel(gradient, px - min_x, y - min_y, bbox_w, bbox_h, px, y);
                    egui_canvas_draw_point_limit(px, y, color, mix);
                }
            }
        }

        /* Right AA zone */
        for (egui_dim_t px = aa_right_start; px <= aa_right_end; px++)
        {
            int32_t crossR = right_edx * (y - right_oy) - right_edy * (px - right_ox);
            egui_alpha_t aR = gradient_triangle_edge_alpha(crossR, right_sign, right_len);

            int32_t crossL = left_edx * (y - left_oy) - left_edy * (px - left_ox);
            egui_alpha_t aL = gradient_triangle_edge_alpha(crossL, left_sign, left_len);

            egui_alpha_t cov = (aL < aR) ? aL : aR;
            if (cov > 0)
            {
                egui_alpha_t mix = egui_color_alpha_mix(base_alpha, cov);
                if (mix > 0)
                {
                    egui_color_t color = gradient_color_at_pixel(gradient, px - min_x, y - min_y, bbox_w, bbox_h, px, y);
                    egui_canvas_draw_point_limit(px, y, color, mix);
                }
            }
        }
    }

#undef GRAD_TRI_AA_MARGIN
}

/* ========================== Ellipse Fill Gradient ========================== */

/* 64-bit integer square root for gradient magnitude in ellipse SDF. */
static uint64_t gradient_isqrt64(uint64_t n)
{
    if (n == 0)
    {
        return 0;
    }
    uint64_t root = 0;
    uint64_t bit = 1ULL << 62;
    while (bit > n)
    {
        bit >>= 2;
    }
    while (bit != 0)
    {
        if (n >= root + bit)
        {
            n -= root + bit;
            root = (root >> 1) + bit;
        }
        else
        {
            root >>= 1;
        }
        bit >>= 2;
    }
    return root;
}

/* SDF-based anti-aliased edge alpha for ellipse pixels. */
__EGUI_STATIC_INLINE__ egui_alpha_t gradient_ellipse_edge_alpha(int32_t dx, int32_t dy, int32_t rx_sq, int32_t ry_sq, int64_t rxry_sq)
{
    int64_t f_val = (int64_t)dx * dx * ry_sq + (int64_t)dy * dy * rx_sq - rxry_sq;

    int64_t gx = (int64_t)dx * ry_sq;
    int64_t gy = (int64_t)dy * rx_sq;
    uint64_t grad_half_sq = (uint64_t)(gx * gx + gy * gy);

    if (grad_half_sq == 0)
    {
        return (f_val <= 0) ? EGUI_ALPHA_100 : EGUI_ALPHA_0;
    }

    uint64_t grad_half = gradient_isqrt64(grad_half_sq);
    if (grad_half == 0)
    {
        grad_half = 1;
    }

    int64_t dist_q8 = f_val * 128 / (int64_t)grad_half;

    if (dist_q8 < -128)
    {
        return EGUI_ALPHA_100;
    }
    if (dist_q8 > 128)
    {
        return EGUI_ALPHA_0;
    }

    int32_t dist_clamped = (int32_t)dist_q8;
    int32_t alpha_val = ((128 - dist_clamped) * (int32_t)EGUI_ALPHA_100 + 128) >> 8;
    if (alpha_val < 0)
    {
        alpha_val = 0;
    }
    if (alpha_val > (int32_t)EGUI_ALPHA_100)
    {
        alpha_val = (int32_t)EGUI_ALPHA_100;
    }

    return (egui_alpha_t)alpha_val;
}

void egui_canvas_draw_ellipse_fill_gradient(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius_x, egui_dim_t radius_y, const egui_gradient_t *gradient)
{
    egui_canvas_t *self = &canvas_data;

    if (radius_x <= 0 || radius_y <= 0)
    {
        return;
    }

    egui_dim_t bbox_w = (radius_x << 1) + 1;
    egui_dim_t bbox_h = (radius_y << 1) + 1;
    egui_dim_t bbox_x = center_x - radius_x;
    egui_dim_t bbox_y = center_y - radius_y;

    EGUI_REGION_DEFINE(region, bbox_x, bbox_y, bbox_w, bbox_h);
    egui_region_t region_intersect;
    egui_region_intersect(&region, &self->base_view_work_region, &region_intersect);
    if (egui_region_is_empty(&region_intersect))
    {
        return;
    }

    egui_alpha_t base_alpha = egui_color_alpha_mix(self->alpha, gradient->alpha);
    if (base_alpha == 0)
    {
        return;
    }

    int32_t rx_sq = (int32_t)radius_x * radius_x;
    int32_t ry_sq = (int32_t)radius_y * radius_y;
    int64_t rxry_sq = (int64_t)rx_sq * ry_sq;

    egui_dim_t y_start = EGUI_MAX(-radius_y, region_intersect.location.y - center_y);
    egui_dim_t y_end = EGUI_MIN(radius_y, region_intersect.location.y + region_intersect.size.height - 1 - center_y);
    egui_dim_t x_start = EGUI_MAX(-radius_x, region_intersect.location.x - center_x);
    egui_dim_t x_end = EGUI_MIN(radius_x, region_intersect.location.x + region_intersect.size.width - 1 - center_x);

    for (egui_dim_t dy = y_start; dy <= y_end; dy++)
    {
        egui_dim_t abs_y = center_y + dy;
        egui_dim_t abs_dy = EGUI_ABS(dy);

        // Compute scanline x-boundary using isqrt
        int64_t dy_term = (int64_t)abs_dy * abs_dy * rx_sq;
        if (dy_term > rxry_sq)
        {
            continue;
        }
        int32_t dx_max_sq = (int32_t)((rxry_sq - dy_term) / ry_sq);
        int32_t dx_max = (int32_t)gradient_isqrt((uint32_t)dx_max_sq);

        // Extend scan range for smooth AA at poles (same as egui_canvas_ellipse.c)
        int32_t dx_scan = dx_max;
        if (abs_dy > 0)
        {
            int32_t abs_dy_adj = abs_dy - 1;
            int64_t dy_term_adj = (int64_t)abs_dy_adj * abs_dy_adj * rx_sq;
            if (dy_term_adj < rxry_sq)
            {
                int32_t dx_adj = (int32_t)gradient_isqrt((uint32_t)((rxry_sq - dy_term_adj) / ry_sq));
                if (dx_adj > dx_scan)
                {
                    dx_scan = dx_adj;
                }
            }
        }

        // Inner boundary by local gradient margin at scanline boundary.
        int32_t dx_inner = 0;
        if (dx_max > 0)
        {
            int64_t f_at_boundary = (int64_t)dx_max * dx_max * ry_sq + dy_term - rxry_sq;
            int64_t gx = (int64_t)dx_max * ry_sq;
            int64_t gy = (int64_t)abs_dy * rx_sq;
            uint64_t grad_half = gradient_isqrt64((uint64_t)(gx * gx + gy * gy));
            int64_t f_plus_grad = (int64_t)grad_half + f_at_boundary;

            if (f_plus_grad <= 0)
            {
                dx_inner = dx_max;
            }
            else
            {
                int64_t denom = 2 * gx;
                int32_t margin = (int32_t)((f_plus_grad + denom - 1) / denom);
                dx_inner = (dx_max > margin) ? (dx_max - margin) : 0;
            }
        }

        egui_dim_t scan_left = EGUI_MAX(-(egui_dim_t)(dx_scan + 1), x_start);
        egui_dim_t scan_right = EGUI_MIN((egui_dim_t)(dx_scan + 1), x_end);
        egui_dim_t inner_left = 1;
        egui_dim_t inner_right = 0;
        if (dx_inner > 0)
        {
            inner_left = EGUI_MAX(-(egui_dim_t)dx_inner, x_start);
            inner_right = EGUI_MIN((egui_dim_t)dx_inner, x_end);
        }

        if (gradient->type == EGUI_GRADIENT_TYPE_LINEAR_VERTICAL)
        {
            uint8_t t = gradient_linear_t(abs_y - bbox_y, bbox_h);
            egui_color_t color = egui_gradient_get_color(gradient->stops, gradient->stop_count, t);

            // Interior span with fillrect
            if (dx_inner > 0 && inner_left <= inner_right)
            {
                egui_canvas_draw_fillrect(center_x + inner_left, abs_y, inner_right - inner_left + 1, 1, color, base_alpha);
            }

            // Left edge pixels
            for (egui_dim_t dx = scan_left; dx < inner_left; dx++)
            {
                egui_dim_t abs_dx = EGUI_ABS(dx);
                egui_alpha_t cov = gradient_ellipse_edge_alpha((int32_t)abs_dx, (int32_t)abs_dy, rx_sq, ry_sq, rxry_sq);
                if (cov > 0)
                {
                    egui_alpha_t mix = egui_color_alpha_mix(base_alpha, cov);
                    if (mix > 0)
                    {
                        egui_canvas_draw_point_limit(center_x + dx, abs_y, color, mix);
                    }
                }
            }

            // Right edge pixels
            for (egui_dim_t dx = inner_right + 1; dx <= scan_right; dx++)
            {
                egui_dim_t abs_dx = EGUI_ABS(dx);
                egui_alpha_t cov = gradient_ellipse_edge_alpha((int32_t)abs_dx, (int32_t)abs_dy, rx_sq, ry_sq, rxry_sq);
                if (cov > 0)
                {
                    egui_alpha_t mix = egui_color_alpha_mix(base_alpha, cov);
                    if (mix > 0)
                    {
                        egui_canvas_draw_point_limit(center_x + dx, abs_y, color, mix);
                    }
                }
            }
        }
        else
        {
            // Horizontal or radial: per-pixel color, but still skip outside ellipse
            for (egui_dim_t dx = scan_left; dx <= scan_right; dx++)
            {
                egui_dim_t abs_dx = EGUI_ABS(dx);
                egui_alpha_t cov = gradient_ellipse_edge_alpha((int32_t)abs_dx, (int32_t)abs_dy, rx_sq, ry_sq, rxry_sq);
                if (cov == 0)
                {
                    continue;
                }

                egui_color_t color = gradient_color_at_pixel(gradient, center_x + dx - bbox_x, abs_y - bbox_y, bbox_w, bbox_h, center_x + dx, abs_y);
                egui_alpha_t mix = egui_color_alpha_mix(base_alpha, cov);
                if (mix > 0)
                {
                    egui_canvas_draw_point_limit(center_x + dx, abs_y, color, mix);
                }
            }
        }
    }
}

/* ========================== Polygon Fill Gradient ========================== */

#ifndef EGUI_CANVAS_POLYGON_MAX_VERTICES
#define EGUI_CANVAS_POLYGON_MAX_VERTICES 16
#endif

/**
 * @brief Intersection record: x position + originating edge index.
 */
/**
 * @brief Integer square root for edge length computation.
 */
static uint32_t gradient_polygon_isqrt(uint32_t n)
{
    if (n == 0)
    {
        return 0;
    }
    uint32_t root = 0;
    uint32_t bit = 1UL << 30;
    while (bit > n)
    {
        bit >>= 2;
    }
    while (bit != 0)
    {
        if (n >= root + bit)
        {
            n -= root + bit;
            root = (root >> 1) + bit;
        }
        else
        {
            root >>= 1;
        }
        bit >>= 2;
    }
    return root;
}

static void gradient_sort_polygon_isects(egui_float_t *x_arr, uint8_t *edge_idx_arr, int count)
{
    for (int i = 1; i < count; i++)
    {
        egui_float_t key_x = x_arr[i];
        uint8_t key_edge_idx = edge_idx_arr[i];
        int j = i - 1;
        while (j >= 0 && x_arr[j] > key_x)
        {
            x_arr[j + 1] = x_arr[j];
            edge_idx_arr[j + 1] = edge_idx_arr[j];
            j--;
        }
        x_arr[j + 1] = key_x;
        edge_idx_arr[j + 1] = key_edge_idx;
    }
}

/**
 * @brief SDF-based edge alpha with 1.5-pixel-wide AA zone.
 */
__EGUI_STATIC_INLINE__ egui_alpha_t gradient_polygon_edge_alpha(int32_t cross, int32_t sign, uint32_t len)
{
    int32_t d_q8 = (int32_t)(sign * ((int64_t)cross * 256 / (int32_t)len));

    if (d_q8 <= -192)
    {
        return EGUI_ALPHA_0;
    }
    if (d_q8 >= 192)
    {
        return EGUI_ALPHA_100;
    }

    int32_t alpha_val = ((d_q8 + 192) * 170 + 128) >> 8;
    if (alpha_val < 0)
    {
        alpha_val = 0;
    }
    if (alpha_val > (int32_t)EGUI_ALPHA_100)
    {
        alpha_val = (int32_t)EGUI_ALPHA_100;
    }
    return (egui_alpha_t)alpha_val;
}

void egui_canvas_draw_polygon_fill_gradient(const egui_dim_t *points, uint8_t count, const egui_gradient_t *gradient)
{
    egui_canvas_t *self = &canvas_data;

    if (count < 3 || count > EGUI_CANVAS_POLYGON_MAX_VERTICES)
    {
        return;
    }

    /* Bounding box */
    egui_dim_t min_x = points[0], max_x = points[0];
    egui_dim_t min_y = points[1], max_y = points[1];
    for (uint8_t i = 1; i < count; i++)
    {
        egui_dim_t px = points[i * 2];
        egui_dim_t py = points[i * 2 + 1];
        if (px < min_x)
        {
            min_x = px;
        }
        if (px > max_x)
        {
            max_x = px;
        }
        if (py < min_y)
        {
            min_y = py;
        }
        if (py > max_y)
        {
            max_y = py;
        }
    }

    EGUI_REGION_DEFINE(region, min_x, min_y, max_x - min_x + 1, max_y - min_y + 1);
    if (!egui_region_is_intersect(&region, &self->base_view_work_region))
    {
        return;
    }

    egui_alpha_t base_alpha = egui_color_alpha_mix(self->alpha, gradient->alpha);
    if (base_alpha == 0)
    {
        return;
    }

    egui_dim_t bbox_w = max_x - min_x + 1;
    egui_dim_t bbox_h = max_y - min_y + 1;

    /* Precompute edge vectors, lengths, and inside-direction signs */
    int32_t edx[EGUI_CANVAS_POLYGON_MAX_VERTICES];
    int32_t edy[EGUI_CANVAS_POLYGON_MAX_VERTICES];
    uint16_t elen[EGUI_CANVAS_POLYGON_MAX_VERTICES];
    int8_t esign[EGUI_CANVAS_POLYGON_MAX_VERTICES];

    int32_t cx_sum = 0, cy_sum = 0;
    for (uint8_t i = 0; i < count; i++)
    {
        cx_sum += points[i * 2];
        cy_sum += points[i * 2 + 1];
    }

    for (uint8_t i = 0; i < count; i++)
    {
        uint8_t j = (i + 1) % count;
        edx[i] = points[j * 2] - points[i * 2];
        edy[i] = points[j * 2 + 1] - points[i * 2 + 1];
        uint32_t len_sq = (uint32_t)(edx[i] * edx[i] + edy[i] * edy[i]);
        elen[i] = gradient_polygon_isqrt(len_sq);
        if (elen[i] == 0)
        {
            elen[i] = 1;
        }

        int32_t cross_c = edx[i] * (cy_sum - (int32_t)count * points[i * 2 + 1]) - edy[i] * (cx_sum - (int32_t)count * points[i * 2]);
        esign[i] = (cross_c >= 0) ? 1 : -1;
    }

    egui_dim_t work_y_start = self->base_view_work_region.location.y;
    egui_dim_t work_y_end = work_y_start + self->base_view_work_region.size.height;
    egui_dim_t work_x_start = self->base_view_work_region.location.x;
    egui_dim_t work_x_end = work_x_start + self->base_view_work_region.size.width - 1;
    egui_dim_t scan_y_start = EGUI_MAX(min_y, work_y_start);
    egui_dim_t scan_y_end = EGUI_MIN(max_y, work_y_end - 1);

    egui_float_t intersection_x[EGUI_CANVAS_POLYGON_MAX_VERTICES];
    uint8_t intersection_edge_idx[EGUI_CANVAS_POLYGON_MAX_VERTICES];

#define GRAD_POLY_AA_MARGIN 2

    for (egui_dim_t y = scan_y_start; y <= scan_y_end; y++)
    {
        int intersection_count = 0;

        for (uint8_t i = 0; i < count; i++)
        {
            uint8_t j = (i + 1) % count;
            egui_dim_t y0 = points[i * 2 + 1];
            egui_dim_t y1 = points[j * 2 + 1];
            egui_dim_t x0 = points[i * 2];
            egui_dim_t x1 = points[j * 2];

            if (y0 == y1)
            {
                continue;
            }

            egui_dim_t y_min = EGUI_MIN(y0, y1);
            egui_dim_t y_max = EGUI_MAX(y0, y1);
            if (y < y_min || y >= y_max)
            {
                continue;
            }

            egui_float_t ix = EGUI_FLOAT_VALUE_INT(x0) +
                              EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(x1 - x0), EGUI_FLOAT_DIV(EGUI_FLOAT_VALUE_INT(y - y0), EGUI_FLOAT_VALUE_INT(y1 - y0)));

            if (intersection_count < EGUI_CANVAS_POLYGON_MAX_VERTICES)
            {
                intersection_x[intersection_count] = ix;
                intersection_edge_idx[intersection_count] = i;
                intersection_count++;
            }
        }

        if (intersection_count < 2)
        {
            continue;
        }

        gradient_sort_polygon_isects(intersection_x, intersection_edge_idx, intersection_count);

        for (int k = 0; k + 1 < intersection_count; k += 2)
        {
            egui_float_t left_fx = intersection_x[k];
            egui_float_t right_fx = intersection_x[k + 1];
            uint8_t left_edge = intersection_edge_idx[k];
            uint8_t right_edge = intersection_edge_idx[k + 1];

            egui_dim_t ix_left = EGUI_FLOAT_INT_PART(left_fx);
            egui_dim_t ix_right = EGUI_FLOAT_INT_PART(right_fx);

            if (ix_right + 1 < work_x_start || ix_left - 1 > work_x_end)
            {
                continue;
            }

            egui_dim_t le_ox = points[left_edge * 2];
            egui_dim_t le_oy = points[left_edge * 2 + 1];
            egui_dim_t re_ox = points[right_edge * 2];
            egui_dim_t re_oy = points[right_edge * 2 + 1];

            /* AA zone boundaries */
            egui_dim_t aa_left_start = EGUI_MAX(ix_left - 1, work_x_start);
            egui_dim_t aa_left_end = EGUI_MIN(ix_left + GRAD_POLY_AA_MARGIN - 1, work_x_end);
            egui_dim_t aa_right_start = EGUI_MAX(ix_right - GRAD_POLY_AA_MARGIN + 1, work_x_start);
            egui_dim_t aa_right_end = EGUI_MIN(ix_right + 1, work_x_end);

            egui_dim_t interior_left = aa_left_end + 1;
            egui_dim_t interior_right = aa_right_start - 1;

            /* Thin span: all pixels need per-pixel SDF */
            if (interior_left > interior_right)
            {
                egui_dim_t span_start = EGUI_MAX(ix_left - 1, work_x_start);
                egui_dim_t span_end = EGUI_MIN(ix_right + 1, work_x_end);

                for (egui_dim_t px = span_start; px <= span_end; px++)
                {
                    int32_t crossL = edx[left_edge] * (y - le_oy) - edy[left_edge] * (px - le_ox);
                    egui_alpha_t aL = gradient_polygon_edge_alpha(crossL, esign[left_edge], elen[left_edge]);

                    int32_t crossR = edx[right_edge] * (y - re_oy) - edy[right_edge] * (px - re_ox);
                    egui_alpha_t aR = gradient_polygon_edge_alpha(crossR, esign[right_edge], elen[right_edge]);

                    egui_alpha_t cov = (aL < aR) ? aL : aR;
                    if (cov > 0)
                    {
                        egui_color_t color = gradient_color_at_pixel(gradient, px - min_x, y - min_y, bbox_w, bbox_h, px, y);
                        egui_alpha_t mix = egui_color_alpha_mix(base_alpha, cov);
                        if (mix > 0)
                        {
                            egui_canvas_draw_point_limit(px, y, color, mix);
                        }
                    }
                }
                continue;
            }

            /* Interior span */
            if (interior_left <= interior_right)
            {
                egui_dim_t len = interior_right - interior_left + 1;
                if (gradient->type == EGUI_GRADIENT_TYPE_LINEAR_VERTICAL)
                {
                    uint8_t t = gradient_linear_t(y - min_y, bbox_h);
                    egui_color_t color = egui_gradient_get_color(gradient->stops, gradient->stop_count, t);
                    egui_canvas_draw_hline(interior_left, y, len, color, base_alpha);
                }
                else
                {
                    egui_dim_t draw_start = EGUI_MAX(interior_left, work_x_start);
                    egui_dim_t draw_end = EGUI_MIN(interior_right, work_x_end);
                    for (egui_dim_t px = draw_start; px <= draw_end; px++)
                    {
                        egui_color_t color = gradient_color_at_pixel(gradient, px - min_x, y - min_y, bbox_w, bbox_h, px, y);
                        egui_canvas_draw_point_limit(px, y, color, base_alpha);
                    }
                }
            }

            /* Left AA zone */
            for (egui_dim_t px = aa_left_start; px <= aa_left_end; px++)
            {
                int32_t crossL = edx[left_edge] * (y - le_oy) - edy[left_edge] * (px - le_ox);
                egui_alpha_t aL = gradient_polygon_edge_alpha(crossL, esign[left_edge], elen[left_edge]);

                int32_t crossR = edx[right_edge] * (y - re_oy) - edy[right_edge] * (px - re_ox);
                egui_alpha_t aR = gradient_polygon_edge_alpha(crossR, esign[right_edge], elen[right_edge]);

                egui_alpha_t cov = (aL < aR) ? aL : aR;
                if (cov > 0)
                {
                    egui_color_t color = gradient_color_at_pixel(gradient, px - min_x, y - min_y, bbox_w, bbox_h, px, y);
                    egui_alpha_t mix = egui_color_alpha_mix(base_alpha, cov);
                    if (mix > 0)
                    {
                        egui_canvas_draw_point_limit(px, y, color, mix);
                    }
                }
            }

            /* Right AA zone */
            for (egui_dim_t px = aa_right_start; px <= aa_right_end; px++)
            {
                int32_t crossR = edx[right_edge] * (y - re_oy) - edy[right_edge] * (px - re_ox);
                egui_alpha_t aR = gradient_polygon_edge_alpha(crossR, esign[right_edge], elen[right_edge]);

                int32_t crossL = edx[left_edge] * (y - le_oy) - edy[left_edge] * (px - le_ox);
                egui_alpha_t aL = gradient_polygon_edge_alpha(crossL, esign[left_edge], elen[left_edge]);

                egui_alpha_t cov = (aL < aR) ? aL : aR;
                if (cov > 0)
                {
                    egui_color_t color = gradient_color_at_pixel(gradient, px - min_x, y - min_y, bbox_w, bbox_h, px, y);
                    egui_alpha_t mix = egui_color_alpha_mix(base_alpha, cov);
                    if (mix > 0)
                    {
                        egui_canvas_draw_point_limit(px, y, color, mix);
                    }
                }
            }
        }
    }

#undef GRAD_POLY_AA_MARGIN
}

/* ========================== Public gradient color utility ========================== */

egui_color_t egui_gradient_color_at_pos(const egui_gradient_t *gradient, egui_dim_t px, egui_dim_t py, egui_dim_t w, egui_dim_t h)
{
    return gradient_color_at(gradient, px, py, w, h);
}

/* ========================== Circle Ring (Annulus) Fill Gradient ========================== */

/* Compute ring coverage for a pixel at absolute distance squared d_sq from center.
 * outer_cov: coverage inside outer circle (with AA).
 * inner_inv_cov: inverse coverage of inner circle (255 = fully outside inner, 0 = fully inside inner).
 * Returns combined ring alpha. */
static egui_alpha_t gradient_ring_alpha(int32_t d_sq, int32_t outer_r_inner_sq, int32_t outer_r_outer_sq, int32_t inner_r_inner_sq, int32_t inner_r_outer_sq,
                                        uint32_t inv_outer_r, uint32_t inv_inner_r)
{
    /* Outside outer circle */
    if (d_sq > outer_r_outer_sq)
    {
        return 0;
    }
    /* Inside inner circle */
    if (d_sq < inner_r_inner_sq)
    {
        return 0;
    }

    /* Compute outer coverage using SDF reciprocal */
    egui_alpha_t outer_cov;
    if (d_sq <= outer_r_inner_sq)
    {
        outer_cov = EGUI_ALPHA_100;
    }
    else
    {
        uint32_t c = ((uint32_t)(outer_r_outer_sq - d_sq) * inv_outer_r) >> 8;
        outer_cov = (egui_alpha_t)(c > 255 ? 255 : c);
    }

    if (outer_cov == 0)
    {
        return 0;
    }

    /* Compute inner coverage using SDF reciprocal */
    egui_alpha_t inner_cov;
    if (d_sq >= inner_r_outer_sq)
    {
        inner_cov = 0; /* fully outside inner circle */
    }
    else if (d_sq <= inner_r_inner_sq)
    {
        inner_cov = EGUI_ALPHA_100; /* fully inside inner circle */
    }
    else
    {
        uint32_t c = ((uint32_t)(inner_r_outer_sq - d_sq) * inv_inner_r) >> 8;
        inner_cov = (egui_alpha_t)(c > 255 ? 255 : c);
    }

    /* Ring coverage = outside inner AND inside outer */
    egui_alpha_t ring_cov = (egui_alpha_t)(((uint16_t)outer_cov * (uint16_t)(EGUI_ALPHA_100 - inner_cov) + 127) / 255);
    return ring_cov;
}

void egui_canvas_draw_circle_ring_fill_gradient(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t outer_r, egui_dim_t inner_r,
                                                const egui_gradient_t *gradient)
{
    egui_canvas_t *self = &canvas_data;

    if (outer_r <= 0 || inner_r >= outer_r)
    {
        return;
    }
    if (inner_r < 0)
    {
        inner_r = 0;
    }

    egui_dim_t bbox_size = (outer_r << 1) + 1;
    egui_dim_t bbox_x = center_x - outer_r;
    egui_dim_t bbox_y = center_y - outer_r;

    EGUI_REGION_DEFINE(region, bbox_x, bbox_y, bbox_size, bbox_size);
    egui_region_t ri;
    egui_region_intersect(&region, &self->base_view_work_region, &ri);
    if (egui_region_is_empty(&ri))
    {
        return;
    }

    egui_alpha_t base_alpha = egui_color_alpha_mix(self->alpha, gradient->alpha);
    if (base_alpha == 0)
    {
        return;
    }

    int32_t outer_r_inner_sq = (int32_t)(outer_r - 1) * (outer_r - 1);
    int32_t outer_r_outer_sq = (int32_t)(outer_r + 1) * (outer_r + 1);

    int32_t inner_r_inner_sq = (inner_r > 1) ? (int32_t)(inner_r - 1) * (inner_r - 1) : 0;
    int32_t inner_r_outer_sq = (int32_t)(inner_r + 1) * (inner_r + 1);

    uint32_t inv_outer_r = (outer_r_outer_sq > outer_r_inner_sq) ? ((255U << 8) / (uint32_t)(outer_r_outer_sq - outer_r_inner_sq)) : 0;
    uint32_t inv_inner_r = (inner_r_outer_sq > inner_r_inner_sq) ? ((255U << 8) / (uint32_t)(inner_r_outer_sq - inner_r_inner_sq)) : 0;

    egui_dim_t y_start = ri.location.y;
    egui_dim_t y_end = ri.location.y + ri.size.height;
    egui_dim_t x_start = ri.location.x;
    egui_dim_t x_end = ri.location.x + ri.size.width;

    /* Direct PFB write setup — skip draw_point() overhead when no mask */
    int use_direct = (self->mask == NULL) ? 1 : 0;
    egui_dim_t pfb_w = 0, pfb_ox = 0, pfb_oy = 0;
    if (use_direct)
    {
        pfb_w = self->pfb_region.size.width;
        pfb_ox = self->pfb_location_in_base_view.x;
        pfb_oy = self->pfb_location_in_base_view.y;
    }

    for (egui_dim_t y = y_start; y < y_end; y++)
    {
        int32_t dy = (int32_t)(y - center_y);
        int32_t dy_sq = dy * dy;
        if (dy_sq > outer_r_outer_sq)
        {
            continue;
        }

        /* Narrow x range to outer circle extent on this scanline */
        int32_t x_outer_half = (int32_t)gradient_isqrt((uint32_t)(outer_r_outer_sq - dy_sq));
        egui_dim_t scan_xs = EGUI_MAX(x_start, (egui_dim_t)(center_x - x_outer_half));
        egui_dim_t scan_xe = EGUI_MIN(x_end, (egui_dim_t)(center_x + x_outer_half + 1));

        /* Precompute row base pointer for direct PFB path */
        egui_color_t *row_base = NULL;
        if (use_direct)
        {
            row_base = &((egui_color_t *)self->pfb)[(y - pfb_oy) * pfb_w - pfb_ox];
        }

        /* Precompute row color for LINEAR_VERTICAL (same for whole row) */
        int lv_precomputed = 0;
        egui_color_t lv_color;
        if (gradient->type == EGUI_GRADIENT_TYPE_LINEAR_VERTICAL)
        {
            uint8_t t = gradient_linear_t(y - bbox_y, bbox_size);
            lv_color = egui_gradient_get_color(gradient->stops, gradient->stop_count, t);
            lv_precomputed = 1;
        }

        /* Inner circle x-boundary: skip pixels fully inside inner circle */
        egui_dim_t inner_skip_lo = scan_xs;
        egui_dim_t inner_skip_hi = scan_xs - 1; /* empty range by default */
        if (dy_sq < inner_r_inner_sq)
        {
            int32_t x_inner_half = (int32_t)gradient_isqrt((uint32_t)(inner_r_inner_sq - dy_sq));
            inner_skip_lo = (egui_dim_t)(center_x - x_inner_half);
            inner_skip_hi = (egui_dim_t)(center_x + x_inner_half);
        }

        for (egui_dim_t x = scan_xs; x < scan_xe; x++)
        {
            /* Jump over inner gap (d_sq < inner_r_inner_sq → ring_cov = 0) */
            if (x >= inner_skip_lo && x <= inner_skip_hi)
            {
                x = inner_skip_hi;
                continue;
            }

            int32_t dx = (int32_t)(x - center_x);
            int32_t d_sq = dx * dx + dy_sq;

            egui_alpha_t ring_cov = gradient_ring_alpha(d_sq, outer_r_inner_sq, outer_r_outer_sq, inner_r_inner_sq, inner_r_outer_sq, inv_outer_r, inv_inner_r);
            if (ring_cov == 0)
            {
                continue;
            }

            egui_color_t color;
            if (lv_precomputed)
            {
                color = lv_color;
            }
            else
            {
                color = gradient_color_at_pixel(gradient, x - bbox_x, y - bbox_y, bbox_size, bbox_size, x, y);
            }

            egui_alpha_t m = egui_color_alpha_mix(base_alpha, ring_cov);
            if (m > 0)
            {
                if (use_direct)
                {
                    egui_color_t *back_color = &row_base[x];
                    if (m == EGUI_ALPHA_100)
                    {
                        *back_color = color;
                    }
                    else
                    {
                        egui_rgb_mix_ptr(back_color, &color, back_color, m);
                    }
                }
                else
                {
                    egui_canvas_draw_point(x, y, color, m);
                }
            }
        }
    }
}

/* ========================== Rectangle Ring Fill Gradient ========================== */

void egui_canvas_draw_rectangle_ring_fill_gradient(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_dim_t stroke_w,
                                                   const egui_gradient_t *gradient)
{
    egui_canvas_t *self = &canvas_data;

    if (width <= 0 || height <= 0 || stroke_w <= 0)
    {
        return;
    }

    EGUI_REGION_DEFINE(region, x, y, width, height);
    if (!egui_region_is_intersect(&region, &self->base_view_work_region))
    {
        return;
    }

    egui_alpha_t base_alpha = egui_color_alpha_mix(self->alpha, gradient->alpha);
    if (base_alpha == 0)
    {
        return;
    }

    /* Clamp stroke width */
    if (stroke_w > (width >> 1))
    {
        stroke_w = width >> 1;
    }
    if (stroke_w > (height >> 1))
    {
        stroke_w = height >> 1;
    }

    /* Clip to PFB work region */
    egui_dim_t vis_y_start = EGUI_MAX(0, (egui_dim_t)(self->base_view_work_region.location.y - y));
    egui_dim_t vis_y_end = EGUI_MIN(height, (egui_dim_t)(self->base_view_work_region.location.y + self->base_view_work_region.size.height - y));

    for (egui_dim_t row = vis_y_start; row < vis_y_end; row++)
    {
        /* Determine if row is in top/bottom band or side strips */
        int in_top = (row < stroke_w);
        int in_bottom = (row >= height - stroke_w);

        int32_t row_abs = y + (int32_t)row;

        if (gradient->type == EGUI_GRADIENT_TYPE_LINEAR_VERTICAL)
        {
            uint8_t t = gradient_linear_t(row, height);
            egui_color_t color = egui_gradient_get_color(gradient->stops, gradient->stop_count, t);

            if (in_top || in_bottom)
            {
                /* Full row */
                egui_canvas_draw_fillrect(x, row_abs, width, 1, color, base_alpha);
            }
            else
            {
                /* Left strip */
                egui_canvas_draw_fillrect(x, row_abs, stroke_w, 1, color, base_alpha);
                /* Right strip */
                egui_canvas_draw_fillrect(x + width - stroke_w, row_abs, stroke_w, 1, color, base_alpha);
            }
        }
        else
        {
            if (in_top || in_bottom)
            {
                for (egui_dim_t col = 0; col < width; col++)
                {
                    egui_color_t color = gradient_color_at_pixel(gradient, col, row, width, height, x + col, row_abs);
                    egui_canvas_draw_point(x + col, row_abs, color, base_alpha);
                }
            }
            else
            {
                /* Left strip */
                for (egui_dim_t col = 0; col < stroke_w; col++)
                {
                    egui_color_t color = gradient_color_at_pixel(gradient, col, row, width, height, x + col, row_abs);
                    egui_canvas_draw_point(x + col, row_abs, color, base_alpha);
                }
                /* Right strip */
                for (egui_dim_t col = width - stroke_w; col < width; col++)
                {
                    egui_color_t color = gradient_color_at_pixel(gradient, col, row, width, height, x + col, row_abs);
                    egui_canvas_draw_point(x + col, row_abs, color, base_alpha);
                }
            }
        }
    }
}

/* ========================== Round Rectangle Ring Fill Gradient ========================== */

void egui_canvas_draw_round_rectangle_ring_fill_gradient(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_dim_t stroke_w,
                                                         egui_dim_t radius, const egui_gradient_t *gradient)
{
    egui_canvas_t *self = &canvas_data;

    if (width <= 0 || height <= 0 || stroke_w <= 0)
    {
        return;
    }

    EGUI_REGION_DEFINE(region, x, y, width, height);
    if (!egui_region_is_intersect(&region, &self->base_view_work_region))
    {
        return;
    }

    egui_alpha_t base_alpha = egui_color_alpha_mix(self->alpha, gradient->alpha);
    if (base_alpha == 0)
    {
        return;
    }

    /* Clamp radius */
    egui_dim_t half_w = width >> 1;
    egui_dim_t half_h = height >> 1;
    if (radius >= half_w)
    {
        radius = half_w - 1;
    }
    if (radius >= half_h)
    {
        radius = half_h - 1;
    }

    /* Clamp stroke */
    if (stroke_w > half_w)
    {
        stroke_w = half_w;
    }
    if (stroke_w > half_h)
    {
        stroke_w = half_h;
    }

    egui_dim_t inner_radius = (stroke_w <= radius) ? (radius - stroke_w) : 0;

    /* Outer and inner circle AA parameters */
    int32_t outer_r_inner_sq = (radius > 1) ? (int32_t)(radius - 1) * (radius - 1) : 0;
    int32_t outer_r_outer_sq = (int32_t)(radius + 1) * (radius + 1);

    int32_t inner_r_inner_sq = (inner_radius > 1) ? (int32_t)(inner_radius - 1) * (inner_radius - 1) : 0;
    int32_t inner_r_outer_sq = (int32_t)(inner_radius + 1) * (inner_radius + 1);

    uint32_t inv_outer_r = (outer_r_outer_sq > outer_r_inner_sq) ? ((255U << 8) / (uint32_t)(outer_r_outer_sq - outer_r_inner_sq)) : 0;
    uint32_t inv_inner_r = (inner_r_outer_sq > inner_r_inner_sq) ? ((255U << 8) / (uint32_t)(inner_r_outer_sq - inner_r_inner_sq)) : 0;

    /* Clip to PFB work region */
    egui_dim_t rr_vis_y_start = EGUI_MAX(0, (egui_dim_t)(self->base_view_work_region.location.y - y));
    egui_dim_t rr_vis_y_end = EGUI_MIN(height, (egui_dim_t)(self->base_view_work_region.location.y + self->base_view_work_region.size.height - y));

    /* PFB x-clipping to avoid iterating invisible columns */
    egui_dim_t rr_vis_x_start = self->base_view_work_region.location.x;
    egui_dim_t rr_vis_x_end = self->base_view_work_region.location.x + self->base_view_work_region.size.width;

    /* Direct PFB write setup */
    int use_direct = (self->mask == NULL) ? 1 : 0;
    egui_dim_t pfb_w = self->pfb_region.size.width;
    egui_dim_t pfb_ox = self->pfb_location_in_base_view.x;
    egui_dim_t pfb_oy = self->pfb_location_in_base_view.y;
    egui_alpha_t eff_alpha = egui_color_alpha_mix(self->alpha, base_alpha);

    for (egui_dim_t row = rr_vis_y_start; row < rr_vis_y_end; row++)
    {
        egui_dim_t abs_row = y + row;

        /* Which corner row? Include the tangent row so circle AA is applied at the arc-to-straight join. */
        int in_top_corner = (row <= radius);
        int in_bottom_corner = (row >= height - 1 - radius);

        /* Outer boundary for this row (from round rect geometry) */
        egui_dim_t out_left_inset = 0;
        egui_dim_t out_right_inset = 0;
        egui_dim_t corner_dy = 0;

        if (in_top_corner)
        {
            corner_dy = radius - row;
            out_left_inset = round_rect_corner_inset(corner_dy, radius);
            out_right_inset = out_left_inset;
        }
        else if (in_bottom_corner)
        {
            corner_dy = radius - (height - 1 - row);
            out_left_inset = round_rect_corner_inset(corner_dy, radius);
            out_right_inset = out_left_inset;
        }

        /* Inner boundary from inner round rect geometry */
        egui_dim_t in_left_inset = 0;
        egui_dim_t in_right_inset = 0;

        if (inner_radius > 0)
        {
            /* The inner rect has top/bottom extent at row = stroke_w .. height-1-stroke_w
             * Corner centers are at (x+radius, y+radius) and mirrored */
            int inner_row = (int)row - (int)stroke_w; /* row within inner rect */
            int inner_height = (int)height - 2 * (int)stroke_w;
            if (inner_row >= 0 && inner_row < inner_height)
            {
                int in_top_c = (inner_row < (int)inner_radius);
                int in_bot_c = (inner_row >= inner_height - (int)inner_radius);
                egui_dim_t icd = 0;
                if (in_top_c)
                {
                    icd = inner_radius - inner_row;
                    in_left_inset = round_rect_corner_inset(icd, inner_radius);
                    in_right_inset = in_left_inset;
                }
                else if (in_bot_c)
                {
                    icd = inner_radius - (inner_height - 1 - inner_row);
                    in_left_inset = round_rect_corner_inset(icd, inner_radius);
                    in_right_inset = in_left_inset;
                }

                /* Convert inner rect offsets to outer rect coordinate space */
                in_left_inset += stroke_w;
                in_right_inset += stroke_w;
            }
            else
            {
                /* Row is in the stroke band (top/bottom) - full row is drawn */
                in_left_inset = width; /* no inner opening */
                in_right_inset = width;
            }
        }
        else
        {
            in_left_inset = width;
            in_right_inset = width;
        }

        /* Extend outer boundary by 1 pixel for corner AA */
        egui_dim_t out_row_left = x + out_left_inset;
        egui_dim_t out_row_right = x + width - 1 - out_right_inset;
        if ((in_top_corner || in_bottom_corner) && radius > 0)
        {
            if (out_left_inset > 0)
            {
                out_row_left = x + out_left_inset - 1;
            }
            if (out_right_inset > 0)
            {
                out_row_right = x + width - out_right_inset;
            }
        }

        egui_dim_t in_row_left = (in_left_inset < width) ? (x + in_left_inset) : (out_row_right + 1);
        egui_dim_t in_row_right = (in_right_inset < width) ? (x + width - 1 - in_right_inset) : (out_row_left - 1);

        if (out_row_left > out_row_right)
        {
            continue;
        }

        /* Corner center relative to this row */
        egui_dim_t corner_center_x_left = x + radius;
        egui_dim_t corner_center_x_right = x + width - 1 - radius;

        /* Clip column range to PFB x-range to avoid iterating invisible pixels */
        egui_dim_t clip_left = EGUI_MAX(out_row_left, rr_vis_x_start);
        egui_dim_t clip_right = EGUI_MIN(out_row_right, rr_vis_x_end - 1);

        /* Split iteration into left band + right band to skip inner gap.
         * Left band: [clip_left, min(in_row_left-1, clip_right)]
         * Right band: [max(in_row_right+1, clip_left), clip_right]
         * For corner rows where ring_cov varies, process all clipped pixels. */
        int is_corner_row = (in_top_corner || in_bottom_corner);

        /* Pre-compute gradient color once per row for linear vertical */
        egui_color_t row_color = {0};
        int row_color_valid = 0;
        if (gradient->type == EGUI_GRADIENT_TYPE_LINEAR_VERTICAL)
        {
            uint8_t t = gradient_linear_t(row, height);
            row_color = egui_gradient_get_color(gradient->stops, gradient->stop_count, t);
            row_color_valid = 1;
        }

        /* Direct PFB row pointer */
        egui_color_t *dst_row = NULL;
        if (use_direct)
        {
            dst_row = (egui_color_t *)&self->pfb[(abs_row - pfb_oy) * pfb_w - pfb_ox];
        }

/* Emit one ring pixel: computes ring coverage, gradient color, and writes to PFB or via draw_point */
#define RR_RING_EMIT(col_val, cov_val)                                                                                                                         \
    do                                                                                                                                                         \
    {                                                                                                                                                          \
        if ((cov_val) > 0)                                                                                                                                     \
        {                                                                                                                                                      \
            egui_color_t _c = row_color_valid ? row_color : gradient_color_at_pixel(gradient, (col_val) - x, row, width, height, (col_val), abs_row);          \
            egui_alpha_t _m = egui_color_alpha_mix(base_alpha, (cov_val));                                                                                     \
            if (_m > 0)                                                                                                                                        \
            {                                                                                                                                                  \
                if (use_direct)                                                                                                                                \
                {                                                                                                                                              \
                    egui_alpha_t _ma = egui_color_alpha_mix(eff_alpha, (cov_val));                                                                             \
                    if (_ma == EGUI_ALPHA_100)                                                                                                                 \
                    {                                                                                                                                          \
                        dst_row[(col_val)] = _c;                                                                                                               \
                    }                                                                                                                                          \
                    else if (_ma > 0)                                                                                                                          \
                    {                                                                                                                                          \
                        egui_rgb_mix_ptr(&dst_row[(col_val)], &_c, &dst_row[(col_val)], _ma);                                                                  \
                    }                                                                                                                                          \
                }                                                                                                                                              \
                else                                                                                                                                           \
                {                                                                                                                                              \
                    egui_canvas_draw_point((col_val), abs_row, _c, _m);                                                                                        \
                }                                                                                                                                              \
            }                                                                                                                                                  \
        }                                                                                                                                                      \
    } while (0)

        if (is_corner_row)
        {
            /* Corner rows: need per-pixel ring_alpha computation */
            for (egui_dim_t col = clip_left; col <= clip_right; col++)
            {
                egui_alpha_t ring_cov;

                int in_left_corner_zone = (col < corner_center_x_left);
                int in_right_corner_zone = (col > corner_center_x_right);

                if (in_left_corner_zone || in_right_corner_zone)
                {
                    egui_dim_t cx = in_left_corner_zone ? corner_center_x_left : corner_center_x_right;
                    int32_t dx = (int32_t)(col - cx);
                    int32_t dy_c;
                    if (in_top_corner)
                    {
                        dy_c = (int32_t)row - (int32_t)radius;
                    }
                    else
                    {
                        dy_c = (int32_t)row - (int32_t)(height - 1 - radius);
                    }
                    int32_t d_sq = dx * dx + dy_c * dy_c;

                    ring_cov = gradient_ring_alpha(d_sq, outer_r_inner_sq, outer_r_outer_sq, inner_r_inner_sq, inner_r_outer_sq, inv_outer_r, inv_inner_r);
                }
                else
                {
                    if (col >= in_row_left && col <= in_row_right)
                    {
                        continue;
                    }
                    ring_cov = EGUI_ALPHA_100;
                }

                RR_RING_EMIT(col, ring_cov);
            }
        }
        else
        {
            /* Straight rows: split into left band + right band, skip inner gap */
            egui_dim_t left_end = EGUI_MIN(in_row_left - 1, clip_right);
            egui_dim_t right_begin = EGUI_MAX(in_row_right + 1, clip_left);

            /* Left band */
            if (clip_left <= left_end)
            {
                if (row_color_valid && use_direct)
                {
                    /* Batch write: all pixels have same color and full coverage */
                    for (egui_dim_t col = clip_left; col <= left_end; col++)
                    {
                        if (eff_alpha == EGUI_ALPHA_100)
                        {
                            dst_row[col] = row_color;
                        }
                        else
                        {
                            egui_rgb_mix_ptr(&dst_row[col], &row_color, &dst_row[col], eff_alpha);
                        }
                    }
                }
                else
                {
                    for (egui_dim_t col = clip_left; col <= left_end; col++)
                    {
                        RR_RING_EMIT(col, EGUI_ALPHA_100);
                    }
                }
            }

            /* Right band */
            if (right_begin <= clip_right)
            {
                if (row_color_valid && use_direct)
                {
                    for (egui_dim_t col = right_begin; col <= clip_right; col++)
                    {
                        if (eff_alpha == EGUI_ALPHA_100)
                        {
                            dst_row[col] = row_color;
                        }
                        else
                        {
                            egui_rgb_mix_ptr(&dst_row[col], &row_color, &dst_row[col], eff_alpha);
                        }
                    }
                }
                else
                {
                    for (egui_dim_t col = right_begin; col <= clip_right; col++)
                    {
                        RR_RING_EMIT(col, EGUI_ALPHA_100);
                    }
                }
            }
        }
#undef RR_RING_EMIT
    }
}

/* ========================== Capsule Line Fill Gradient ========================== */

void egui_canvas_draw_line_capsule_fill_gradient(egui_dim_t x1, egui_dim_t y1, egui_dim_t x2, egui_dim_t y2, egui_dim_t stroke_w,
                                                 const egui_gradient_t *gradient)
{
    egui_canvas_t *self = &canvas_data;

    if (stroke_w <= 0)
    {
        return;
    }

    egui_dim_t half_w = stroke_w >> 1;
    if (half_w <= 0)
    {
        half_w = 1;
    }

    /* Bounding box including capsule end circles */
    egui_dim_t min_x = EGUI_MIN(x1, x2) - half_w - 1;
    egui_dim_t min_y = EGUI_MIN(y1, y2) - half_w - 1;
    egui_dim_t max_x = EGUI_MAX(x1, x2) + half_w + 1;
    egui_dim_t max_y = EGUI_MAX(y1, y2) + half_w + 1;

    egui_dim_t bbox_w = max_x - min_x + 1;
    egui_dim_t bbox_h = max_y - min_y + 1;

    EGUI_REGION_DEFINE(region, min_x, min_y, bbox_w, bbox_h);
    egui_region_t ri;
    egui_region_intersect(&region, &self->base_view_work_region, &ri);
    if (egui_region_is_empty(&ri))
    {
        return;
    }

    egui_alpha_t base_alpha = egui_color_alpha_mix(self->alpha, gradient->alpha);
    if (base_alpha == 0)
    {
        return;
    }

    /* Line direction vector */
    int32_t abx = (int32_t)x2 - (int32_t)x1;
    int32_t aby = (int32_t)y2 - (int32_t)y1;
    int32_t ab_sq = abx * abx + aby * aby;

    int32_t hw = half_w;
    int32_t hw_sq = hw * hw;
    uint32_t r_outer_sq = (uint32_t)(hw + 1) * (hw + 1);

    egui_dim_t scan_y_start = ri.location.y;
    egui_dim_t scan_y_end = ri.location.y + ri.size.height;
    egui_dim_t scan_x_start = ri.location.x;
    egui_dim_t scan_x_end = ri.location.x + ri.size.width;

    for (egui_dim_t py = scan_y_start; py < scan_y_end; py++)
    {
        int32_t apy = (int32_t)py - (int32_t)y1;
        for (egui_dim_t px = scan_x_start; px < scan_x_end; px++)
        {
            int32_t apx = (int32_t)px - (int32_t)x1;
            int32_t dist_sq;

            if (ab_sq == 0)
            {
                /* Degenerate: single-point capsule = circle */
                dist_sq = apx * apx + apy * apy;
            }
            else
            {
                int32_t dot = apx * abx + apy * aby;
                if (dot <= 0)
                {
                    /* Before start: distance to endpoint 1 */
                    dist_sq = apx * apx + apy * apy;
                }
                else if (dot >= ab_sq)
                {
                    /* After end: distance to endpoint 2 */
                    int32_t bpx = apx - abx;
                    int32_t bpy = apy - aby;
                    dist_sq = bpx * bpx + bpy * bpy;
                }
                else
                {
                    /* Body: perpendicular distance via cross product.
                     * cross = abx*apy - aby*apx
                     * perp_dist_sq = cross^2 / ab_sq  (exact, no quantization) */
                    int64_t cross = (int64_t)abx * apy - (int64_t)aby * apx;
                    dist_sq = (int32_t)(cross * cross / ab_sq);
                }
            }

            if ((uint32_t)dist_sq > r_outer_sq)
            {
                continue;
            }

            egui_alpha_t cov;
            if (dist_sq <= (hw - 1) * (hw - 1) && hw > 1)
            {
                cov = EGUI_ALPHA_100;
            }
            else
            {
                /* SDF-based AA: linearized signed distance
                 * d ≈ (dist_sq - half_w²) / (2 * half_w)
                 * Map d in [-0.5, +0.5] to alpha in [255, 0] */
                int32_t diff = dist_sq - hw_sq;
                int32_t d_shifted = diff * 128 / hw;
                int32_t a = 128 - d_shifted;
                if (a <= 0)
                {
                    continue;
                }
                cov = (a >= 255) ? EGUI_ALPHA_100 : (egui_alpha_t)a;
            }

            egui_color_t color = gradient_color_at_pixel(gradient, (egui_dim_t)(px - min_x), (egui_dim_t)(py - min_y), bbox_w, bbox_h, px, py);
            egui_alpha_t m = egui_color_alpha_mix(base_alpha, cov);
            if (m > 0)
            {
                egui_canvas_draw_point(px, py, color, m);
            }
        }
    }
}

/* ========================== Arc Ring Fill Gradient ========================== */

/* Q15 quarter sine table: sin_q15[i] = round(sin(i deg) * 32767), i = 0..90 (1 deg steps) */
static const int16_t gradient_sin_q15[91] = {
        0,     572,   1144,  1715,  2286,  2856,  3425,  3993,  4560,  5126,  5690,  6252,  6813,  7371,  7927,  8481,  9032,  9580,  10126,
        10668, 11207, 11743, 12275, 12803, 13328, 13848, 14365, 14876, 15384, 15886, 16384, 16877, 17364, 17847, 18324, 18795, 19261, 19720,
        20174, 20622, 21063, 21498, 21926, 22348, 22763, 23170, 23571, 23965, 24351, 24730, 25102, 25466, 25822, 26170, 26510, 26842, 27166,
        27482, 27789, 28088, 28378, 28660, 28932, 29197, 29452, 29698, 29935, 30163, 30382, 30592, 30792, 30983, 31164, 31336, 31499, 31651,
        31795, 31928, 32052, 32167, 32270, 32365, 32449, 32524, 32588, 32643, 32689, 32724, 32749, 32764, 32767,
};

/* sin(deg) * 256, deg in degrees (arbitrary integer). Uses Q15 table for 1-deg precision. */
static int32_t gradient_sin_deg(int32_t deg)
{
    deg = ((deg % 360) + 360) % 360;
    int32_t sign = 1;
    if (deg > 180)
    {
        deg = 360 - deg;
        sign = -1;
    }
    if (deg > 90)
    {
        deg = 180 - deg;
    }
    /* deg is now 0..90, direct lookup, no interpolation needed */
    int32_t v = (gradient_sin_q15[deg] + 64) >> 7; /* Q15 -> Q8 with rounding */
    return v * sign;
}

static int32_t gradient_cos_deg(int32_t deg)
{
    return gradient_sin_deg(deg + 90);
}

/* Check if pixel vector (dx, dy) is within a clockwise arc [start_deg, end_deg].
 * sx,sy = start direction vector * 256; ex,ey = end direction vector * 256.
 * span360 = arc span in degrees (0..360). */
__EGUI_STATIC_INLINE__ int gradient_in_arc_sector(int32_t dx, int32_t dy, int32_t sx, int32_t sy, int32_t ex, int32_t ey, int32_t span360)
{
    /* Cross products (dx,dy scaled by 256 on direction side) */
    int32_t cross_start = sx * dy - sy * dx; /* positive = CW from start direction */
    int32_t cross_end = ex * dy - ey * dx;   /* positive = CW from end direction */

    if (span360 <= 180)
    {
        return (cross_start > 0) && (cross_end < 0);
    }
    else
    {
        return (cross_start > 0) || (cross_end < 0);
    }
}

/* Soft AA for the radial cut edges of an arc (distance from a radial line).
 * Returns alpha based on signed distance from the edge ray.
 * dir = (dx, dy) is the edge direction vector (scaled by 256).
 * pixel = (px, py) is the pixel offset from center.
 * Returns EGUI_ALPHA_100 if well inside, scaled alpha at edge, 0 if outside. */
__EGUI_STATIC_INLINE__ egui_alpha_t gradient_arc_edge_alpha(int32_t px, int32_t py, int32_t dir_x, int32_t dir_y)
{
    /* Perpendicular distance from the ray = cross(dir, P) / |dir|
     * |dir| = 256 (our scale factor), so:
     * perp_dist_scaled = (dir_x * py - dir_y * px) */
    int32_t perp_scaled = dir_x * py - dir_y * px;
    /* This is scaled by 256 (since dir is scaled by 256, P is in pixels).
     * 1 pixel = 256 units. AA zone = 1 pixel => 256 units. */
    if (perp_scaled < -256)
    {
        return 0; /* outside by more than 1 pixel */
    }
    if (perp_scaled >= 0)
    {
        return EGUI_ALPHA_100;
    }
    /* Linear interpolation in [-256, 0] range */
    return (egui_alpha_t)((uint32_t)(perp_scaled + 256) * 255 / 256);
}

/* Symmetric AA for outside-sector pixels: alpha peaks at the edge line and
 * falls off on BOTH sides within a 2-pixel zone. Used only for pixels that
 * failed the sector check but may be in the AA fringe. */
__EGUI_STATIC_INLINE__ egui_alpha_t gradient_arc_edge_alpha_symmetric(int32_t px, int32_t py, int32_t dir_x, int32_t dir_y)
{
    int32_t perp_scaled = dir_x * py - dir_y * px;
    /* Absolute distance from edge line */
    if (perp_scaled < 0)
    {
        perp_scaled = -perp_scaled;
    }
    /* 2-pixel AA zone (512 units) for smoother edges on thick strokes */
    if (perp_scaled >= 512)
    {
        return 0;
    }
    /* Linear falloff: 255 at edge, 0 at 2 pixels away */
    return (egui_alpha_t)((uint32_t)(512 - perp_scaled) * 255 / 512);
}

void egui_canvas_draw_arc_ring_fill_gradient(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t outer_r, egui_dim_t inner_r, int32_t start_angle_deg,
                                             int32_t end_angle_deg, const egui_gradient_t *gradient)
{
    egui_canvas_t *self = &canvas_data;

    if (outer_r <= 0 || inner_r >= outer_r)
    {
        return;
    }
    if (inner_r < 0)
    {
        inner_r = 0;
    }

    /* Normalize angles */
    start_angle_deg = ((start_angle_deg % 360) + 360) % 360;
    end_angle_deg = ((end_angle_deg % 360) + 360) % 360;

    int32_t span = ((end_angle_deg - start_angle_deg) + 360) % 360;
    if (span == 0)
    {
        span = 360; /* full circle */
    }

    /* Direction vectors (scaled by 256) */
    int32_t sx = gradient_cos_deg(start_angle_deg);
    int32_t sy = gradient_sin_deg(start_angle_deg);
    int32_t ex = gradient_cos_deg(end_angle_deg);
    int32_t ey = gradient_sin_deg(end_angle_deg);

    egui_dim_t bbox_size = (outer_r << 1) + 1;
    egui_dim_t bbox_x = center_x - outer_r;
    egui_dim_t bbox_y = center_y - outer_r;

    EGUI_REGION_DEFINE(region, bbox_x, bbox_y, bbox_size, bbox_size);
    egui_region_t ri;
    egui_region_intersect(&region, &self->base_view_work_region, &ri);
    if (egui_region_is_empty(&ri))
    {
        return;
    }

    egui_alpha_t base_alpha = egui_color_alpha_mix(self->alpha, gradient->alpha);
    if (base_alpha == 0)
    {
        return;
    }

    int32_t outer_r_inner_sq = (int32_t)(outer_r - 1) * (outer_r - 1);
    int32_t outer_r_outer_sq = (int32_t)(outer_r + 1) * (outer_r + 1);

    int32_t inner_r_inner_sq = (inner_r > 1) ? (int32_t)(inner_r - 1) * (inner_r - 1) : 0;
    int32_t inner_r_outer_sq = (int32_t)(inner_r + 1) * (inner_r + 1);

    uint32_t inv_outer_r = (outer_r_outer_sq > outer_r_inner_sq) ? ((255U << 8) / (uint32_t)(outer_r_outer_sq - outer_r_inner_sq)) : 0;
    uint32_t inv_inner_r = (inner_r_outer_sq > inner_r_inner_sq) ? ((255U << 8) / (uint32_t)(inner_r_outer_sq - inner_r_inner_sq)) : 0;

    int full_circle = (span == 360);

    egui_dim_t y_start = ri.location.y;
    egui_dim_t y_end = ri.location.y + ri.size.height;
    egui_dim_t x_start = ri.location.x;
    egui_dim_t x_end = ri.location.x + ri.size.width;

    /* Direct PFB write setup — skip draw_point() overhead when no mask */
    int use_direct = (self->mask == NULL) ? 1 : 0;
    egui_dim_t pfb_w = 0, pfb_ox = 0, pfb_oy = 0;
    if (use_direct)
    {
        pfb_w = self->pfb_region.size.width;
        pfb_ox = self->pfb_location_in_base_view.x;
        pfb_oy = self->pfb_location_in_base_view.y;
    }

    for (egui_dim_t y = y_start; y < y_end; y++)
    {
        int32_t dy = (int32_t)(y - center_y);
        int32_t dy_sq = dy * dy;
        if (dy_sq > outer_r_outer_sq)
        {
            continue;
        }

        /* Narrow x range to outer circle extent on this scanline */
        int32_t x_outer_half = (int32_t)gradient_isqrt((uint32_t)(outer_r_outer_sq - dy_sq));
        egui_dim_t scan_xs = EGUI_MAX(x_start, (egui_dim_t)(center_x - x_outer_half));
        egui_dim_t scan_xe = EGUI_MIN(x_end, (egui_dim_t)(center_x + x_outer_half + 1));

        /* Precompute row base pointer for direct PFB path */
        egui_color_t *row_base = NULL;
        if (use_direct)
        {
            row_base = &((egui_color_t *)self->pfb)[(y - pfb_oy) * pfb_w - pfb_ox];
        }

        /* Precompute row color for LINEAR_VERTICAL (same for whole row) */
        int lv_precomputed = 0;
        egui_color_t lv_color;
        if (gradient->type == EGUI_GRADIENT_TYPE_LINEAR_VERTICAL)
        {
            uint8_t t = gradient_linear_t(y - bbox_y, bbox_size);
            lv_color = egui_gradient_get_color(gradient->stops, gradient->stop_count, t);
            lv_precomputed = 1;
        }

        /* Inner circle x-boundary: skip pixels fully inside inner circle */
        egui_dim_t inner_skip_lo = scan_xs;
        egui_dim_t inner_skip_hi = scan_xs - 1; /* empty range by default */
        if (dy_sq < inner_r_inner_sq)
        {
            int32_t x_inner_half = (int32_t)gradient_isqrt((uint32_t)(inner_r_inner_sq - dy_sq));
            inner_skip_lo = (egui_dim_t)(center_x - x_inner_half);
            inner_skip_hi = (egui_dim_t)(center_x + x_inner_half);
        }

        for (egui_dim_t x = scan_xs; x < scan_xe; x++)
        {
            /* Jump over inner gap (d_sq < inner_r_inner_sq → ring_cov = 0) */
            if (x >= inner_skip_lo && x <= inner_skip_hi)
            {
                x = inner_skip_hi; /* for-loop will increment to inner_skip_hi + 1 */
                continue;
            }

            int32_t dx = (int32_t)(x - center_x);
            int32_t d_sq = dx * dx + dy_sq;

            /* Ring radial coverage */
            egui_alpha_t ring_cov = gradient_ring_alpha(d_sq, outer_r_inner_sq, outer_r_outer_sq, inner_r_inner_sq, inner_r_outer_sq, inv_outer_r, inv_inner_r);
            if (ring_cov == 0)
            {
                continue;
            }

            /* Angular sector coverage */
            egui_alpha_t sector_cov = EGUI_ALPHA_100;
            if (!full_circle)
            {
                if (!gradient_in_arc_sector(dx, dy, sx, sy, ex, ey, span))
                {
                    egui_alpha_t start_a = gradient_arc_edge_alpha_symmetric(dx, dy, sx, sy);
                    egui_alpha_t end_a = gradient_arc_edge_alpha_symmetric(dx, dy, ex, ey);

                    if (start_a > 0 && (dx * sx + dy * sy) <= 0)
                    {
                        start_a = 0;
                    }
                    if (end_a > 0 && (dx * ex + dy * ey) <= 0)
                    {
                        end_a = 0;
                    }

                    sector_cov = (start_a > end_a) ? start_a : end_a;
                    if (sector_cov == 0)
                    {
                        continue;
                    }
                }
            }

            egui_alpha_t cov = full_circle ? ring_cov : (egui_alpha_t)(((uint16_t)ring_cov * (uint16_t)sector_cov + 127) / 255);
            if (cov == 0)
            {
                continue;
            }

            egui_color_t color;
            if (lv_precomputed)
            {
                color = lv_color;
            }
            else if (gradient->type == EGUI_GRADIENT_TYPE_ANGULAR)
            {
                uint8_t t = gradient_angular_arc_t(dy, dx, start_angle_deg, span);
#if EGUI_CONFIG_FUNCTION_GRADIENT_DITHERING
                {
                    uint8_t bayer_row = ((uint8_t)y + (uint8_t)(d_sq >> 4)) & 3;
                    uint8_t bayer_col = ((uint8_t)x + (uint8_t)(t >> 2)) & 3;
                    uint8_t threshold = egui_gradient_bayer_4x4[bayer_row * 4 + bayer_col];
                    int16_t t_adj = (int16_t)t + (int16_t)threshold - 8;
                    if (t_adj < 0)
                    {
                        t_adj = 0;
                    }
                    if (t_adj > 255)
                    {
                        t_adj = 255;
                    }
                    t = (uint8_t)t_adj;
                }
                color = egui_gradient_get_color(gradient->stops, gradient->stop_count, t);
#else
                color = gradient_get_color_pixel(gradient->stops, gradient->stop_count, t, x, y);
#endif
            }
            else
            {
                color = gradient_color_at_pixel(gradient, x - bbox_x, y - bbox_y, bbox_size, bbox_size, x, y);
            }

            egui_alpha_t m = egui_color_alpha_mix(base_alpha, cov);
            if (m > 0)
            {
                if (use_direct)
                {
                    egui_color_t *back_color = &row_base[x];
                    if (m == EGUI_ALPHA_100)
                    {
                        *back_color = color;
                    }
                    else
                    {
                        egui_rgb_mix_ptr(back_color, &color, back_color, m);
                    }
                }
                else
                {
                    egui_canvas_draw_point(x, y, color, m);
                }
            }
        }
    }
}

/* Cap mode flags */
#define EGUI_ARC_CAP_NONE  0
#define EGUI_ARC_CAP_END   1
#define EGUI_ARC_CAP_START 2
#define EGUI_ARC_CAP_BOTH  3

void egui_canvas_draw_arc_ring_fill_gradient_round_cap(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t outer_r, egui_dim_t inner_r,
                                                       int32_t start_angle_deg, int32_t end_angle_deg, const egui_gradient_t *gradient, uint8_t cap_mode)
{
    /* 1. Draw the gradient arc ring */
    egui_canvas_draw_arc_ring_fill_gradient(center_x, center_y, outer_r, inner_r, start_angle_deg, end_angle_deg, gradient);

    /* 2. Draw round caps */
    if (cap_mode == EGUI_ARC_CAP_NONE)
    {
        return;
    }

    egui_dim_t stroke_w = outer_r - inner_r;
    egui_dim_t cap_r = (stroke_w >> 1) - 1; /* -1 to keep cap strictly inside ring */
    if (cap_r <= 0)
    {
        return;
    }
    egui_dim_t mid_r = inner_r + (stroke_w >> 1); /* center of ring */
    if (mid_r <= 0)
    {
        return;
    }

    egui_alpha_t cap_alpha = gradient->alpha;

    /* Bounding box used by arc ring gradient sampling (must match draw_arc_ring_fill_gradient) */
    egui_dim_t bbox_size = (outer_r << 1) + 1;
    egui_dim_t bbox_x = center_x - outer_r;
    egui_dim_t bbox_y = center_y - outer_r;

    /* Angle to position conversion using integer trig (scale=256) with rounding */
    if (cap_mode & EGUI_ARC_CAP_START)
    {
        int32_t sa_cos = gradient_cos_deg(start_angle_deg);
        int32_t sa_sin = gradient_sin_deg(start_angle_deg);
        int32_t sx_raw = (int32_t)mid_r * sa_cos;
        int32_t sy_raw = (int32_t)mid_r * sa_sin;
        egui_dim_t sx = center_x + (egui_dim_t)((sx_raw + (sx_raw >= 0 ? 128 : -128)) / 256);
        egui_dim_t sy = center_y + (egui_dim_t)((sy_raw + (sy_raw >= 0 ? 128 : -128)) / 256);
        /* For angular gradient use the first stop; for all other types sample the gradient
         * at the cap center position within the same bounding box as the arc ring. */
        egui_color_t start_cap_color;
        if (gradient->type == EGUI_GRADIENT_TYPE_ANGULAR)
            start_cap_color = gradient->stops[0].color;
        else
            start_cap_color = egui_gradient_color_at_pos(gradient, sx - bbox_x, sy - bbox_y, bbox_size, bbox_size);
        egui_canvas_draw_circle_fill(sx, sy, cap_r, start_cap_color, cap_alpha);
    }

    if (cap_mode & EGUI_ARC_CAP_END)
    {
        int32_t ea_cos = gradient_cos_deg(end_angle_deg);
        int32_t ea_sin = gradient_sin_deg(end_angle_deg);
        int32_t ex_raw = (int32_t)mid_r * ea_cos;
        int32_t ey_raw = (int32_t)mid_r * ea_sin;
        egui_dim_t ex = center_x + (egui_dim_t)((ex_raw + (ex_raw >= 0 ? 128 : -128)) / 256);
        egui_dim_t ey = center_y + (egui_dim_t)((ey_raw + (ey_raw >= 0 ? 128 : -128)) / 256);
        /* For angular gradient use the last stop; for all other types sample the gradient. */
        egui_color_t end_cap_color;
        if (gradient->type == EGUI_GRADIENT_TYPE_ANGULAR)
            end_cap_color = gradient->stops[gradient->stop_count - 1].color;
        else
            end_cap_color = egui_gradient_color_at_pos(gradient, ex - bbox_x, ey - bbox_y, bbox_size, bbox_size);
        egui_canvas_draw_circle_fill(ex, ey, cap_r, end_cap_color, cap_alpha);
    }
}

/* ========================== Image + Gradient Overlay ========================== */

void egui_canvas_draw_image_gradient_overlay(const egui_image_t *img, egui_dim_t x, egui_dim_t y, egui_dim_t w, egui_dim_t h, const egui_gradient_t *gradient,
                                             egui_alpha_t overlay_alpha)
{
    if (img == NULL || gradient == NULL)
    {
        return;
    }

#if EGUI_CONFIG_FUNCTION_SUPPORT_MASK
    egui_mask_t *prev_mask = egui_canvas_get_mask();

    egui_mask_gradient_t grad_mask;
    egui_mask_gradient_init((egui_mask_t *)&grad_mask);
    egui_mask_gradient_set_gradient((egui_mask_t *)&grad_mask, gradient);
    egui_mask_gradient_set_overlay_alpha((egui_mask_t *)&grad_mask, overlay_alpha);
    egui_mask_set_position((egui_mask_t *)&grad_mask, x, y);
    egui_mask_set_size((egui_mask_t *)&grad_mask, w, h);

    egui_canvas_set_mask((egui_mask_t *)&grad_mask);
    egui_canvas_draw_image_resize(img, x, y, w, h);
    egui_canvas_set_mask(prev_mask);
#else
    EGUI_UNUSED(overlay_alpha);
    egui_canvas_draw_image_resize(img, x, y, w, h);
#endif
}
