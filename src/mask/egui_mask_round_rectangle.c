#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_mask_round_rectangle.h"
#include "core/egui_common.h"
#include "core/egui_api.h"
#include "core/egui_canvas.h"

extern const egui_circle_info_t egui_res_circle_info_arr[];

void egui_mask_round_rectangle_set_radius(egui_mask_t *self, egui_dim_t radius)
{
    EGUI_LOCAL_INIT(egui_mask_round_rectangle_t);
    local->radius = radius;
}

void egui_mask_round_rectangle_mask_point(egui_mask_t *self, egui_dim_t x, egui_dim_t y, egui_color_t *color, egui_alpha_t *alpha)
{
    EGUI_LOCAL_INIT(egui_mask_round_rectangle_t);
    egui_dim_t radius = local->radius;
    egui_dim_t width = self->region.size.width;
    egui_dim_t height = self->region.size.height;
    egui_dim_t sel_x = self->region.location.x;
    egui_dim_t sel_y = self->region.location.y;

    if (egui_region_pt_in_rect(&self->region, x, y))
    {
        egui_region_t region;
        // check in the middle rectangle.
        egui_region_init(&region, sel_x + radius, sel_y, width - (radius << 1), height);
        if (egui_region_pt_in_rect(&region, x, y))
        {
            return;
        }

        // check in the left and right rectangles.
        egui_region_init(&region, sel_x, sel_y + radius, radius, height - (radius << 1));
        if (egui_region_pt_in_rect(&region, x, y))
        {
            return;
        }

        egui_region_init(&region, sel_x + width - radius, sel_y + radius, radius, height - (radius << 1));
        if (egui_region_pt_in_rect(&region, x, y))
        {
            return;
        }

        // check in the corners.
        if (egui_canvas_get_circle_left_top(sel_x + radius, sel_y + radius, radius, x, y, alpha))
        {
            return;
        }
        if (egui_canvas_get_circle_left_bottom(sel_x + radius, sel_y + height - radius - 1, radius, x, y, alpha))
        {
            return;
        }
        if (egui_canvas_get_circle_right_top(sel_x + width - radius - 1, sel_y + radius, radius, x, y, alpha))
        {
            return;
        }
        if (egui_canvas_get_circle_right_bottom(sel_x + width - radius - 1, sel_y + height - radius - 1, radius, x, y, alpha))
        {
            return;
        }
    }

    // clear value.
    color->full = 0;
    *alpha = 0;
}

// Compute the opaque boundary column for a circle corner quadrant at a given row.
// Returns the first column index (in corner coords 0..radius-1) that is guaranteed fully opaque.
static egui_dim_t egui_mask_circle_corner_get_opaque_boundary(egui_dim_t row_in_corner, const egui_circle_info_t *info)
{
    egui_dim_t left_boundary;
    egui_dim_t item_count = (egui_dim_t)info->item_count;

    // Primary half boundary (col >= row_in_corner): use items[row_in_corner]
    if (row_in_corner < item_count)
    {
        const egui_circle_item_t *item = &((const egui_circle_item_t *)info->items)[row_in_corner];
        left_boundary = (egui_dim_t)item->start_offset + (egui_dim_t)item->valid_count;
    }
    else
    {
        // row deep inside: all cols >= item_count are opaque from primary perspective
        left_boundary = item_count;
    }

    // Mirror half boundary (col < min(row_in_corner, item_count))
    // items[col].start_offset + valid_count is monotonically decreasing as col increases.
    // Scan from high col to low: the first col where row < (so+vc) sets the boundary.
    egui_dim_t mirror_limit = EGUI_MIN(row_in_corner, item_count);
    for (egui_dim_t col = mirror_limit - 1; col >= 0; col--)
    {
        const egui_circle_item_t *item = &((const egui_circle_item_t *)info->items)[col];
        egui_dim_t threshold = (egui_dim_t)item->start_offset + (egui_dim_t)item->valid_count;
        if (row_in_corner >= threshold)
        {
            // This col and all higher cols are opaque; boundary is col
            if (col + 1 > left_boundary)
            {
                left_boundary = col + 1;
            }
            break;
        }
        // row < threshold: this col is NOT fully opaque.
        // Since threshold increases for smaller col, all smaller cols are also NOT opaque.
        left_boundary = EGUI_MAX(left_boundary, col + 1);
        break;
    }

    return left_boundary;
}

static uint32_t egui_mask_round_rectangle_isqrt(uint32_t n)
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

