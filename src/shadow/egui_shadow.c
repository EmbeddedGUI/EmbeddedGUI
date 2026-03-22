#include <stdio.h>
#include <assert.h>

#include "egui_shadow.h"
#include "core/egui_canvas.h"
#include "core/egui_api.h"

#if EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW

// clang-format off

// Pre-computed Gaussian-approximation falloff LUT (64 bytes ROM)
// Formula: lut[i] = 255 * erfc(1.8 * (i / 63.0))
// This curve mimics Android's elevation shadow (Gaussian/Erfc)
// providing a steeper initial falloff and a softer tail.
// Index 0 = inner edge (max alpha), Index 63 = outer edge (min alpha)
static const uint8_t egui_shadow_alpha_lut[EGUI_SHADOW_LUT_SIZE] =
{
    0xff, 0xf6, 0xee, 0xe6, 0xde, 0xd6, 0xce, 0xc6,
    0xbe, 0xb6, 0xaf, 0xa7, 0xa0, 0x98, 0x91, 0x8a,
    0x84, 0x7d, 0x77, 0x70, 0x6a, 0x65, 0x5f, 0x59,
    0x54, 0x4f, 0x4a, 0x46, 0x41, 0x3d, 0x39, 0x35,
    0x32, 0x2e, 0x2b, 0x28, 0x25, 0x22, 0x1f, 0x1d,
    0x1a, 0x18, 0x16, 0x14, 0x13, 0x11, 0x10, 0x0e,
    0x0d, 0x0c, 0x0b, 0x0a, 0x09, 0x08, 0x07, 0x06,
    0x06, 0x05, 0x04, 0x04, 0x03, 0x03, 0x02, 0x00,
};

// clang-format on

// Division-free integer square root (bit-by-bit method).
// Uses only shifts, additions and comparisons - fast on MCUs without hardware divider.
static uint32_t egui_shadow_isqrt(uint32_t n)
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

// Sub-pixel precision shift for corner zone isqrt (8x scaling)
#define EGUI_SHADOW_DIST_SHIFT 3

// LUT offset: simulates Gaussian blur where the inner half is under the widget.
// distance=0 maps to LUT midpoint (~50% of peak) instead of full peak,
// eliminating the hard "ring" at the shadow inner edge.
#define EGUI_SHADOW_LUT_INNER_OFFSET (EGUI_SHADOW_LUT_SIZE / 4)

// Get shadow alpha from LUT based on distance to inner edge
static egui_alpha_t egui_shadow_get_alpha(egui_dim_t distance, egui_dim_t width, egui_alpha_t opa)
{
    if (distance < 0)
    {
        distance = 0;
    }
    if (distance >= width)
    {
        return 0;
    }

    uint8_t idx = EGUI_SHADOW_LUT_INNER_OFFSET + ((uint32_t)distance * (EGUI_SHADOW_LUT_SIZE - 1 - EGUI_SHADOW_LUT_INNER_OFFSET)) / width;
    if (idx >= EGUI_SHADOW_LUT_SIZE)
    {
        return 0;
    }

    return ((uint16_t)egui_shadow_alpha_lut[idx] * opa) >> 8;
}

// Maximum entries in the per-shadow d_sq→alpha lookup table.
// The table maps distance-squared directly to alpha, eliminating isqrt per pixel.
#define EGUI_SHADOW_DSQ_LUT_MAX 256

