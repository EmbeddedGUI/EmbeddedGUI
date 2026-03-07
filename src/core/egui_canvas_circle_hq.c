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
 *
 * Guarded by EGUI_CONFIG_FUNCTION_CANVAS_DRAW_CIRCLE_HQ.
 */

#if EGUI_CONFIG_FUNCTION_CANVAS_DRAW_CIRCLE_HQ

__EGUI_STATIC_INLINE__ egui_alpha_t circle_hq_get_edge_alpha(int32_t dx, int32_t dy, int32_t radius)
{
    /* SDF-based anti-aliasing for circles.
     * Signed distance from boundary at point (dx,dy):
     *   d = (dx^2 + dy^2 - r^2) / (2*r)
     * Map d in [-0.5, +0.5] to alpha in [255, 0] linearly.
     */
    int32_t d_sq = dx * dx + dy * dy;
    int32_t r_sq = radius * radius;
    int32_t diff = d_sq - r_sq;
    /* Near boundary |diff| <= 2*radius+1, so diff*128 fits int32_t for radius < 8M */
#if EGUI_CONFIG_CIRCLE_HQ_INT32_ONLY
    int32_t d_shifted = diff * 128 / radius;
#else
    int32_t d_shifted = (int32_t)((int64_t)diff * 128 / radius);
#endif
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

    int32_t r_inner_sq = (int32_t)(radius - 1) * (radius - 1);
    int32_t r_outer_sq = (int32_t)(radius + 1) * (radius + 1);
    egui_dim_t y_start = ri.location.y, y_end = ri.location.y + ri.size.height;
    egui_dim_t x_start = ri.location.x, x_end = ri.location.x + ri.size.width;

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

        if (x_inner_half > 0)
        {
            egui_dim_t il = center_x - x_inner_half + 1, ir = center_x + x_inner_half - 1;
            for (egui_dim_t x = ol; x < il && x <= or_; x++)
            {
                egui_alpha_t cov = circle_hq_get_edge_alpha((int32_t)(x - center_x), dy, radius);
                if (cov > 0)
                {
                    egui_alpha_t m = egui_color_alpha_mix(alpha, cov);
                    if (m > 0)
                        egui_canvas_draw_point(x, y, color, m);
                }
            }
            egui_dim_t bl = il < x_start ? x_start : il, br = ir >= x_end ? x_end - 1 : ir;
            if (bl <= br)
                egui_canvas_draw_fillrect(bl, y, br - bl + 1, 1, color, alpha);
            egui_dim_t rs = ir + 1 < x_start ? x_start : ir + 1;
            for (egui_dim_t x = rs; x <= or_; x++)
            {
                egui_alpha_t cov = circle_hq_get_edge_alpha((int32_t)(x - center_x), dy, radius);
                if (cov > 0)
                {
                    egui_alpha_t m = egui_color_alpha_mix(alpha, cov);
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
                if (d_sq > r_outer_sq)
                    continue;
                if (d_sq < r_inner_sq)
                {
                    egui_canvas_draw_point(x, y, color, alpha);
                    continue;
                }
                egui_alpha_t cov = circle_hq_get_edge_alpha(dx, dy, radius);
                if (cov > 0)
                {
                    egui_alpha_t m = egui_color_alpha_mix(alpha, cov);
                    if (m > 0)
                        egui_canvas_draw_point(x, y, color, m);
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

    egui_dim_t y_start = ri.location.y, y_end = ri.location.y + ri.size.height;
    egui_dim_t x_start = ri.location.x, x_end = ri.location.x + ri.size.width;

    for (egui_dim_t y = y_start; y < y_end; y++)
    {
        int32_t dy = (int32_t)(y - center_y), dy_sq = dy * dy;
        for (egui_dim_t x = x_start; x < x_end; x++)
        {
            int32_t dx = (int32_t)(x - center_x), d_sq = dx * dx + dy_sq;
            if (d_sq > outer_outer_sq)
                continue;

            egui_alpha_t oc;
            if (d_sq < outer_inner_sq)
                oc = EGUI_ALPHA_100;
            else
            {
                oc = circle_hq_get_edge_alpha(dx, dy, radius);
                if (oc == 0)
                    continue;
            }

            egui_alpha_t ic = 0;
            if (d_sq < inner_inner_sq)
                ic = EGUI_ALPHA_100;
            else if (d_sq <= inner_outer_sq)
                ic = circle_hq_get_edge_alpha(dx, dy, ir);

            if (oc > ic)
            {
                egui_alpha_t m = egui_color_alpha_mix(alpha, oc - ic);
                if (m > 0)
                    egui_canvas_draw_point(x, y, color, m);
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

    int32_t r_inner_sq = (int32_t)(radius - 1) * (radius - 1);
    int32_t r_outer_sq = (int32_t)(radius + 1) * (radius + 1);
    int is_full = (sa == 0 && ea == 90);

    egui_dim_t y_s = ri.location.y, y_e = ri.location.y + ri.size.height;
    egui_dim_t x_s = ri.location.x, x_e = ri.location.x + ri.size.width;

    for (egui_dim_t y = y_s; y < y_e; y++)
    {
        for (egui_dim_t x = x_s; x < x_e; x++)
        {
            int32_t dx = (int32_t)(x - cx), dy = (int32_t)(y - cy);
            int32_t d_sq = dx * dx + dy * dy;
            if (d_sq > r_outer_sq)
                continue;

            egui_alpha_t cc;
            if (d_sq < r_inner_sq)
                cc = EGUI_ALPHA_100;
            else
            {
                cc = circle_hq_get_edge_alpha(dx, dy, radius);
                if (cc == 0)
                    continue;
            }

            egui_alpha_t ac;
            if (is_full)
                ac = EGUI_ALPHA_100;
            else
            {
                egui_dim_t qx, qy;
                circle_hq_to_quadrant_coords(x, y, cx, cy, type, &qx, &qy);
                ac = circle_hq_arc_angle_alpha(qx, qy, sa, ea);
                if (ac == 0)
                    continue;
            }

            egui_alpha_t m = egui_color_alpha_mix(alpha, egui_color_alpha_mix(cc, ac));
            if (m > 0)
                egui_canvas_draw_point(x, y, color, m);
        }
    }
}

/* ========================== Arc Fill HQ ========================== */

void egui_canvas_draw_arc_fill_hq(egui_dim_t cx, egui_dim_t cy, egui_dim_t radius, int16_t start_angle, int16_t end_angle, egui_color_t color,
                                  egui_alpha_t alpha)
{
    int16_t sa_tmp, ea_tmp;
    int is_need_middle = 0;

    if (start_angle < 0 || end_angle < 0 || start_angle > end_angle || start_angle == end_angle)
        return;
    if (start_angle > 720 || end_angle > 720)
        return;

#if EGUI_CONFIG_CIRCLE_HQ_INT32_ONLY
    EGUI_ASSERT(radius <= EGUI_CONFIG_CIRCLE_HQ_MAX_RADIUS);
#endif

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
    int is_full = (sa == 0 && ea == 90);

    egui_dim_t y_s = ri.location.y, y_e = ri.location.y + ri.size.height;
    egui_dim_t x_s = ri.location.x, x_e = ri.location.x + ri.size.width;

    for (egui_dim_t y = y_s; y < y_e; y++)
    {
        for (egui_dim_t x = x_s; x < x_e; x++)
        {
            int32_t dx = (int32_t)(x - cx), dy = (int32_t)(y - cy);
            int32_t d_sq = dx * dx + dy * dy;
            if (d_sq > outer_outer_sq)
                continue;

            egui_alpha_t oc;
            if (d_sq < outer_inner_sq)
                oc = EGUI_ALPHA_100;
            else
            {
                oc = circle_hq_get_edge_alpha(dx, dy, radius);
                if (oc == 0)
                    continue;
            }

            egui_alpha_t ic = 0;
            if (d_sq < inner_inner_sq)
                ic = EGUI_ALPHA_100;
            else if (d_sq <= inner_outer_sq)
                ic = circle_hq_get_edge_alpha(dx, dy, inner_r);
            if (oc <= ic)
                continue;

            egui_alpha_t cc = oc - ic;
            egui_alpha_t ac;
            if (is_full)
                ac = EGUI_ALPHA_100;
            else
            {
                egui_dim_t qx, qy;
                circle_hq_to_quadrant_coords(x, y, cx, cy, type, &qx, &qy);
                ac = circle_hq_arc_angle_alpha(qx, qy, sa, ea);
                if (ac == 0)
                    continue;
            }

            egui_alpha_t m = egui_color_alpha_mix(alpha, egui_color_alpha_mix(cc, ac));
            if (m > 0)
                egui_canvas_draw_point(x, y, color, m);
        }
    }
}

/* ========================== Arc Stroke HQ ========================== */

void egui_canvas_draw_arc_hq(egui_dim_t cx, egui_dim_t cy, egui_dim_t radius, int16_t start_angle, int16_t end_angle, egui_dim_t stroke_width,
                             egui_color_t color, egui_alpha_t alpha)
{
    int16_t sa_tmp, ea_tmp;

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

#endif /* EGUI_CONFIG_FUNCTION_CANVAS_DRAW_CIRCLE_HQ */