int egui_mask_round_rectangle_get_row_range(egui_mask_t *self, egui_dim_t y, egui_dim_t x_min, egui_dim_t x_max, egui_dim_t *x_start, egui_dim_t *x_end)
{
    EGUI_LOCAL_INIT(egui_mask_round_rectangle_t);
    egui_dim_t radius = local->radius;
    egui_dim_t sel_x = self->region.location.x;
    egui_dim_t sel_y = self->region.location.y;
    egui_dim_t width = self->region.size.width;
    egui_dim_t height = self->region.size.height;

    // Outside mask region vertically
    if (y < sel_y || y >= sel_y + height)
    {
        return EGUI_MASK_ROW_OUTSIDE;
    }

    // Middle band - no corners, full width opaque
    if (y >= sel_y + radius && y < sel_y + height - radius)
    {
        *x_start = EGUI_MAX(sel_x, x_min);
        *x_end = EGUI_MIN(sel_x + width, x_max);
        return EGUI_MASK_ROW_INSIDE;
    }

    // Corner band - compute opaque span using circle lookup table
    egui_dim_t row_in_corner;
    if (y < sel_y + radius)
    {
        row_in_corner = y - sel_y; // Top corners: 0 at top edge
    }
    else
    {
        row_in_corner = sel_y + height - 1 - y; // Bottom corners: 0 at bottom edge
    }

    const egui_circle_info_t *info = egui_canvas_get_circle_item(radius);
    if (info == NULL)
    {
        // No lookup table available, cannot optimize
        *x_start = x_min;
        *x_end = x_min;
        return EGUI_MASK_ROW_PARTIAL;
    }

    egui_dim_t boundary = egui_mask_circle_corner_get_opaque_boundary(row_in_corner, info);

    *x_start = EGUI_MAX(sel_x + boundary, x_min);
    *x_end = EGUI_MIN(sel_x + width - boundary, x_max);

    if (*x_start >= *x_end)
    {
        // No opaque span in this row (very edge row)
        *x_start = x_min;
        *x_end = x_min;
    }

    return EGUI_MASK_ROW_PARTIAL;
}

static int egui_mask_round_rectangle_get_row_visible_range(egui_mask_t *self, egui_dim_t y, egui_dim_t x_min, egui_dim_t x_max, egui_dim_t *x_start,
                                                           egui_dim_t *x_end)
{
    EGUI_LOCAL_INIT(egui_mask_round_rectangle_t);
    egui_dim_t radius = local->radius;
    egui_dim_t sel_x = self->region.location.x;
    egui_dim_t sel_y = self->region.location.y;
    egui_dim_t width = self->region.size.width;
    egui_dim_t height = self->region.size.height;

    if (y < sel_y || y >= sel_y + height)
    {
        return 0;
    }

    if (radius <= 0 || (y >= sel_y + radius && y < sel_y + height - radius))
    {
        *x_start = EGUI_MAX(sel_x, x_min);
        *x_end = EGUI_MIN(sel_x + width, x_max);
        return (*x_start < *x_end);
    }

    egui_dim_t center_y = (y < sel_y + radius) ? (sel_y + radius) : (sel_y + height - radius - 1);
    egui_dim_t dy = (y > center_y) ? (y - center_y) : (center_y - y);
    uint32_t visible_radius_sq = (uint32_t)(radius + 1) * (uint32_t)(radius + 1);
    egui_dim_t visible_half = (egui_dim_t)egui_mask_round_rectangle_isqrt((dy * (uint32_t)dy < visible_radius_sq) ? (visible_radius_sq - dy * (uint32_t)dy) : 0);
    egui_dim_t left_center_x = sel_x + radius;
    egui_dim_t right_center_x = sel_x + width - radius - 1;

    *x_start = EGUI_MAX(left_center_x - visible_half, x_min);
    *x_end = EGUI_MIN(right_center_x + visible_half + 1, x_max);
    return (*x_start < *x_end);
}

// name must be _type##_api_table, it will be used by EGUI_VIEW_DEFINE to init api.
const egui_mask_api_t egui_mask_round_rectangle_t_api_table = {
        .mask_point = egui_mask_round_rectangle_mask_point,
        .mask_get_row_range = egui_mask_round_rectangle_get_row_range,
        .mask_get_row_visible_range = egui_mask_round_rectangle_get_row_visible_range,
        .mask_blend_row_color = NULL,
};

void egui_mask_round_rectangle_init(egui_mask_t *self)
{
    EGUI_LOCAL_INIT(egui_mask_round_rectangle_t);
    // call super init.
    egui_mask_init(self);
    // update api.
    self->api = &egui_mask_round_rectangle_t_api_table;

    // init local data.
}
