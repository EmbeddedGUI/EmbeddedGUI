#include "egui_canvas.h"

/**
 * @brief Improved line drawing with Wu's anti-aliasing and correct thick line rendering.
 *
 * For thin lines (stroke_width == 1): Wu's anti-aliased line algorithm.
 * For thick lines (stroke_width > 1): Parallel line scanning perpendicular to the line direction.
 * Horizontal and vertical lines are optimized to use fillrect directly.
 */

/**
 * \brief           Draw line from point 1 to point 2 with anti-aliasing
 * \param[in]       x1: Line start X position
 * \param[in]       y1: Line start Y position
 * \param[in]       x2: Line end X position
 * \param[in]       y2: Line end Y position
 * \param[in]       stroke_width: Width of the line
 * \param[in]       color: Color used for drawing operation
 * \param[in]       alpha: Alpha value for blending
 */
void egui_canvas_draw_line(egui_dim_t x1, egui_dim_t y1, egui_dim_t x2, egui_dim_t y2, egui_dim_t stroke_width, egui_color_t color, egui_alpha_t alpha)
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

    // Calculate bounding box for early PFB rejection
    egui_dim_t bbox_x = EGUI_MIN(x1, x2) - stroke_width;
    egui_dim_t bbox_y = EGUI_MIN(y1, y2) - stroke_width;
    egui_dim_t bbox_w = deltax + (stroke_width << 1) + 1;
    egui_dim_t bbox_h = deltay + (stroke_width << 1) + 1;
    EGUI_REGION_DEFINE(region, bbox_x, bbox_y, bbox_w, bbox_h);
    if (!egui_region_is_intersect(&region, &self->base_view_work_region))
    {
        return;
    }

    // Ensure we draw from left to right (x1 <= x2)
    if (x1 > x2)
    {
        EGUI_SWAP(x1, x2);
        EGUI_SWAP(y1, y2);
    }

    egui_dim_t dx = x2 - x1;
    egui_dim_t dy = y2 - y1;
    egui_dim_t steep = EGUI_ABS(dy) > dx;

    if (stroke_width <= 1)
    {
        // Wu's anti-aliased line for thin lines
        if (steep)
        {
            // Swap x and y for steep lines
            EGUI_SWAP(x1, y1);
            EGUI_SWAP(x2, y2);
            if (x1 > x2)
            {
                EGUI_SWAP(x1, x2);
                EGUI_SWAP(y1, y2);
            }
            dx = x2 - x1;
            dy = y2 - y1;
        }

        egui_dim_t yinc = (dy >= 0) ? 1 : -1;
        if (dy < 0)
        {
            dy = -dy;
        }

        egui_float_t gradient = EGUI_FLOAT_DIV_LIMIT(dy, dx);
        egui_float_t tmp_e = 0;
        egui_dim_t y = y1;

        egui_dim_t x_start = self->base_view_work_region.location.x;
        egui_dim_t x_end = self->base_view_work_region.location.x + self->base_view_work_region.size.width;
        egui_dim_t y_start = self->base_view_work_region.location.y;
        egui_dim_t y_end = self->base_view_work_region.location.y + self->base_view_work_region.size.height;

        for (egui_dim_t x = x1; x <= x2; x++)
        {
            egui_alpha_t alpha_0, alpha_1;
#if !EGUI_CONFIG_PERFORMANCE_USE_FLOAT
            alpha_0 = tmp_e >> (EGUI_FLOAT_FRAC - 8);
#else
            alpha_0 = EGUI_FLOAT_MULT(tmp_e, EGUI_ALPHA_100);
#endif
            alpha_1 = EGUI_ALPHA_100 - alpha_0;

            if (alpha != EGUI_ALPHA_100)
            {
                alpha_0 = egui_color_alpha_mix(alpha, alpha_0);
                alpha_1 = egui_color_alpha_mix(alpha, alpha_1);
            }

            if (steep)
            {
                // For steep lines, x and y are swapped
                if (x >= y_start && x < y_end)
                {
                    if (y >= x_start && y < x_end)
                    {
                        egui_canvas_draw_point(y, x, color, alpha_1);
                    }
                    if ((y + yinc) >= x_start && (y + yinc) < x_end)
                    {
                        egui_canvas_draw_point(y + yinc, x, color, alpha_0);
                    }
                }
            }
            else
            {
                if (x >= x_start && x < x_end)
                {
                    if (y >= y_start && y < y_end)
                    {
                        egui_canvas_draw_point(x, y, color, alpha_1);
                    }
                    if ((y + yinc) >= y_start && (y + yinc) < y_end)
                    {
                        egui_canvas_draw_point(x, y + yinc, color, alpha_0);
                    }
                }
            }

            tmp_e += gradient;
            if (tmp_e >= EGUI_FLOAT_VALUE(1.0f))
            {
                y += yinc;
                tmp_e -= EGUI_FLOAT_VALUE(1.0f);
            }
        }
    }
    else
    {
        // Thick line: scan perpendicular to line direction
        // Use the line equation to determine coverage for each pixel

        // Normalize direction: compute perpendicular offset using fixed-point
        // Line direction: (dx, dy), perpendicular: (-dy, dx)
        // Length of direction vector for normalization
        // We use the half-width offset along perpendicular direction

        // For thick anti-aliased lines, we scan the bounding box
        // and for each pixel, compute distance to the line segment,
        // then map distance to alpha coverage.

        // Line segment from (x1,y1) to (x2,y2)
        // For a point (px, py), distance to infinite line = |cross_product| / line_length
        // cross_product = (x2-x1)*(y1-py) - (x1-px)*(y2-y1)

        // Use fixed-point for distance calculation
        // line_length_sq = dx*dx + dy*dy (avoid sqrt by comparing squared distances)

        dx = x2 - x1;
        dy = y2 - y1;
        int32_t line_len_sq = (int32_t)dx * dx + (int32_t)dy * dy;

        // Compute scan bounds
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

        // Pre-compute AA thresholds (constant for all pixels)
        int32_t outer_px = (stroke_width + 1);
        int64_t outer_thresh_sq = (int64_t)outer_px * outer_px * line_len_sq;
        outer_thresh_sq >>= 2;

        int32_t inner_px = (stroke_width - 1);
        if (inner_px < 0)
        {
            inner_px = 0;
        }
        int64_t inner_thresh_sq = (int64_t)inner_px * inner_px * line_len_sq;
        inner_thresh_sq >>= 2;

        // Adaptive shift for AA interpolation precision.
        // Fixed >>16 destroyed precision for segments shorter than ~114px.
        // Find minimum shift so (aa_range >> shift) * 255 fits in int32.
        int64_t aa_range = outer_thresh_sq - inner_thresh_sq;
        int aa_shift = 0;
        {
            int64_t tmp = aa_range;
            while (tmp > 8000000LL)
            {
                tmp >>= 1;
                aa_shift++;
            }
        }
        int32_t aa_range_shifted = (int32_t)(aa_range >> aa_shift);

        for (egui_dim_t py = scan_y1; py <= scan_y2; py++)
        {
            for (egui_dim_t px = scan_x1; px <= scan_x2; px++)
            {
                // Compute cross product (distance * line_length)
                int32_t cross = (int32_t)dx * (y1 - py) - (int32_t)(x1 - px) * dy;

                // Also check projection along line to cap endpoints
                int32_t dot = (int32_t)(px - x1) * dx + (int32_t)(py - y1) * dy;

                // Compute squared distance: cross^2 / line_len_sq
                int64_t cross_sq = (int64_t)cross * cross;

                if (cross_sq > outer_thresh_sq)
                {
                    continue;
                }

                egui_alpha_t px_alpha;
                if (cross_sq <= inner_thresh_sq)
                {
                    // Fully inside
                    px_alpha = alpha;
                }
                else
                {
                    // Anti-aliasing zone
                    if (aa_range_shifted <= 0)
                    {
                        px_alpha = alpha >> 1;
                    }
                    else
                    {
                        int32_t dist_from_inner = (int32_t)((cross_sq - inner_thresh_sq) >> aa_shift);
                        int32_t coverage = EGUI_ALPHA_100 - (int32_t)dist_from_inner * EGUI_ALPHA_100 / aa_range_shifted;
                        if (coverage < 0)
                        {
                            coverage = 0;
                        }
                        px_alpha = egui_color_alpha_mix(alpha, (egui_alpha_t)coverage);
                    }
                }

                // Endpoint capping
                if (dot < 0)
                {
                    int32_t end_dist_sq = (int32_t)(px - x1) * (px - x1) + (int32_t)(py - y1) * (py - y1);
                    int32_t half_w_sq = (stroke_width * stroke_width) >> 2;
                    int32_t outer_cap_sq = ((stroke_width + 2) * (stroke_width + 2)) >> 2;
                    if (end_dist_sq > outer_cap_sq)
                    {
                        continue;
                    }
                    if (end_dist_sq > half_w_sq)
                    {
                        int32_t range = outer_cap_sq - half_w_sq;
                        if (range > 0)
                        {
                            int32_t dist_from_inner = end_dist_sq - half_w_sq;
                            int32_t cap_coverage = EGUI_ALPHA_100 - (int32_t)dist_from_inner * EGUI_ALPHA_100 / range;
                            if (cap_coverage < 0)
                            {
                                cap_coverage = 0;
                            }
                            px_alpha = egui_color_alpha_mix(px_alpha, (egui_alpha_t)cap_coverage);
                        }
                    }
                }
                else if (dot > line_len_sq)
                {
                    int32_t end_dist_sq = (int32_t)(px - x2) * (px - x2) + (int32_t)(py - y2) * (py - y2);
                    int32_t half_w_sq = (stroke_width * stroke_width) >> 2;
                    int32_t outer_cap_sq = ((stroke_width + 2) * (stroke_width + 2)) >> 2;
                    if (end_dist_sq > outer_cap_sq)
                    {
                        continue;
                    }
                    if (end_dist_sq > half_w_sq)
                    {
                        int32_t range = outer_cap_sq - half_w_sq;
                        if (range > 0)
                        {
                            int32_t dist_from_inner = end_dist_sq - half_w_sq;
                            int32_t cap_coverage = EGUI_ALPHA_100 - (int32_t)dist_from_inner * EGUI_ALPHA_100 / range;
                            if (cap_coverage < 0)
                            {
                                cap_coverage = 0;
                            }
                            px_alpha = egui_color_alpha_mix(px_alpha, (egui_alpha_t)cap_coverage);
                        }
                    }
                }

                if (px_alpha > 0)
                {
                    egui_canvas_draw_point(px, py, color, px_alpha);
                }
            }
        }
    }
}

