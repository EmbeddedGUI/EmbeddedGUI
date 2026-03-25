#include "egui_canvas.h"

/**
 * @brief High-quality line/polyline drawing with sub-pixel sampling.
 *
 * Uses NxN sub-pixel sampling (same approach as circle_hq) for smooth
 * anti-aliased edges on thick lines. Thin lines (stroke_width <= 1)
 * delegate to the existing Wu's algorithm in egui_canvas_line.c.
 */

/* ========================== Config & Alpha Tables ========================== */

/* NOTE: Scale must be 2x the logical sample count so that sub-pixel offsets
 * (±1, ±3) land INSIDE the pixel boundary (±0.5 original pixel).
 * 2x2 mode: SCALE=4 → offsets ±1 map to ±0.25 pixel  (within ±0.5) ✓
 * 4x4 mode: SCALE=8 → offsets ±1,±3 map to ±0.125,±0.375 pixel (within ±0.5) ✓
 */
#if EGUI_CONFIG_LINE_HQ_SAMPLE_2X2
#define LINE_HQ_SCALE    4
#define LINE_HQ_SCALE_SQ 16
static const egui_alpha_t line_hq_alpha_table[] = {
        0, 64, 128, 191, 255,
};
#else
#define LINE_HQ_SCALE    8
#define LINE_HQ_SCALE_SQ 64
static const egui_alpha_t line_hq_alpha_table[] = {
        0, 16, 32, 48, 64, 80, 96, 112, 128, 143, 159, 175, 191, 207, 223, 239, 255,
};
#endif

/* Use the same degree trig approximation style as arc gradient round-cap path.
 * sin_q1[i] = sin(i*5 deg) * 256, i=0..18.
 */
static const int16_t line_hq_sin_q1[19] = {
        0, 22, 44, 66, 88, 108, 128, 147, 165, 181, 196, 210, 222, 232, 241, 247, 252, 255, 256,
};

static int32_t line_hq_sin_deg(int32_t deg)
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

    int32_t idx = deg / 5;
    int32_t frac = deg % 5;
    int32_t value = line_hq_sin_q1[idx];
    if (frac > 0 && idx < 18)
    {
        value = value + ((line_hq_sin_q1[idx + 1] - value) * frac + 2) / 5;
    }
    return value * sign;
}

static int32_t line_hq_cos_deg(int32_t deg)
{
    return line_hq_sin_deg(deg + 90);
}

/* Integer square root (32-bit input, 16-bit result) */
static uint16_t line_hq_isqrt32(uint32_t n)
{
    uint32_t result = 0;
    uint32_t bit = 1u << 30;
    while (bit > n)
        bit >>= 2;
    while (bit != 0)
    {
        if (n >= result + bit)
        {
            n -= result + bit;
            result = (result >> 1) + bit;
        }
        else
        {
            result >>= 1;
        }
        bit >>= 2;
    }
    return (uint16_t)result;
}

/* ========================== Sub-pixel Edge Sampling ========================== */

#if EGUI_CONFIG_LINE_HQ_SAMPLE_2X2
#define LINE_HQ_SAMPLE_DIM          2
#define LINE_HQ_SAMPLE_FIRST_OFFSET (-1)
#else
#define LINE_HQ_SAMPLE_DIM          4
#define LINE_HQ_SAMPLE_FIRST_OFFSET (-3)
#endif

#define LINE_HQ_SAMPLE_STEP 2

typedef struct line_hq_sample_ctx_t
{
    int32_t dx;
    int32_t dy;
    int32_t dx_s;
    int32_t dy_s;
    int64_t len_sq_scaled;
    int64_t hw_sq_len_sq_scaled;
    int64_t hw_sq_scaled;
    int64_t cross_step_x;
    int64_t dot_step_x;
} line_hq_sample_ctx_t;

