#include "egui_canvas.h"

/**
 * @brief Polygon and polyline drawing with scanline fill and SDF anti-aliasing.
 *
 * Fill uses scanline + edge table with even-odd rule.
 * Anti-aliasing uses signed-distance-field (SDF) perpendicular to each edge,
 * matching the triangle fill approach for smooth edges at all angles.
 * Outline/polyline delegates to egui_canvas_draw_line() for each edge.
 * Maximum vertex count limited to EGUI_CANVAS_POLYGON_MAX_VERTICES.
 */

#ifndef EGUI_CANVAS_POLYGON_MAX_VERTICES
#define EGUI_CANVAS_POLYGON_MAX_VERTICES 16
#endif

/**
 * @brief Intersection record: x position + originating edge index.
 */
/**
 * @brief Integer square root (bit-by-bit) for edge length computation.
 */
static uint32_t polygon_isqrt(uint32_t n)
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
 * @brief Simple insertion sort for intersection records (max 16 elements).
 */
static void sort_polygon_intersections(egui_float_t *x_arr, uint8_t *edge_idx_arr, int count)
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
 * @brief Compute SDF-based alpha for a pixel relative to one polygon edge.
 *
 * The signed distance from pixel center (px, py) to the edge line gives
 * smooth coverage independent of edge angle.
 *   cross = edx * (py - ey) - edy * (px - ex)
 *   d = sign * cross / len
 *
 * Uses a 1.5-pixel-wide AA zone (d in [-0.75, +0.75]) for smoother edges,
 * especially on near-45-degree edges where the standard 1-pixel zone
 * produces visible stepping.
 *
 * @return alpha value (0 = fully outside, EGUI_ALPHA_100 = fully inside)
 */