/**
 * \brief           Draw thick line segment with butt caps (no endpoint extension).
 *                  Used internally by bezier curves to avoid joint artifacts.
 *                  Pixels outside the projection range [0, line_len] are skipped.
 */
void egui_canvas_draw_line_segment(egui_dim_t x1, egui_dim_t y1, egui_dim_t x2, egui_dim_t y2, egui_dim_t stroke_width, egui_color_t color, egui_alpha_t alpha)
{
    egui_canvas_t *self = &canvas_data;

    egui_dim_t deltax = EGUI_ABS(x2 - x1);
    egui_dim_t deltay = EGUI_ABS(y2 - y1);

    // Degenerate: zero-length segment
    if (deltax == 0 && deltay == 0)
    {
        return;
    }

    // For thin lines, delegate to normal draw_line.
    // Keep axis-aligned thick segments in this function to avoid draw_line's
    // endpoint extension, which can cause joint bulges in polygon/polyline.
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

    // Ensure x1 <= x2
    if (x1 > x2)
    {
        EGUI_SWAP(x1, x2);
        EGUI_SWAP(y1, y2);
    }

    egui_dim_t dx = x2 - x1;
    egui_dim_t dy = y2 - y1;
    int32_t line_len_sq = (int32_t)dx * dx + (int32_t)dy * dy;

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

    // Pre-compute AA thresholds (constant for all pixels)
    int32_t outer_px = (stroke_width + 1);
    int64_t outer_thresh_sq = (int64_t)outer_px * outer_px * line_len_sq;
    outer_thresh_sq >>= 2;

    int32_t inner_px = (stroke_width - 1);
    if (inner_px < 0)
    {
        inner_px = 0;
    }
    int64_t inner_thresh_sq = (int64_t)inner_px * inner_px * line_len_sq;
    inner_thresh_sq >>= 2;

    // Adaptive shift for AA interpolation precision.
    // Fixed >>16 destroyed precision for segments shorter than ~114px.
    int64_t aa_range = outer_thresh_sq - inner_thresh_sq;
    int aa_shift = 0;
    {
        int64_t tmp = aa_range;
        while (tmp > 8000000LL)
        {
            tmp >>= 1;
            aa_shift++;
        }
    }
    int32_t aa_range_shifted = (int32_t)(aa_range >> aa_shift);

    for (egui_dim_t py = scan_y1; py <= scan_y2; py++)
    {
        for (egui_dim_t px = scan_x1; px <= scan_x2; px++)
        {
            int32_t cross = (int32_t)dx * (y1 - py) - (int32_t)(x1 - px) * dy;
            int32_t dot = (int32_t)(px - x1) * dx + (int32_t)(py - y1) * dy;

            // Closed projection interval [start, end].
            // This preserves continuity at shared vertices without endpoint
            // extension.
            if (dot < 0 || dot > line_len_sq)
            {
                continue;
            }

            int64_t cross_sq = (int64_t)cross * cross;

            if (cross_sq > outer_thresh_sq)
            {
                continue;
            }

            egui_alpha_t px_alpha;
            if (cross_sq <= inner_thresh_sq)
            {
                px_alpha = alpha;
            }
            else
            {
                if (aa_range_shifted <= 0)
                {
                    px_alpha = alpha >> 1;
                }
                else
                {
                    int32_t dist_from_inner = (int32_t)((cross_sq - inner_thresh_sq) >> aa_shift);
                    int32_t coverage = EGUI_ALPHA_100 - (int32_t)dist_from_inner * EGUI_ALPHA_100 / aa_range_shifted;
                    if (coverage < 0)
                    {
                        coverage = 0;
                    }
                    px_alpha = egui_color_alpha_mix(alpha, (egui_alpha_t)coverage);
                }
            }

            if (px_alpha > 0)
            {
                egui_canvas_draw_point(px, py, color, px_alpha);
            }
        }
    }
}
