#include "egui_canvas.h"

/**
 * @brief Ellipse drawing with midpoint algorithm and anti-aliasing.
 *
 * Uses scanline approach with the ellipse equation for edge detection.
 * Anti-aliasing via 4-point corner sampling (similar to existing arc AA).
 */

/* Integer square root (bit-by-bit) for scanline boundary computation. */
static uint32_t ellipse_isqrt(uint32_t n)
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

/* 64-bit integer square root (bit-by-bit) for gradient magnitude. */
static uint64_t ellipse_isqrt64(uint64_t n)
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

/**
 * @brief Alpha-max-beta-min approximation for sqrt(a² + b²).
 *
 * Max error ~6.8%, sufficient for anti-aliasing coverage computation.
 * Replaces the expensive 64-bit integer square root (ellipse_isqrt64)
 * for gradient magnitude estimation.
 */
static uint32_t ellipse_grad_approx(int64_t a, int64_t b)
{
    uint32_t abs_a = (a >= 0) ? (uint32_t)a : (uint32_t)(-a);
    uint32_t abs_b = (b >= 0) ? (uint32_t)b : (uint32_t)(-b);
    uint32_t hi, lo;
    if (abs_a >= abs_b)
    {
        hi = abs_a;
        lo = abs_b;
    }
    else
    {
        hi = abs_b;
        lo = abs_a;
    }
    return hi + ((lo * 3) >> 3);
}

/**
 * @brief Integer square root with Newton-Raphson warm start.
 *
 * Uses previous scanline's result as starting guess. One NR iteration
 * brings the estimate within 1-2 of the true value. Falls back to
 * bit-by-bit algorithm when guess is zero.
 */
static uint32_t ellipse_isqrt_warm(uint32_t n, uint32_t guess)
{
    if (n == 0)
    {
        return 0;
    }
    if (guess == 0)
    {
        return ellipse_isqrt(n);
    }

    /* Two NR iterations for robust convergence */
    uint32_t r = (guess + n / guess) >> 1;
    r = (r + n / r) >> 1;

    /* Fine-tune: typically 0-1 corrections */
    while (r > 0 && r * r > n)
    {
        r--;
    }
    if ((r + 1) * (r + 1) <= n)
    {
        r++;
    }

    return r;
}

/**
 * @brief Compute inv_grad_q24 = (128 << 24) / grad_half using AMPM approximation.
 *
 * Replaces ellipse_isqrt64 + 64-bit division with alpha-max-beta-min + 32-bit division.
 * Uses 32-bit hardware division (UDIV on Cortex-M3) for the common case.
 */
static int64_t ellipse_inv_grad_from_approx(int64_t gx, int64_t gy)
{
    uint32_t grad_half = ellipse_grad_approx(gx, gy);
    if (grad_half == 0)
    {
        grad_half = 1;
    }
    if (grad_half >= 128)
    {
        /* Common path: result fits in uint32_t, use 32-bit division */
        return (int64_t)(uint32_t)(0x80000000UL / grad_half);
    }
    else
    {
        /* Rare: very small gradient (tiny ellipse) */
        return ((int64_t)128 << 24) / (int64_t)grad_half;
    }
}

/**
 * @brief Get anti-aliased alpha for an ellipse edge pixel using signed distance field.
 *
 * The ellipse implicit function F(x,y) = x^2*ry^2 + y^2*rx^2 - rx^2*ry^2 defines
 * interior (F<0), boundary (F=0), and exterior (F>0). The approximate signed
 * distance to the boundary is d = F / |grad F|, which gives smooth coverage at
 * ALL orientations (poles, sides, and everything in between).
 */
