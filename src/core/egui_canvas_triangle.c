#include "egui_canvas.h"

/**
 * @brief Triangle drawing with scanline fill algorithm and SDF anti-aliasing.
 *
 * Uses signed-distance-field (SDF) for edge anti-aliasing:
 * - Precompute edge direction vectors and lengths once
 * - For each scanline, find interior span (fast hline) and edge zone (SDF AA)
 * - Perpendicular distance to each edge determines smooth alpha coverage
 * - Works correctly at all edge angles, vertices, and thin tips
 *
 * Outline delegates to egui_canvas_draw_line() for each edge.
 */

/* Integer square root (bit-by-bit) for edge length computation. */
static uint32_t triangle_isqrt(uint32_t n)
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

/**
 * @brief Compute SDF-based alpha for a pixel relative to one triangle edge.
 *
 * The signed distance from pixel center (px, py) to the edge line from
 * (ex, ey) with direction (edx, edy) and precomputed length is:
 *   cross = edx * (py - ey) - edy * (px - ex)
 *   d = sign * cross / len
 * where sign ensures positive = inside the triangle.
 *
 * Maps d in [-0.5, +0.5] to alpha linearly.
 *
 * @return alpha value (0 = fully outside, EGUI_ALPHA_100 = fully inside)
 */
__EGUI_STATIC_INLINE__ egui_alpha_t triangle_edge_alpha(int32_t cross, int32_t sign, uint32_t len)
{
    // d_q8 = sign * cross * 256 / len  (Q8 fixed-point, represents [-0.5, +0.5] as [-128, +128])
    int32_t d_q8 = (int32_t)(sign * ((int64_t)cross * 256 / (int32_t)len));

    if (d_q8 <= -128)
    {
        return EGUI_ALPHA_0;
    }
    if (d_q8 >= 128)
    {
        return EGUI_ALPHA_100;
    }

    // Linear ramp: alpha = (d_q8 + 128) * ALPHA_100 / 256
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

/**
 * \brief           Draw filled triangle with SDF anti-aliased edges
 * \param[in]       x1, y1: First vertex
 * \param[in]       x2, y2: Second vertex
 * \param[in]       x3, y3: Third vertex
 * \param[in]       color: Color used for drawing operation
 * \param[in]       alpha: Alpha value for blending
 */
void egui_canvas_draw_triangle_fill(egui_dim_t x1, egui_dim_t y1, egui_dim_t x2, egui_dim_t y2, egui_dim_t x3, egui_dim_t y3, egui_color_t color,
                                    egui_alpha_t alpha)
{
    egui_canvas_t *self = &canvas_data;

    // Calculate bounding box for early rejection
    egui_dim_t min_x = EGUI_MIN(EGUI_MIN(x1, x2), x3);
    egui_dim_t min_y = EGUI_MIN(EGUI_MIN(y1, y2), y3);
    egui_dim_t max_x = EGUI_MAX(EGUI_MAX(x1, x2), x3);
    egui_dim_t max_y = EGUI_MAX(EGUI_MAX(y1, y2), y3);

    EGUI_REGION_DEFINE(region, min_x, min_y, max_x - min_x + 1, max_y - min_y + 1);
    if (!egui_region_is_intersect(&region, &self->base_view_work_region))
    {
        return;
    }

    // Sort vertices by y-coordinate: v0.y <= v1.y <= v2.y
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

    // Degenerate: all on same horizontal line
    if (vy0 == vy2)
    {
        egui_dim_t left = EGUI_MIN(EGUI_MIN(vx0, vx1), vx2);
        egui_dim_t right = EGUI_MAX(EGUI_MAX(vx0, vx1), vx2);
        egui_canvas_draw_hline(left, vy0, right - left + 1, color, alpha);
        return;
    }

    // =========================================================================
    // Precompute edge vectors, lengths, and inside-direction signs
    // =========================================================================
    // Edge L: v0→v2 (long edge, spans full height)
    int32_t eLdx = vx2 - vx0, eLdy = vy2 - vy0;
    uint32_t lenL = triangle_isqrt((uint32_t)(eLdx * eLdx + eLdy * eLdy));
    if (lenL == 0)
    {
        lenL = 1;
    }

    // Edge A: v0→v1 (short edge, top half)
    int32_t eAdx = vx1 - vx0, eAdy = vy1 - vy0;
    uint32_t lenA = triangle_isqrt((uint32_t)(eAdx * eAdx + eAdy * eAdy));
    if (lenA == 0)
    {
        lenA = 1;
    }

    // Edge B: v1→v2 (short edge, bottom half)
    int32_t eBdx = vx2 - vx1, eBdy = vy2 - vy1;
    uint32_t lenB = triangle_isqrt((uint32_t)(eBdx * eBdx + eBdy * eBdy));
    if (lenB == 0)
    {
        lenB = 1;
    }

    // Determine inside sign for each edge using centroid test
    // cross(edge_dir, centroid - edge_origin) > 0 means centroid is on the left side
    int32_t cx3 = vx0 + vx1 + vx2; // 3 * centroid_x
    int32_t cy3 = vy0 + vy1 + vy2; // 3 * centroid_y

    // Sign for long edge (v0→v2): positive cross means inside is on left
    int32_t sL = (eLdx * (cy3 - 3 * vy0) - eLdy * (cx3 - 3 * vx0) >= 0) ? 1 : -1;
    // Sign for edge A (v0→v1)
    int32_t sA = (eAdx * (cy3 - 3 * vy0) - eAdy * (cx3 - 3 * vx0) >= 0) ? 1 : -1;
    // Sign for edge B (v1→v2)
    int32_t sB = (eBdx * (cy3 - 3 * vy1) - eBdy * (cx3 - 3 * vx1) >= 0) ? 1 : -1;

    // =========================================================================
    // Scanline loop
    // =========================================================================
    egui_dim_t work_y_start = self->base_view_work_region.location.y;
    egui_dim_t work_y_end = work_y_start + self->base_view_work_region.size.height;
    egui_dim_t work_x_start = self->base_view_work_region.location.x;
    egui_dim_t work_x_end = work_x_start + self->base_view_work_region.size.width - 1;

    egui_dim_t scan_y_start = EGUI_MAX(vy0, work_y_start);
    egui_dim_t scan_y_end = EGUI_MIN(vy2, work_y_end - 1);

// AA zone width: 2 pixels on each side of the edge crossing
#define TRI_AA_MARGIN 2

    for (egui_dim_t y = scan_y_start; y <= scan_y_end; y++)
    {
        egui_float_t left_x, right_x;

        // Long edge: v0 -> v2
        if (vy2 != vy0)
        {
            left_x = EGUI_FLOAT_VALUE_INT(vx0) +
                     EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(vx2 - vx0), EGUI_FLOAT_DIV(EGUI_FLOAT_VALUE_INT(y - vy0), EGUI_FLOAT_VALUE_INT(vy2 - vy0)));
        }
        else
        {
            left_x = EGUI_FLOAT_VALUE_INT(vx0);
        }

        // Short edges: v0->v1 or v1->v2
        // Determine which short edge is active and its SDF parameters
        int32_t act_edx, act_edy, act_sign;
        uint32_t act_len;
        egui_dim_t act_ox, act_oy;

        if (y <= vy1)
        {
            // Top half: v0 -> v1
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
            // Bottom half: v1 -> v2
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

        // Determine which edge is left and which is right
        int32_t long_edx, long_edy, long_sign;
        uint32_t long_len;
        egui_dim_t long_ox, long_oy;
        int32_t short_edx, short_edy, short_sign;
        uint32_t short_len;
        egui_dim_t short_ox, short_oy;

        if (left_x <= right_x)
        {
            // Long edge is on the left
            long_edx = eLdx;
            long_edy = eLdy;
            long_len = lenL;
            long_sign = sL;
            long_ox = vx0;
            long_oy = vy0;
            short_edx = act_edx;
            short_edy = act_edy;
            short_len = act_len;
            short_sign = act_sign;
            short_ox = act_ox;
            short_oy = act_oy;
        }
        else
        {
            // Short edge is on the left, swap
            egui_float_t tmp = left_x;
            left_x = right_x;
            right_x = tmp;
            long_edx = act_edx;
            long_edy = act_edy;
            long_len = act_len;
            long_sign = act_sign;
            long_ox = act_ox;
            long_oy = act_oy;
            short_edx = eLdx;
            short_edy = eLdy;
            short_len = lenL;
            short_sign = sL;
            short_ox = vx0;
            short_oy = vy0;
        }

        // Integer edge positions
        egui_dim_t ix_left = EGUI_FLOAT_INT_PART(left_x);
        egui_dim_t ix_right = EGUI_FLOAT_INT_PART(right_x);

        // Left AA zone: ix_left - 1 to ix_left + AA_MARGIN - 1
        egui_dim_t aa_left_start = EGUI_MAX(ix_left - 1, work_x_start);
        egui_dim_t aa_left_end = EGUI_MIN(ix_left + TRI_AA_MARGIN - 1, work_x_end);

        // Right AA zone: ix_right - AA_MARGIN + 1 to ix_right + 1
        egui_dim_t aa_right_start = EGUI_MAX(ix_right - TRI_AA_MARGIN + 1, work_x_start);
        egui_dim_t aa_right_end = EGUI_MIN(ix_right + 1, work_x_end);

        // Interior span (fully inside, no AA needed)
        egui_dim_t interior_left = aa_left_end + 1;
        egui_dim_t interior_right = aa_right_start - 1;

        // Handle thin triangle (AA zones overlap)
        if (interior_left > interior_right)
        {
            // Entire span needs per-pixel SDF
            egui_dim_t span_start = EGUI_MAX(ix_left - 1, work_x_start);
            egui_dim_t span_end = EGUI_MIN(ix_right + 1, work_x_end);

            for (egui_dim_t px = span_start; px <= span_end; px++)
            {
                // SDF to left edge (long_* variables are the left-side edge)
                int32_t crossL = long_edx * (y - long_oy) - long_edy * (px - long_ox);
                egui_alpha_t aL = triangle_edge_alpha(crossL, long_sign, long_len);

                // SDF to right edge
                int32_t crossR = short_edx * (y - short_oy) - short_edy * (px - short_ox);
                egui_alpha_t aR = triangle_edge_alpha(crossR, short_sign, short_len);

                // Coverage = min of both edge distances
                egui_alpha_t cov = (aL < aR) ? aL : aR;
                if (cov > 0)
                {
                    egui_alpha_t mix = egui_color_alpha_mix(alpha, cov);
                    if (mix > 0)
                    {
                        egui_canvas_draw_point(px, y, color, mix);
                    }
                }
            }
            continue;
        }

        // Draw interior span with hline (fast path)
        if (interior_left <= interior_right)
        {
            egui_canvas_draw_hline(interior_left, y, interior_right - interior_left + 1, color, alpha);
        }

        // Left AA zone: use SDF to the left-side edge
        for (egui_dim_t px = aa_left_start; px <= aa_left_end; px++)
        {
            int32_t crossL = long_edx * (y - long_oy) - long_edy * (px - long_ox);
            egui_alpha_t aL = triangle_edge_alpha(crossL, long_sign, long_len);

            // Also check right edge (for narrow triangles near vertex)
            int32_t crossR = short_edx * (y - short_oy) - short_edy * (px - short_ox);
            egui_alpha_t aR = triangle_edge_alpha(crossR, short_sign, short_len);

            egui_alpha_t cov = (aL < aR) ? aL : aR;
            if (cov > 0)
            {
                egui_alpha_t mix = egui_color_alpha_mix(alpha, cov);
                if (mix > 0)
                {
                    egui_canvas_draw_point(px, y, color, mix);
                }
            }
        }

        // Right AA zone: use SDF to the right-side edge
        for (egui_dim_t px = aa_right_start; px <= aa_right_end; px++)
        {
            int32_t crossR = short_edx * (y - short_oy) - short_edy * (px - short_ox);
            egui_alpha_t aR = triangle_edge_alpha(crossR, short_sign, short_len);

            // Also check left edge (for narrow triangles near vertex)
            int32_t crossL = long_edx * (y - long_oy) - long_edy * (px - long_ox);
            egui_alpha_t aL = triangle_edge_alpha(crossL, long_sign, long_len);

            egui_alpha_t cov = (aL < aR) ? aL : aR;
            if (cov > 0)
            {
                egui_alpha_t mix = egui_color_alpha_mix(alpha, cov);
                if (mix > 0)
                {
                    egui_canvas_draw_point(px, y, color, mix);
                }
            }
        }
    }

#undef TRI_AA_MARGIN
}

/**
 * \brief           Draw triangle outline
 * \param[in]       x1, y1: First vertex
 * \param[in]       x2, y2: Second vertex
 * \param[in]       x3, y3: Third vertex
 * \param[in]       color: Color used for drawing operation
 * \param[in]       alpha: Alpha value for blending
 */
void egui_canvas_draw_triangle(egui_dim_t x1, egui_dim_t y1, egui_dim_t x2, egui_dim_t y2, egui_dim_t x3, egui_dim_t y3, egui_color_t color, egui_alpha_t alpha)
{
    egui_canvas_draw_line(x1, y1, x2, y2, 1, color, alpha);
    egui_canvas_draw_line(x2, y2, x3, y3, 1, color, alpha);
    egui_canvas_draw_line(x3, y3, x1, y1, 1, color, alpha);
}
