#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_mask_circle.h"
#include "core/egui_common.h"
#include "core/egui_api.h"
#include "core/egui_canvas.h"

extern const egui_circle_info_t egui_res_circle_info_arr[];

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
    local->point_cached_y = -32768;
    local->point_cached_row_index = 0;
    local->point_cached_row_valid = 0;
}

static uint32_t egui_mask_circle_isqrt(uint32_t n)
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

static egui_dim_t egui_mask_circle_get_visible_half(egui_mask_circle_t *local, egui_dim_t dy)
{
    if (dy == local->visible_cached_dy)
    {
        return local->visible_cached_half;
    }

    uint32_t dy_sq = (uint32_t)dy * (uint32_t)dy;
    egui_dim_t half = local->visible_cached_half;

    if (local->visible_cached_dy >= 0)
    {
        egui_dim_t delta = dy - local->visible_cached_dy;
        if (delta < 0)
        {
            delta = -delta;
        }

        if (delta <= 4)
        {
            while (half > 0 && ((uint32_t)half * (uint32_t)half + dy_sq) > local->visible_radius_sq)
            {
                half--;
            }

            while (((uint32_t)(half + 1) * (uint32_t)(half + 1) + dy_sq) <= local->visible_radius_sq)
            {
                half++;
            }

            local->visible_cached_dy = dy;
            local->visible_cached_half = half;
            return half;
        }
    }

    half = (egui_dim_t)egui_mask_circle_isqrt((dy_sq < local->visible_radius_sq) ? (local->visible_radius_sq - dy_sq) : 0);
    local->visible_cached_dy = dy;
    local->visible_cached_half = half;
    return half;
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
        if (dx <= local->radius && local->info != NULL)
        {
            egui_alpha_t mix_alpha = egui_canvas_get_circle_corner_value(row_index, local->radius - dx, local->info);
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

// Compute the opaque boundary column for a circle corner quadrant at a given row.
// Returns the first column index (in corner coords 0..radius-1) that is guaranteed fully opaque.
static egui_dim_t egui_mask_circle_corner_get_opaque_boundary(egui_dim_t row_in_corner, const egui_circle_info_t *info)
{
    const egui_circle_item_t *items = (const egui_circle_item_t *)info->items;
    egui_dim_t item_count = (egui_dim_t)info->item_count;
    egui_dim_t left_boundary;

    if (row_in_corner < item_count)
    {
        const egui_circle_item_t *item = &items[row_in_corner];
        left_boundary = (egui_dim_t)item->start_offset + (egui_dim_t)item->valid_count;
    }
    else
    {
        left_boundary = row_in_corner;
    }

    egui_dim_t mirror_limit = EGUI_MIN(row_in_corner, item_count);
    if (mirror_limit > 0)
    {
        egui_dim_t low = 0;
        egui_dim_t high = mirror_limit;

        while (low < high)
        {
            egui_dim_t mid = low + ((high - low) >> 1);
            const egui_circle_item_t *item = &items[mid];
            egui_dim_t threshold = (egui_dim_t)item->start_offset + (egui_dim_t)item->valid_count;

            if (row_in_corner >= threshold)
            {
                high = mid;
            }
            else
            {
                low = mid + 1;
            }
        }

        if (low < mirror_limit)
        {
            left_boundary = EGUI_MIN(left_boundary, low);
        }
    }

    return left_boundary;
}

int egui_mask_circle_get_row_range(egui_mask_t *self, egui_dim_t y, egui_dim_t x_min, egui_dim_t x_max, egui_dim_t *x_start, egui_dim_t *x_end)
{
    egui_mask_circle_t *local = (egui_mask_circle_t *)self;
    egui_mask_circle_refresh_cache(self);

    egui_dim_t center_x = local->center_x;
    egui_dim_t center_y = local->center_y;
    egui_dim_t radius = local->radius;

    egui_dim_t opaque_x_start;
    egui_dim_t opaque_x_end;

    // Outside circle vertical extent
    if (y < center_y - radius || y > center_y + radius)
    {
        local->point_cached_y = y;
        local->point_cached_row_valid = 0;
        return EGUI_MASK_ROW_OUTSIDE;
    }

    // Center row: horizontal line
    if (y == center_y)
    {
        local->point_cached_y = y;
        local->point_cached_row_index = radius;
        local->point_cached_row_valid = (local->info != NULL);
        opaque_x_start = center_x - radius;
        opaque_x_end = center_x + radius + 1;
    }
    else
    {
        if (local->info == NULL)
        {
            *x_start = x_min;
            *x_end = x_min;
            return EGUI_MASK_ROW_PARTIAL;
        }

        // Compute distance from center and corner row index
        egui_dim_t dist = (y < center_y) ? (center_y - y) : (y - center_y);
        egui_dim_t row_in_corner = radius - dist; // 0 at outer edge, radius-1 near center

        local->point_cached_y = y;
        local->point_cached_row_index = row_in_corner;
        local->point_cached_row_valid = 1;

        egui_dim_t boundary = egui_mask_circle_corner_get_opaque_boundary(row_in_corner, local->info);

        // Left quadrant opaque: [center_x - radius + boundary, center_x)
        // Center column: center_x (vertical line)
        // Right quadrant opaque: [center_x + 1, center_x + radius - boundary + 1)
        // Combined: [center_x - radius + boundary, center_x + radius - boundary + 1)
        opaque_x_start = center_x - radius + boundary;
        opaque_x_end = center_x + radius - boundary + 1;
    }

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

static int egui_mask_circle_get_row_visible_range(egui_mask_t *self, egui_dim_t y, egui_dim_t x_min, egui_dim_t x_max, egui_dim_t *x_start, egui_dim_t *x_end)
{
    egui_mask_circle_t *local = (egui_mask_circle_t *)self;
    egui_mask_circle_refresh_cache(self);

    egui_dim_t radius = local->radius;
    egui_dim_t center_x = local->center_x;
    egui_dim_t center_y = local->center_y;

    if (y < center_y - radius || y > center_y + radius)
    {
        return 0;
    }

    egui_dim_t dy = (y > center_y) ? (y - center_y) : (center_y - y);
    egui_dim_t visible_half = egui_mask_circle_get_visible_half(local, dy);
    egui_dim_t visible_x_start = center_x - visible_half;
    egui_dim_t visible_x_end = center_x + visible_half + 1;

    *x_start = EGUI_MAX(visible_x_start, x_min);
    *x_end = EGUI_MIN(visible_x_end, x_max);

    return (*x_start < *x_end);
}

// name must be _type##_api_table, it will be used by EGUI_VIEW_DEFINE to init api.
const egui_mask_api_t egui_mask_circle_t_api_table = {
        .mask_point = egui_mask_circle_mask_point,
        .mask_get_row_range = egui_mask_circle_get_row_range,
        .mask_get_row_visible_range = egui_mask_circle_get_row_visible_range,
        .mask_blend_row_color = NULL,
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
    local->point_cached_y = -32768;
    local->point_cached_row_index = 0;
    local->point_cached_row_valid = 0;
    local->info = NULL;
}