__EGUI_STATIC_INLINE__ egui_alpha_t ellipse_get_edge_alpha(egui_dim_t px, egui_dim_t py, int32_t rx_sq, int32_t ry_sq, int64_t rxry_sq)
{
    // F(x,y) = x^2*ry^2 + y^2*rx^2 - rx^2*ry^2
    // Positive = outside, Negative = inside
    int64_t f_val = (int64_t)px * px * ry_sq + (int64_t)py * py * rx_sq - rxry_sq;

    // Gradient half-components: grad_F/2 = (x*ry^2, y*rx^2)
    int64_t gx = (int64_t)px * ry_sq;
    int64_t gy = (int64_t)py * rx_sq;
    uint64_t grad_half_sq = (uint64_t)(gx * gx + gy * gy);

    if (grad_half_sq == 0)
    {
        return (f_val <= 0) ? EGUI_ALPHA_100 : EGUI_ALPHA_0;
    }

    uint64_t grad_half = ellipse_isqrt64(grad_half_sq);
    if (grad_half == 0)
    {
        grad_half = 1;
    }

    // signed_dist = F / |grad_F| = f_val / (2*grad_half)  [pixel units]
    // Map signed_dist in [-0.5, +0.5] to alpha in [ALPHA_100, 0]
    // Use Q8: dist_q8 = signed_dist * 256 = f_val * 128 / grad_half
    int64_t dist_q8 = f_val * 128 / (int64_t)grad_half;

    if (dist_q8 < -128)
    {
        return EGUI_ALPHA_100;
    }
    if (dist_q8 > 128)
    {
        return EGUI_ALPHA_0;
    }

    // Linear AA ramp with rounding
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

/**
 * @brief Fast edge alpha using precomputed reciprocal of gradient magnitude.
 *
 * Avoids the expensive 64-bit division per pixel by using a reciprocal
 * pre-computed once per scanline: inv_grad_q24 = (128 << 24) / grad_half_ref.
 * dist_q8 = (f_val * inv_grad_q24) >> 24  鈮? f_val * 128 / grad_half_ref.
 */
__EGUI_STATIC_INLINE__ egui_alpha_t ellipse_get_edge_alpha_fast(egui_dim_t px, egui_dim_t py, int32_t rx_sq, int32_t ry_sq, int64_t rxry_sq,
                                                                int64_t inv_grad_q24)
{
    int64_t f_val = (int64_t)px * px * ry_sq + (int64_t)py * py * rx_sq - rxry_sq;

    int64_t dist_q8 = (f_val * inv_grad_q24) >> 24;

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

/**
 * \brief           Draw filled ellipse with anti-aliased edges
 * \param[in]       center_x: Center X position
 * \param[in]       center_y: Center Y position
 * \param[in]       radius_x: Horizontal radius
 * \param[in]       radius_y: Vertical radius
 * \param[in]       color: Color used for drawing operation
 * \param[in]       alpha: Alpha value for blending
 */
void egui_canvas_draw_ellipse_fill(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius_x, egui_dim_t radius_y, egui_color_t color, egui_alpha_t alpha)
{
    egui_canvas_t *self = &canvas_data;

    if (radius_x <= 0 || radius_y <= 0)
    {
        return;
    }

    // Bounding box
    EGUI_REGION_DEFINE(region, center_x - radius_x, center_y - radius_y, (radius_x << 1) + 1, (radius_y << 1) + 1);
    egui_region_t region_intersect;
    egui_region_intersect(&region, &self->base_view_work_region, &region_intersect);
    if (egui_region_is_empty(&region_intersect))
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

    /* Direct PFB write setup for no-mask fast path */
    int use_direct_pfb = (self->mask == NULL) ? 1 : 0;
    egui_dim_t pfb_width = self->pfb_region.size.width;
    egui_dim_t pfb_ofs_x = self->pfb_location_in_base_view.x;
    egui_dim_t pfb_ofs_y = self->pfb_location_in_base_view.y;
    egui_alpha_t canvas_alpha = self->alpha;

    uint32_t warm_dx_max = 0; /* NR warm start for isqrt across scanlines */

    for (egui_dim_t dy = y_start; dy <= y_end; dy++)
    {
        egui_dim_t abs_y = center_y + dy;
        egui_dim_t abs_dy = EGUI_ABS(dy);

        // Compute scanline x-boundary: dx_max where dx^2 * ry_sq + dy^2 * rx_sq = rxry_sq
        int64_t dy_term = (int64_t)abs_dy * abs_dy * rx_sq;
        if (dy_term > rxry_sq)
        {
            continue;
        }
        int32_t dx_max_sq = (int32_t)((rxry_sq - dy_term) / ry_sq);
        int32_t dx_max = (int32_t)ellipse_isqrt_warm((uint32_t)dx_max_sq, warm_dx_max);
        warm_dx_max = (uint32_t)dx_max;

        // Extend scan range for smooth AA at poles: the adjacent inner row's
        // boundary determines how far we need to scan for partial coverage.
        int32_t dx_scan = dx_max;
        if (abs_dy > 0)
        {
            int32_t abs_dy_adj = abs_dy - 1;
            int64_t dy_term_adj = (int64_t)abs_dy_adj * abs_dy_adj * rx_sq;
            if (dy_term_adj < rxry_sq)
            {
                int32_t dx_adj = (int32_t)ellipse_isqrt_warm((uint32_t)((rxry_sq - dy_term_adj) / ry_sq), warm_dx_max);
                if (dx_adj > dx_scan)
                {
                    dx_scan = dx_adj;
                }
            }
        }

        // Inner boundary: compute proper margin using the gradient magnitude
        // at the scanline boundary. Reuse grad_half for edge AA.
        int32_t dx_inner = 0;
        int64_t inv_grad_q24 = (int64_t)128 << 24; // reciprocal of grad_half_ref, default for grad=1
        if (dx_max > 0)
        {
            int64_t gx = (int64_t)dx_max * ry_sq;
            int64_t gy = (int64_t)abs_dy * rx_sq;
            uint32_t grad_half_ref = ellipse_grad_approx(gx, gy);
            if (grad_half_ref == 0)
            {
                grad_half_ref = 1;
            }
            inv_grad_q24 = ellipse_inv_grad_from_approx(gx, gy);
            int64_t f_at_boundary = (int64_t)dx_max * dx_max * ry_sq + dy_term - rxry_sq;
            int64_t f_plus_grad = (int64_t)grad_half_ref + f_at_boundary;

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

        // Clamp to visible region (using extended scan range)
        egui_dim_t scan_left = EGUI_MAX(-(egui_dim_t)(dx_scan + 1), x_start);
        egui_dim_t scan_right = EGUI_MIN((egui_dim_t)(dx_scan + 1), x_end);
        egui_dim_t inner_left = 1;
        egui_dim_t inner_right = 0;
        if (dx_inner > 0)
        {
            inner_left = EGUI_MAX(-(egui_dim_t)dx_inner, x_start);
            inner_right = EGUI_MIN((egui_dim_t)dx_inner, x_end);
        }

        if (use_direct_pfb)
        {
            egui_color_t *dst_base = (egui_color_t *)&self->pfb[(abs_y - pfb_ofs_y) * pfb_width - pfb_ofs_x];
            egui_alpha_t eff_alpha = egui_color_alpha_mix(canvas_alpha, alpha);

            // Interior span: direct fill
            if (dx_inner > 0 && inner_left <= inner_right)
            {
                egui_dim_t abs_left = center_x + inner_left;
                egui_dim_t abs_right = center_x + inner_right;
                egui_dim_t fill_left = EGUI_MAX(abs_left, self->base_view_work_region.location.x);
                egui_dim_t fill_right = EGUI_MIN(abs_right, self->base_view_work_region.location.x + self->base_view_work_region.size.width - 1);
                if (fill_left <= fill_right)
                {
                    egui_dim_t count = fill_right - fill_left + 1;
                    egui_color_t *dst = &dst_base[fill_left];
                    if (eff_alpha == EGUI_ALPHA_100)
                    {
                        for (egui_dim_t i = 0; i < count; i++)
                        {
                            dst[i] = color;
                        }
                    }
                    else if (eff_alpha > 0)
                    {
                        for (egui_dim_t i = 0; i < count; i++)
                        {
                            egui_rgb_mix_ptr(&dst[i], &color, &dst[i], eff_alpha);
                        }
                    }
                }
            }

            // Left edge pixels: use precomputed gradient
            for (egui_dim_t dx = scan_left; dx < inner_left; dx++)
            {
                egui_dim_t abs_x = center_x + dx;
                if (abs_x < self->base_view_work_region.location.x || abs_x >= self->base_view_work_region.location.x + self->base_view_work_region.size.width)
                {
                    continue;
                }
                egui_dim_t abs_dx = EGUI_ABS(dx);
                egui_alpha_t cov = ellipse_get_edge_alpha_fast(abs_dx, abs_dy, rx_sq, ry_sq, rxry_sq, inv_grad_q24);
                if (cov > 0)
                {
                    egui_alpha_t m = egui_color_alpha_mix(eff_alpha, cov);
                    if (m > 0)
                    {
                        egui_color_t *back_color = &dst_base[abs_x];
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

            // Right edge pixels: use precomputed gradient
            for (egui_dim_t dx = inner_right + 1; dx <= scan_right; dx++)
            {
                egui_dim_t abs_x = center_x + dx;
                if (abs_x < self->base_view_work_region.location.x || abs_x >= self->base_view_work_region.location.x + self->base_view_work_region.size.width)
                {
                    continue;
                }
                egui_dim_t abs_dx = EGUI_ABS(dx);
                egui_alpha_t cov = ellipse_get_edge_alpha_fast(abs_dx, abs_dy, rx_sq, ry_sq, rxry_sq, inv_grad_q24);
                if (cov > 0)
                {
                    egui_alpha_t m = egui_color_alpha_mix(eff_alpha, cov);
                    if (m > 0)
                    {
                        egui_color_t *back_color = &dst_base[abs_x];
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
            // Fallback path: with mask
            // Draw interior span with fillrect (no AA needed)
            if (dx_inner > 0 && inner_left <= inner_right)
            {
                egui_canvas_draw_fillrect(center_x + inner_left, abs_y, inner_right - inner_left + 1, 1, color, alpha);
            }

            // Left edge pixels
            for (egui_dim_t dx = scan_left; dx < inner_left; dx++)
            {
                egui_dim_t abs_dx = EGUI_ABS(dx);
                egui_alpha_t cov = ellipse_get_edge_alpha_fast(abs_dx, abs_dy, rx_sq, ry_sq, rxry_sq, inv_grad_q24);
                if (cov > 0)
                {
                    egui_alpha_t mix = egui_color_alpha_mix(alpha, cov);
                    if (mix > 0)
                    {
                        egui_canvas_draw_point(center_x + dx, abs_y, color, mix);
                    }
                }
            }

            // Right edge pixels
            for (egui_dim_t dx = inner_right + 1; dx <= scan_right; dx++)
            {
                egui_dim_t abs_dx = EGUI_ABS(dx);
                egui_alpha_t cov = ellipse_get_edge_alpha_fast(abs_dx, abs_dy, rx_sq, ry_sq, rxry_sq, inv_grad_q24);
                if (cov > 0)
                {
                    egui_alpha_t mix = egui_color_alpha_mix(alpha, cov);
                    if (mix > 0)
                    {
                        egui_canvas_draw_point(center_x + dx, abs_y, color, mix);
                    }
                }
            }
        }
    }
}

/**
 * \brief           Draw ellipse outline with anti-aliased edges
 * \param[in]       center_x: Center X position
 * \param[in]       center_y: Center Y position
 * \param[in]       radius_x: Horizontal radius
 * \param[in]       radius_y: Vertical radius
 * \param[in]       stroke_width: Width of the outline
 * \param[in]       color: Color used for drawing operation
 * \param[in]       alpha: Alpha value for blending
 */
void egui_canvas_draw_ellipse(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius_x, egui_dim_t radius_y, egui_dim_t stroke_width, egui_color_t color,
                              egui_alpha_t alpha)
{
    egui_canvas_t *self = &canvas_data;

    if (radius_x <= 0 || radius_y <= 0)
    {
        return;
    }

    // If stroke_width >= min radius, draw filled ellipse
    if (stroke_width >= radius_x || stroke_width >= radius_y)
    {
        egui_canvas_draw_ellipse_fill(center_x, center_y, radius_x, radius_y, color, alpha);
        return;
    }

    // Bounding box
    EGUI_REGION_DEFINE(region, center_x - radius_x, center_y - radius_y, (radius_x << 1) + 1, (radius_y << 1) + 1);
    egui_region_t region_intersect;
    egui_region_intersect(&region, &self->base_view_work_region, &region_intersect);
    if (egui_region_is_empty(&region_intersect))
    {
        return;
    }

    // Outer ellipse
    int32_t orx = radius_x;
    int32_t ory = radius_y;
    int32_t orx_sq = orx * orx;
    int32_t ory_sq = ory * ory;
    int64_t orxry_sq = (int64_t)orx_sq * ory_sq;

    // Inner ellipse (hole)
    int32_t irx = radius_x - stroke_width;
    int32_t iry = radius_y - stroke_width;
    int32_t irx_sq = irx * irx;
    int32_t iry_sq = iry * iry;
    int64_t irxry_sq = (int64_t)irx_sq * iry_sq;

    egui_dim_t y_start = EGUI_MAX(-radius_y, region_intersect.location.y - center_y);
    egui_dim_t y_end = EGUI_MIN(radius_y, region_intersect.location.y + region_intersect.size.height - 1 - center_y);
    egui_dim_t x_start = EGUI_MAX(-radius_x, region_intersect.location.x - center_x);
    egui_dim_t x_end = EGUI_MIN(radius_x, region_intersect.location.x + region_intersect.size.width - 1 - center_x);

    /* Direct PFB write setup for no-mask fast path */
    int use_direct_pfb = (self->mask == NULL) ? 1 : 0;
    egui_dim_t pfb_width = self->pfb_region.size.width;
    egui_dim_t pfb_ofs_x = self->pfb_location_in_base_view.x;
    egui_dim_t pfb_ofs_y = self->pfb_location_in_base_view.y;
    egui_alpha_t canvas_alpha = self->alpha;

    uint32_t warm_o_dx_max = 0; /* NR warm start for outer isqrt */
    uint32_t warm_i_dx_max = 0; /* NR warm start for inner isqrt */

    for (egui_dim_t dy = y_start; dy <= y_end; dy++)
    {
        egui_dim_t abs_dy = EGUI_ABS(dy);
        egui_dim_t abs_y = center_y + dy;

        // Compute outer ellipse x-boundary for this row
        int64_t o_dy_term = (int64_t)abs_dy * abs_dy * orx_sq;
        if (o_dy_term > orxry_sq)
        {
            continue;
        }
        int32_t o_dx_max = (int32_t)ellipse_isqrt_warm((uint32_t)((orxry_sq - o_dy_term) / ory_sq), warm_o_dx_max);
        warm_o_dx_max = (uint32_t)o_dx_max;

        // Extend outer scan range for smooth AA at poles
        int32_t o_dx_scan = o_dx_max;
        if (abs_dy > 0)
        {
            int32_t abs_dy_adj = abs_dy - 1;
            int64_t o_dy_term_adj = (int64_t)abs_dy_adj * abs_dy_adj * orx_sq;
            if (o_dy_term_adj < orxry_sq)
            {
                int32_t o_dx_adj = (int32_t)ellipse_isqrt_warm((uint32_t)((orxry_sq - o_dy_term_adj) / ory_sq), warm_o_dx_max);
                if (o_dx_adj > o_dx_scan)
                {
                    o_dx_scan = o_dx_adj;
                }
            }
        }

        // Compute inner ellipse x-boundary for this row
        int32_t i_dx_max = 0;
        if (irx > 0 && iry > 0 && abs_dy < iry)
        {
            int64_t i_dy_term = (int64_t)abs_dy * abs_dy * irx_sq;
            if (i_dy_term < irxry_sq)
            {
                i_dx_max = (int32_t)ellipse_isqrt_warm((uint32_t)((irxry_sq - i_dy_term) / iry_sq), warm_i_dx_max);
                warm_i_dx_max = (uint32_t)i_dx_max;
            }
        }

        // Precompute inv_grad for outer ellipse at boundary (AMPM approximation)
        int64_t o_inv_grad_q24 = (int64_t)128 << 24;
        if (o_dx_max > 0)
        {
            int64_t ogx = (int64_t)o_dx_max * ory_sq;
            int64_t ogy = (int64_t)abs_dy * orx_sq;
            o_inv_grad_q24 = ellipse_inv_grad_from_approx(ogx, ogy);
        }

        // Precompute inv_grad for inner ellipse at boundary (AMPM approximation)
        int64_t i_inv_grad_q24 = (int64_t)128 << 24;
        if (i_dx_max > 0 && irx > 0 && iry > 0)
        {
            int64_t igx = (int64_t)i_dx_max * iry_sq;
            int64_t igy = (int64_t)abs_dy * irx_sq;
            i_inv_grad_q24 = ellipse_inv_grad_from_approx(igx, igy);
        }

        // Only iterate over the stroke band: [-(o_dx_scan+1), -(i_dx_max-1)] and [(i_dx_max-1), (o_dx_scan+1)]
        egui_dim_t outer_left = EGUI_MAX(-(egui_dim_t)(o_dx_scan + 1), x_start);
        egui_dim_t outer_right = EGUI_MIN((egui_dim_t)(o_dx_scan + 1), x_end);
        egui_dim_t inner_skip_left = (i_dx_max > 1) ? -(egui_dim_t)(i_dx_max - 1) : 0;
        egui_dim_t inner_skip_right = (i_dx_max > 1) ? (egui_dim_t)(i_dx_max - 1) : 0;

        if (use_direct_pfb)
        {
            egui_color_t *dst_base = (egui_color_t *)&self->pfb[(abs_y - pfb_ofs_y) * pfb_width - pfb_ofs_x];
            egui_alpha_t eff_alpha = egui_color_alpha_mix(canvas_alpha, alpha);

            // Left band
            egui_dim_t left_end = EGUI_MIN(inner_skip_left - 1, outer_right);
            for (egui_dim_t dx = outer_left; dx <= left_end; dx++)
            {
                egui_dim_t abs_dx = EGUI_ABS(dx);

                egui_alpha_t outer_cov = ellipse_get_edge_alpha_fast(abs_dx, abs_dy, orx_sq, ory_sq, orxry_sq, o_inv_grad_q24);
                if (outer_cov == 0)
                {
                    continue;
                }

                egui_alpha_t inner_cov = 0;
                if (irx > 0 && iry > 0 && abs_dx <= irx && abs_dy <= iry)
                {
                    inner_cov = ellipse_get_edge_alpha_fast(abs_dx, abs_dy, irx_sq, iry_sq, irxry_sq, i_inv_grad_q24);
                }

                if (outer_cov > inner_cov)
                {
                    egui_alpha_t band_cov = outer_cov - inner_cov;
                    egui_alpha_t m = egui_color_alpha_mix(eff_alpha, band_cov);
                    if (m > 0)
                    {
                        egui_color_t *back_color = &dst_base[center_x + dx];
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

            // Right band
            egui_dim_t right_start = EGUI_MAX(inner_skip_right + 1, outer_left);
            if (i_dx_max <= 1)
            {
                right_start = EGUI_MAX(left_end + 1, outer_left);
            }
            for (egui_dim_t dx = right_start; dx <= outer_right; dx++)
            {
                egui_dim_t abs_dx = EGUI_ABS(dx);

                egui_alpha_t outer_cov = ellipse_get_edge_alpha_fast(abs_dx, abs_dy, orx_sq, ory_sq, orxry_sq, o_inv_grad_q24);
                if (outer_cov == 0)
                {
                    continue;
                }

                egui_alpha_t inner_cov = 0;
                if (irx > 0 && iry > 0 && abs_dx <= irx && abs_dy <= iry)
                {
                    inner_cov = ellipse_get_edge_alpha_fast(abs_dx, abs_dy, irx_sq, iry_sq, irxry_sq, i_inv_grad_q24);
                }

                if (outer_cov > inner_cov)
                {
                    egui_alpha_t band_cov = outer_cov - inner_cov;
                    egui_alpha_t m = egui_color_alpha_mix(eff_alpha, band_cov);
                    if (m > 0)
                    {
                        egui_color_t *back_color = &dst_base[center_x + dx];
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
            // Fallback: with mask, use original draw_point path but still use fast alpha
            // Left band
            egui_dim_t left_end = EGUI_MIN(inner_skip_left - 1, outer_right);
            for (egui_dim_t dx = outer_left; dx <= left_end; dx++)
            {
                egui_dim_t abs_dx = EGUI_ABS(dx);

                egui_alpha_t outer_cov = ellipse_get_edge_alpha_fast(abs_dx, abs_dy, orx_sq, ory_sq, orxry_sq, o_inv_grad_q24);
                if (outer_cov == 0)
                {
                    continue;
                }

                egui_alpha_t inner_cov = 0;
                if (irx > 0 && iry > 0 && abs_dx <= irx && abs_dy <= iry)
                {
                    inner_cov = ellipse_get_edge_alpha_fast(abs_dx, abs_dy, irx_sq, iry_sq, irxry_sq, i_inv_grad_q24);
                }

                if (outer_cov > inner_cov)
                {
                    egui_alpha_t band_cov = outer_cov - inner_cov;
                    egui_alpha_t mix = egui_color_alpha_mix(alpha, band_cov);
                    if (mix > 0)
                    {
                        egui_canvas_draw_point(center_x + dx, abs_y, color, mix);
                    }
                }
            }

            // Right band
            egui_dim_t right_start = EGUI_MAX(inner_skip_right + 1, outer_left);
            if (i_dx_max <= 1)
            {
                right_start = EGUI_MAX(left_end + 1, outer_left);
            }
            for (egui_dim_t dx = right_start; dx <= outer_right; dx++)
            {
                egui_dim_t abs_dx = EGUI_ABS(dx);

                egui_alpha_t outer_cov = ellipse_get_edge_alpha_fast(abs_dx, abs_dy, orx_sq, ory_sq, orxry_sq, o_inv_grad_q24);
                if (outer_cov == 0)
                {
                    continue;
                }

                egui_alpha_t inner_cov = 0;
                if (irx > 0 && iry > 0 && abs_dx <= irx && abs_dy <= iry)
                {
                    inner_cov = ellipse_get_edge_alpha_fast(abs_dx, abs_dy, irx_sq, iry_sq, irxry_sq, i_inv_grad_q24);
                }

                if (outer_cov > inner_cov)
                {
                    egui_alpha_t band_cov = outer_cov - inner_cov;
                    egui_alpha_t mix = egui_color_alpha_mix(alpha, band_cov);
                    if (mix > 0)
                    {
                        egui_canvas_draw_point(center_x + dx, abs_y, color, mix);
                    }
                }
            }
        }
    }
}
