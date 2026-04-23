#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_mask_circle.h"
#include "core/egui_common.h"
#include "core/egui_api.h"
#include "canvas/egui_canvas.h"

#if EGUI_CONFIG_FUNCTION_SUPPORT_MASK

extern const egui_circle_info_t egui_res_circle_info_arr[];

/**
 * @brief Invalidate the optional per-row cache used by scanline helpers.
 *
 * The current circle mask keeps only a tiny point cache, so there is nothing

 * * to clear here. The hook stays in place because the row-based API is a good
 * extension point for future caching experiments.
 */
__EGUI_STATIC_INLINE__ void egui_mask_circle_invalidate_row_cache(egui_mask_circle_t *local)
{
    (void)local;
}

/**
 * @brief Try to reuse previously computed scanline metrics for one absolute row.
 *
 * Returning 0 tells the caller to recompute the row geometry. The
 * signature is
 * intentionally shaped like a real cache lookup so higher-level code does not
 * need to change if a cache is added later.
 */
__EGUI_STATIC_INLINE__ int egui_mask_circle_get_cached_row(egui_mask_circle_t *local, egui_dim_t y, egui_dim_t *visible_half, egui_dim_t *opaque_boundary)
{
    (void)local;
    (void)y;
    (void)visible_half;
    (void)opaque_boundary;
    return 0;
}

/**
 * @brief Store scanline metrics for later reuse.
 *
 * This is a no-op in the lightweight implementation, but keeping the helper
 * makes the scanline
 * pipeline easier to read: lookup, compute, then store.
 */
__EGUI_STATIC_INLINE__ void egui_mask_circle_store_row(egui_mask_circle_t *local, egui_dim_t y, egui_dim_t visible_half, egui_dim_t opaque_boundary)
{
    (void)local;
    (void)y;
    (void)visible_half;
    (void)opaque_boundary;
}

/**
 * @brief Refresh cached circle geometry after the mask region changes.
 *
 * The public mask API stores only a rectangle. This helper converts that
 *
 * rectangle into the circle data used by the rasterizer: bounding box, center,
 * radius, and quick point-query state.
 */
__EGUI_STATIC_INLINE__ void egui_mask_circle_refresh_cache(egui_mask_t *self)
{
    egui_mask_circle_t *local = (egui_mask_circle_t *)self;

    if (local->cached_x == self->region.location.x && local->cached_y == self->region.location.y && local->cached_width == self->region.size.width &&
        local->cached_height == self->region.size.height)
    {
        return;
    }

    local->cached_x = self->region.location.x;
    local->cached_y = self->region.location.y;
    local->cached_width = self->region.size.width;
    local->cached_height = self->region.size.height;
    local->cached_x_end = self->region.location.x + self->region.size.width;
    local->cached_y_end = self->region.location.y + self->region.size.height;
    local->center_x = self->region.location.x + (self->region.size.width >> 1);
    local->center_y = self->region.location.y + (self->region.size.height >> 1);
    local->radius = EGUI_MIN(self->region.size.width, self->region.size.height);
    local->radius = (local->radius >> 1) - 1;
    if (local->radius < 0)
    {
        local->radius = 0;
    }
    /* Basic-range lookup only; spec circle info is canvas-specific and not
       available inside the mask layer, so we index the global array directly. */
    if (local->radius < EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE)
    {
        const egui_circle_info_t *info = &egui_res_circle_info_arr[local->radius];
        local->info = (info->radius == (uint16_t)local->radius) ? info : NULL;
    }
    else
    {
        local->info = NULL;
    }
    local->point_cached_y = -32768;
    local->point_cached_row_index = 0;
    local->point_cached_row_valid = 0;
    egui_mask_circle_invalidate_row_cache(local);
}

#define EGUI_MASK_CIRCLE_AA_HALF_256  192
#define EGUI_MASK_CIRCLE_ROW_EDGE_256 183

/**
 * @brief Compute floor(sqrt(n)) without using floating point.
 *
 * The mask code works on squared distances so it can stay in integer math.
 * When a
 * scanline needs a horizontal half-width, this helper converts the
 * squared form back into a pixel-space radius.
 */
