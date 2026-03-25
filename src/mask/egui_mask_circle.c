#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_mask_circle.h"
#include "core/egui_common.h"
#include "core/egui_api.h"
#include "core/egui_canvas.h"

#if EGUI_CONFIG_FUNCTION_SUPPORT_MASK

extern const egui_circle_info_t egui_res_circle_info_arr[];

__EGUI_STATIC_INLINE__ void egui_mask_circle_invalidate_row_cache(egui_mask_circle_t *local)
{
    for (egui_dim_t i = 0; i < EGUI_CONFIG_PFB_HEIGHT; i++)
    {
        local->row_cache_y[i] = -32768;
    }
}

__EGUI_STATIC_INLINE__ egui_dim_t egui_mask_circle_get_row_cache_slot(egui_dim_t y)
{
    return ((uint16_t)y) % EGUI_CONFIG_PFB_HEIGHT;
}

__EGUI_STATIC_INLINE__ int egui_mask_circle_get_cached_row(egui_mask_circle_t *local, egui_dim_t y, egui_dim_t *visible_half, egui_dim_t *opaque_boundary)
{
    egui_dim_t slot = egui_mask_circle_get_row_cache_slot(y);

    if (local->row_cache_y[slot] != y)
    {
        return 0;
    }

    if (visible_half != NULL)
    {
        *visible_half = local->row_cache_visible_half[slot];
    }

    if (opaque_boundary != NULL)
    {
        *opaque_boundary = local->row_cache_opaque_boundary[slot];
    }

    return 1;
}

__EGUI_STATIC_INLINE__ void egui_mask_circle_store_row(egui_mask_circle_t *local, egui_dim_t y, egui_dim_t visible_half, egui_dim_t opaque_boundary)
{
    egui_dim_t slot = egui_mask_circle_get_row_cache_slot(y);

    local->row_cache_y[slot] = y;
    local->row_cache_visible_half[slot] = visible_half;
    local->row_cache_opaque_boundary[slot] = opaque_boundary;
}

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
    local->visible_radius_sq = (uint32_t)(local->radius + 1) * (uint32_t)(local->radius + 1);
    local->info = egui_canvas_get_circle_item(local->radius);
    local->visible_cached_dy = -1;
    local->visible_cached_half = 0;
    local->opaque_cached_row_index = -1;
    local->opaque_cached_boundary = 0;
    local->point_cached_y = -32768;
    local->point_cached_row_index = 0;
    local->point_cached_row_valid = 0;
    egui_mask_circle_invalidate_row_cache(local);
}

#define EGUI_MASK_CIRCLE_AA_HALF_256 192
#define EGUI_MASK_CIRCLE_ROW_EDGE_256 183

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

__EGUI_STATIC_INLINE__ uint32_t egui_mask_circle_get_edge_diff_threshold(egui_dim_t radius)
{
    int32_t numerator = EGUI_MASK_CIRCLE_ROW_EDGE_256 * (int32_t)radius - (radius >> 1);

    return (uint32_t)((numerator + 127) >> 7);
}

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
    local->visible_radius_sq = 0;
    local->visible_cached_dy = -1;
    local->visible_cached_half = 0;
    local->opaque_cached_row_index = -1;
    local->opaque_cached_boundary = 0;
    local->point_cached_y = -32768;
    local->point_cached_row_index = 0;
    local->point_cached_row_valid = 0;
    egui_mask_circle_invalidate_row_cache(local);
    local->info = NULL;
}

#endif
