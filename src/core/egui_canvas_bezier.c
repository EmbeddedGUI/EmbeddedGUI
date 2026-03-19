#include "egui_canvas.h"

/**
 * @brief Bezier curve drawing using De Casteljau adaptive subdivision.
 *
 * Recursively subdivides the curve until segments are flat enough,
 * then draws each segment as a line. Anti-aliasing is provided by
 * the underlying egui_canvas_draw_line().
 */

#ifndef EGUI_CANVAS_BEZIER_MAX_DEPTH
#define EGUI_CANVAS_BEZIER_MAX_DEPTH 8
#endif

#ifndef EGUI_CANVAS_BEZIER_FLATNESS_THRESHOLD
// Flatness threshold in pixels (squared distance from control point to line)
// A value of 1 gives good quality; higher values = fewer segments
#define EGUI_CANVAS_BEZIER_FLATNESS_THRESHOLD 1
#endif

/**
 * @brief Recursive quadratic bezier subdivision.
 * P0 = (x0, y0), Control = (cx, cy), P1 = (x1, y1)
 * Uses De Casteljau algorithm at t=0.5 to split.
 */
static void bezier_quad_recursive(egui_dim_t x0, egui_dim_t y0, egui_dim_t cx, egui_dim_t cy, egui_dim_t x1, egui_dim_t y1, egui_dim_t stroke_width,
                                  egui_color_t color, egui_alpha_t alpha, int depth)
{
    // Flatness test: distance from control point to midpoint of P0-P1
    egui_dim_t mx = (x0 + x1) >> 1;
    egui_dim_t my = (y0 + y1) >> 1;
    int32_t dx = cx - mx;
    int32_t dy = cy - my;
    int32_t dist_sq = dx * dx + dy * dy;

    // Do NOT scale flatness by stroke_width. The threshold is purely geometric:
    // it determines how closely the piecewise-linear approximation matches the
    // true curve. Thick strokes make direction changes MORE visible at joints,
    // so they need MORE subdivision, not less.
    int32_t flatness_thresh = EGUI_CANVAS_BEZIER_FLATNESS_THRESHOLD;

    if (dist_sq <= flatness_thresh || depth >= EGUI_CANVAS_BEZIER_MAX_DEPTH)
    {
        // Flat enough, draw as a line.
        // For thick curves, prefer HQ line to avoid tiny gaps
        // between adjacent flattened segments.
        if (stroke_width > 1)
        {
            egui_canvas_draw_line_hq(x0, y0, x1, y1, stroke_width, color, alpha);
        }
        else
        {
            egui_canvas_draw_line(x0, y0, x1, y1, stroke_width, color, alpha);
        }
        return;
    }

    // De Casteljau subdivision at t=0.5
    // Mid01 = (P0 + C) / 2
    egui_dim_t mid01_x = (x0 + cx) >> 1;
    egui_dim_t mid01_y = (y0 + cy) >> 1;
    // Mid12 = (C + P1) / 2
    egui_dim_t mid12_x = (cx + x1) >> 1;
    egui_dim_t mid12_y = (cy + y1) >> 1;
    // Mid = (Mid01 + Mid12) / 2 = point on curve at t=0.5
    egui_dim_t mid_x = (mid01_x + mid12_x) >> 1;
    egui_dim_t mid_y = (mid01_y + mid12_y) >> 1;

    // Recurse on two halves
    bezier_quad_recursive(x0, y0, mid01_x, mid01_y, mid_x, mid_y, stroke_width, color, alpha, depth + 1);
    bezier_quad_recursive(mid_x, mid_y, mid12_x, mid12_y, x1, y1, stroke_width, color, alpha, depth + 1);
}

/**
 * \brief           Draw quadratic bezier curve (1 control point)
 * \param[in]       x0, y0: Start point
 * \param[in]       cx, cy: Control point
 * \param[in]       x1, y1: End point
 * \param[in]       stroke_width: Width of the curve
 * \param[in]       color: Color used for drawing operation
 * \param[in]       alpha: Alpha value for blending
 */
void egui_canvas_draw_bezier_quad(egui_dim_t x0, egui_dim_t y0, egui_dim_t cx, egui_dim_t cy, egui_dim_t x1, egui_dim_t y1, egui_dim_t stroke_width,
                                  egui_color_t color, egui_alpha_t alpha)
{
    egui_canvas_t *self = &canvas_data;

    // Quick bounding box check
    egui_dim_t min_x = EGUI_MIN(EGUI_MIN(x0, cx), x1) - stroke_width;
    egui_dim_t min_y = EGUI_MIN(EGUI_MIN(y0, cy), y1) - stroke_width;
    egui_dim_t max_x = EGUI_MAX(EGUI_MAX(x0, cx), x1) + stroke_width;
    egui_dim_t max_y = EGUI_MAX(EGUI_MAX(y0, cy), y1) + stroke_width;

    EGUI_REGION_DEFINE(region, min_x, min_y, max_x - min_x + 1, max_y - min_y + 1);
    if (!egui_region_is_intersect(&region, &self->base_view_work_region))
    {
        return;
    }

    bezier_quad_recursive(x0, y0, cx, cy, x1, y1, stroke_width, color, alpha, 0);
}