__EGUI_STATIC_INLINE__ egui_alpha_t polygon_edge_alpha(int32_t cross, int32_t sign, uint32_t len)
{
    // d_q8 = sign * cross * 256 / len  (Q8 fixed-point, 256 units = 1 pixel)
    int32_t d_q8 = (int32_t)(sign * ((int64_t)cross * 256 / (int32_t)len));

    // 1.5-pixel AA zone: [-192, +192] in Q8 (corresponds to [-0.75, +0.75] pixels)
    if (d_q8 <= -192)
    {
        return EGUI_ALPHA_0;
    }
    if (d_q8 >= 192)
    {
        return EGUI_ALPHA_100;
    }

    // Linear ramp: alpha = (d_q8 + 192) * 255 / 384
    // Approximated as (d_q8 + 192) * 170 >> 8 (170/256 ≈ 255/384)
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

__EGUI_STATIC_INLINE__ void polygon_blend_direct(egui_color_t *dst, egui_color_t color, egui_alpha_t alpha)
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

__EGUI_STATIC_INLINE__ void polygon_fill_direct_span(egui_color_t *dst_row, egui_dim_t pfb_ofs_x, egui_dim_t x_start, egui_dim_t x_end, egui_color_t color,
                                                     egui_alpha_t alpha)
{
    egui_color_int_t *dst;
    uint32_t count;

    if (alpha == 0 || x_start > x_end)
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

/**
 * \brief           Draw filled polygon with SDF anti-aliased edges
 * \param[in]       points: Array of vertex coordinates [x0,y0,x1,y1,...], flat array
 * \param[in]       count: Number of vertices (not coordinate count)
 * \param[in]       color: Color used for drawing operation
 * \param[in]       alpha: Alpha value for blending
 */
void egui_canvas_draw_polygon_fill(const egui_dim_t *points, uint8_t count, egui_color_t color, egui_alpha_t alpha)
{
    egui_canvas_t *self = &canvas_data;

    if (count < 3 || count > EGUI_CANVAS_POLYGON_MAX_VERTICES)
    {
        return;
    }

    // Calculate bounding box
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

    // =========================================================================
    // Precompute edge vectors, lengths, and inside-direction signs
    // =========================================================================
    int32_t edx[EGUI_CANVAS_POLYGON_MAX_VERTICES];
    int32_t edy[EGUI_CANVAS_POLYGON_MAX_VERTICES];
    uint16_t elen[EGUI_CANVAS_POLYGON_MAX_VERTICES];
    int8_t esign[EGUI_CANVAS_POLYGON_MAX_VERTICES];

    // Compute centroid (scaled by count to avoid division) for inside determination
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
        elen[i] = polygon_isqrt(len_sq);
        if (elen[i] == 0)
        {
            elen[i] = 1;
        }

        // Inside sign: centroid should be on the inside
        // cross = edx * (cy*count - count*oy) - edy * (cx*count - count*ox)
        int32_t cross_c = edx[i] * (cy_sum - (int32_t)count * points[i * 2 + 1]) - edy[i] * (cx_sum - (int32_t)count * points[i * 2]);
        esign[i] = (cross_c >= 0) ? 1 : -1;
    }

    // =========================================================================
    // Scanline loop
    // =========================================================================
    egui_dim_t work_y_start = self->base_view_work_region.location.y;
    egui_dim_t work_y_end = work_y_start + self->base_view_work_region.size.height;
    egui_dim_t work_x_start = self->base_view_work_region.location.x;
    egui_dim_t work_x_end = work_x_start + self->base_view_work_region.size.width - 1;
    egui_dim_t scan_y_start = EGUI_MAX(min_y, work_y_start);
    egui_dim_t scan_y_end = EGUI_MIN(max_y, work_y_end - 1);

    // Intersection buffer with edge tracking
    egui_float_t intersection_x[EGUI_CANVAS_POLYGON_MAX_VERTICES];
    uint8_t intersection_edge_idx[EGUI_CANVAS_POLYGON_MAX_VERTICES];
    int use_direct_pfb = (self->mask == NULL) ? 1 : 0;
    egui_dim_t pfb_width = self->pfb_region.size.width;
    egui_dim_t pfb_height = self->pfb_region.size.height;
    egui_dim_t pfb_ofs_x = self->pfb_location_in_base_view.x;
    egui_dim_t pfb_ofs_y = self->pfb_location_in_base_view.y;
    egui_dim_t tile_x_start = pfb_ofs_x;
    egui_dim_t tile_x_end = pfb_ofs_x + pfb_width - 1;
    egui_dim_t tile_y_start = pfb_ofs_y;
    egui_dim_t tile_y_end = pfb_ofs_y + pfb_height - 1;
    egui_alpha_t eff_alpha = egui_color_alpha_mix(self->alpha, alpha);
    int apply_draw_alpha = (alpha != EGUI_ALPHA_100);
    int apply_canvas_alpha = (eff_alpha != EGUI_ALPHA_100);

    if (use_direct_pfb)
    {
        work_x_start = EGUI_MAX(work_x_start, tile_x_start);
        work_x_end = EGUI_MIN(work_x_end, tile_x_end);
        scan_y_start = EGUI_MAX(scan_y_start, tile_y_start);
        scan_y_end = EGUI_MIN(scan_y_end, tile_y_end);
    }

// AA zone width: 2 pixels on each side of the edge crossing
#define POLY_AA_MARGIN 2

    for (egui_dim_t y = scan_y_start; y <= scan_y_end; y++)
    {
        int intersection_count = 0;
        egui_color_t *dst_row = NULL;

        if (use_direct_pfb)
        {
            dst_row = (egui_color_t *)&self->pfb[(y - pfb_ofs_y) * pfb_width];
        }

        // Find intersections with all edges
        for (uint8_t i = 0; i < count; i++)
        {
            uint8_t j = (i + 1) % count;
            egui_dim_t y0 = points[i * 2 + 1];
            egui_dim_t y1 = points[j * 2 + 1];
            egui_dim_t x0 = points[i * 2];
            egui_dim_t x1 = points[j * 2];

            // Skip horizontal edges
            if (y0 == y1)
            {
                continue;
            }

            // Check if scanline intersects this edge
            egui_dim_t y_min = EGUI_MIN(y0, y1);
            egui_dim_t y_max = EGUI_MAX(y0, y1);

            if (y < y_min || y >= y_max)
            {
                continue;
            }

            // Calculate x intersection using fixed-point
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

        // Sort intersections by x
        sort_polygon_intersections(intersection_x, intersection_edge_idx, intersection_count);

        // Fill between pairs of intersections (even-odd rule)
        for (int k = 0; k + 1 < intersection_count; k += 2)
        {
            egui_float_t left_fx = intersection_x[k];
            egui_float_t right_fx = intersection_x[k + 1];
            uint8_t left_edge = intersection_edge_idx[k];
            uint8_t right_edge = intersection_edge_idx[k + 1];

            egui_dim_t ix_left = EGUI_FLOAT_INT_PART(left_fx);
            egui_dim_t ix_right = EGUI_FLOAT_INT_PART(right_fx);

            // Left AA zone: ix_left - 1 to ix_left + POLY_AA_MARGIN - 1
            egui_dim_t aa_left_start = EGUI_MAX(ix_left - 1, work_x_start);
            egui_dim_t aa_left_end = EGUI_MIN(ix_left + POLY_AA_MARGIN - 1, work_x_end);

            // Right AA zone: ix_right - POLY_AA_MARGIN + 1 to ix_right + 1
            egui_dim_t aa_right_start = EGUI_MAX(ix_right - POLY_AA_MARGIN + 1, work_x_start);
            egui_dim_t aa_right_end = EGUI_MIN(ix_right + 1, work_x_end);

            // Interior span (fully inside, no AA needed)
            egui_dim_t interior_left = aa_left_end + 1;
            egui_dim_t interior_right = aa_right_start - 1;

            if (use_direct_pfb)
            {
                aa_left_start = EGUI_MAX(aa_left_start, tile_x_start);
                aa_left_end = EGUI_MIN(aa_left_end, tile_x_end);
                aa_right_start = EGUI_MAX(aa_right_start, tile_x_start);
                aa_right_end = EGUI_MIN(aa_right_end, tile_x_end);
                interior_left = EGUI_MAX(interior_left, tile_x_start);
                interior_right = EGUI_MIN(interior_right, tile_x_end);
            }

            // Edge origins for SDF computation
            egui_dim_t le_ox = points[left_edge * 2];
            egui_dim_t le_oy = points[left_edge * 2 + 1];
            egui_dim_t re_ox = points[right_edge * 2];
            egui_dim_t re_oy = points[right_edge * 2 + 1];

            // Handle thin polygon (AA zones overlap)
            if (interior_left > interior_right)
            {
                // Entire span needs per-pixel SDF
                egui_dim_t span_start = EGUI_MAX(ix_left - 1, work_x_start);
                egui_dim_t span_end = EGUI_MIN(ix_right + 1, work_x_end);

                if (use_direct_pfb)
                {
                    span_start = EGUI_MAX(span_start, tile_x_start);
                    span_end = EGUI_MIN(span_end, tile_x_end);
                }

                for (egui_dim_t px = span_start; px <= span_end; px++)
                {
                    int32_t crossL = edx[left_edge] * (y - le_oy) - edy[left_edge] * (px - le_ox);
                    egui_alpha_t aL = polygon_edge_alpha(crossL, esign[left_edge], elen[left_edge]);

                    int32_t crossR = edx[right_edge] * (y - re_oy) - edy[right_edge] * (px - re_ox);
                    egui_alpha_t aR = polygon_edge_alpha(crossR, esign[right_edge], elen[right_edge]);

                    egui_alpha_t cov = (aL < aR) ? aL : aR;
                    if (cov > 0)
                    {
                        if (use_direct_pfb)
                        {
                            egui_alpha_t mix = apply_canvas_alpha ? egui_color_alpha_mix(eff_alpha, cov) : cov;
                            polygon_blend_direct(&dst_row[px - pfb_ofs_x], color, mix);
                        }
                        else
                        {
                            egui_alpha_t mix = apply_draw_alpha ? egui_color_alpha_mix(alpha, cov) : cov;
                            if (mix > 0)
                            {
                                egui_canvas_draw_point(px, y, color, mix);
                            }
                        }
                    }
                }
                continue;
            }

            // Draw interior span with hline (fast path)
            if (interior_left <= interior_right)
            {
                if (use_direct_pfb)
                {
                    polygon_fill_direct_span(dst_row, pfb_ofs_x, interior_left, interior_right, color, eff_alpha);
                }
                else
                {
                    egui_canvas_draw_hline(interior_left, y, interior_right - interior_left + 1, color, alpha);
                }
            }

            // Left AA zone: use SDF to the left-side edge, also check right edge near vertices
            for (egui_dim_t px = aa_left_start; px <= aa_left_end; px++)
            {
                int32_t crossL = edx[left_edge] * (y - le_oy) - edy[left_edge] * (px - le_ox);
                egui_alpha_t aL = polygon_edge_alpha(crossL, esign[left_edge], elen[left_edge]);

                int32_t crossR = edx[right_edge] * (y - re_oy) - edy[right_edge] * (px - re_ox);
                egui_alpha_t aR = polygon_edge_alpha(crossR, esign[right_edge], elen[right_edge]);

                egui_alpha_t cov = (aL < aR) ? aL : aR;
                if (cov > 0)
                {
                    if (use_direct_pfb)
                    {
                        egui_alpha_t mix = apply_canvas_alpha ? egui_color_alpha_mix(eff_alpha, cov) : cov;
                        polygon_blend_direct(&dst_row[px - pfb_ofs_x], color, mix);
                    }
                    else
                    {
                        egui_alpha_t mix = apply_draw_alpha ? egui_color_alpha_mix(alpha, cov) : cov;
                        if (mix > 0)
                        {
                            egui_canvas_draw_point(px, y, color, mix);
                        }
                    }
                }
            }

            // Right AA zone: use SDF to the right-side edge, also check left edge near vertices
            for (egui_dim_t px = aa_right_start; px <= aa_right_end; px++)
            {
                int32_t crossR = edx[right_edge] * (y - re_oy) - edy[right_edge] * (px - re_ox);
                egui_alpha_t aR = polygon_edge_alpha(crossR, esign[right_edge], elen[right_edge]);

                int32_t crossL = edx[left_edge] * (y - le_oy) - edy[left_edge] * (px - le_ox);
                egui_alpha_t aL = polygon_edge_alpha(crossL, esign[left_edge], elen[left_edge]);

                egui_alpha_t cov = (aL < aR) ? aL : aR;
                if (cov > 0)
                {
                    if (use_direct_pfb)
                    {
                        egui_alpha_t mix = apply_canvas_alpha ? egui_color_alpha_mix(eff_alpha, cov) : cov;
                        polygon_blend_direct(&dst_row[px - pfb_ofs_x], color, mix);
                    }
                    else
                    {
                        egui_alpha_t mix = apply_draw_alpha ? egui_color_alpha_mix(alpha, cov) : cov;
                        if (mix > 0)
                        {
                            egui_canvas_draw_point(px, y, color, mix);
                        }
                    }
                }
            }
        }
    }

#undef POLY_AA_MARGIN
}

/**
 * \brief           Draw polygon outline
 * \param[in]       points: Array of vertex coordinates [x0,y0,x1,y1,...], flat array
 * \param[in]       count: Number of vertices
 * \param[in]       stroke_width: Width of the outline
 * \param[in]       color: Color used for drawing operation
 * \param[in]       alpha: Alpha value for blending
 */
void egui_canvas_draw_polygon(const egui_dim_t *points, uint8_t count, egui_dim_t stroke_width, egui_color_t color, egui_alpha_t alpha)
{
    if (count < 2 || count > EGUI_CANVAS_POLYGON_MAX_VERTICES)
    {
        return;
    }

    for (uint8_t i = 0; i < count; i++)
    {
        uint8_t j = (i + 1) % count;
        egui_canvas_draw_line_segment(points[i * 2], points[i * 2 + 1], points[j * 2], points[j * 2 + 1], stroke_width, color, alpha);
    }
}

/**
 * \brief           Draw polyline (open polygon, does not close)
 * \param[in]       points: Array of vertex coordinates [x0,y0,x1,y1,...], flat array
 * \param[in]       count: Number of vertices
 * \param[in]       stroke_width: Width of the line
 * \param[in]       color: Color used for drawing operation
 * \param[in]       alpha: Alpha value for blending
 */
void egui_canvas_draw_polyline(const egui_dim_t *points, uint8_t count, egui_dim_t stroke_width, egui_color_t color, egui_alpha_t alpha)
{
    if (count < 2 || count > EGUI_CANVAS_POLYGON_MAX_VERTICES)
    {
        return;
    }

    for (uint8_t i = 0; i + 1 < count; i++)
    {
        egui_canvas_draw_line_segment(points[i * 2], points[i * 2 + 1], points[(i + 1) * 2], points[(i + 1) * 2 + 1], stroke_width, color, alpha);
    }

    // Bridge tiny AA pinholes at internal joints with minimal fill
    // (center + 4-neighbors) to avoid visible seams without large bulges.
    if (stroke_width > 1 && count > 2)
    {
        for (uint8_t i = 1; i + 1 < count; i++)
        {
            egui_dim_t x = points[i * 2];
            egui_dim_t y = points[i * 2 + 1];
            egui_canvas_draw_point(x, y, color, alpha);
            egui_canvas_draw_point(x - 1, y, color, alpha);
            egui_canvas_draw_point(x + 1, y, color, alpha);
            egui_canvas_draw_point(x, y - 1, color, alpha);
            egui_canvas_draw_point(x, y + 1, color, alpha);
        }
    }
}