__EGUI_STATIC_INLINE__ uint32_t egui_mask_circle_isqrt(uint32_t n)
{
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
 * @brief Convert the AA edge width into a squared-distance threshold.
 *
 * Row metrics are derived in distance-squared space to avoid repeated square
 *
 * roots. This threshold marks how far we extend the visible fringe and how far
 * we shrink the fully opaque core for one-pixel anti-aliasing.
 */
__EGUI_STATIC_INLINE__ uint32_t egui_mask_circle_get_edge_diff_threshold(egui_dim_t radius)
{
    int32_t numerator = EGUI_MASK_CIRCLE_ROW_EDGE_256 * (int32_t)radius - (radius >> 1);

    return (uint32_t)((numerator + 127) >> 7);
}

/**
 * @brief Map a signed edge distance to alpha with a smoothstep curve.
 *
 * `signed_dist_256` is measured in 1/256 pixel units. Negative values are
 *
 * inside the circle, positive values are outside. Smoothstep gives a softer
 * transition than a straight linear ramp, so small circles look less jagged.
 */
__EGUI_STATIC_INLINE__ egui_alpha_t egui_mask_circle_edge_smoothstep(int32_t signed_dist_256)
{
    int32_t coverage;
    int32_t t;
    int32_t t_sq;
    int32_t smooth;
    int32_t alpha_range_sq = EGUI_ALPHA_100 * EGUI_ALPHA_100;

    if (signed_dist_256 <= -EGUI_MASK_CIRCLE_AA_HALF_256)
    {
        return EGUI_ALPHA_100;
    }

    if (signed_dist_256 >= EGUI_MASK_CIRCLE_AA_HALF_256)
    {
        return EGUI_ALPHA_0;
    }

    coverage = EGUI_MASK_CIRCLE_AA_HALF_256 - signed_dist_256;
    t = (coverage * EGUI_ALPHA_100 + EGUI_MASK_CIRCLE_AA_HALF_256) / (EGUI_MASK_CIRCLE_AA_HALF_256 << 1);
    t_sq = t * t;
    smooth = (3 * t_sq * EGUI_ALPHA_100 - 2 * t_sq * t + (alpha_range_sq >> 1)) / alpha_range_sq;

    if (smooth <= 0)
    {
        return EGUI_ALPHA_0;
    }

    if (smooth >= EGUI_ALPHA_100)
    {
        return EGUI_ALPHA_100;
    }

    return (egui_alpha_t)smooth;
}

/**
 * @brief Sample one quarter-circle pixel and return its coverage alpha.
 *
 * The row/column indices live in a canonical corner space where `(radius,
 *
 * radius)` is the circle center. The rest of the rasterizer mirrors this
 * quarter-circle result to all four quadrants of the full mask.
 */
egui_alpha_t egui_mask_circle_get_corner_alpha(egui_dim_t radius, egui_dim_t row_index, egui_dim_t col_index)
{
    if (radius <= 0)
    {
        return (row_index == 0 && col_index == 0) ? EGUI_ALPHA_100 : EGUI_ALPHA_0;
    }

    if (row_index < 0 || col_index < 0 || row_index > radius || col_index > radius)
    {
        return EGUI_ALPHA_0;
    }

    {
        int32_t dx = (int32_t)radius - (int32_t)col_index;
        int32_t dy = (int32_t)radius - (int32_t)row_index;
        int32_t radius_sq = (int32_t)radius * (int32_t)radius;
        int32_t dist_sq = dx * dx + dy * dy;
        int32_t inner_radius = (radius > 0) ? (radius - 1) : 0;
        int32_t inner_radius_sq = inner_radius * inner_radius;
        int32_t outer_radius = radius + 1;
        int32_t outer_radius_sq = outer_radius * outer_radius;
        int32_t diff;
        int32_t signed_dist_256;

        if (dist_sq <= inner_radius_sq)
        {
            return EGUI_ALPHA_100;
        }

        if (dist_sq >= outer_radius_sq)
        {
            return EGUI_ALPHA_0;
        }

        diff = dist_sq - radius_sq;
        if (diff >= 0)
        {
            signed_dist_256 = (diff * 128 + (radius >> 1)) / radius;
        }
        else
        {
            signed_dist_256 = -(((-diff) * 128 + (radius >> 1)) / radius);
        }

        return egui_mask_circle_edge_smoothstep(signed_dist_256);
    }
}

/**
 * @brief Derive scanline geometry for one symmetric row of the circle.
 *
 * `visible_half` tells how far the row reaches from the center before alpha
 *
 * drops to zero. `opaque_boundary` marks where the fully opaque middle band
 * starts, letting callers treat the row as:
 *   AA edge | fully opaque center |
 * AA edge
 */
void egui_mask_circle_get_row_metrics(egui_dim_t radius, egui_dim_t row_index, egui_dim_t *visible_half, egui_dim_t *opaque_boundary)
{
    egui_dim_t first_visible;
    egui_dim_t first_opaque;
    uint32_t radius_sq;
    uint32_t dy_sq;
    uint32_t edge_diff_threshold;
    uint32_t visible_limit_sq;
    int32_t opaque_limit_sq;

    if (visible_half == NULL || opaque_boundary == NULL)
    {
        return;
    }

    if (radius <= 0)
    {
        *visible_half = 0;
        *opaque_boundary = 0;
        return;
    }

    if (radius == 1)
    {
        first_visible = radius + 1;
        first_opaque = radius + 1;

        for (egui_dim_t col_index = 0; col_index <= radius; col_index++)
        {
            egui_alpha_t alpha = egui_mask_circle_get_corner_alpha(radius, row_index, col_index);

            if (first_visible > radius && alpha != EGUI_ALPHA_0)
            {
                first_visible = col_index;
            }

            if (first_opaque > radius && alpha == EGUI_ALPHA_100)
            {
                first_opaque = col_index;
                break;
            }
        }

        if (first_visible > radius)
        {
            *visible_half = -1;
            *opaque_boundary = radius + 1;
            return;
        }

        *visible_half = radius - first_visible;
        *opaque_boundary = first_opaque;
        return;
    }

    if (row_index < 0 || row_index > radius)
    {
        *visible_half = -1;
        *opaque_boundary = radius + 1;
        return;
    }

    radius_sq = (uint32_t)radius * (uint32_t)radius;
    dy_sq = (uint32_t)(radius - row_index) * (uint32_t)(radius - row_index);
    edge_diff_threshold = egui_mask_circle_get_edge_diff_threshold(radius);
    visible_limit_sq = radius_sq + edge_diff_threshold - 1U;
    opaque_limit_sq = (int32_t)radius_sq - (int32_t)edge_diff_threshold;

    if (dy_sq > visible_limit_sq)
    {
        *visible_half = -1;
        *opaque_boundary = radius + 1;
        return;
    }

    *visible_half = (egui_dim_t)egui_mask_circle_isqrt(visible_limit_sq - dy_sq);

    if (opaque_limit_sq < 0 || dy_sq > (uint32_t)opaque_limit_sq)
    {
        *opaque_boundary = radius + 1;
        return;
    }

    *opaque_boundary = radius - (egui_dim_t)egui_mask_circle_isqrt((uint32_t)opaque_limit_sq - dy_sq);
}

/**
 * @brief Prepare cached metrics for one absolute y coordinate.
 *
 * The helper first maps `y` into the symmetric row index used by the corner
 * sampler,
 * then fetches or computes the visible and opaque spans for that row.
 * Most row-oriented APIs call this once and reuse the results for all pixels.
 */
int egui_mask_circle_prepare_row(egui_mask_circle_t *local, egui_dim_t y, egui_dim_t *row_index, egui_dim_t *visible_half, egui_dim_t *opaque_boundary)
{
    egui_mask_t *self = (egui_mask_t *)local;
    egui_dim_t current_row_index;
    egui_dim_t current_visible_half;
    egui_dim_t current_opaque_boundary;
    egui_dim_t dy;

    egui_mask_circle_refresh_cache(self);

    if (y < local->center_y - local->radius || y > local->center_y + local->radius)
    {
        local->point_cached_y = y;
        local->point_cached_row_valid = 0;
        return 0;
    }

    dy = (y < local->center_y) ? (local->center_y - y) : (y - local->center_y);
    current_row_index = local->radius - dy;

    local->point_cached_y = y;
    local->point_cached_row_index = current_row_index;
    local->point_cached_row_valid = 1;

    if (!egui_mask_circle_get_cached_row(local, y, &current_visible_half, &current_opaque_boundary))
    {
        egui_mask_circle_get_row_metrics(local->radius, current_row_index, &current_visible_half, &current_opaque_boundary);
        egui_mask_circle_store_row(local, y, current_visible_half, current_opaque_boundary);
    }

    if (row_index != NULL)
    {
        *row_index = current_row_index;
    }

    if (visible_half != NULL)
    {
        *visible_half = current_visible_half;
    }

    if (opaque_boundary != NULL)
    {
        *opaque_boundary = current_opaque_boundary;
    }

    return 1;
}

/**
 * @brief Release frame-level resources associated with circle masks.
 *
 * The current implementation keeps no external frame cache, but the function is
 *
 * kept so the mask module can share one lifecycle contract across mask types.
 */
void egui_mask_circle_release_frame_cache(egui_core_t *core)
{
    EGUI_UNUSED(core);
}

/**
 * @brief Blend a contiguous fully covered span with one solid color.
 *
 * This is the fast path used for the opaque center of a scanline. Edge pixels
 *
 * still go through per-pixel alpha sampling, but the middle segment can be
 * filled or blended in one tight loop.
 */
static void egui_mask_circle_blend_solid_row(egui_color_int_t *dst, egui_dim_t count, egui_color_t color, egui_alpha_t alpha)
{
    if (count <= 0 || alpha == 0)
    {
        return;
    }

    if (alpha == EGUI_ALPHA_100)
    {
        egui_canvas_fill_color_buffer(dst, count, color);
        return;
    }

    for (egui_dim_t i = 0; i < count; i++)
    {
        egui_rgb_mix_ptr((egui_color_t *)&dst[i], &color, (egui_color_t *)&dst[i], alpha);
    }
}

/**
 * @brief Apply the circle mask to a single point sample.
 *
 * This is the generic mask entry used by slow paths that need per-pixel
 * answers. It reuses
 * a tiny cache for the last queried row because many callers
 * walk one scanline from left to right.
 */
void egui_mask_circle_mask_point(egui_mask_t *self, egui_dim_t x, egui_dim_t y, egui_color_t *color, egui_alpha_t *alpha)
{
    egui_mask_circle_t *local = (egui_mask_circle_t *)self;
    egui_mask_circle_refresh_cache(self);

    if (x >= local->cached_x && x < local->cached_x_end && y >= local->cached_y && y < local->cached_y_end)
    {
        egui_dim_t row_index;

        if (y == local->point_cached_y)
        {
            if (!local->point_cached_row_valid)
            {
                color->full = 0;
                *alpha = 0;
                return;
            }

            row_index = local->point_cached_row_index;
        }
        else
        {
            egui_dim_t dy = (y > local->center_y) ? (y - local->center_y) : (local->center_y - y);
            if (dy > local->radius)
            {
                local->point_cached_y = y;
                local->point_cached_row_valid = 0;
                color->full = 0;
                *alpha = 0;
                return;
            }

            row_index = local->radius - dy;
            local->point_cached_y = y;
            local->point_cached_row_index = row_index;
            local->point_cached_row_valid = 1;
        }

        egui_dim_t dx = (x > local->center_x) ? (x - local->center_x) : (local->center_x - x);
        if (dx <= local->radius)
        {
            egui_alpha_t mix_alpha = egui_mask_circle_get_corner_alpha(local->radius, row_index, local->radius - dx);
            if (mix_alpha != 0)
            {
                *alpha = egui_color_alpha_mix(mix_alpha, *alpha);
                return;
            }
        }
    }

    color->full = 0;
    *alpha = 0;
}

/**
 * @brief Report the fully opaque horizontal band for one row.
 *
 * Callers use this to skip needless per-pixel work in the middle of the row.
 * Even when
 * the opaque band is empty, the function can still return PARTIAL to
 * signal that anti-aliased edge pixels exist inside the requested range.
 */
int egui_mask_circle_get_row_range(egui_mask_t *self, egui_dim_t y, egui_dim_t x_min, egui_dim_t x_max, egui_dim_t *x_start, egui_dim_t *x_end)
{
    egui_mask_circle_t *local = (egui_mask_circle_t *)self;
    egui_dim_t center_x;
    egui_dim_t radius;
    egui_dim_t opaque_x_start;
    egui_dim_t opaque_x_end;
    egui_dim_t opaque_boundary;

    if (!egui_mask_circle_prepare_row(local, y, NULL, NULL, &opaque_boundary))
    {
        return EGUI_MASK_ROW_OUTSIDE;
    }

    center_x = local->center_x;
    radius = local->radius;
    opaque_x_start = center_x - radius + opaque_boundary;
    opaque_x_end = center_x + radius - opaque_boundary + 1;

    *x_start = EGUI_MAX(opaque_x_start, x_min);
    *x_end = EGUI_MIN(opaque_x_end, x_max);

    if (*x_start >= *x_end)
    {
        // Opaque band doesn't overlap with requested range, but the row IS within
        // the circle's vertical extent, so AA edge pixels may still exist in range.
        // Return PARTIAL with empty band so all pixels go through per-pixel mask.
        *x_start = x_min;
        *x_end = x_min;
        return EGUI_MASK_ROW_PARTIAL;
    }

    if (*x_start <= x_min && *x_end >= x_max)
    {
        return EGUI_MASK_ROW_INSIDE;
    }

    return EGUI_MASK_ROW_PARTIAL;
}

/**
 * @brief Paint one scanline segment through the circle mask.
 *
 * The implementation mirrors the geometry returned by
 *
 * `egui_mask_circle_get_row_metrics`: blend the left AA fringe, fill the solid
 * middle span quickly, then blend the right AA fringe.
 */
int egui_mask_circle_fill_row_segment(egui_mask_t *self, egui_color_int_t *dst, egui_dim_t y, egui_dim_t x_start, egui_dim_t x_end, egui_color_t color,
                                      egui_alpha_t alpha)
{
    egui_mask_circle_t *local = (egui_mask_circle_t *)self;
    egui_dim_t center_x;
    egui_dim_t radius;
    egui_dim_t row_index;
    egui_dim_t visible_half;
    egui_dim_t opaque_boundary;
    egui_dim_t seg_start;
    egui_dim_t seg_end;
    egui_dim_t opaque_x_start;
    egui_dim_t opaque_x_end;

    if (x_start >= x_end)
    {
        return 1;
    }

    if (!egui_mask_circle_prepare_row(local, y, &row_index, &visible_half, &opaque_boundary))
    {
        return 1;
    }

    center_x = local->center_x;
    radius = local->radius;

    seg_start = EGUI_MAX(x_start, center_x - visible_half);
    seg_end = EGUI_MIN(x_end, center_x + visible_half + 1);
    if (seg_start >= seg_end)
    {
        return 1;
    }

    opaque_x_start = center_x - radius + opaque_boundary;
    opaque_x_end = center_x + radius - opaque_boundary + 1;

    if (seg_start < opaque_x_start)
    {
        egui_dim_t left_end = EGUI_MIN(seg_end, opaque_x_start);
        egui_color_int_t *left_dst = dst + (seg_start - x_start);
        egui_dim_t corner_col = seg_start - (center_x - radius);

        for (egui_dim_t xp = seg_start; xp < left_end; xp++, left_dst++, corner_col++)
        {
            egui_alpha_t pixel_alpha;

            if (corner_col < 0 || corner_col > radius)
            {
                continue;
            }

            pixel_alpha = egui_mask_circle_get_corner_alpha(radius, row_index, corner_col);
            if (alpha != EGUI_ALPHA_100)
            {
                pixel_alpha = egui_color_alpha_mix(pixel_alpha, alpha);
            }

            if (pixel_alpha == 0)
            {
                continue;
            }

            if (pixel_alpha == EGUI_ALPHA_100)
            {
                *left_dst = color.full;
            }
            else
            {
                egui_rgb_mix_ptr((egui_color_t *)left_dst, &color, (egui_color_t *)left_dst, pixel_alpha);
            }
        }
    }

    {
        egui_dim_t mid_start = EGUI_MAX(seg_start, opaque_x_start);
        egui_dim_t mid_end = EGUI_MIN(seg_end, opaque_x_end);

        if (mid_start < mid_end)
        {
            egui_mask_circle_blend_solid_row(dst + (mid_start - x_start), mid_end - mid_start, color, alpha);
        }
    }

    if (seg_end > opaque_x_end)
    {
        egui_dim_t right_start = EGUI_MAX(seg_start, opaque_x_end);
        egui_color_int_t *right_dst = dst + (right_start - x_start);
        egui_dim_t corner_col = center_x + radius - right_start;

        for (egui_dim_t xp = right_start; xp < seg_end; xp++, right_dst++, corner_col--)
        {
            egui_alpha_t pixel_alpha;

            if (corner_col < 0 || corner_col > radius)
            {
                continue;
            }

            pixel_alpha = egui_mask_circle_get_corner_alpha(radius, row_index, corner_col);
            if (alpha != EGUI_ALPHA_100)
            {
                pixel_alpha = egui_color_alpha_mix(pixel_alpha, alpha);
            }

            if (pixel_alpha == 0)
            {
                continue;
            }

            if (pixel_alpha == EGUI_ALPHA_100)
            {
                *right_dst = color.full;
            }
            else
            {
                egui_rgb_mix_ptr((egui_color_t *)right_dst, &color, (egui_color_t *)right_dst, pixel_alpha);
            }
        }
    }

    return 1;
}

/**
 * @brief Return the broad visible range of a row, including AA-only pixels.
 *
 * This range is wider than the opaque band and is useful for clipping
 * before a
 * caller decides whether it needs the slower per-pixel edge sampling.
 */
static int egui_mask_circle_get_row_visible_range(egui_mask_t *self, egui_dim_t y, egui_dim_t x_min, egui_dim_t x_max, egui_dim_t *x_start, egui_dim_t *x_end)
{
    egui_mask_circle_t *local = (egui_mask_circle_t *)self;
    egui_dim_t center_x;
    egui_dim_t visible_half;

    if (!egui_mask_circle_prepare_row(local, y, NULL, &visible_half, NULL))
    {
        return 0;
    }

    center_x = local->center_x;

    egui_dim_t visible_x_start = center_x - visible_half;
    egui_dim_t visible_x_end = center_x + visible_half + 1;

    *x_start = EGUI_MAX(visible_x_start, x_min);
    *x_end = EGUI_MIN(visible_x_end, x_max);

    return (*x_start < *x_end);
}

// name must be _type##_api_table, it will be used by EGUI_VIEW_DEFINE to init api.
const egui_mask_api_t egui_mask_circle_t_api_table = {
        .kind = EGUI_MASK_KIND_CIRCLE,
        .mask_point = egui_mask_circle_mask_point,
        .mask_get_row_range = egui_mask_circle_get_row_range,
        .mask_get_row_visible_range = egui_mask_circle_get_row_visible_range,
        .mask_blend_row_color = NULL,
        .mask_get_row_overlay = NULL,
};

/**
 * @brief Initialize the circle mask and reset all cached geometry.
 *
 * The actual circle is derived later from the mask region, so init only needs
 * to
 * wire the API table and put every cache field into a known empty state.
 */
void egui_mask_circle_init(egui_mask_t *self)
{
    EGUI_LOCAL_INIT(egui_mask_circle_t);
    // call super init.
    egui_mask_init(self);
    // update api.
    self->api = &egui_mask_circle_t_api_table;

    // init local data.
    local->cached_x = -1;
    local->cached_y = -1;
    local->cached_width = -1;
    local->cached_height = -1;
    local->cached_x_end = -1;
    local->cached_y_end = -1;
    local->center_x = 0;
    local->center_y = 0;
    local->radius = 0;
    local->point_cached_y = -32768;
    local->point_cached_row_index = 0;
    local->point_cached_row_valid = 0;
    local->info = NULL;
}

#else

/**
 * @brief Fallback stub when mask support is compiled out.
 */
egui_alpha_t egui_mask_circle_get_corner_alpha(egui_dim_t radius, egui_dim_t row_index, egui_dim_t col_index)
{
    EGUI_UNUSED(radius);
    EGUI_UNUSED(row_index);
    EGUI_UNUSED(col_index);
    return EGUI_ALPHA_0;
}

/**
 * @brief Fallback stub that reports no visible or opaque span.
 */
void egui_mask_circle_get_row_metrics(egui_dim_t radius, egui_dim_t row_index, egui_dim_t *visible_half, egui_dim_t *opaque_boundary)
{
    EGUI_UNUSED(radius);
    EGUI_UNUSED(row_index);

    if (visible_half != NULL)
    {
        *visible_half = -1;
    }

    if (opaque_boundary != NULL)
    {
        *opaque_boundary = 0;
    }
}

/**
 * @brief Fallback stub that always reports the row as outside the mask.
 */
int egui_mask_circle_prepare_row(egui_mask_circle_t *local, egui_dim_t y, egui_dim_t *row_index, egui_dim_t *visible_half, egui_dim_t *opaque_boundary)
{
    EGUI_UNUSED(local);
    EGUI_UNUSED(y);

    if (row_index != NULL)
    {
        *row_index = 0;
    }

    if (visible_half != NULL)
    {
        *visible_half = -1;
    }

    if (opaque_boundary != NULL)
    {
        *opaque_boundary = 0;
    }

    return 0;
}

/**
 * @brief Fallback stub kept for a consistent public API.
 */
void egui_mask_circle_release_frame_cache(egui_core_t *core)
{
    EGUI_UNUSED(core);
}

#endif