__EGUI_STATIC_INLINE__ void line_hq_blend_direct(egui_color_t *dst, egui_color_t color, egui_alpha_t alpha)
{
    if (alpha == 0)
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

static void line_hq_prepare_alpha_table(egui_alpha_t alpha, egui_alpha_t canvas_alpha, egui_alpha_t *sample_alpha_table, egui_alpha_t *direct_alpha_table)
{
    uint32_t i;

    for (i = 0; i < EGUI_ARRAY_SIZE(line_hq_alpha_table); i++)
    {
        egui_alpha_t mixed = line_hq_alpha_table[i];

        if (alpha != EGUI_ALPHA_100)
        {
            mixed = egui_color_alpha_mix(alpha, mixed);
        }

        sample_alpha_table[i] = mixed;
        direct_alpha_table[i] = (canvas_alpha == EGUI_ALPHA_100) ? mixed : egui_color_alpha_mix(canvas_alpha, mixed);
    }
}

static void line_hq_init_sample_ctx(line_hq_sample_ctx_t *ctx, int32_t dx, int32_t dy, int64_t line_len_sq, int64_t half_w_sq_x_len_sq)
{
    ctx->dx = dx;
    ctx->dy = dy;
    ctx->dx_s = dx * LINE_HQ_SCALE;
    ctx->dy_s = dy * LINE_HQ_SCALE;
    ctx->len_sq_scaled = line_len_sq * LINE_HQ_SCALE_SQ;
    ctx->hw_sq_len_sq_scaled = half_w_sq_x_len_sq * LINE_HQ_SCALE_SQ;
    ctx->hw_sq_scaled = half_w_sq_x_len_sq * LINE_HQ_SCALE_SQ / line_len_sq;
    ctx->cross_step_x = (int64_t)dy * LINE_HQ_SAMPLE_STEP;
    ctx->dot_step_x = (int64_t)ctx->dx_s * LINE_HQ_SAMPLE_STEP;
}

__EGUI_STATIC_INLINE__ int32_t line_hq_fp16_to_int_trunc0(int64_t value)
{
    if (value >= 0)
    {
        return (int32_t)(value >> 16);
    }

    return -(int32_t)((-value) >> 16);
}

/**
 * @brief Compute sub-pixel coverage index for a pixel near a line segment (butt cap).
 *
 * For each sub-pixel sample point, we test:
 *   1. Perpendicular distance to line < half_w (inside the stroke)
 *   2. Projection along line is within [0, line_len_sq] (inside segment endpoints)
 */
static uint8_t line_hq_get_pixel_coverage(int32_t rel_x, int32_t rel_y, const line_hq_sample_ctx_t *ctx)
{
    int32_t spx = rel_x * LINE_HQ_SCALE + LINE_HQ_SAMPLE_FIRST_OFFSET;
    int32_t spy = rel_y * LINE_HQ_SCALE + LINE_HQ_SAMPLE_FIRST_OFFSET;
    uint8_t count = 0;
    uint8_t sy;

    for (sy = 0; sy < LINE_HQ_SAMPLE_DIM; sy++)
    {
        int64_t cross = (int64_t)ctx->dx * spy - (int64_t)ctx->dy * spx;
        int64_t dot = (int64_t)spx * ctx->dx_s + (int64_t)spy * ctx->dy_s;
        uint8_t sx;

        for (sx = 0; sx < LINE_HQ_SAMPLE_DIM; sx++)
        {
            int64_t cross_sq = cross * cross;
            if (cross_sq <= ctx->hw_sq_len_sq_scaled && dot >= 0 && dot <= ctx->len_sq_scaled)
            {
                count++;
            }

            cross -= ctx->cross_step_x;
            dot += ctx->dot_step_x;
        }

        spy += LINE_HQ_SAMPLE_STEP;
    }

    return count;
}

/**
 * @brief Compute sub-pixel coverage index for a pixel near a line segment (round cap).
 *
 * Same as line_hq_get_pixel_coverage but with round caps at endpoints:
 * when a sub-pixel sample falls outside the segment projection, it checks
 * distance to the nearest endpoint instead of rejecting.
 *
 * cap_start: 1 = round cap at start, 0 = butt cap at start
 * cap_end:   1 = round cap at end,   0 = butt cap at end
 */
static uint8_t line_hq_get_pixel_coverage_round_cap(int32_t rel_x, int32_t rel_y, const line_hq_sample_ctx_t *ctx, int32_t cap_start, int32_t cap_end)
{
    int32_t spx_row = rel_x * LINE_HQ_SCALE + LINE_HQ_SAMPLE_FIRST_OFFSET;
    int32_t spy = rel_y * LINE_HQ_SCALE + LINE_HQ_SAMPLE_FIRST_OFFSET;
    uint8_t count = 0;
    uint8_t sy;

    for (sy = 0; sy < LINE_HQ_SAMPLE_DIM; sy++)
    {
        int32_t spx = spx_row;
        int32_t epx = spx - ctx->dx_s;
        int32_t epy = spy - ctx->dy_s;
        int64_t cross = (int64_t)ctx->dx * spy - (int64_t)ctx->dy * spx;
        int64_t dot = (int64_t)spx * ctx->dx_s + (int64_t)spy * ctx->dy_s;
        uint8_t sx;

        for (sx = 0; sx < LINE_HQ_SAMPLE_DIM; sx++)
        {
            if (dot < 0)
            {
                if (cap_start)
                {
                    int64_t d_sq = (int64_t)spx * spx + (int64_t)spy * spy;
                    if (d_sq < ctx->hw_sq_scaled)
                    {
                        count++;
                    }
                }
            }
            else if (dot > ctx->len_sq_scaled)
            {
                if (cap_end)
                {
                    int64_t d_sq = (int64_t)epx * epx + (int64_t)epy * epy;
                    if (d_sq < ctx->hw_sq_scaled)
                    {
                        count++;
                    }
                }
            }
            else if (cross * cross <= ctx->hw_sq_len_sq_scaled)
            {
                count++;
            }

            spx += LINE_HQ_SAMPLE_STEP;
            epx += LINE_HQ_SAMPLE_STEP;
            cross -= ctx->cross_step_x;
            dot += ctx->dot_step_x;
        }

        spy += LINE_HQ_SAMPLE_STEP;
    }

    return count;
}

/* ========================== Line HQ ========================== */

void egui_canvas_draw_line_hq(egui_dim_t x1, egui_dim_t y1, egui_dim_t x2, egui_dim_t y2, egui_dim_t stroke_width, egui_color_t color, egui_alpha_t alpha)
{
    egui_canvas_t *self = &canvas_data;

    egui_dim_t deltax = EGUI_ABS(x2 - x1);
    egui_dim_t deltay = EGUI_ABS(y2 - y1);

    // Special case: vertical line
    if (deltax == 0)
    {
        egui_dim_t min_y = EGUI_MIN(y1, y2);
        egui_dim_t half_w = stroke_width >> 1;
        egui_canvas_draw_fillrect(x1 - half_w, min_y, stroke_width, deltay + 1, color, alpha);
        return;
    }
    // Special case: horizontal line
    if (deltay == 0)
    {
        egui_dim_t min_x = EGUI_MIN(x1, x2);
        egui_dim_t half_w = stroke_width >> 1;
        egui_canvas_draw_fillrect(min_x, y1 - half_w, deltax + 1, stroke_width, color, alpha);
        return;
    }

    // Thin lines: delegate to existing Wu's algorithm
    if (stroke_width <= 1)
    {
        egui_canvas_draw_line(x1, y1, x2, y2, stroke_width, color, alpha);
        return;
    }

    // Bounding box check
    egui_dim_t bbox_x = EGUI_MIN(x1, x2) - stroke_width;
    egui_dim_t bbox_y = EGUI_MIN(y1, y2) - stroke_width;
    egui_dim_t bbox_w = deltax + (stroke_width << 1) + 1;
    egui_dim_t bbox_h = deltay + (stroke_width << 1) + 1;
    EGUI_REGION_DEFINE(region, bbox_x, bbox_y, bbox_w, bbox_h);
    if (!egui_region_is_intersect(&region, &self->base_view_work_region))
    {
        return;
    }

    int32_t dx = (int32_t)(x2 - x1);
    int32_t dy = (int32_t)(y2 - y1);
    int64_t line_len_sq = (int64_t)dx * dx + (int64_t)dy * dy;

    // half_w in fixed-point: stroke_width / 2, but we need (half_w)^2 * line_len_sq
    // half_w = stroke_width * 0.5
    // half_w_sq_x_len_sq = (stroke_width/2)^2 * line_len_sq = stroke_width^2 * line_len_sq / 4
    int64_t half_w_sq_x_len_sq = (int64_t)stroke_width * stroke_width * line_len_sq / 4;

    // Scan bounds
    egui_dim_t half_sw = (stroke_width >> 1) + 1;
    egui_dim_t scan_x1 = EGUI_MIN(x1, x2) - half_sw;
    egui_dim_t scan_y1 = EGUI_MIN(y1, y2) - half_sw;
    egui_dim_t scan_x2 = EGUI_MAX(x1, x2) + half_sw;
    egui_dim_t scan_y2 = EGUI_MAX(y1, y2) + half_sw;

    // Clamp to work region
    egui_dim_t work_x1 = self->base_view_work_region.location.x;
    egui_dim_t work_y1 = self->base_view_work_region.location.y;
    egui_dim_t work_x2 = work_x1 + self->base_view_work_region.size.width;
    egui_dim_t work_y2 = work_y1 + self->base_view_work_region.size.height;

    scan_x1 = EGUI_MAX(scan_x1, work_x1);
    scan_y1 = EGUI_MAX(scan_y1, work_y1);
    scan_x2 = EGUI_MIN(scan_x2, work_x2 - 1);
    scan_y2 = EGUI_MIN(scan_y2, work_y2 - 1);

    // Pre-compute outer threshold for fast rejection
    // outer = (half_w + 1)^2 * line_len_sq
    int32_t outer_w = stroke_width + 2;
    int64_t outer_thresh = (int64_t)outer_w * outer_w * line_len_sq / 4;

    // Pre-compute inner threshold (constant for all pixels)
    int32_t inner_w = stroke_width - 2;
    int64_t inner_thresh = 0;
    if (inner_w > 0)
    {
        inner_thresh = (int64_t)inner_w * inner_w * line_len_sq / 4;
    }

    // Direct PFB write setup
    int use_direct_pfb = (self->mask == NULL);
    egui_dim_t pfb_width = self->pfb_region.size.width;
    egui_dim_t pfb_ofs_x = self->pfb_location_in_base_view.x;
    egui_dim_t pfb_ofs_y = self->pfb_location_in_base_view.y;
    egui_alpha_t self_alpha = self->alpha;
    egui_alpha_t sample_alpha_table[EGUI_ARRAY_SIZE(line_hq_alpha_table)];
    egui_alpha_t direct_alpha_table[EGUI_ARRAY_SIZE(line_hq_alpha_table)];
    egui_alpha_t solid_alpha;
    egui_alpha_t solid_direct_alpha;
    line_hq_sample_ctx_t sample_ctx;
    int32_t margin = stroke_width * stroke_width;

    line_hq_prepare_alpha_table(alpha, self_alpha, sample_alpha_table, direct_alpha_table);
    solid_alpha = sample_alpha_table[EGUI_ARRAY_SIZE(sample_alpha_table) - 1];
    solid_direct_alpha = direct_alpha_table[EGUI_ARRAY_SIZE(direct_alpha_table) - 1];
    line_hq_init_sample_ctx(&sample_ctx, dx, dy, line_len_sq, half_w_sq_x_len_sq);

    // Pre-compute for per-row x-range narrowing
    // cross = dx*(y1-py) + dy*(px-x1) is linear in px for each row.
    // Solving |cross| <= sqrt(outer_thresh) gives x in [x_center - half_range, x_center + half_range]
    int32_t ady = (dy > 0) ? dy : -dy;
    uint16_t line_len = line_hq_isqrt32((uint32_t)line_len_sq);
    int32_t half_range_x = (int32_t)((int64_t)outer_w * (line_len + 1) / (2 * ady)) + 2;
    int64_t x_step_fp = ((int64_t)dx << 16) / dy;
    int64_t x_center_fp = ((int64_t)x1 << 16) + x_step_fp * (scan_y1 - y1);

    for (egui_dim_t py = scan_y1; py <= scan_y2; py++)
    {
        egui_color_t *dst_row = NULL;
        if (use_direct_pfb)
        {
            dst_row = (egui_color_t *)&self->pfb[(py - pfb_ofs_y) * pfb_width];
        }

        // Narrow x scan range based on cross product geometry
        int32_t x_center = line_hq_fp16_to_int_trunc0(x_center_fp);
        egui_dim_t row_x1 = (egui_dim_t)EGUI_MAX((int32_t)scan_x1, x_center - half_range_x);
        egui_dim_t row_x2 = (egui_dim_t)EGUI_MIN((int32_t)scan_x2, x_center + half_range_x);
        int32_t rel_y = py - y1;
        int32_t rel_x = row_x1 - x1;
        int64_t cross = (int64_t)dx * (y1 - py) - (int64_t)(x1 - row_x1) * dy;
        int64_t dot = (int64_t)(row_x1 - x1) * dx + (int64_t)(py - y1) * dy;

        for (egui_dim_t px = row_x1; px <= row_x2; px++, rel_x++, cross += dy, dot += dx)
        {
            // Fast rejection: check if pixel is near the line
            int64_t cross_sq = cross * cross;
            if (cross_sq > outer_thresh)
            {
                continue;
            }

            // Also reject pixels far beyond endpoints
            if (dot < -(int64_t)margin || dot > line_len_sq + (int64_t)margin)
            {
                continue;
            }

            // Inner zone: fully inside the stroke, skip sampling
            if (inner_w > 0 && cross_sq <= inner_thresh && dot >= 0 && dot <= line_len_sq)
            {
                if (use_direct_pfb)
                {
                    line_hq_blend_direct(&dst_row[px - pfb_ofs_x], color, solid_direct_alpha);
                }
                else
                {
                    egui_canvas_draw_point(px, py, color, solid_alpha);
                }
                continue;
            }

            // Edge zone: sub-pixel sampling
            uint8_t cov_idx = line_hq_get_pixel_coverage(rel_x, rel_y, &sample_ctx);
            if (cov_idx > 0)
            {
                if (use_direct_pfb)
                {
                    line_hq_blend_direct(&dst_row[px - pfb_ofs_x], color, direct_alpha_table[cov_idx]);
                }
                else
                {
                    egui_canvas_draw_point(px, py, color, sample_alpha_table[cov_idx]);
                }
            }
        }

        x_center_fp += x_step_fp;
    }
}

/* ========================== Line Segment HQ ========================== */

void egui_canvas_draw_line_segment_hq(egui_dim_t x1, egui_dim_t y1, egui_dim_t x2, egui_dim_t y2, egui_dim_t stroke_width, egui_color_t color,
                                      egui_alpha_t alpha)
{
    // For HQ, line and line_segment use the same sub-pixel sampling
    // which naturally handles butt caps via the dot product test
    egui_canvas_draw_line_hq(x1, y1, x2, y2, stroke_width, color, alpha);
}

/* ========================== Line Round Cap HQ ========================== */

/**
 * @brief Draw a line with round caps at both endpoints.
 *
 * The round cap extends the line by half_w at each endpoint as a semicircle,
 * giving a smooth rounded appearance. Uses sub-pixel sampling for AA.
 */
void egui_canvas_draw_line_round_cap_hq(egui_dim_t x1, egui_dim_t y1, egui_dim_t x2, egui_dim_t y2, egui_dim_t stroke_width, egui_color_t color,
                                        egui_alpha_t alpha)
{
    egui_canvas_t *self = &canvas_data;

    egui_dim_t deltax = EGUI_ABS(x2 - x1);
    egui_dim_t deltay = EGUI_ABS(y2 - y1);

    // Degenerate: single point -> draw a filled circle
    if (deltax == 0 && deltay == 0)
    {
        egui_dim_t r = stroke_width >> 1;
        if (r > 0)
        {
            egui_canvas_draw_circle_fill_hq(x1, y1, r, color, alpha);
        }
        else
        {
            egui_canvas_draw_point(x1, y1, color, alpha);
        }
        return;
    }

    // Thin lines: delegate to existing Wu's algorithm
    if (stroke_width <= 1)
    {
        egui_canvas_draw_line(x1, y1, x2, y2, stroke_width, color, alpha);
        return;
    }

    // Bounding box check (extended by half_w for round caps)
    egui_dim_t half_sw = (stroke_width >> 1) + 1;
    egui_dim_t bbox_x = EGUI_MIN(x1, x2) - half_sw;
    egui_dim_t bbox_y = EGUI_MIN(y1, y2) - half_sw;
    egui_dim_t bbox_w = deltax + (half_sw << 1) + 1;
    egui_dim_t bbox_h = deltay + (half_sw << 1) + 1;
    EGUI_REGION_DEFINE(region, bbox_x, bbox_y, bbox_w, bbox_h);
    if (!egui_region_is_intersect(&region, &self->base_view_work_region))
    {
        return;
    }

    int32_t dx = (int32_t)(x2 - x1);
    int32_t dy = (int32_t)(y2 - y1);
    int64_t line_len_sq = (int64_t)dx * dx + (int64_t)dy * dy;
    int64_t half_w_sq_x_len_sq = (int64_t)stroke_width * stroke_width * line_len_sq / 4;

    // Scan bounds
    egui_dim_t scan_x1 = EGUI_MIN(x1, x2) - half_sw;
    egui_dim_t scan_y1 = EGUI_MIN(y1, y2) - half_sw;
    egui_dim_t scan_x2 = EGUI_MAX(x1, x2) + half_sw;
    egui_dim_t scan_y2 = EGUI_MAX(y1, y2) + half_sw;

    // Clamp to work region
    egui_dim_t work_x1 = self->base_view_work_region.location.x;
    egui_dim_t work_y1 = self->base_view_work_region.location.y;
    egui_dim_t work_x2 = work_x1 + self->base_view_work_region.size.width;
    egui_dim_t work_y2 = work_y1 + self->base_view_work_region.size.height;

    scan_x1 = EGUI_MAX(scan_x1, work_x1);
    scan_y1 = EGUI_MAX(scan_y1, work_y1);
    scan_x2 = EGUI_MIN(scan_x2, work_x2 - 1);
    scan_y2 = EGUI_MIN(scan_y2, work_y2 - 1);

    // Outer threshold for fast rejection (includes cap radius)
    int32_t outer_w = stroke_width + 2;
    int64_t outer_thresh = (int64_t)outer_w * outer_w * line_len_sq / 4;
    int32_t half_w_sq = (stroke_width * stroke_width) / 4 + stroke_width + 1;

    // Inner threshold
    int32_t inner_w = stroke_width - 2;
    int64_t inner_thresh = (inner_w > 0) ? (int64_t)inner_w * inner_w * line_len_sq / 4 : 0;

    // Direct PFB write setup
    int use_direct_pfb = (self->mask == NULL);
    egui_dim_t pfb_width = self->pfb_region.size.width;
    egui_dim_t pfb_ofs_x = self->pfb_location_in_base_view.x;
    egui_dim_t pfb_ofs_y = self->pfb_location_in_base_view.y;
    egui_alpha_t self_alpha = self->alpha;
    egui_alpha_t sample_alpha_table[EGUI_ARRAY_SIZE(line_hq_alpha_table)];
    egui_alpha_t direct_alpha_table[EGUI_ARRAY_SIZE(line_hq_alpha_table)];
    egui_alpha_t solid_alpha;
    egui_alpha_t solid_direct_alpha;
    line_hq_sample_ctx_t sample_ctx;

    line_hq_prepare_alpha_table(alpha, self_alpha, sample_alpha_table, direct_alpha_table);
    solid_alpha = sample_alpha_table[EGUI_ARRAY_SIZE(sample_alpha_table) - 1];
    solid_direct_alpha = direct_alpha_table[EGUI_ARRAY_SIZE(direct_alpha_table) - 1];
    line_hq_init_sample_ctx(&sample_ctx, dx, dy, line_len_sq, half_w_sq_x_len_sq);

    // Pre-compute for per-row x-range narrowing
    int32_t ady = (dy > 0) ? dy : -dy;
    int32_t half_range_x = 32767; // default: no narrowing (for horizontal lines)
    if (ady > 0)
    {
        uint16_t line_len = line_hq_isqrt32((uint32_t)line_len_sq);
        half_range_x = (int32_t)((int64_t)outer_w * (line_len + 1) / (2 * ady)) + 2;
    }

    for (egui_dim_t py = scan_y1; py <= scan_y2; py++)
    {
        egui_color_t *dst_row = NULL;
        if (use_direct_pfb)
        {
            dst_row = (egui_color_t *)&self->pfb[(py - pfb_ofs_y) * pfb_width];
        }

        // Narrow x scan range based on cross product geometry
        int32_t x_center = (ady > 0) ? x1 + (int32_t)((int64_t)dx * (py - y1) / dy) : (x1 + x2) / 2;
        egui_dim_t row_x1 = (egui_dim_t)EGUI_MAX((int32_t)scan_x1, x_center - half_range_x);
        egui_dim_t row_x2 = (egui_dim_t)EGUI_MIN((int32_t)scan_x2, x_center + half_range_x);
        int32_t rel_y = py - y1;
        int32_t rel_x = row_x1 - x1;
        int64_t cross = (int64_t)dx * (y1 - py) - (int64_t)(x1 - row_x1) * dy;
        int64_t dot = (int64_t)(row_x1 - x1) * dx + (int64_t)(py - y1) * dy;

        for (egui_dim_t px = row_x1; px <= row_x2; px++, rel_x++, cross += dy, dot += dx)
        {
            int64_t cross_sq = cross * cross;

            // Fast rejection: check if pixel is near the line or endpoints
            if (dot >= 0 && dot <= line_len_sq)
            {
                // Within segment projection
                if (cross_sq > outer_thresh)
                {
                    continue;
                }
                // Inner zone fast path
                if (inner_w > 0 && cross_sq <= inner_thresh)
                {
                    if (use_direct_pfb)
                    {
                        line_hq_blend_direct(&dst_row[px - pfb_ofs_x], color, solid_direct_alpha);
                    }
                    else
                    {
                        egui_canvas_draw_point(px, py, color, solid_alpha);
                    }
                    continue;
                }
            }
            else if (dot < 0)
            {
                // Near start cap: check distance to start point
                int32_t dpx = rel_x;
                int32_t dpy = rel_y;
                int32_t d_sq = dpx * dpx + dpy * dpy;
                if (d_sq > half_w_sq)
                {
                    continue;
                }
            }
            else
            {
                // Near end cap: check distance to end point
                int32_t dpx = rel_x - dx;
                int32_t dpy = rel_y - dy;
                int32_t d_sq = dpx * dpx + dpy * dpy;
                if (d_sq > half_w_sq)
                {
                    continue;
                }
            }

            // Sub-pixel sampling with round caps
            uint8_t cov_idx = line_hq_get_pixel_coverage_round_cap(rel_x, rel_y, &sample_ctx, 1, 1);
            if (cov_idx > 0)
            {
                if (use_direct_pfb)
                {
                    line_hq_blend_direct(&dst_row[px - pfb_ofs_x], color, direct_alpha_table[cov_idx]);
                }
                else
                {
                    egui_canvas_draw_point(px, py, color, sample_alpha_table[cov_idx]);
                }
            }
        }
    }
}

/* ========================== Polyline HQ ========================== */

/**
 * @brief Internal polyline drawing with configurable endpoint caps.
 *
 * round_cap: when 1, first and last endpoints get round caps.
 */
static void line_hq_draw_polyline_internal(const egui_dim_t *points, uint8_t count, egui_dim_t stroke_width, egui_color_t color, egui_alpha_t alpha,
                                           int32_t round_cap)
{
    egui_canvas_t *self = &canvas_data;

    if (count < 2)
    {
        return;
    }

    // Thin lines: delegate to existing polyline
    if (stroke_width <= 1)
    {
        egui_canvas_draw_polyline(points, count, stroke_width, color, alpha);
        return;
    }

    // For each pair of adjacent segments, draw with max-coverage at joints
    uint8_t seg_count = count - 1;
    int use_direct_pfb = (self->mask == NULL);
    egui_dim_t pfb_width = self->pfb_region.size.width;
    egui_dim_t pfb_ofs_x = self->pfb_location_in_base_view.x;
    egui_dim_t pfb_ofs_y = self->pfb_location_in_base_view.y;
    egui_alpha_t self_alpha = self->alpha;
    egui_dim_t work_x1 = self->base_view_work_region.location.x;
    egui_dim_t work_y1 = self->base_view_work_region.location.y;
    egui_dim_t work_x2 = work_x1 + self->base_view_work_region.size.width;
    egui_dim_t work_y2 = work_y1 + self->base_view_work_region.size.height;
    egui_alpha_t sample_alpha_table[EGUI_ARRAY_SIZE(line_hq_alpha_table)];
    egui_alpha_t direct_alpha_table[EGUI_ARRAY_SIZE(line_hq_alpha_table)];
    egui_alpha_t solid_alpha;
    egui_alpha_t solid_direct_alpha;

    line_hq_prepare_alpha_table(alpha, self_alpha, sample_alpha_table, direct_alpha_table);
    solid_alpha = sample_alpha_table[EGUI_ARRAY_SIZE(sample_alpha_table) - 1];
    solid_direct_alpha = direct_alpha_table[EGUI_ARRAY_SIZE(direct_alpha_table) - 1];

    // Pre-compute segment data
    for (uint8_t seg = 0; seg < seg_count; seg++)
    {
        int32_t sx1 = points[seg * 2];
        int32_t sy1 = points[seg * 2 + 1];
        int32_t sx2 = points[(seg + 1) * 2];
        int32_t sy2 = points[(seg + 1) * 2 + 1];

        int32_t dx = sx2 - sx1;
        int32_t dy = sy2 - sy1;
        int64_t line_len_sq = (int64_t)dx * dx + (int64_t)dy * dy;

        if (line_len_sq == 0)
        {
            continue;
        }

        int64_t half_w_sq_x_len_sq = (int64_t)stroke_width * stroke_width * line_len_sq / 4;

        // Compute scan bounds for this segment
        egui_dim_t half_sw = (stroke_width >> 1) + 1;
        egui_dim_t scan_x1 = EGUI_MIN(sx1, sx2) - half_sw;
        egui_dim_t scan_y1 = EGUI_MIN(sy1, sy2) - half_sw;
        egui_dim_t scan_x2 = EGUI_MAX(sx1, sx2) + half_sw;
        egui_dim_t scan_y2 = EGUI_MAX(sy1, sy2) + half_sw;

        scan_x1 = EGUI_MAX(scan_x1, work_x1);
        scan_y1 = EGUI_MAX(scan_y1, work_y1);
        scan_x2 = EGUI_MIN(scan_x2, work_x2 - 1);
        scan_y2 = EGUI_MIN(scan_y2, work_y2 - 1);

        // Outer threshold for fast rejection
        int32_t outer_w = stroke_width + 2;
        int64_t outer_thresh = (int64_t)outer_w * outer_w * line_len_sq / 4;

        // Next segment data (for joint handling)
        int32_t has_next = (seg + 1 < seg_count);
        int32_t nx1 = 0, ny1 = 0, nx2 = 0, ny2 = 0;
        int32_t ndx = 0, ndy = 0;
        int64_t nline_len_sq = 0;
        int64_t nhalf_w_sq_x_len_sq = 0;
        line_hq_sample_ctx_t next_sample_ctx = {0};
        if (has_next)
        {
            nx1 = points[(seg + 1) * 2];
            ny1 = points[(seg + 1) * 2 + 1];
            nx2 = points[(seg + 2) * 2];
            ny2 = points[(seg + 2) * 2 + 1];
            ndx = nx2 - nx1;
            ndy = ny2 - ny1;
            nline_len_sq = (int64_t)ndx * ndx + (int64_t)ndy * ndy;
            nhalf_w_sq_x_len_sq = (int64_t)stroke_width * stroke_width * nline_len_sq / 4;
            if (nline_len_sq > 0)
            {
                line_hq_init_sample_ctx(&next_sample_ctx, ndx, ndy, nline_len_sq, nhalf_w_sq_x_len_sq);
            }
        }

        // Inner threshold
        int32_t inner_w = stroke_width - 2;
        int64_t inner_thresh = (inner_w > 0) ? (int64_t)inner_w * inner_w * line_len_sq / 4 : 0;

        int32_t is_first = (seg == 0);
        int32_t is_last = (seg == seg_count - 1);
        int32_t cap_start = (round_cap && is_first);
        int32_t cap_end = (round_cap && is_last);
        int32_t margin = stroke_width * stroke_width;
        int32_t half_w_sq = (stroke_width * stroke_width) / 4 + stroke_width + 1;
        line_hq_sample_ctx_t sample_ctx;

        line_hq_init_sample_ctx(&sample_ctx, dx, dy, line_len_sq, half_w_sq_x_len_sq);

        // Pre-compute for per-row x-range narrowing
        int32_t ady = (dy > 0) ? dy : -dy;
        int32_t half_range_x = 32767;
        if (ady > 0)
        {
            uint16_t line_len = line_hq_isqrt32((uint32_t)line_len_sq);
            half_range_x = (int32_t)((int64_t)outer_w * (line_len + 1) / (2 * ady)) + 2;
        }

        for (egui_dim_t py = scan_y1; py <= scan_y2; py++)
        {
            egui_color_t *dst_row = NULL;
            if (use_direct_pfb)
            {
                dst_row = (egui_color_t *)&self->pfb[(py - pfb_ofs_y) * pfb_width];
            }

            // Narrow x scan range based on cross product geometry
            int32_t x_center = (ady > 0) ? sx1 + (int32_t)((int64_t)dx * (py - sy1) / dy) : (sx1 + sx2) / 2;
            egui_dim_t row_x1 = (egui_dim_t)EGUI_MAX((int32_t)scan_x1, x_center - half_range_x);
            egui_dim_t row_x2 = (egui_dim_t)EGUI_MIN((int32_t)scan_x2, x_center + half_range_x);
            int32_t rel_y = py - sy1;
            int32_t rel_x = row_x1 - sx1;
            int64_t cross = (int64_t)dx * (sy1 - py) - (int64_t)(sx1 - row_x1) * dy;
            int64_t dot = (int64_t)(row_x1 - sx1) * dx + (int64_t)(py - sy1) * dy;

            for (egui_dim_t px = row_x1; px <= row_x2; px++, rel_x++, cross += dy, dot += dx)
            {
                int64_t cross_sq = cross * cross;

                if (dot < 0)
                {
                    if (cap_start)
                    {
                        // Round cap at start: check distance to start point
                        int32_t dpx = rel_x;
                        int32_t dpy = rel_y;
                        if (dpx * dpx + dpy * dpy > half_w_sq)
                        {
                            continue;
                        }
                    }
                    else if (is_first)
                    {
                        if (dot < -(int64_t)margin)
                        {
                            continue;
                        }
                    }
                    else
                    {
                        continue; // Previous segment handles this region
                    }
                }
                else if (dot > line_len_sq)
                {
                    if (cap_end)
                    {
                        // Round cap at end: check distance to end point
                        int32_t dpx = rel_x - dx;
                        int32_t dpy = rel_y - dy;
                        if (dpx * dpx + dpy * dpy > half_w_sq)
                        {
                            continue;
                        }
                    }
                    else if (is_last)
                    {
                        if (dot > line_len_sq + (int64_t)margin)
                        {
                            continue;
                        }
                    }
                    else
                    {
                        continue; // Next segment handles this region
                    }
                }
                else
                {
                    // Within segment projection
                    if (cross_sq > outer_thresh)
                    {
                        continue;
                    }
                    // Inner zone fast path
                    if (inner_w > 0 && cross_sq <= inner_thresh)
                    {
                        if (use_direct_pfb)
                        {
                            line_hq_blend_direct(&dst_row[px - pfb_ofs_x], color, solid_direct_alpha);
                        }
                        else
                        {
                            egui_canvas_draw_point(px, py, color, solid_alpha);
                        }
                        continue;
                    }
                }

                // Edge zone: sub-pixel sampling
                uint8_t cov_idx;
                if (cap_start || cap_end)
                {
                    cov_idx = line_hq_get_pixel_coverage_round_cap(rel_x, rel_y, &sample_ctx, cap_start, cap_end);
                }
                else
                {
                    cov_idx = line_hq_get_pixel_coverage(rel_x, rel_y, &sample_ctx);
                }

                // At the end joint, also sample next segment and take max
                if (has_next && dot > line_len_sq - (int64_t)margin && nline_len_sq > 0)
                {
                    uint8_t ncov_idx = line_hq_get_pixel_coverage(rel_x - dx, rel_y - dy, &next_sample_ctx);
                    if (ncov_idx > cov_idx)
                    {
                        cov_idx = ncov_idx;
                    }
                }

                if (cov_idx > 0)
                {
                    if (use_direct_pfb)
                    {
                        line_hq_blend_direct(&dst_row[px - pfb_ofs_x], color, direct_alpha_table[cov_idx]);
                    }
                    else
                    {
                        egui_canvas_draw_point(px, py, color, sample_alpha_table[cov_idx]);
                    }
                }
            }
        }
    }

    // Fill interior joints to avoid tiny seam pixels between adjacent segments
    // at sharp direction changes.
    if (count > 2)
    {
        egui_dim_t joint_r = stroke_width >> 1;
        if (joint_r > 0)
        {
            for (uint8_t i = 1; i < count - 1; i++)
            {
                egui_canvas_draw_circle_fill_hq(points[i * 2], points[i * 2 + 1], joint_r, color, alpha);
            }
        }
    }
}

void egui_canvas_draw_polyline_hq(const egui_dim_t *points, uint8_t count, egui_dim_t stroke_width, egui_color_t color, egui_alpha_t alpha)
{
    line_hq_draw_polyline_internal(points, count, stroke_width, color, alpha, 0);
}

void egui_canvas_draw_polyline_round_cap_hq(const egui_dim_t *points, uint8_t count, egui_dim_t stroke_width, egui_color_t color, egui_alpha_t alpha)
{
    line_hq_draw_polyline_internal(points, count, stroke_width, color, alpha, 1);
}

/* ========================== Arc Round Cap HQ ========================== */

/* ========================== Arc Round Cap HQ ========================== */

/**
 * @brief Draw an arc stroke with round caps at both endpoints.
 *
 * Draws the arc using egui_canvas_draw_arc_hq(), then adds filled circles
 * at the start and end points of the arc stroke (at the middle radius).
 */
void egui_canvas_draw_arc_round_cap_hq(egui_dim_t cx, egui_dim_t cy, egui_dim_t radius, int16_t start_angle, int16_t end_angle, egui_dim_t stroke_width,
                                       egui_color_t color, egui_alpha_t alpha)
{
    if (radius <= stroke_width)
    {
        egui_canvas_draw_arc_fill_hq(cx, cy, radius, start_angle, end_angle, color, alpha);
        return;
    }
    if (start_angle < 0 || end_angle < 0 || start_angle > end_angle || start_angle == end_angle)
        return;
    if (start_angle > 720 || end_angle > 720)
        return;

    // Draw the arc stroke
    egui_canvas_draw_arc_hq(cx, cy, radius, start_angle, end_angle, stroke_width, color, alpha);

    // Compute cap radius and middle radius (align with activity_ring cap geometry)
    egui_dim_t half_sw = stroke_width >> 1;
    egui_dim_t cap_r = half_sw;
    if (cap_r <= 0)
    {
        return;
    }
    egui_dim_t mid_r = radius - half_sw;
    if (mid_r <= 0)
    {
        return;
    }

    // Normalize angles to [0, 360)
    int16_t sa_norm = start_angle % 360;
    int16_t ea_norm = end_angle % 360;
    if (sa_norm < 0)
        sa_norm += 360;
    if (ea_norm < 0)
        ea_norm += 360;

    // Start/end points using integer trig (scale=256) + signed rounding.
    int32_t sa_cos = line_hq_cos_deg(sa_norm);
    int32_t sa_sin = line_hq_sin_deg(sa_norm);
    int32_t ea_cos = line_hq_cos_deg(ea_norm);
    int32_t ea_sin = line_hq_sin_deg(ea_norm);

    int32_t sx_raw = (int32_t)mid_r * sa_cos;
    int32_t sy_raw = (int32_t)mid_r * sa_sin;
    int32_t ex_raw = (int32_t)mid_r * ea_cos;
    int32_t ey_raw = (int32_t)mid_r * ea_sin;

    egui_dim_t sx = cx + (egui_dim_t)((sx_raw + (sx_raw >= 0 ? 128 : -128)) / 256);
    egui_dim_t sy = cy + (egui_dim_t)((sy_raw + (sy_raw >= 0 ? 128 : -128)) / 256);
    egui_dim_t ex = cx + (egui_dim_t)((ex_raw + (ex_raw >= 0 ? 128 : -128)) / 256);
    egui_dim_t ey = cy + (egui_dim_t)((ey_raw + (ey_raw >= 0 ? 128 : -128)) / 256);

    // Draw round caps as filled circles
    egui_canvas_draw_circle_fill_hq(sx, sy, cap_r, color, alpha);
    egui_canvas_draw_circle_fill_hq(ex, ey, cap_r, color, alpha);
}