/**
 * @brief Recursive cubic bezier subdivision.
 * P0 = (x0, y0), C0 = (cx0, cy0), C1 = (cx1, cy1), P1 = (x1, y1)
 */
static void bezier_cubic_recursive(egui_dim_t x0, egui_dim_t y0, egui_dim_t cx0, egui_dim_t cy0, egui_dim_t cx1, egui_dim_t cy1, egui_dim_t x1, egui_dim_t y1,
                                   egui_dim_t stroke_width, egui_color_t color, egui_alpha_t alpha, int depth)
{
    // Flatness test: max distance from control points to line P0-P1
    int32_t dx = x1 - x0;
    int32_t dy = y1 - y0;

    // Distance from C0 to line P0-P1
    int32_t d0_cross = EGUI_ABS((int32_t)(cx0 - x0) * dy - (int32_t)(cy0 - y0) * dx);
    // Distance from C1 to line P0-P1
    int32_t d1_cross = EGUI_ABS((int32_t)(cx1 - x0) * dy - (int32_t)(cy1 - y0) * dx);

    int32_t line_len_sq = dx * dx + dy * dy;
    // Using squared comparison to avoid sqrt
    // d_cross^2 / line_len_sq < threshold^2
    int64_t max_cross = EGUI_MAX(d0_cross, d1_cross);
    int64_t max_cross_sq = max_cross * max_cross;
    int64_t threshold_sq = (int64_t)EGUI_CANVAS_BEZIER_FLATNESS_THRESHOLD * EGUI_CANVAS_BEZIER_FLATNESS_THRESHOLD * line_len_sq;

    if (max_cross_sq <= threshold_sq || depth >= EGUI_CANVAS_BEZIER_MAX_DEPTH)
    {
        if (stroke_width > 1)
        {
            egui_canvas_draw_line_hq(x0, y0, x1, y1, stroke_width, color, alpha);
        }
        else
        {
            egui_canvas_draw_line(x0, y0, x1, y1, stroke_width, color, alpha);
        }
        return;
    }

    // De Casteljau subdivision at t=0.5
    egui_dim_t m01_x = (x0 + cx0) >> 1;
    egui_dim_t m01_y = (y0 + cy0) >> 1;
    egui_dim_t m12_x = (cx0 + cx1) >> 1;
    egui_dim_t m12_y = (cy0 + cy1) >> 1;
    egui_dim_t m23_x = (cx1 + x1) >> 1;
    egui_dim_t m23_y = (cy1 + y1) >> 1;

    egui_dim_t m012_x = (m01_x + m12_x) >> 1;
    egui_dim_t m012_y = (m01_y + m12_y) >> 1;
    egui_dim_t m123_x = (m12_x + m23_x) >> 1;
    egui_dim_t m123_y = (m12_y + m23_y) >> 1;

    egui_dim_t mid_x = (m012_x + m123_x) >> 1;
    egui_dim_t mid_y = (m012_y + m123_y) >> 1;

    bezier_cubic_recursive(x0, y0, m01_x, m01_y, m012_x, m012_y, mid_x, mid_y, stroke_width, color, alpha, depth + 1);
    bezier_cubic_recursive(mid_x, mid_y, m123_x, m123_y, m23_x, m23_y, x1, y1, stroke_width, color, alpha, depth + 1);
}

/**
 * \brief           Draw cubic bezier curve (2 control points)
 * \param[in]       x0, y0: Start point
 * \param[in]       cx0, cy0: First control point
 * \param[in]       cx1, cy1: Second control point
 * \param[in]       x1, y1: End point
 * \param[in]       stroke_width: Width of the curve
 * \param[in]       color: Color used for drawing operation
 * \param[in]       alpha: Alpha value for blending
 */
void egui_canvas_draw_bezier_cubic(egui_dim_t x0, egui_dim_t y0, egui_dim_t cx0, egui_dim_t cy0, egui_dim_t cx1, egui_dim_t cy1, egui_dim_t x1, egui_dim_t y1,
                                   egui_dim_t stroke_width, egui_color_t color, egui_alpha_t alpha)
{
    egui_canvas_t *self = &canvas_data;

    // Quick bounding box check
    egui_dim_t min_x = EGUI_MIN(EGUI_MIN(x0, x1), EGUI_MIN(cx0, cx1)) - stroke_width;
    egui_dim_t min_y = EGUI_MIN(EGUI_MIN(y0, y1), EGUI_MIN(cy0, cy1)) - stroke_width;
    egui_dim_t max_x = EGUI_MAX(EGUI_MAX(x0, x1), EGUI_MAX(cx0, cx1)) + stroke_width;
    egui_dim_t max_y = EGUI_MAX(EGUI_MAX(y0, y1), EGUI_MAX(cy0, cy1)) + stroke_width;

    EGUI_REGION_DEFINE(region, min_x, min_y, max_x - min_x + 1, max_y - min_y + 1);
    if (!egui_region_is_intersect(&region, &self->base_view_work_region))
    {
        return;
    }

    bezier_cubic_recursive(x0, y0, cx0, cy0, cx1, cy1, x1, y1, stroke_width, color, alpha, 0);
}
