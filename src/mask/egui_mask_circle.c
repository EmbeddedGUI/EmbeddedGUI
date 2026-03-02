#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_mask_circle.h"
#include "core/egui_common.h"
#include "core/egui_api.h"
#include "core/egui_canvas.h"

extern const egui_circle_info_t egui_res_circle_info_arr[];

void egui_mask_circle_mask_point(egui_mask_t *self, egui_dim_t x, egui_dim_t y, egui_color_t *color, egui_alpha_t *alpha)
{
    EGUI_LOCAL_INIT(egui_mask_circle_t);

    if (egui_region_pt_in_rect(&self->region, x, y))
    {
        // Get the radio.
        egui_dim_t radius = EGUI_MIN(self->region.size.width, self->region.size.height);
        radius = (radius >> 1) - 1;

        // Get the center of the circle.
        egui_dim_t center_x = self->region.location.x + (self->region.size.width >> 1);
        egui_dim_t center_y = self->region.location.y + (self->region.size.height >> 1);

        if (egui_canvas_get_circle_left_top(center_x, center_y, radius, x, y, alpha))
        {
            return;
        }
        if (egui_canvas_get_circle_left_bottom(center_x, center_y, radius, x, y, alpha))
        {
            return;
        }
        if (egui_canvas_get_circle_right_top(center_x, center_y, radius, x, y, alpha))
        {
            return;
        }
        if (egui_canvas_get_circle_right_bottom(center_x, center_y, radius, x, y, alpha))
        {
            return;
        }

        egui_region_t region;
        egui_region_init(&region, center_x - radius, center_y, (radius << 1) + 1, 1);
        if (egui_region_pt_in_rect(&region, x, y))
        {
            return;
        }

        egui_region_init(&region, center_x, center_y - radius, 1, (radius << 1) + 1);
        if (egui_region_pt_in_rect(&region, x, y))
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

    if (row_in_corner < item_count)
    {
        const egui_circle_item_t *item = &((const egui_circle_item_t *)info->items)[row_in_corner];
        left_boundary = (egui_dim_t)item->start_offset + (egui_dim_t)item->valid_count;
    }
    else
    {
        left_boundary = item_count;
    }

    egui_dim_t mirror_limit = EGUI_MIN(row_in_corner, item_count);
    for (egui_dim_t col = mirror_limit - 1; col >= 0; col--)
    {
        const egui_circle_item_t *item = &((const egui_circle_item_t *)info->items)[col];
        egui_dim_t threshold = (egui_dim_t)item->start_offset + (egui_dim_t)item->valid_count;
        if (row_in_corner >= threshold)
        {
            if (col + 1 > left_boundary)
            {
                left_boundary = col + 1;
            }
            break;
        }
        left_boundary = EGUI_MAX(left_boundary, col + 1);
        break;
    }

    return left_boundary;
}

int egui_mask_circle_get_row_range(egui_mask_t *self, egui_dim_t y, egui_dim_t x_min, egui_dim_t x_max, egui_dim_t *x_start, egui_dim_t *x_end)
{
    EGUI_LOCAL_INIT(egui_mask_circle_t);

    egui_dim_t radius = EGUI_MIN(self->region.size.width, self->region.size.height);
    radius = (radius >> 1) - 1;

    egui_dim_t center_x = self->region.location.x + (self->region.size.width >> 1);
    egui_dim_t center_y = self->region.location.y + (self->region.size.height >> 1);

    egui_dim_t opaque_x_start;
    egui_dim_t opaque_x_end;

    // Outside circle vertical extent
    if (y < center_y - radius || y > center_y + radius)
    {
        return EGUI_MASK_ROW_OUTSIDE;
    }

    // Center row: horizontal line
    if (y == center_y)
    {
        opaque_x_start = center_x - radius;
        opaque_x_end = center_x + radius + 1;
    }
    else
    {
        const egui_circle_info_t *info = egui_canvas_get_circle_item(radius);
        if (info == NULL)
        {
            *x_start = x_min;
            *x_end = x_min;
            return EGUI_MASK_ROW_PARTIAL;
        }

        // Compute distance from center and corner row index
        egui_dim_t dist = (y < center_y) ? (center_y - y) : (y - center_y);
        egui_dim_t row_in_corner = radius - dist; // 0 at outer edge, radius-1 near center

        egui_dim_t boundary = egui_mask_circle_corner_get_opaque_boundary(row_in_corner, info);

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

// name must be _type##_api_table, it will be used by EGUI_VIEW_DEFINE to init api.
const egui_mask_api_t egui_mask_circle_t_api_table = {
        .mask_point = egui_mask_circle_mask_point,
        .mask_get_row_range = egui_mask_circle_get_row_range,
};

void egui_mask_circle_init(egui_mask_t *self)
{
    EGUI_LOCAL_INIT(egui_mask_circle_t);
    // call super init.
    egui_mask_init(self);
    // update api.
    self->api = &egui_mask_circle_t_api_table;

    // init local data.
}
