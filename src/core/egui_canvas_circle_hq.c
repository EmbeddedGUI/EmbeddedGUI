#include "egui_canvas.h"
#include "egui_api.h"
#include "utils/egui_fixmath.h"

/**
 * @brief High-quality circle/arc drawing with SDF-based anti-aliasing.
 *
 * Uses signed-distance-field (SDF) for smooth edge coverage:
 * d = (dx^2 + dy^2 - r^2) / (2*r), mapped linearly to alpha.
 * Works correctly at all orientations without sampling artifacts.
 *
 * Performance options:
 * - EGUI_CONFIG_CIRCLE_HQ_INT32_ONLY: Limit radius to avoid 64-bit overflow
 */

/* Precomputed reciprocal: inv128_r = (128 << 16) / radius, computed once per draw call.
 * In the inner loop:  diff * inv128_r >> 16  ≈  diff * 128 / radius
 * For edge pixels |diff| <= 2*radius+1, product fits int32_t. */
#define CIRCLE_HQ_INV_SHIFT 16

__EGUI_STATIC_INLINE__ int32_t circle_hq_precompute_inv128(int32_t radius)
{
    return (128 << CIRCLE_HQ_INV_SHIFT) / radius;
}

__EGUI_STATIC_INLINE__ egui_alpha_t circle_hq_get_edge_alpha(int32_t dx, int32_t dy, int32_t r_sq, int32_t inv128_r)
{
    /* SDF-based anti-aliasing for circles using precomputed reciprocal.
     * Signed distance from boundary at point (dx,dy):
     *   d = (dx^2 + dy^2 - r^2) / (2*r)
     * Map d in [-0.5, +0.5] to alpha in [255, 0] linearly.
     */
    int32_t d_sq = dx * dx + dy * dy;
    int32_t diff = d_sq - r_sq;
    int32_t d_shifted = diff * inv128_r >> CIRCLE_HQ_INV_SHIFT;
    int32_t alpha = 128 - d_shifted;
    if (alpha <= 0)
        return 0;
    if (alpha >= 255)
        return 255;
    return (egui_alpha_t)alpha;
}

/* Variant that takes pre-computed d_sq to avoid redundant dx*dx+dy*dy */
__EGUI_STATIC_INLINE__ egui_alpha_t circle_hq_get_edge_alpha_from_dsq(int32_t d_sq, int32_t r_sq, int32_t inv128_r)
{
    int32_t diff = d_sq - r_sq;
    int32_t d_shifted = diff * inv128_r >> CIRCLE_HQ_INV_SHIFT;
    int32_t alpha = 128 - d_shifted;
    if (alpha <= 0)
        return 0;
    if (alpha >= 255)
        return 255;
    return (egui_alpha_t)alpha;
}

__EGUI_STATIC_INLINE__ int32_t circle_hq_isqrt(int32_t n)
{
    if (n <= 0)
        return 0;
    int32_t x = n;
    int32_t y = (x + 1) >> 1;
    while (y < x)
    {
        x = y;
        y = (x + n / x) >> 1;
    }
    return x;
}

__EGUI_STATIC_INLINE__ void circle_hq_blend_direct(egui_color_t *dst, egui_color_t color, egui_alpha_t alpha)
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