// Draw one corner zone with sub-pixel precision.
// bx0..bx1, by0..by1: iteration bounds (pre-clipped to PFB work region).
// cx, cy: arc center coordinates.
static void egui_shadow_draw_corner(egui_dim_t bx0, egui_dim_t by0, egui_dim_t bx1, egui_dim_t by1, egui_dim_t cx, egui_dim_t cy, egui_dim_t R, egui_dim_t W,
                                    egui_color_t color, egui_alpha_t opa, egui_alpha_t center_opa)
{
    uint32_t W_scaled = (uint32_t)W << EGUI_SHADOW_DIST_SHIFT;
    uint32_t lut_range = EGUI_SHADOW_LUT_SIZE - 1 - EGUI_SHADOW_LUT_INNER_OFFSET;

    // Get canvas info for direct PFB write optimization
    egui_canvas_t *canvas = egui_canvas_get_canvas();

    // Fast path: direct PFB write when no mask is present (typical for shadow)
    if (canvas->mask == NULL)
    {
        egui_alpha_t effective_opa = egui_color_alpha_mix(canvas->alpha, opa);
        egui_alpha_t effective_center_opa = egui_color_alpha_mix(canvas->alpha, center_opa);
        egui_dim_t pfb_width = canvas->pfb_region.size.width;
        egui_dim_t pfb_ofs_x = canvas->pfb_location_in_base_view.x;
        egui_dim_t pfb_ofs_y = canvas->pfb_location_in_base_view.y;

        if (R == 0)
        {
            // Precompute reciprocal to replace per-pixel division with multiply-shift
            uint32_t inv_W_scaled = ((uint32_t)lut_range << 16) / W_scaled;

            for (egui_dim_t py = by0; py < by1; py++)
            {
                egui_dim_t dy = EGUI_ABS(py - cy);
                uint32_t sdy = (uint32_t)EGUI_ABS(py - cy) << EGUI_SHADOW_DIST_SHIFT;
                egui_color_int_t *dst_base = &canvas->pfb[(py - pfb_ofs_y) * pfb_width - pfb_ofs_x];

                // Precompute row-constant alpha (for pixels where sdy dominates)
                egui_alpha_t row_alpha = 0;
                if (sdy < W_scaled)
                {
                    uint8_t row_idx = EGUI_SHADOW_LUT_INNER_OFFSET + (uint8_t)((sdy * inv_W_scaled) >> 16);
                    row_alpha = ((uint16_t)egui_shadow_alpha_lut[row_idx] * effective_opa) >> 8;
                }

                if (bx1 <= cx)
                {
                    egui_dim_t inner_start = bx1;
                    egui_dim_t outer_start = EGUI_MAX(bx0, cx - W + 1);
                    egui_dim_t outer_end;

                    if (row_alpha != 0)
                    {
                        inner_start = EGUI_MAX(bx0, cx - dy);
                        if (inner_start < bx1)
                        {
                            egui_canvas_blend_color_buffer_alpha(&dst_base[inner_start], bx1 - inner_start, color, row_alpha);
                        }
                    }

                    outer_end = EGUI_MIN(bx1, inner_start);
                    for (egui_dim_t px = outer_start; px < outer_end; px++)
                    {
                        uint32_t sdx = (uint32_t)(cx - px) << EGUI_SHADOW_DIST_SHIFT;
                        uint8_t idx = EGUI_SHADOW_LUT_INNER_OFFSET + (uint8_t)((sdx * inv_W_scaled) >> 16);
                        egui_alpha_t alpha = ((uint16_t)egui_shadow_alpha_lut[idx] * effective_opa) >> 8;

                        if (alpha > 0)
                        {
                            egui_color_t *back_color = (egui_color_t *)&dst_base[px];
                            if (alpha == EGUI_ALPHA_100)
                            {
                                *back_color = color;
                            }
                            else
                            {
                                egui_rgb_mix_ptr(back_color, &color, back_color, alpha);
                            }
                        }
                    }
                }
                else if (bx0 > cx)
                {
                    egui_dim_t inner_end = bx0;
                    egui_dim_t outer_start;
                    egui_dim_t outer_end = EGUI_MIN(bx1, cx + W);

                    if (row_alpha != 0)
                    {
                        inner_end = EGUI_MIN(bx1, cx + dy + 1);
                        if (bx0 < inner_end)
                        {
                            egui_canvas_blend_color_buffer_alpha(&dst_base[bx0], inner_end - bx0, color, row_alpha);
                        }
                    }

                    outer_start = EGUI_MAX(bx0, inner_end);
                    for (egui_dim_t px = outer_start; px < outer_end; px++)
                    {
                        uint32_t sdx = (uint32_t)(px - cx) << EGUI_SHADOW_DIST_SHIFT;
                        uint8_t idx = EGUI_SHADOW_LUT_INNER_OFFSET + (uint8_t)((sdx * inv_W_scaled) >> 16);
                        egui_alpha_t alpha = ((uint16_t)egui_shadow_alpha_lut[idx] * effective_opa) >> 8;

                        if (alpha == 0)
                        {
                            continue;
                        }

                        egui_color_t *back_color = (egui_color_t *)&dst_base[px];
                        if (alpha == EGUI_ALPHA_100)
                        {
                            *back_color = color;
                        }
                        else
                        {
                            egui_rgb_mix_ptr(back_color, &color, back_color, alpha);
                        }
                    }
                }
                else
                {
                    for (egui_dim_t px = bx0; px < bx1; px++)
                    {
                        uint32_t sdx = (uint32_t)EGUI_ABS(px - cx) << EGUI_SHADOW_DIST_SHIFT;
                        egui_alpha_t alpha;

                        if (sdx <= sdy)
                        {
                            alpha = row_alpha;
                        }
                        else if (sdx < W_scaled)
                        {
                            uint8_t idx = EGUI_SHADOW_LUT_INNER_OFFSET + (uint8_t)((sdx * inv_W_scaled) >> 16);
                            alpha = ((uint16_t)egui_shadow_alpha_lut[idx] * effective_opa) >> 8;
                        }
                        else
                        {
                            continue;
                        }

                        if (alpha > 0)
                        {
                            egui_color_t *back_color = (egui_color_t *)&dst_base[px];
                            if (alpha == EGUI_ALPHA_100)
                            {
                                *back_color = color;
                            }
                            else
                            {
                                egui_rgb_mix_ptr(back_color, &color, back_color, alpha);
                            }
                        }
                    }
                }
            }
            return;
        }

        uint32_t R_scaled = (uint32_t)R << EGUI_SHADOW_DIST_SHIFT;
        uint32_t R_scaled_sq = R_scaled * R_scaled;
        uint32_t RW_scaled = R_scaled + W_scaled;
        uint32_t RW_scaled_sq = RW_scaled * RW_scaled;

        // Build d_sq→alpha LUT to eliminate per-pixel isqrt.
        // Maps (d_sq - R_scaled_sq) >> shift to pre-computed alpha value.
        uint32_t dsq_delta = RW_scaled_sq - R_scaled_sq;
        uint32_t dsq_shift = 0;
        while ((dsq_delta >> dsq_shift) > EGUI_SHADOW_DSQ_LUT_MAX)
        {
            dsq_shift++;
        }
        uint32_t dsq_lut_size = (dsq_delta >> dsq_shift) + 1;
        uint8_t dsq_alpha_lut[EGUI_SHADOW_DSQ_LUT_MAX + 1];
        for (uint32_t k = 0; k < dsq_lut_size; k++)
        {
            // Recover approximate d_sq and compute alpha
            uint32_t d_sq_approx = R_scaled_sq + (k << dsq_shift) + (1U << dsq_shift >> 1);
            if (d_sq_approx > RW_scaled_sq)
            {
                d_sq_approx = RW_scaled_sq;
            }
            uint32_t d_scaled = egui_shadow_isqrt(d_sq_approx);
            int32_t dist_scaled = (int32_t)d_scaled - (int32_t)R_scaled;
            if (dist_scaled < 0)
            {
                dist_scaled = 0;
            }
            uint8_t idx = EGUI_SHADOW_LUT_INNER_OFFSET + ((uint32_t)dist_scaled * lut_range) / W_scaled;
            if (idx >= EGUI_SHADOW_LUT_SIZE)
            {
                dsq_alpha_lut[k] = 0;
            }
            else
            {
                dsq_alpha_lut[k] = ((uint16_t)egui_shadow_alpha_lut[idx] * effective_opa) >> 8;
            }
        }

        for (egui_dim_t py = by0; py < by1; py++)
        {
            uint32_t dy = (uint32_t)EGUI_ABS(py - cy);
            uint32_t sdy = dy << EGUI_SHADOW_DIST_SHIFT;
            uint32_t sdy2 = sdy * sdy;
            egui_color_t *dst_base = (egui_color_t *)&canvas->pfb[(py - pfb_ofs_y) * pfb_width - pfb_ofs_x];

            uint32_t sstep = 1U << EGUI_SHADOW_DIST_SHIFT;
            uint32_t sstep_sq = sstep * sstep;
            int step_dir = 0;
            if (bx1 <= cx)
            {
                step_dir = -1;
            }
            else if (bx0 > cx)
            {
                step_dir = 1;
            }

            if (step_dir != 0)
            {
                uint32_t dx0 = (uint32_t)EGUI_ABS(bx0 - cx);
                uint32_t sdx0 = dx0 << EGUI_SHADOW_DIST_SHIFT;
                uint32_t sdx2 = sdx0 * sdx0;

                /* Second-order incremental d²: eliminates 64-bit multiply per pixel.
                 * delta[i+1] = delta[i] + 2*sstep², regardless of direction. */
                int32_t d_sq_delta;
                if (step_dir > 0)
                    d_sq_delta = (int32_t)(sstep * ((sdx0 << 1) + sstep));
                else
                    d_sq_delta = (int32_t)sstep_sq - (int32_t)(sstep * (sdx0 << 1));
                int32_t d_sq_delta_step = (int32_t)(sstep_sq << 1);

                for (egui_dim_t px = bx0; px < bx1; px++)
                {
                    uint32_t d_sq = sdx2 + sdy2;

                    if (d_sq <= R_scaled_sq)
                    {
                        if (effective_center_opa > 0)
                        {
                            egui_color_t *back_color = &dst_base[px];
                            if (effective_center_opa == EGUI_ALPHA_100)
                            {
                                *back_color = color;
                            }
                            else
                            {
                                egui_rgb_mix_ptr(back_color, &color, back_color, effective_center_opa);
                            }
                        }
                    }
                    else if (d_sq < RW_scaled_sq)
                    {
                        egui_alpha_t alpha = dsq_alpha_lut[(d_sq - R_scaled_sq) >> dsq_shift];
                        if (alpha > 0)
                        {
                            egui_color_t *back_color = &dst_base[px];
                            if (alpha == EGUI_ALPHA_100)
                            {
                                *back_color = color;
                            }
                            else
                            {
                                egui_rgb_mix_ptr(back_color, &color, back_color, alpha);
                            }
                        }
                    }

                    sdx2 = (uint32_t)((int32_t)sdx2 + d_sq_delta);
                    d_sq_delta += d_sq_delta_step;
                }
            }
            else
            {
                /* Center-crossing: split into right half (cx→bx1) and left half (cx→bx0)
                 * with incremental d² to avoid per-pixel ABS + multiply. */

                /* Macro for shadow pixel blending (direct PFB path) */
                #define SHADOW_CORNER_BLEND_DIRECT(px_val, d_sq_val)                                            \
                    do {                                                                                       \
                        if ((d_sq_val) <= R_scaled_sq)                                                         \
                        {                                                                                      \
                            if (effective_center_opa > 0)                                                      \
                            {                                                                                  \
                                egui_color_t *back_color = &dst_base[(px_val)];                                \
                                if (effective_center_opa == EGUI_ALPHA_100)                                    \
                                    *back_color = color;                                                       \
                                else                                                                           \
                                    egui_rgb_mix_ptr(back_color, &color, back_color, effective_center_opa);    \
                            }                                                                                  \
                        }                                                                                      \
                        else if ((d_sq_val) < RW_scaled_sq)                                                    \
                        {                                                                                      \
                            egui_alpha_t _a = dsq_alpha_lut[((d_sq_val) - R_scaled_sq) >> dsq_shift];          \
                            if (_a > 0)                                                                        \
                            {                                                                                  \
                                egui_color_t *back_color = &dst_base[(px_val)];                                \
                                if (_a == EGUI_ALPHA_100)                                                      \
                                    *back_color = color;                                                       \
                                else                                                                           \
                                    egui_rgb_mix_ptr(back_color, &color, back_color, _a);                      \
                            }                                                                                  \
                        }                                                                                      \
                    } while (0)

                /* Right half: cx → bx1 (increasing dx) */
                if (cx < bx1)
                {
                    egui_dim_t r_start = EGUI_MAX(cx, bx0);
                    uint32_t rdx0 = (uint32_t)(r_start - cx) << EGUI_SHADOW_DIST_SHIFT;
                    uint32_t rsdx2 = rdx0 * rdx0;
                    int32_t r_delta = (int32_t)(sstep * ((rdx0 << 1) + sstep));
                    int32_t r_delta_step = (int32_t)(sstep_sq << 1);
                    for (egui_dim_t px = r_start; px < bx1; px++)
                    {
                        uint32_t d_sq = rsdx2 + sdy2;
                        SHADOW_CORNER_BLEND_DIRECT(px, d_sq);
                        rsdx2 = (uint32_t)((int32_t)rsdx2 + r_delta);
                        r_delta += r_delta_step;
                    }
                }
                /* Left half: cx-1 → bx0 (increasing dx) */
                if (cx - 1 >= bx0)
                {
                    egui_dim_t l_start = EGUI_MIN(cx - 1, bx1 - 1);
                    uint32_t ldx0 = (uint32_t)(cx - l_start) << EGUI_SHADOW_DIST_SHIFT;
                    uint32_t lsdx2 = ldx0 * ldx0;
                    int32_t l_delta = (int32_t)(sstep * ((ldx0 << 1) + sstep));
                    int32_t l_delta_step = (int32_t)(sstep_sq << 1);
                    for (egui_dim_t px = l_start; px >= bx0; px--)
                    {
                        uint32_t d_sq = lsdx2 + sdy2;
                        SHADOW_CORNER_BLEND_DIRECT(px, d_sq);
                        lsdx2 = (uint32_t)((int32_t)lsdx2 + l_delta);
                        l_delta += l_delta_step;
                    }
                }
                #undef SHADOW_CORNER_BLEND_DIRECT
            }
        }
        return;
    }

    // Fallback path with mask support (original implementation)

    // When corner_radius == 0 (rectangular shadow), use the L-infinity norm
    // max(|dx|, |dy|) as the effective distance so that corner pixels receive
    // the same shadow strength as the adjacent straight edge pixels.  The
    // Euclidean formula would give a lighter diagonal and create a visible
    // "missing corner" artifact on rectangular widgets.
    if (R == 0)
    {
        uint32_t inv_W_scaled = ((uint32_t)lut_range << 16) / W_scaled;

        for (egui_dim_t py = by0; py < by1; py++)
        {
            uint32_t dy = (uint32_t)EGUI_ABS(py - cy);
            uint32_t sdy = dy << EGUI_SHADOW_DIST_SHIFT;

            egui_alpha_t row_alpha = 0;
            if (sdy < W_scaled)
            {
                uint8_t row_idx = EGUI_SHADOW_LUT_INNER_OFFSET + (uint8_t)((sdy * inv_W_scaled) >> 16);
                row_alpha = ((uint16_t)egui_shadow_alpha_lut[row_idx] * opa) >> 8;
            }

            for (egui_dim_t px = bx0; px < bx1; px++)
            {
                uint32_t dx = (uint32_t)EGUI_ABS(px - cx);
                uint32_t sdx = dx << EGUI_SHADOW_DIST_SHIFT;

                egui_alpha_t alpha;
                if (sdx <= sdy)
                {
                    alpha = row_alpha;
                }
                else if (sdx < W_scaled)
                {
                    uint8_t idx = EGUI_SHADOW_LUT_INNER_OFFSET + (uint8_t)((sdx * inv_W_scaled) >> 16);
                    alpha = ((uint16_t)egui_shadow_alpha_lut[idx] * opa) >> 8;
                }
                else
                {
                    continue;
                }

                if (alpha > 0)
                {
                    egui_canvas_draw_point_limit(px, py, color, alpha);
                }
            }
        }
        return;
    }

    uint32_t R_scaled = (uint32_t)R << EGUI_SHADOW_DIST_SHIFT;
    uint32_t R_scaled_sq = R_scaled * R_scaled;
    uint32_t RW_scaled = R_scaled + W_scaled;
    uint32_t RW_scaled_sq = RW_scaled * RW_scaled;

    // Precompute d_sq -> alpha LUT for fallback path (same as fast path)
    uint32_t fb_dsq_range = RW_scaled_sq - R_scaled_sq;
    uint8_t fb_dsq_shift = 0;
    {
        uint32_t tmp = fb_dsq_range;
        while (tmp > EGUI_SHADOW_DSQ_LUT_MAX)
        {
            tmp >>= 1;
            fb_dsq_shift++;
        }
    }
    uint32_t fb_dsq_entries = (fb_dsq_range >> fb_dsq_shift) + 1;
    egui_alpha_t fb_dsq_alpha_lut[EGUI_SHADOW_DSQ_LUT_MAX + 1];
    for (uint32_t i = 0; i < fb_dsq_entries; i++)
    {
        uint32_t d_sq_sample = R_scaled_sq + (i << fb_dsq_shift) + (1U << fb_dsq_shift >> 1);
        if (d_sq_sample > RW_scaled_sq)
        {
            d_sq_sample = RW_scaled_sq;
        }
        uint32_t d_s = egui_shadow_isqrt(d_sq_sample);
        int32_t dist_s = (int32_t)d_s - (int32_t)R_scaled;
        uint8_t idx = EGUI_SHADOW_LUT_INNER_OFFSET + ((uint32_t)dist_s * lut_range) / W_scaled;
        fb_dsq_alpha_lut[i] = ((uint16_t)egui_shadow_alpha_lut[idx] * opa) >> 8;
    }

    for (egui_dim_t py = by0; py < by1; py++)
    {
        uint32_t dy = (uint32_t)EGUI_ABS(py - cy);
        uint32_t sdy = dy << EGUI_SHADOW_DIST_SHIFT;
        uint32_t sdy2 = sdy * sdy;

        uint32_t sstep = 1U << EGUI_SHADOW_DIST_SHIFT;
        uint32_t sstep_sq = sstep * sstep;
        int step_dir = 0;
        if (bx1 <= cx)
        {
            step_dir = -1;
        }
        else if (bx0 > cx)
        {
            step_dir = 1;
        }

        if (step_dir != 0)
        {
            uint32_t dx0 = (uint32_t)EGUI_ABS(bx0 - cx);
            uint32_t sdx0 = dx0 << EGUI_SHADOW_DIST_SHIFT;
            uint32_t sdx2 = sdx0 * sdx0;

            int32_t d_sq_delta;
            if (step_dir > 0)
                d_sq_delta = (int32_t)(sstep * ((sdx0 << 1) + sstep));
            else
                d_sq_delta = (int32_t)sstep_sq - (int32_t)(sstep * (sdx0 << 1));
            int32_t d_sq_delta_step = (int32_t)(sstep_sq << 1);

            for (egui_dim_t px = bx0; px < bx1; px++)
            {
                uint32_t d_sq = sdx2 + sdy2;

                if (d_sq <= R_scaled_sq)
                {
                    egui_canvas_draw_point_limit(px, py, color, center_opa);
                }
                else if (d_sq < RW_scaled_sq)
                {
                    egui_alpha_t alpha = fb_dsq_alpha_lut[(d_sq - R_scaled_sq) >> fb_dsq_shift];
                    if (alpha > 0)
                    {
                        egui_canvas_draw_point_limit(px, py, color, alpha);
                    }
                }

                sdx2 = (uint32_t)((int32_t)sdx2 + d_sq_delta);
                d_sq_delta += d_sq_delta_step;
            }
        }
        else
        {
            /* Center-crossing: split into right and left incremental sweeps */
            if (cx < bx1)
            {
                egui_dim_t r_start = EGUI_MAX(cx, bx0);
                uint32_t rdx0 = (uint32_t)(r_start - cx) << EGUI_SHADOW_DIST_SHIFT;
                uint32_t rsdx2 = rdx0 * rdx0;
                int32_t r_delta = (int32_t)(sstep * ((rdx0 << 1) + sstep));
                int32_t r_delta_step = (int32_t)(sstep_sq << 1);
                for (egui_dim_t px = r_start; px < bx1; px++)
                {
                    uint32_t d_sq = rsdx2 + sdy2;
                    if (d_sq <= R_scaled_sq)
                    {
                        egui_canvas_draw_point_limit(px, py, color, center_opa);
                    }
                    else if (d_sq < RW_scaled_sq)
                    {
                        egui_alpha_t alp = fb_dsq_alpha_lut[(d_sq - R_scaled_sq) >> fb_dsq_shift];
                        if (alp > 0)
                            egui_canvas_draw_point_limit(px, py, color, alp);
                    }
                    rsdx2 = (uint32_t)((int32_t)rsdx2 + r_delta);
                    r_delta += r_delta_step;
                }
            }
            if (cx - 1 >= bx0)
            {
                egui_dim_t l_start = EGUI_MIN(cx - 1, bx1 - 1);
                uint32_t ldx0 = (uint32_t)(cx - l_start) << EGUI_SHADOW_DIST_SHIFT;
                uint32_t lsdx2 = ldx0 * ldx0;
                int32_t l_delta = (int32_t)(sstep * ((ldx0 << 1) + sstep));
                int32_t l_delta_step = (int32_t)(sstep_sq << 1);
                for (egui_dim_t px = l_start; px >= bx0; px--)
                {
                    uint32_t d_sq = lsdx2 + sdy2;
                    if (d_sq <= R_scaled_sq)
                    {
                        egui_canvas_draw_point_limit(px, py, color, center_opa);
                    }
                    else if (d_sq < RW_scaled_sq)
                    {
                        egui_alpha_t alp = fb_dsq_alpha_lut[(d_sq - R_scaled_sq) >> fb_dsq_shift];
                        if (alp > 0)
                            egui_canvas_draw_point_limit(px, py, color, alp);
                    }
                    lsdx2 = (uint32_t)((int32_t)lsdx2 + l_delta);
                    l_delta += l_delta_step;
                }
            }
        }
    }
}