__EGUI_STATIC_INLINE__ void circle_hq_fill_direct_span(egui_color_t *dst_base, egui_dim_t x_start, egui_dim_t x_end, egui_color_t color, egui_alpha_t alpha)
{
    egui_color_int_t *dst;
    uint32_t count;

    if (alpha == 0 || x_start > x_end)
    {
        return;
    }

    dst = (egui_color_int_t *)&dst_base[x_start];
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

/* ========================== Circle Fill HQ ========================== */

void egui_canvas_draw_circle_fill_hq(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, egui_color_t color, egui_alpha_t alpha)
{
    egui_canvas_t *self = &canvas_data;
    if (radius <= 0)
    {
        if (radius == 0)
            egui_canvas_draw_point(center_x, center_y, color, alpha);
        return;
    }
#if EGUI_CONFIG_CIRCLE_HQ_INT32_ONLY
    EGUI_ASSERT(radius <= EGUI_CONFIG_CIRCLE_HQ_MAX_RADIUS);
#endif
    EGUI_REGION_DEFINE(region, center_x - radius, center_y - radius, (radius << 1) + 1, (radius << 1) + 1);
    egui_region_t ri;
    egui_region_intersect(&region, &self->base_view_work_region, &ri);
    if (egui_region_is_empty(&ri))
        return;

    int32_t r_sq = (int32_t)radius * radius;
    int32_t inv128_r = circle_hq_precompute_inv128(radius);
    int32_t r_inner_sq = (int32_t)(radius - 1) * (radius - 1);
    int32_t r_outer_sq = (int32_t)(radius + 1) * (radius + 1);
    egui_dim_t y_start = ri.location.y, y_end = ri.location.y + ri.size.height;
    egui_dim_t x_start = ri.location.x, x_end = ri.location.x + ri.size.width;

    /* Direct PFB write setup */
    int use_direct_pfb = (self->mask == NULL) ? 1 : 0;
    egui_dim_t pfb_width = self->pfb_region.size.width;
    egui_dim_t pfb_ofs_x = self->pfb_location_in_base_view.x;
    egui_dim_t pfb_ofs_y = self->pfb_location_in_base_view.y;
    egui_alpha_t eff_alpha = egui_color_alpha_mix(self->alpha, alpha);
    int apply_draw_alpha = (alpha != EGUI_ALPHA_100);
    int apply_canvas_alpha = (eff_alpha != EGUI_ALPHA_100);

    for (egui_dim_t y = y_start; y < y_end; y++)
    {
        int32_t dy = (int32_t)(y - center_y);
        int32_t dy_sq = dy * dy;
        if (dy_sq > r_outer_sq)
            continue;

        int32_t x_inner_half = (dy_sq < r_inner_sq) ? circle_hq_isqrt(r_inner_sq - dy_sq) : 0;
        int32_t x_outer_half = circle_hq_isqrt(r_outer_sq - dy_sq);

        egui_dim_t ol = center_x - x_outer_half, or_ = center_x + x_outer_half;
        if (ol < x_start)
            ol = x_start;
        if (or_ >= x_end)
            or_ = x_end - 1;

        if (use_direct_pfb)
        {
            egui_color_t *dst_base = (egui_color_t *)&self->pfb[(y - pfb_ofs_y) * pfb_width - pfb_ofs_x];

            if (x_inner_half > 0)
            {
                egui_dim_t il = center_x - x_inner_half + 1, i_r = center_x + x_inner_half - 1;
                /* Left edge */
                for (egui_dim_t x = ol; x < il && x <= or_; x++)
                {
                    egui_alpha_t cov = circle_hq_get_edge_alpha((int32_t)(x - center_x), dy, r_sq, inv128_r);
                    if (cov > 0)
                    {
                        egui_alpha_t m = apply_canvas_alpha ? egui_color_alpha_mix(eff_alpha, cov) : cov;
                        if (m > 0)
                        {
                            egui_color_t *back_color = &dst_base[x];
                            if (m == EGUI_ALPHA_100)
                                *back_color = color;
                            else
                                egui_rgb_mix_ptr(back_color, &color, back_color, m);
                        }
                    }
                }
                /* Interior: direct fill */
                egui_dim_t bl = il < x_start ? x_start : il, br = i_r >= x_end ? x_end - 1 : i_r;
                if (bl <= br)
                {
                    circle_hq_fill_direct_span(dst_base, bl, br, color, eff_alpha);
                }
                /* Right edge */
                egui_dim_t rs = i_r + 1 < x_start ? x_start : i_r + 1;
                for (egui_dim_t x = rs; x <= or_; x++)
                {
                    egui_alpha_t cov = circle_hq_get_edge_alpha((int32_t)(x - center_x), dy, r_sq, inv128_r);
                    if (cov > 0)
                    {
                        egui_alpha_t m = apply_canvas_alpha ? egui_color_alpha_mix(eff_alpha, cov) : cov;
                        if (m > 0)
                        {
                            egui_color_t *back_color = &dst_base[x];
                            if (m == EGUI_ALPHA_100)
                                *back_color = color;
                            else
                                egui_rgb_mix_ptr(back_color, &color, back_color, m);
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
                    if (d_sq < r_inner_sq)
                    {
                        egui_color_t *back_color = &dst_base[x];
                        if (eff_alpha == EGUI_ALPHA_100)
                            *back_color = color;
                        else
                            egui_rgb_mix_ptr(back_color, &color, back_color, eff_alpha);
                        continue;
                    }
                    egui_alpha_t cov = circle_hq_get_edge_alpha(dx, dy, r_sq, inv128_r);
                    if (cov > 0)
                    {
                        egui_alpha_t m = apply_canvas_alpha ? egui_color_alpha_mix(eff_alpha, cov) : cov;
                        if (m > 0)
                        {
                            egui_color_t *back_color = &dst_base[x];
                            if (m == EGUI_ALPHA_100)
                                *back_color = color;
                            else
                                egui_rgb_mix_ptr(back_color, &color, back_color, m);
                        }
                    }
                }
            }
        }
        else
        {
            if (x_inner_half > 0)
            {
                egui_dim_t il = center_x - x_inner_half + 1, i_r = center_x + x_inner_half - 1;
                for (egui_dim_t x = ol; x < il && x <= or_; x++)
                {
                    egui_alpha_t cov = circle_hq_get_edge_alpha((int32_t)(x - center_x), dy, r_sq, inv128_r);
                    if (cov > 0)
                    {
                        egui_alpha_t m = apply_draw_alpha ? egui_color_alpha_mix(alpha, cov) : cov;
                        if (m > 0)
                            egui_canvas_draw_point(x, y, color, m);
                    }
                }
                egui_dim_t bl = il < x_start ? x_start : il, br = i_r >= x_end ? x_end - 1 : i_r;
                if (bl <= br)
                    egui_canvas_draw_fillrect(bl, y, br - bl + 1, 1, color, alpha);
                egui_dim_t rs = i_r + 1 < x_start ? x_start : i_r + 1;
                for (egui_dim_t x = rs; x <= or_; x++)
                {
                    egui_alpha_t cov = circle_hq_get_edge_alpha((int32_t)(x - center_x), dy, r_sq, inv128_r);
                    if (cov > 0)
                    {
                        egui_alpha_t m = apply_draw_alpha ? egui_color_alpha_mix(alpha, cov) : cov;
                        if (m > 0)
                            egui_canvas_draw_point(x, y, color, m);
                    }
                }
            }
            else
            {
                for (egui_dim_t x = ol; x <= or_; x++)
                {
                    int32_t dx = (int32_t)(x - center_x);
                    int32_t d_sq = dx * dx + dy_sq;
                    if (d_sq < r_inner_sq)
                    {
                        egui_canvas_draw_point(x, y, color, alpha);
                        continue;
                    }
                    egui_alpha_t cov = circle_hq_get_edge_alpha(dx, dy, r_sq, inv128_r);
                    if (cov > 0)
                    {
                        egui_alpha_t m = apply_draw_alpha ? egui_color_alpha_mix(alpha, cov) : cov;
                        if (m > 0)
                            egui_canvas_draw_point(x, y, color, m);
                    }
                }
            }
        }
    }
}

/* ========================== Arc helpers ========================== */

static const egui_float_t circle_hq_tan_val_list[91] = {
        EGUI_FLOAT_VALUE(0.000000f),  EGUI_FLOAT_VALUE(0.017455f),  EGUI_FLOAT_VALUE(0.034921f),  EGUI_FLOAT_VALUE(0.052408f),  EGUI_FLOAT_VALUE(0.069927f),
        EGUI_FLOAT_VALUE(0.087489f),  EGUI_FLOAT_VALUE(0.105104f),  EGUI_FLOAT_VALUE(0.122785f),  EGUI_FLOAT_VALUE(0.140541f),  EGUI_FLOAT_VALUE(0.158384f),
        EGUI_FLOAT_VALUE(0.176327f),  EGUI_FLOAT_VALUE(0.194380f),  EGUI_FLOAT_VALUE(0.212557f),  EGUI_FLOAT_VALUE(0.230868f),  EGUI_FLOAT_VALUE(0.249328f),
        EGUI_FLOAT_VALUE(0.267949f),  EGUI_FLOAT_VALUE(0.286745f),  EGUI_FLOAT_VALUE(0.305731f),  EGUI_FLOAT_VALUE(0.324920f),  EGUI_FLOAT_VALUE(0.344328f),
        EGUI_FLOAT_VALUE(0.363970f),  EGUI_FLOAT_VALUE(0.383864f),  EGUI_FLOAT_VALUE(0.404026f),  EGUI_FLOAT_VALUE(0.424475f),  EGUI_FLOAT_VALUE(0.445229f),
        EGUI_FLOAT_VALUE(0.466308f),  EGUI_FLOAT_VALUE(0.487733f),  EGUI_FLOAT_VALUE(0.509525f),  EGUI_FLOAT_VALUE(0.531709f),  EGUI_FLOAT_VALUE(0.554309f),
        EGUI_FLOAT_VALUE(0.577350f),  EGUI_FLOAT_VALUE(0.600861f),  EGUI_FLOAT_VALUE(0.624869f),  EGUI_FLOAT_VALUE(0.649408f),  EGUI_FLOAT_VALUE(0.674509f),
        EGUI_FLOAT_VALUE(0.700208f),  EGUI_FLOAT_VALUE(0.726543f),  EGUI_FLOAT_VALUE(0.753554f),  EGUI_FLOAT_VALUE(0.781286f),  EGUI_FLOAT_VALUE(0.809784f),
        EGUI_FLOAT_VALUE(0.839100f),  EGUI_FLOAT_VALUE(0.869287f),  EGUI_FLOAT_VALUE(0.900404f),  EGUI_FLOAT_VALUE(0.932515f),  EGUI_FLOAT_VALUE(0.965689f),
        EGUI_FLOAT_VALUE(1.000000f),  EGUI_FLOAT_VALUE(1.035530f),  EGUI_FLOAT_VALUE(1.072369f),  EGUI_FLOAT_VALUE(1.110613f),  EGUI_FLOAT_VALUE(1.150368f),
        EGUI_FLOAT_VALUE(1.191754f),  EGUI_FLOAT_VALUE(1.234897f),  EGUI_FLOAT_VALUE(1.279942f),  EGUI_FLOAT_VALUE(1.327045f),  EGUI_FLOAT_VALUE(1.376382f),
        EGUI_FLOAT_VALUE(1.428148f),  EGUI_FLOAT_VALUE(1.482561f),  EGUI_FLOAT_VALUE(1.539865f),  EGUI_FLOAT_VALUE(1.600335f),  EGUI_FLOAT_VALUE(1.664279f),
        EGUI_FLOAT_VALUE(1.732051f),  EGUI_FLOAT_VALUE(1.804048f),  EGUI_FLOAT_VALUE(1.880726f),  EGUI_FLOAT_VALUE(1.962611f),  EGUI_FLOAT_VALUE(2.050304f),
        EGUI_FLOAT_VALUE(2.144507f),  EGUI_FLOAT_VALUE(2.246037f),  EGUI_FLOAT_VALUE(2.355852f),  EGUI_FLOAT_VALUE(2.475087f),  EGUI_FLOAT_VALUE(2.605089f),
        EGUI_FLOAT_VALUE(2.747477f),  EGUI_FLOAT_VALUE(2.904211f),  EGUI_FLOAT_VALUE(3.077684f),  EGUI_FLOAT_VALUE(3.270853f),  EGUI_FLOAT_VALUE(3.487414f),
        EGUI_FLOAT_VALUE(3.732051f),  EGUI_FLOAT_VALUE(4.010781f),  EGUI_FLOAT_VALUE(4.331476f),  EGUI_FLOAT_VALUE(4.704630f),  EGUI_FLOAT_VALUE(5.144554f),
        EGUI_FLOAT_VALUE(5.671282f),  EGUI_FLOAT_VALUE(6.313752f),  EGUI_FLOAT_VALUE(7.115370f),  EGUI_FLOAT_VALUE(8.144346f),  EGUI_FLOAT_VALUE(9.514364f),
        EGUI_FLOAT_VALUE(11.430052f), EGUI_FLOAT_VALUE(14.300666f), EGUI_FLOAT_VALUE(19.081137f), EGUI_FLOAT_VALUE(28.636253f), EGUI_FLOAT_VALUE(57.289962f),
        EGUI_FLOAT_VALUE(9999.0f),
};

static const egui_float_t circle_hq_cos_val_list[91] = {
        EGUI_FLOAT_VALUE(1.000000f), EGUI_FLOAT_VALUE(0.999848f), EGUI_FLOAT_VALUE(0.999391f), EGUI_FLOAT_VALUE(0.998630f), EGUI_FLOAT_VALUE(0.997564f),
        EGUI_FLOAT_VALUE(0.996195f), EGUI_FLOAT_VALUE(0.994522f), EGUI_FLOAT_VALUE(0.992546f), EGUI_FLOAT_VALUE(0.990268f), EGUI_FLOAT_VALUE(0.987688f),
        EGUI_FLOAT_VALUE(0.984808f), EGUI_FLOAT_VALUE(0.981627f), EGUI_FLOAT_VALUE(0.978148f), EGUI_FLOAT_VALUE(0.974370f), EGUI_FLOAT_VALUE(0.970296f),
        EGUI_FLOAT_VALUE(0.965926f), EGUI_FLOAT_VALUE(0.961262f), EGUI_FLOAT_VALUE(0.956305f), EGUI_FLOAT_VALUE(0.951057f), EGUI_FLOAT_VALUE(0.945519f),
        EGUI_FLOAT_VALUE(0.939693f), EGUI_FLOAT_VALUE(0.933580f), EGUI_FLOAT_VALUE(0.927184f), EGUI_FLOAT_VALUE(0.920505f), EGUI_FLOAT_VALUE(0.913545f),
        EGUI_FLOAT_VALUE(0.906308f), EGUI_FLOAT_VALUE(0.898794f), EGUI_FLOAT_VALUE(0.891007f), EGUI_FLOAT_VALUE(0.882948f), EGUI_FLOAT_VALUE(0.874620f),
        EGUI_FLOAT_VALUE(0.866025f), EGUI_FLOAT_VALUE(0.857167f), EGUI_FLOAT_VALUE(0.848048f), EGUI_FLOAT_VALUE(0.838671f), EGUI_FLOAT_VALUE(0.829038f),
        EGUI_FLOAT_VALUE(0.819152f), EGUI_FLOAT_VALUE(0.809017f), EGUI_FLOAT_VALUE(0.798636f), EGUI_FLOAT_VALUE(0.788011f), EGUI_FLOAT_VALUE(0.777146f),
        EGUI_FLOAT_VALUE(0.766044f), EGUI_FLOAT_VALUE(0.754710f), EGUI_FLOAT_VALUE(0.743145f), EGUI_FLOAT_VALUE(0.731354f), EGUI_FLOAT_VALUE(0.719340f),
        EGUI_FLOAT_VALUE(0.707107f), EGUI_FLOAT_VALUE(0.694658f), EGUI_FLOAT_VALUE(0.681998f), EGUI_FLOAT_VALUE(0.669131f), EGUI_FLOAT_VALUE(0.656059f),
        EGUI_FLOAT_VALUE(0.642788f), EGUI_FLOAT_VALUE(0.629320f), EGUI_FLOAT_VALUE(0.615661f), EGUI_FLOAT_VALUE(0.601815f), EGUI_FLOAT_VALUE(0.587785f),
        EGUI_FLOAT_VALUE(0.573576f), EGUI_FLOAT_VALUE(0.559193f), EGUI_FLOAT_VALUE(0.544639f), EGUI_FLOAT_VALUE(0.529919f), EGUI_FLOAT_VALUE(0.515038f),
        EGUI_FLOAT_VALUE(0.500000f), EGUI_FLOAT_VALUE(0.484810f), EGUI_FLOAT_VALUE(0.469472f), EGUI_FLOAT_VALUE(0.453990f), EGUI_FLOAT_VALUE(0.438371f),
        EGUI_FLOAT_VALUE(0.422618f), EGUI_FLOAT_VALUE(0.406737f), EGUI_FLOAT_VALUE(0.390731f), EGUI_FLOAT_VALUE(0.374607f), EGUI_FLOAT_VALUE(0.358368f),
        EGUI_FLOAT_VALUE(0.342020f), EGUI_FLOAT_VALUE(0.325568f), EGUI_FLOAT_VALUE(0.309017f), EGUI_FLOAT_VALUE(0.292372f), EGUI_FLOAT_VALUE(0.275637f),
        EGUI_FLOAT_VALUE(0.258819f), EGUI_FLOAT_VALUE(0.241922f), EGUI_FLOAT_VALUE(0.224951f), EGUI_FLOAT_VALUE(0.207912f), EGUI_FLOAT_VALUE(0.190809f),
        EGUI_FLOAT_VALUE(0.173648f), EGUI_FLOAT_VALUE(0.156434f), EGUI_FLOAT_VALUE(0.139173f), EGUI_FLOAT_VALUE(0.121869f), EGUI_FLOAT_VALUE(0.104528f),
        EGUI_FLOAT_VALUE(0.087156f), EGUI_FLOAT_VALUE(0.069756f), EGUI_FLOAT_VALUE(0.052336f), EGUI_FLOAT_VALUE(0.034899f), EGUI_FLOAT_VALUE(0.017452f),
        EGUI_FLOAT_VALUE(0.000000f),
};

#define CIRCLE_HQ_ARC_AA_HALF 49152

__EGUI_STATIC_INLINE__ egui_alpha_t circle_hq_arc_edge_smoothstep(egui_float_t signed_dist)
{
    if (signed_dist >= CIRCLE_HQ_ARC_AA_HALF)
        return EGUI_ALPHA_100;
    if (signed_dist <= -CIRCLE_HQ_ARC_AA_HALF)
        return EGUI_ALPHA_0;
    int32_t coverage = signed_dist + CIRCLE_HQ_ARC_AA_HALF;
    int32_t t = (int32_t)(((int64_t)coverage * 43691) >> EGUI_FLOAT_FRAC);
    if (t < 0)
        t = 0;
    else if (t > (1 << EGUI_FLOAT_FRAC))
        t = (1 << EGUI_FLOAT_FRAC);
    int32_t t_sq = (int32_t)(((int64_t)t * t) >> EGUI_FLOAT_FRAC);
    int32_t smooth = (int32_t)(((int64_t)t_sq * (3 * (1 << EGUI_FLOAT_FRAC) - 2 * t)) >> EGUI_FLOAT_FRAC);
    egui_alpha_t a = (egui_alpha_t)((smooth * EGUI_ALPHA_100) >> EGUI_FLOAT_FRAC);
    return (a > EGUI_ALPHA_100) ? EGUI_ALPHA_100 : a;
}

__EGUI_STATIC_INLINE__ egui_alpha_t circle_hq_arc_angle_alpha(egui_dim_t x, egui_dim_t y, int16_t start_angle, int16_t end_angle)
{
    egui_float_t px = EGUI_FLOAT_VALUE_INT(x);
    egui_float_t py = EGUI_FLOAT_VALUE_INT(y);
    egui_alpha_t alpha_start = EGUI_ALPHA_100, alpha_end = EGUI_ALPHA_100;

    if (start_angle != 0)
    {
        egui_float_t raw = py - EGUI_FLOAT_MULT(circle_hq_tan_val_list[start_angle], px);
        alpha_start = circle_hq_arc_edge_smoothstep(EGUI_FLOAT_MULT(raw, circle_hq_cos_val_list[start_angle]));
        if (alpha_start == EGUI_ALPHA_0)
            return EGUI_ALPHA_0;
    }
    if (end_angle != 90)
    {
        egui_float_t raw = EGUI_FLOAT_MULT(circle_hq_tan_val_list[end_angle], px) - py;
        alpha_end = circle_hq_arc_edge_smoothstep(EGUI_FLOAT_MULT(raw, circle_hq_cos_val_list[end_angle]));
        if (alpha_end == EGUI_ALPHA_0)
            return EGUI_ALPHA_0;
    }
    return egui_color_alpha_mix(alpha_start, alpha_end);
}

typedef struct circle_hq_arc_row_ctx_t
{
    egui_float_t start_signed;
    egui_float_t end_signed;
    egui_float_t start_step;
    egui_float_t end_step;
    uint8_t has_start;
    uint8_t has_end;
} circle_hq_arc_row_ctx_t;

static void circle_hq_arc_row_ctx_init(circle_hq_arc_row_ctx_t *ctx, egui_dim_t qx, egui_dim_t qy, int16_t start_angle, int16_t end_angle, int32_t qx_step)
{
    egui_float_t qx_f = EGUI_FLOAT_VALUE_INT(qx);
    egui_float_t qy_f = EGUI_FLOAT_VALUE_INT(qy);

    ctx->has_start = (start_angle != 0);
    ctx->has_end = (end_angle != 90);

    if (ctx->has_start)
    {
        egui_float_t tan_sa = circle_hq_tan_val_list[start_angle];
        egui_float_t cos_sa = circle_hq_cos_val_list[start_angle];
        egui_float_t step = EGUI_FLOAT_MULT(tan_sa, cos_sa);
        egui_float_t raw = qy_f - EGUI_FLOAT_MULT(tan_sa, qx_f);

        ctx->start_signed = EGUI_FLOAT_MULT(raw, cos_sa);
        ctx->start_step = (qx_step > 0) ? -step : step;
    }
    else
    {
        ctx->start_signed = 0;
        ctx->start_step = 0;
    }

    if (ctx->has_end)
    {
        egui_float_t tan_ea = circle_hq_tan_val_list[end_angle];
        egui_float_t cos_ea = circle_hq_cos_val_list[end_angle];
        egui_float_t step = EGUI_FLOAT_MULT(tan_ea, cos_ea);
        egui_float_t raw = EGUI_FLOAT_MULT(tan_ea, qx_f) - qy_f;

        ctx->end_signed = EGUI_FLOAT_MULT(raw, cos_ea);
        ctx->end_step = (qx_step > 0) ? step : -step;
    }
    else
    {
        ctx->end_signed = 0;
        ctx->end_step = 0;
    }
}

__EGUI_STATIC_INLINE__ egui_alpha_t circle_hq_arc_angle_alpha_from_ctx(const circle_hq_arc_row_ctx_t *ctx)
{
    egui_alpha_t alpha_start = EGUI_ALPHA_100;
    egui_alpha_t alpha_end = EGUI_ALPHA_100;

    if (ctx->has_start)
    {
        alpha_start = circle_hq_arc_edge_smoothstep(ctx->start_signed);
        if (alpha_start == EGUI_ALPHA_0 && !ctx->has_end)
        {
            return EGUI_ALPHA_0;
        }
    }

    if (ctx->has_end)
    {
        alpha_end = circle_hq_arc_edge_smoothstep(ctx->end_signed);
        if (alpha_end == EGUI_ALPHA_0 && alpha_start == EGUI_ALPHA_100)
        {
            return EGUI_ALPHA_0;
        }
    }

    return egui_color_alpha_mix(alpha_start, alpha_end);
}

__EGUI_STATIC_INLINE__ void circle_hq_arc_row_ctx_step(circle_hq_arc_row_ctx_t *ctx)
{
    if (ctx->has_start)
    {
        ctx->start_signed += ctx->start_step;
    }
    if (ctx->has_end)
    {
        ctx->end_signed += ctx->end_step;
    }
}

enum
{
    CIRCLE_HQ_TYPE_LEFT_TOP,
    CIRCLE_HQ_TYPE_LEFT_BOTTOM,
    CIRCLE_HQ_TYPE_RIGHT_TOP,
    CIRCLE_HQ_TYPE_RIGHT_BOTTOM
};

/* ========================== Quadrant bounding box helper ========================== */

static void circle_hq_get_quadrant_region(egui_dim_t cx, egui_dim_t cy, egui_dim_t r, int type, egui_region_t *out)
{
    switch (type)
    {
    case CIRCLE_HQ_TYPE_LEFT_TOP:
        egui_region_init(out, cx - r, cy - r, r, r + 1);
        break;
    case CIRCLE_HQ_TYPE_LEFT_BOTTOM:
        egui_region_init(out, cx - r, cy + 1, r + 1, r);
        break;
    case CIRCLE_HQ_TYPE_RIGHT_TOP:
        egui_region_init(out, cx, cy - r, r + 1, r);
        break;
    case CIRCLE_HQ_TYPE_RIGHT_BOTTOM:
    default:
        egui_region_init(out, cx + 1, cy, r, r + 1);
        break;
    }
}

static void circle_hq_to_quadrant_coords(egui_dim_t x, egui_dim_t y, egui_dim_t cx, egui_dim_t cy, int type, egui_dim_t *qx, egui_dim_t *qy)
{
    switch (type)
    {
    case CIRCLE_HQ_TYPE_LEFT_TOP:
        *qx = cx - x;
        *qy = cy - y;
        break;
    case CIRCLE_HQ_TYPE_LEFT_BOTTOM:
        *qx = cx - x;
        *qy = y - cy;
        break;
    case CIRCLE_HQ_TYPE_RIGHT_TOP:
        *qx = x - cx;
        *qy = cy - y;
        break;
    case CIRCLE_HQ_TYPE_RIGHT_BOTTOM:
    default:
        *qx = x - cx;
        *qy = y - cy;
        break;
    }
}

/* ========================== Circle Stroke HQ ========================== */

void egui_canvas_draw_circle_hq(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, egui_dim_t stroke_width, egui_color_t color, egui_alpha_t alpha)
{
    egui_canvas_t *self = &canvas_data;
    if (radius <= 0)
        return;
    if (radius <= stroke_width)
    {
        egui_canvas_draw_circle_fill_hq(center_x, center_y, radius, color, alpha);
        return;
    }
#if EGUI_CONFIG_CIRCLE_HQ_INT32_ONLY
    EGUI_ASSERT(radius <= EGUI_CONFIG_CIRCLE_HQ_MAX_RADIUS);
#endif
    EGUI_REGION_DEFINE(region, center_x - radius, center_y - radius, (radius << 1) + 1, (radius << 1) + 1);
    egui_region_t ri;
    egui_region_intersect(&region, &self->base_view_work_region, &ri);
    if (egui_region_is_empty(&ri))
        return;

    int32_t outer_inner_sq = (int32_t)(radius - 1) * (radius - 1);
    int32_t outer_outer_sq = (int32_t)(radius + 1) * (radius + 1);
    egui_dim_t ir = radius - stroke_width;
    int32_t inner_inner_sq = (ir > 1) ? (int32_t)(ir - 1) * (ir - 1) : 0;
    int32_t inner_outer_sq = (int32_t)(ir + 1) * (ir + 1);
    int32_t outer_r_sq = (int32_t)radius * radius;
    int32_t outer_inv128 = circle_hq_precompute_inv128(radius);
    int32_t inner_r_sq = (int32_t)ir * ir;
    int32_t inner_inv128 = (ir > 0) ? circle_hq_precompute_inv128(ir) : 0;

    egui_dim_t y_start = ri.location.y, y_end = ri.location.y + ri.size.height;
    egui_dim_t x_start = ri.location.x, x_end = ri.location.x + ri.size.width;

    /* Direct PFB write setup */
    int use_direct_pfb = (self->mask == NULL) ? 1 : 0;
    egui_dim_t pfb_width = self->pfb_region.size.width;
    egui_dim_t pfb_ofs_x = self->pfb_location_in_base_view.x;
    egui_dim_t pfb_ofs_y = self->pfb_location_in_base_view.y;
    egui_alpha_t eff_alpha = egui_color_alpha_mix(self->alpha, alpha);
    int apply_draw_alpha = (alpha != EGUI_ALPHA_100);
    int apply_canvas_alpha = (eff_alpha != EGUI_ALPHA_100);

    for (egui_dim_t y = y_start; y < y_end; y++)
    {
        int32_t dy = (int32_t)(y - center_y), dy_sq = dy * dy;
        if (dy_sq > outer_outer_sq)
            continue;

        /* Narrow x-range to outer circle boundary */
        int32_t x_outer_half = circle_hq_isqrt(outer_outer_sq - dy_sq);
        egui_dim_t ol = EGUI_MAX(center_x - (egui_dim_t)x_outer_half, x_start);
        egui_dim_t or_ = EGUI_MIN(center_x + (egui_dim_t)x_outer_half, x_end - 1);

        if (use_direct_pfb)
        {
            egui_color_t *dst_base = (egui_color_t *)&self->pfb[(y - pfb_ofs_y) * pfb_width - pfb_ofs_x];

            for (egui_dim_t x = ol; x <= or_; x++)
            {
                int32_t dx = (int32_t)(x - center_x);
                int32_t d_sq = dx * dx + dy_sq;

                egui_alpha_t oc;
                if (d_sq < outer_inner_sq)
                    oc = EGUI_ALPHA_100;
                else
                {
                    oc = circle_hq_get_edge_alpha(dx, dy, outer_r_sq, outer_inv128);
                    if (oc == 0)
                        continue;
                }

                egui_alpha_t ic = 0;
                if (d_sq < inner_inner_sq)
                    ic = EGUI_ALPHA_100;
                else if (d_sq <= inner_outer_sq)
                    ic = circle_hq_get_edge_alpha(dx, dy, inner_r_sq, inner_inv128);

                if (oc > ic)
                {
                    egui_alpha_t m = apply_canvas_alpha ? egui_color_alpha_mix(eff_alpha, oc - ic) : (egui_alpha_t)(oc - ic);
                    if (m > 0)
                    {
                        egui_color_t *back_color = &dst_base[x];
                        if (m == EGUI_ALPHA_100)
                            *back_color = color;
                        else
                            egui_rgb_mix_ptr(back_color, &color, back_color, m);
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

                egui_alpha_t oc;
                if (d_sq < outer_inner_sq)
                    oc = EGUI_ALPHA_100;
                else
                {
                    oc = circle_hq_get_edge_alpha(dx, dy, outer_r_sq, outer_inv128);
                    if (oc == 0)
                        continue;
                }

                egui_alpha_t ic = 0;
                if (d_sq < inner_inner_sq)
                    ic = EGUI_ALPHA_100;
                else if (d_sq <= inner_outer_sq)
                    ic = circle_hq_get_edge_alpha(dx, dy, inner_r_sq, inner_inv128);

                if (oc > ic)
                {
                    egui_alpha_t m = apply_draw_alpha ? egui_color_alpha_mix(alpha, oc - ic) : (egui_alpha_t)(oc - ic);
                    if (m > 0)
                        egui_canvas_draw_point(x, y, color, m);
                }
            }
        }
    }
}

/* ========================== Arc Corner Fill HQ ========================== */

static void circle_hq_draw_arc_corner_fill(egui_dim_t cx, egui_dim_t cy, egui_dim_t radius, int16_t sa, int16_t ea, int type, egui_color_t color,
                                           egui_alpha_t alpha)
{
    egui_canvas_t *self = &canvas_data;
    if (radius <= 0)
        return;
    if (ea > 90)
        ea = 90;
    if (sa < 0)
        sa = 0;
    if (sa >= ea)
        return;

    egui_region_t region;
    circle_hq_get_quadrant_region(cx, cy, radius, type, &region);
    egui_region_t ri;
    egui_region_intersect(&region, &self->base_view_work_region, &ri);
    if (egui_region_is_empty(&ri))
        return;

    int32_t r_sq_val = (int32_t)radius * radius;
    int32_t inv128_r_val = circle_hq_precompute_inv128(radius);
    int32_t r_inner_sq = (int32_t)(radius - 1) * (radius - 1);
    int32_t r_outer_sq = (int32_t)(radius + 1) * (radius + 1);
    int is_full = (sa == 0 && ea == 90);

    /* Pre-compute arc angle cotangents for per-row x-range narrowing.
     * The arc boundary lines in quadrant coords: y = tan(sa)*x and y = tan(ea)*x.
     * For each row (fixed qy), solving for qx gives:
     *   qx_max = qy * cot(sa) (start boundary)
     *   qx_min = qy * cot(ea) (end boundary)
     */
    int32_t cot_sa_q16 = 0, cot_ea_q16 = 0;
    int has_start_bound = 0, has_end_bound = 0;
    if (!is_full)
    {
        if (sa > 0 && sa < 90)
        {
            int32_t sin_sa = circle_hq_cos_val_list[90 - sa];
            if (sin_sa > 0)
            {
                cot_sa_q16 = (int32_t)(((int64_t)circle_hq_cos_val_list[sa] << 16) / sin_sa);
                has_start_bound = 1;
            }
        }
        else if (sa == 90)
        {
            cot_sa_q16 = 0;
            has_start_bound = 1;
        }
        if (ea > 0 && ea < 90)
        {
            int32_t sin_ea = circle_hq_cos_val_list[90 - ea];
            if (sin_ea > 0)
            {
                cot_ea_q16 = (int32_t)(((int64_t)circle_hq_cos_val_list[ea] << 16) / sin_ea);
                has_end_bound = 1;
            }
        }
    }

    egui_dim_t y_s = ri.location.y, y_e = ri.location.y + ri.size.height;
    egui_dim_t x_s = ri.location.x, x_e = ri.location.x + ri.size.width;

    /* Direct PFB write setup */
    int use_direct_pfb = (self->mask == NULL) ? 1 : 0;
    egui_dim_t pfb_width = self->pfb_region.size.width;
    egui_dim_t pfb_ofs_x = self->pfb_location_in_base_view.x;
    egui_dim_t pfb_ofs_y = self->pfb_location_in_base_view.y;
    int32_t qx_step = (type == CIRCLE_HQ_TYPE_LEFT_TOP || type == CIRCLE_HQ_TYPE_LEFT_BOTTOM) ? -1 : 1;
    egui_alpha_t eff_alpha = egui_color_alpha_mix(self->alpha, alpha);
    int apply_draw_alpha = (alpha != EGUI_ALPHA_100);
    int apply_canvas_alpha = (eff_alpha != EGUI_ALPHA_100);

    for (egui_dim_t y = y_s; y < y_e; y++)
    {
        int32_t dy = (int32_t)(y - cy);
        int32_t dy_sq = dy * dy;
        if (dy_sq > r_outer_sq)
            continue;

        /* Narrow x-range to circle boundary for this scanline */
        int32_t x_outer_half = circle_hq_isqrt(r_outer_sq - dy_sq);
        egui_dim_t x_lo, x_hi;
        if (type == CIRCLE_HQ_TYPE_LEFT_TOP || type == CIRCLE_HQ_TYPE_LEFT_BOTTOM)
        {
            x_lo = EGUI_MAX((egui_dim_t)(cx - x_outer_half), x_s);
            x_hi = EGUI_MIN(cx, x_e - 1);
        }
        else
        {
            x_lo = EGUI_MAX(cx, x_s);
            x_hi = EGUI_MIN((egui_dim_t)(cx + x_outer_half), x_e - 1);
        }

        /* Further narrow x-range based on arc angle boundaries */
        int32_t qy = 0;
        if (!is_full)
        {
            if (type == CIRCLE_HQ_TYPE_RIGHT_BOTTOM || type == CIRCLE_HQ_TYPE_LEFT_BOTTOM)
                qy = y - cy;
            else
                qy = cy - y;

            if (qy > 0)
            {
                if (has_start_bound)
                {
                    int32_t qx_max = (int32_t)(((int64_t)qy * cot_sa_q16) >> 16) + 2;
                    if (type == CIRCLE_HQ_TYPE_RIGHT_BOTTOM || type == CIRCLE_HQ_TYPE_RIGHT_TOP)
                        x_hi = EGUI_MIN(x_hi, (egui_dim_t)(cx + qx_max));
                    else
                        x_lo = EGUI_MAX(x_lo, (egui_dim_t)(cx - qx_max));
                }
                if (has_end_bound)
                {
                    int32_t qx_min = (int32_t)(((int64_t)qy * cot_ea_q16) >> 16) - 2;
                    if (qx_min < 0)
                        qx_min = 0;
                    if (type == CIRCLE_HQ_TYPE_RIGHT_BOTTOM || type == CIRCLE_HQ_TYPE_RIGHT_TOP)
                        x_lo = EGUI_MAX(x_lo, (egui_dim_t)(cx + qx_min));
                    else
                        x_hi = EGUI_MIN(x_hi, (egui_dim_t)(cx - qx_min));
                }
            }
        }

        if (x_lo > x_hi)
            continue;

        if (use_direct_pfb)
        {
            egui_color_t *dst_base = (egui_color_t *)&self->pfb[(y - pfb_ofs_y) * pfb_width - pfb_ofs_x];

            /* Incremental d²: d_sq[x+1] = d_sq[x] + 2*(x-cx) + 1 */
            int32_t dx_init = (int32_t)(x_lo - cx);
            int32_t d_sq = dx_init * dx_init + dy_sq;
            int32_t d_sq_step = (dx_init << 1) + 1;
            circle_hq_arc_row_ctx_t angle_ctx;

            if (!is_full)
            {
                egui_dim_t qx = (qx_step > 0) ? (egui_dim_t)(x_lo - cx) : (egui_dim_t)(cx - x_lo);
                circle_hq_arc_row_ctx_init(&angle_ctx, qx, qy, sa, ea, qx_step);
            }

            for (egui_dim_t x = x_lo; x <= x_hi; x++)
            {
                egui_alpha_t cc;
                if (d_sq < r_inner_sq)
                    cc = EGUI_ALPHA_100;
                else
                {
                    cc = circle_hq_get_edge_alpha_from_dsq(d_sq, r_sq_val, inv128_r_val);
                    if (cc == 0)
                    {
                        if (!is_full)
                            circle_hq_arc_row_ctx_step(&angle_ctx);
                        d_sq += d_sq_step;
                        d_sq_step += 2;
                        continue;
                    }
                }

                egui_alpha_t ac;
                if (is_full)
                    ac = EGUI_ALPHA_100;
                else
                {
                    ac = circle_hq_arc_angle_alpha_from_ctx(&angle_ctx);
                    if (ac == 0)
                    {
                        circle_hq_arc_row_ctx_step(&angle_ctx);
                        d_sq += d_sq_step;
                        d_sq_step += 2;
                        continue;
                    }
                }

                egui_alpha_t cov = egui_color_alpha_mix(cc, ac);
                egui_alpha_t m = apply_canvas_alpha ? egui_color_alpha_mix(eff_alpha, cov) : cov;
                circle_hq_blend_direct(&dst_base[x], color, m);

                if (!is_full)
                    circle_hq_arc_row_ctx_step(&angle_ctx);
                d_sq += d_sq_step;
                d_sq_step += 2;
            }
        }
        else
        {
            int32_t dx_init = (int32_t)(x_lo - cx);
            int32_t d_sq = dx_init * dx_init + dy_sq;
            int32_t d_sq_step = (dx_init << 1) + 1;
            circle_hq_arc_row_ctx_t angle_ctx;

            if (!is_full)
            {
                egui_dim_t qx = (qx_step > 0) ? (egui_dim_t)(x_lo - cx) : (egui_dim_t)(cx - x_lo);
                circle_hq_arc_row_ctx_init(&angle_ctx, qx, qy, sa, ea, qx_step);
            }

            for (egui_dim_t x = x_lo; x <= x_hi; x++)
            {
                egui_alpha_t cc;
                if (d_sq < r_inner_sq)
                    cc = EGUI_ALPHA_100;
                else
                {
                    cc = circle_hq_get_edge_alpha_from_dsq(d_sq, r_sq_val, inv128_r_val);
                    if (cc == 0)
                    {
                        if (!is_full)
                            circle_hq_arc_row_ctx_step(&angle_ctx);
                        d_sq += d_sq_step;
                        d_sq_step += 2;
                        continue;
                    }
                }

                egui_alpha_t ac;
                if (is_full)
                    ac = EGUI_ALPHA_100;
                else
                {
                    ac = circle_hq_arc_angle_alpha_from_ctx(&angle_ctx);
                    if (ac == 0)
                    {
                        circle_hq_arc_row_ctx_step(&angle_ctx);
                        d_sq += d_sq_step;
                        d_sq_step += 2;
                        continue;
                    }
                }

                egui_alpha_t cov = egui_color_alpha_mix(cc, ac);
                egui_alpha_t m = apply_draw_alpha ? egui_color_alpha_mix(alpha, cov) : cov;
                if (m > 0)
                    egui_canvas_draw_point(x, y, color, m);

                if (!is_full)
                    circle_hq_arc_row_ctx_step(&angle_ctx);
                d_sq += d_sq_step;
                d_sq_step += 2;
            }
        }
    }
}

/* ========================== Arc Fill HQ ========================== */

static int circle_hq_try_get_single_quadrant(int16_t start_angle, int16_t end_angle, int *type, int16_t *sa_local, int16_t *ea_local)
{
    int16_t quadrant;

    if (start_angle < 0 || end_angle <= start_angle || end_angle > 360)
        return 0;

    quadrant = start_angle / 90;
    if (((end_angle - 1) / 90) != quadrant)
        return 0;

    switch (quadrant)
    {
    case 0:
        *type = CIRCLE_HQ_TYPE_RIGHT_BOTTOM;
        *sa_local = start_angle;
        *ea_local = end_angle;
        return 1;
    case 1:
        *type = CIRCLE_HQ_TYPE_LEFT_BOTTOM;
        *sa_local = 180 - end_angle;
        *ea_local = 180 - start_angle;
        return 1;
    case 2:
        *type = CIRCLE_HQ_TYPE_LEFT_TOP;
        *sa_local = start_angle - 180;
        *ea_local = end_angle - 180;
        return 1;
    case 3:
        *type = CIRCLE_HQ_TYPE_RIGHT_TOP;
        *sa_local = 360 - end_angle;
        *ea_local = 360 - start_angle;
        return 1;
    default:
        return 0;
    }
}

void egui_canvas_draw_arc_fill_hq(egui_dim_t cx, egui_dim_t cy, egui_dim_t radius, int16_t start_angle, int16_t end_angle, egui_color_t color,
                                  egui_alpha_t alpha)
{
    int16_t sa_tmp, ea_tmp;
    int type;
    int is_need_middle = 0;

    if (start_angle < 0 || end_angle < 0 || start_angle > end_angle || start_angle == end_angle)
        return;
    if (start_angle > 720 || end_angle > 720)
        return;

#if EGUI_CONFIG_CIRCLE_HQ_INT32_ONLY
    EGUI_ASSERT(radius <= EGUI_CONFIG_CIRCLE_HQ_MAX_RADIUS);
#endif

    if (circle_hq_try_get_single_quadrant(start_angle, end_angle, &type, &sa_tmp, &ea_tmp))
    {
        circle_hq_draw_arc_corner_fill(cx, cy, radius, sa_tmp, ea_tmp, type, color, alpha);

        if ((start_angle <= 0 && end_angle >= 0) || (start_angle <= 90 && end_angle >= 90) || (start_angle <= 180 && end_angle >= 180) ||
            (start_angle <= 270 && end_angle >= 270))
        {
            is_need_middle = 1;
        }

        if (is_need_middle)
            egui_canvas_draw_point(cx, cy, color, alpha);
        return;
    }

    do
    {
        if (start_angle < 90)
        {
            sa_tmp = start_angle;
            ea_tmp = end_angle;
            circle_hq_draw_arc_corner_fill(cx, cy, radius, sa_tmp, ea_tmp, CIRCLE_HQ_TYPE_RIGHT_BOTTOM, color, alpha);
        }
        if (start_angle < 180)
        {
            sa_tmp = start_angle - 90;
            ea_tmp = end_angle - 90;
            circle_hq_draw_arc_corner_fill(cx, cy, radius, 90 - ea_tmp, 90 - sa_tmp, CIRCLE_HQ_TYPE_LEFT_BOTTOM, color, alpha);
        }
        if (start_angle < 270)
        {
            sa_tmp = start_angle - 180;
            ea_tmp = end_angle - 180;
            circle_hq_draw_arc_corner_fill(cx, cy, radius, sa_tmp, ea_tmp, CIRCLE_HQ_TYPE_LEFT_TOP, color, alpha);
        }
        if (start_angle < 360)
        {
            sa_tmp = start_angle - 270;
            ea_tmp = end_angle - 270;
            circle_hq_draw_arc_corner_fill(cx, cy, radius, 90 - ea_tmp, 90 - sa_tmp, CIRCLE_HQ_TYPE_RIGHT_TOP, color, alpha);
        }

        if ((start_angle <= 0 && end_angle >= 0) || (start_angle <= 90 && end_angle >= 90) || (start_angle <= 180 && end_angle >= 180) ||
            (start_angle <= 270 && end_angle >= 270))
        {
            is_need_middle = 1;
        }

        start_angle -= 360;
        if (start_angle < 0)
            start_angle = 0;
        end_angle -= 360;
    } while (end_angle > 0);

    if (is_need_middle)
        egui_canvas_draw_point(cx, cy, color, alpha);
}

/* ========================== Arc Corner Stroke HQ ========================== */

static void circle_hq_draw_arc_corner(egui_dim_t cx, egui_dim_t cy, egui_dim_t radius, int16_t sa, int16_t ea, egui_dim_t stroke_width, int type,
                                      egui_color_t color, egui_alpha_t alpha)
{
    egui_canvas_t *self = &canvas_data;
    if (radius <= 0)
        return;
    if (ea > 90)
        ea = 90;
    if (sa < 0)
        sa = 0;
    if (sa >= ea)
        return;
    if (radius <= stroke_width)
    {
        circle_hq_draw_arc_corner_fill(cx, cy, radius, sa, ea, type, color, alpha);
        return;
    }

    egui_region_t region;
    circle_hq_get_quadrant_region(cx, cy, radius, type, &region);
    egui_region_t ri;
    egui_region_intersect(&region, &self->base_view_work_region, &ri);
    if (egui_region_is_empty(&ri))
        return;

    int32_t outer_inner_sq = (int32_t)(radius - 1) * (radius - 1);
    int32_t outer_outer_sq = (int32_t)(radius + 1) * (radius + 1);
    egui_dim_t inner_r = radius - stroke_width;
    int32_t inner_inner_sq = (inner_r > 1) ? (int32_t)(inner_r - 1) * (inner_r - 1) : 0;
    int32_t inner_outer_sq = (int32_t)(inner_r + 1) * (inner_r + 1);
    int32_t outer_r_sq_v = (int32_t)radius * radius;
    int32_t outer_inv128_v = circle_hq_precompute_inv128(radius);
    int32_t inner_r_sq_v = (int32_t)inner_r * inner_r;
    int32_t inner_inv128_v = (inner_r > 0) ? circle_hq_precompute_inv128(inner_r) : 0;
    int is_full = (sa == 0 && ea == 90);

    /* Pre-compute arc angle cotangents for per-row x-range narrowing */
    int32_t stroke_cot_sa_q16 = 0, stroke_cot_ea_q16 = 0;
    int stroke_has_start_bound = 0, stroke_has_end_bound = 0;
    if (!is_full)
    {
        if (sa > 0 && sa < 90)
        {
            int32_t sin_sa = circle_hq_cos_val_list[90 - sa];
            if (sin_sa > 0)
            {
                stroke_cot_sa_q16 = (int32_t)(((int64_t)circle_hq_cos_val_list[sa] << 16) / sin_sa);
                stroke_has_start_bound = 1;
            }
        }
        else if (sa == 90)
        {
            stroke_cot_sa_q16 = 0;
            stroke_has_start_bound = 1;
        }
        if (ea > 0 && ea < 90)
        {
            int32_t sin_ea = circle_hq_cos_val_list[90 - ea];
            if (sin_ea > 0)
            {
                stroke_cot_ea_q16 = (int32_t)(((int64_t)circle_hq_cos_val_list[ea] << 16) / sin_ea);
                stroke_has_end_bound = 1;
            }
        }
    }

    egui_dim_t y_s = ri.location.y, y_e = ri.location.y + ri.size.height;
    egui_dim_t x_s = ri.location.x, x_e = ri.location.x + ri.size.width;

    /* Direct PFB write setup */
    int use_direct_pfb = (self->mask == NULL) ? 1 : 0;
    egui_dim_t pfb_width = self->pfb_region.size.width;
    egui_dim_t pfb_ofs_x = self->pfb_location_in_base_view.x;
    egui_dim_t pfb_ofs_y = self->pfb_location_in_base_view.y;
    egui_alpha_t eff_alpha = egui_color_alpha_mix(self->alpha, alpha);
    int apply_draw_alpha = (alpha != EGUI_ALPHA_100);
    int apply_canvas_alpha = (eff_alpha != EGUI_ALPHA_100);

    for (egui_dim_t y = y_s; y < y_e; y++)
    {
        int32_t dy_val = (int32_t)(y - cy);
        int32_t dy_sq = dy_val * dy_val;
        if (dy_sq > outer_outer_sq)
            continue;

        /* Narrow x-range to outer circle boundary */
        int32_t x_outer_half = circle_hq_isqrt(outer_outer_sq - dy_sq);
        egui_dim_t x_lo, x_hi;
        if (type == CIRCLE_HQ_TYPE_LEFT_TOP || type == CIRCLE_HQ_TYPE_LEFT_BOTTOM)
        {
            x_lo = EGUI_MAX((egui_dim_t)(cx - x_outer_half), x_s);
            x_hi = EGUI_MIN(cx, x_e - 1);
        }
        else
        {
            x_lo = EGUI_MAX(cx, x_s);
            x_hi = EGUI_MIN((egui_dim_t)(cx + x_outer_half), x_e - 1);
        }

        /* Further narrow x-range based on arc angle boundaries */
        int32_t qy = 0;
        if (!is_full)
        {
            if (type == CIRCLE_HQ_TYPE_RIGHT_BOTTOM || type == CIRCLE_HQ_TYPE_LEFT_BOTTOM)
                qy = y - cy;
            else
                qy = cy - y;

            if (qy > 0)
            {
                if (stroke_has_start_bound)
                {
                    int32_t qx_max = (int32_t)(((int64_t)qy * stroke_cot_sa_q16) >> 16) + 2;
                    if (type == CIRCLE_HQ_TYPE_RIGHT_BOTTOM || type == CIRCLE_HQ_TYPE_RIGHT_TOP)
                        x_hi = EGUI_MIN(x_hi, (egui_dim_t)(cx + qx_max));
                    else
                        x_lo = EGUI_MAX(x_lo, (egui_dim_t)(cx - qx_max));
                }
                if (stroke_has_end_bound)
                {
                    int32_t qx_min = (int32_t)(((int64_t)qy * stroke_cot_ea_q16) >> 16) - 2;
                    if (qx_min < 0)
                        qx_min = 0;
                    if (type == CIRCLE_HQ_TYPE_RIGHT_BOTTOM || type == CIRCLE_HQ_TYPE_RIGHT_TOP)
                        x_lo = EGUI_MAX(x_lo, (egui_dim_t)(cx + qx_min));
                    else
                        x_hi = EGUI_MIN(x_hi, (egui_dim_t)(cx - qx_min));
                }
            }
        }

        if (x_lo > x_hi)
            continue;

        if (use_direct_pfb)
        {
            egui_color_t *dst_base = (egui_color_t *)&self->pfb[(y - pfb_ofs_y) * pfb_width - pfb_ofs_x];

            for (egui_dim_t x = x_lo; x <= x_hi; x++)
            {
                int32_t dx = (int32_t)(x - cx);
                int32_t d_sq = dx * dx + dy_sq;

                egui_alpha_t oc;
                if (d_sq < outer_inner_sq)
                    oc = EGUI_ALPHA_100;
                else
                {
                    oc = circle_hq_get_edge_alpha(dx, dy_val, outer_r_sq_v, outer_inv128_v);
                    if (oc == 0)
                        continue;
                }

                egui_alpha_t ic = 0;
                if (d_sq < inner_inner_sq)
                    ic = EGUI_ALPHA_100;
                else if (d_sq <= inner_outer_sq)
                    ic = circle_hq_get_edge_alpha(dx, dy_val, inner_r_sq_v, inner_inv128_v);
                if (oc <= ic)
                    continue;

                egui_alpha_t cc = oc - ic;
                egui_alpha_t ac;
                if (is_full)
                    ac = EGUI_ALPHA_100;
                else
                {
                    egui_dim_t px, py;
                    circle_hq_to_quadrant_coords(x, y, cx, cy, type, &px, &py);
                    ac = circle_hq_arc_angle_alpha(px, py, sa, ea);
                    if (ac == 0)
                        continue;
                }

                egui_alpha_t cov = egui_color_alpha_mix(cc, ac);
                egui_alpha_t m = apply_canvas_alpha ? egui_color_alpha_mix(eff_alpha, cov) : cov;
                if (m > 0)
                {
                    egui_color_t *back_color = &dst_base[x];
                    if (m == EGUI_ALPHA_100)
                        *back_color = color;
                    else
                        egui_rgb_mix_ptr(back_color, &color, back_color, m);
                }
            }
        }
        else
        {
            for (egui_dim_t x = x_lo; x <= x_hi; x++)
            {
                int32_t dx = (int32_t)(x - cx);
                int32_t d_sq = dx * dx + dy_sq;

                egui_alpha_t oc;
                if (d_sq < outer_inner_sq)
                    oc = EGUI_ALPHA_100;
                else
                {
                    oc = circle_hq_get_edge_alpha(dx, dy_val, outer_r_sq_v, outer_inv128_v);
                    if (oc == 0)
                        continue;
                }

                egui_alpha_t ic = 0;
                if (d_sq < inner_inner_sq)
                    ic = EGUI_ALPHA_100;
                else if (d_sq <= inner_outer_sq)
                    ic = circle_hq_get_edge_alpha(dx, dy_val, inner_r_sq_v, inner_inv128_v);
                if (oc <= ic)
                    continue;

                egui_alpha_t cc = oc - ic;
                egui_alpha_t ac;
                if (is_full)
                    ac = EGUI_ALPHA_100;
                else
                {
                    egui_dim_t px, py;
                    circle_hq_to_quadrant_coords(x, y, cx, cy, type, &px, &py);
                    ac = circle_hq_arc_angle_alpha(px, py, sa, ea);
                    if (ac == 0)
                        continue;
                }

                egui_alpha_t cov = egui_color_alpha_mix(cc, ac);
                egui_alpha_t m = apply_draw_alpha ? egui_color_alpha_mix(alpha, cov) : cov;
                if (m > 0)
                    egui_canvas_draw_point(x, y, color, m);
            }
        }
    }
}

/* ========================== Arc Stroke HQ ========================== */

void egui_canvas_draw_arc_hq(egui_dim_t cx, egui_dim_t cy, egui_dim_t radius, int16_t start_angle, int16_t end_angle, egui_dim_t stroke_width,
                             egui_color_t color, egui_alpha_t alpha)
{
    int16_t sa_tmp, ea_tmp;
    int type;

    if (radius <= stroke_width)
    {
        egui_canvas_draw_arc_fill_hq(cx, cy, radius, start_angle, end_angle, color, alpha);
        return;
    }
    if (start_angle < 0 || end_angle < 0 || start_angle > end_angle || start_angle == end_angle)
        return;
    if (start_angle > 720 || end_angle > 720)
        return;

#if EGUI_CONFIG_CIRCLE_HQ_INT32_ONLY
    EGUI_ASSERT(radius <= EGUI_CONFIG_CIRCLE_HQ_MAX_RADIUS);
#endif

    if (circle_hq_try_get_single_quadrant(start_angle, end_angle, &type, &sa_tmp, &ea_tmp))
    {
        circle_hq_draw_arc_corner(cx, cy, radius, sa_tmp, ea_tmp, stroke_width, type, color, alpha);
        return;
    }

    do
    {
        if (start_angle < 90)
        {
            sa_tmp = start_angle;
            ea_tmp = end_angle;
            circle_hq_draw_arc_corner(cx, cy, radius, sa_tmp, ea_tmp, stroke_width, CIRCLE_HQ_TYPE_RIGHT_BOTTOM, color, alpha);
        }
        if (start_angle < 180)
        {
            sa_tmp = start_angle - 90;
            ea_tmp = end_angle - 90;
            circle_hq_draw_arc_corner(cx, cy, radius, 90 - ea_tmp, 90 - sa_tmp, stroke_width, CIRCLE_HQ_TYPE_LEFT_BOTTOM, color, alpha);
        }
        if (start_angle < 270)
        {
            sa_tmp = start_angle - 180;
            ea_tmp = end_angle - 180;
            circle_hq_draw_arc_corner(cx, cy, radius, sa_tmp, ea_tmp, stroke_width, CIRCLE_HQ_TYPE_LEFT_TOP, color, alpha);
        }
        if (start_angle < 360)
        {
            sa_tmp = start_angle - 270;
            ea_tmp = end_angle - 270;
            circle_hq_draw_arc_corner(cx, cy, radius, 90 - ea_tmp, 90 - sa_tmp, stroke_width, CIRCLE_HQ_TYPE_RIGHT_TOP, color, alpha);
        }

        start_angle -= 360;
        if (start_angle < 0)
            start_angle = 0;
        end_angle -= 360;
    } while (end_angle > 0);
}