void egui_shadow_draw(const egui_shadow_t *shadow, egui_region_t *view_region)
{
    if (shadow->width == 0 && shadow->spread == 0)
    {
        return;
    }

    egui_dim_t W = shadow->width;
    egui_dim_t R = shadow->corner_radius;
    egui_color_t color = shadow->color;
    egui_alpha_t opa = shadow->opa;

    if (opa == 0)
    {
        return;
    }

    egui_dim_t inner_w = view_region->size.width + 2 * shadow->spread;
    egui_dim_t inner_h = view_region->size.height + 2 * shadow->spread;

    // Clamp corner_radius to half of smaller dimension
    if (R > inner_w / 2)
    {
        R = inner_w / 2;
    }
    if (R > inner_h / 2)
    {
        R = inner_h / 2;
    }

    // All coordinates below are in shadow-region local space.
    // The inner rectangle (solid center) starts at offset (W, W).
    egui_dim_t inner_x = W;
    egui_dim_t inner_y = W;

    // Center alpha matches inner edge of blur zone for seamless transition
    egui_alpha_t center_opa = (W > 0) ? egui_shadow_get_alpha(0, W, opa) : opa;

    // ========================
    // 1. Draw center region (solid fill)
    // ========================
    if (R == 0)
    {
        egui_canvas_draw_rectangle_fill(inner_x, inner_y, inner_w, inner_h, color, center_opa);
    }
    else
    {
        // Draw three rectangles excluding corner squares.
        // Corner squares are handled by the corner zone loop below.
        egui_canvas_draw_rectangle_fill(inner_x + R, inner_y, inner_w - 2 * R, inner_h, color, center_opa);
        egui_canvas_draw_rectangle_fill(inner_x, inner_y + R, R, inner_h - 2 * R, color, center_opa);
        egui_canvas_draw_rectangle_fill(inner_x + inner_w - R, inner_y + R, R, inner_h - 2 * R, color, center_opa);
    }

    if (W == 0)
    {
        return;
    }

    // ========================
    // 2. Arc center boundaries for zone splitting
    // ========================
    egui_dim_t arc_x0 = inner_x + R;
    egui_dim_t arc_y0 = inner_y + R;
    egui_dim_t arc_x1 = inner_x + inner_w - R - 1;
    egui_dim_t arc_y1 = inner_y + inner_h - R - 1;

    egui_dim_t shadow_w = inner_w + 2 * W;
    egui_dim_t shadow_h = inner_h + 2 * W;

    // Work region for clipping corner iterations
    egui_canvas_t *canvas = egui_canvas_get_canvas();
    EGUI_REGION_DEFINE(shadow_full, 0, 0, shadow_w, shadow_h);
    egui_region_t work;
    egui_region_intersect(&shadow_full, &canvas->base_view_work_region, &work);
    if (egui_region_is_empty(&work))
    {
        return;
    }

    egui_dim_t wx0 = work.location.x;
    egui_dim_t wy0 = work.location.y;
    egui_dim_t wx1 = wx0 + work.size.width;
    egui_dim_t wy1 = wy0 + work.size.height;

    // ========================
    // 3. Edge strips (rectangle fills with per-row/column constant alpha)
    // ========================
    // Each edge row/column has uniform distance to inner boundary, so we draw
    // entire rows/columns at once using rectangle fill - much faster than per-pixel.
    egui_dim_t edge_h_len = arc_x1 - arc_x0 + 1; // horizontal edge strip length
    egui_dim_t edge_v_len = arc_y1 - arc_y0 + 1; // vertical edge strip length

    for (egui_dim_t d = 1; d <= W; d++)
    {
        egui_alpha_t alpha = egui_shadow_get_alpha(d, W, opa);
        if (alpha > 0)
        {
            if (edge_h_len > 0)
            {
                // Top edge row
                egui_canvas_draw_rectangle_fill(arc_x0, inner_y - d, edge_h_len, 1, color, alpha);
                // Bottom edge row
                egui_canvas_draw_rectangle_fill(arc_x0, inner_y + inner_h + d - 1, edge_h_len, 1, color, alpha);
            }
            if (edge_v_len > 0)
            {
                // Left edge column
                egui_canvas_draw_rectangle_fill(inner_x - d, arc_y0, 1, edge_v_len, color, alpha);
                // Right edge column
                egui_canvas_draw_rectangle_fill(inner_x + inner_w + d - 1, arc_y0, 1, edge_v_len, color, alpha);
            }
        }
    }

    // ========================
    // 4. Corner zones (per-pixel with sub-pixel precision)
    // ========================
    // Process all 4 corners. Bit pattern: bit0 = right side, bit1 = bottom side.
    // Each corner bounding box is clipped to PFB work region before iteration.
    for (int c = 0; c < 4; c++)
    {
        egui_dim_t bx0 = (c & 1) ? (arc_x1 + 1) : 0;
        egui_dim_t by0 = (c & 2) ? (arc_y1 + 1) : 0;
        egui_dim_t bx1 = (c & 1) ? shadow_w : arc_x0;
        egui_dim_t by1 = (c & 2) ? shadow_h : arc_y0;
        egui_dim_t cx = (c & 1) ? arc_x1 : arc_x0;
        egui_dim_t cy = (c & 2) ? arc_y1 : arc_y0;

        // Clip corner bounding box to PFB work region
        if (bx0 < wx0)
        {
            bx0 = wx0;
        }
        if (by0 < wy0)
        {
            by0 = wy0;
        }
        if (bx1 > wx1)
        {
            bx1 = wx1;
        }
        if (by1 > wy1)
        {
            by1 = wy1;
        }

        if (bx0 < bx1 && by0 < by1)
        {
            egui_shadow_draw_corner(bx0, by0, bx1, by1, cx, cy, R, W, color, opa, center_opa);
        }
    }
}

void egui_shadow_get_region(const egui_shadow_t *shadow, egui_region_t *view_region, egui_region_t *shadow_region)
{
    egui_dim_t ext = shadow->width + shadow->spread;

    shadow_region->location.x = view_region->location.x + shadow->ofs_x - ext;
    shadow_region->location.y = view_region->location.y + shadow->ofs_y - ext;
    shadow_region->size.width = view_region->size.width + 2 * ext;
    shadow_region->size.height = view_region->size.height + 2 * ext;
}

#endif // EGUI_CONFIG_FUNCTION_SUPPORT_SHADOW
