#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_mask_round_rectangle.h"
#include "image/egui_image_std.h"
#include "core/egui_common.h"
#include "core/egui_api.h"
#include "core/egui_canvas.h"

#if EGUI_CONFIG_FUNCTION_SUPPORT_MASK

extern const egui_circle_info_t egui_res_circle_info_arr[];

void egui_mask_round_rectangle_set_radius(egui_mask_t *self, egui_dim_t radius)
{
    EGUI_LOCAL_INIT(egui_mask_round_rectangle_t);
    egui_dim_t max_radius = EGUI_MIN(self->region.size.width, self->region.size.height) >> 1;

    if (max_radius < 0)
    {
        max_radius = 0;
    }

    if (radius <= 0)
    {
        local->radius = 0;
    }
    else if (max_radius > 0 && radius > max_radius)
    {
        local->radius = max_radius;
    }
    else
    {
        local->radius = radius;
    }
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
        egui_dim_t middle_width = width - (radius << 1);
        egui_dim_t side_height = height - (radius << 1);

        if (radius <= 0)
        {
            return;
        }

        // check in the middle rectangle.
        if (middle_width > 0)
        {
            egui_region_init(&region, sel_x + radius, sel_y, middle_width, height);
            if (egui_region_pt_in_rect(&region, x, y))
            {
                return;
            }
        }

        // check in the left and right rectangles.
        if (side_height > 0)
        {
            egui_region_init(&region, sel_x, sel_y + radius, radius, side_height);
            if (egui_region_pt_in_rect(&region, x, y))
            {
                return;
            }

            egui_region_init(&region, sel_x + width - radius, sel_y + radius, radius, side_height);
            if (egui_region_pt_in_rect(&region, x, y))
            {
                return;
            }
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

static void egui_mask_round_rectangle_blend_solid_row(egui_color_int_t *dst, egui_dim_t count, egui_color_t color, egui_alpha_t alpha)
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

int egui_mask_round_rectangle_fill_row_segment(egui_mask_t *self, egui_color_int_t *dst, egui_dim_t y, egui_dim_t x_start, egui_dim_t x_end, egui_color_t color,
                                               egui_alpha_t alpha)
{
    EGUI_LOCAL_INIT(egui_mask_round_rectangle_t);
    egui_dim_t radius = local->radius;
    egui_dim_t width = self->region.size.width;
    egui_dim_t height = self->region.size.height;
    egui_dim_t sel_x = self->region.location.x;
    egui_dim_t sel_y = self->region.location.y;
    egui_dim_t seg_start = EGUI_MAX(x_start, sel_x);
    egui_dim_t seg_end = EGUI_MIN(x_end, sel_x + width);

    if (y < sel_y || y >= sel_y + height || seg_start >= seg_end)
    {
        return 1;
    }

    if (radius <= 0 || (y >= sel_y + radius && y < sel_y + height - radius))
    {
        egui_mask_round_rectangle_blend_solid_row(dst + (seg_start - x_start), seg_end - seg_start, color, alpha);
        return 1;
    }

    {
        const egui_circle_info_t *info = egui_canvas_get_circle_item(radius);
        const egui_circle_item_t *items;
        egui_dim_t row_index;

        if (info == NULL)
        {
            return 0;
        }

        items = (const egui_circle_item_t *)info->items;
        row_index = (y < sel_y + radius) ? (y - sel_y) : (sel_y + height - 1 - y);

        {
            egui_dim_t left_end = EGUI_MIN(seg_end, sel_x + radius);
            egui_color_int_t *left_dst = dst + (seg_start - x_start);
            egui_dim_t corner_col = seg_start - sel_x;

            for (egui_dim_t xp = seg_start; xp < left_end; xp++, left_dst++, corner_col++)
            {
                egui_alpha_t pixel_alpha = egui_color_alpha_mix(egui_canvas_get_circle_corner_value_fixed_row(row_index, corner_col, info, items), alpha);

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
            egui_dim_t mid_start = EGUI_MAX(seg_start, sel_x + radius);
            egui_dim_t mid_end = EGUI_MIN(seg_end, sel_x + width - radius);

            if (mid_start < mid_end)
            {
                egui_mask_round_rectangle_blend_solid_row(dst + (mid_start - x_start), mid_end - mid_start, color, alpha);
            }
        }

        {
            egui_dim_t right_start = EGUI_MAX(seg_start, sel_x + width - radius);

            if (right_start < seg_end)
            {
                egui_color_int_t *right_dst = dst + (right_start - x_start);
                egui_dim_t corner_col = sel_x + width - 1 - right_start;

                for (egui_dim_t xp = right_start; xp < seg_end; xp++, right_dst++, corner_col--)
                {
                    egui_alpha_t pixel_alpha = egui_color_alpha_mix(egui_canvas_get_circle_corner_value_fixed_row(row_index, corner_col, info, items), alpha);

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
        }
    }

    return 1;
}

static void egui_mask_round_rectangle_blend_rgb565_alpha8_range(egui_color_int_t *dst_row, const uint16_t *src_row, const uint8_t *src_alpha_row,
                                                                const egui_dim_t *src_x_map, egui_dim_t start_index, egui_dim_t end_index,
                                                                egui_dim_t mask_col_start, int mask_col_step, const egui_circle_info_t *info,
                                                                const egui_circle_item_t *items, egui_dim_t row_index, egui_alpha_t canvas_alpha)
{
    egui_dim_t mask_col = mask_col_start;

    for (egui_dim_t i = start_index; i < end_index; i++, mask_col += mask_col_step)
    {
        egui_dim_t src_x = src_x_map[i];
        egui_alpha_t alpha = src_alpha_row[src_x];

        if (alpha == 0)
        {
            continue;
        }

        alpha = egui_color_alpha_mix(egui_canvas_get_circle_corner_value_fixed_row(row_index, mask_col, info, items), alpha);
        if (canvas_alpha != EGUI_ALPHA_100)
        {
            alpha = egui_color_alpha_mix(canvas_alpha, alpha);
        }

        if (alpha == 0)
        {
            continue;
        }

        egui_image_std_blend_rgb565_src_pixel_fast(&dst_row[i], src_row[src_x], alpha);
    }
}

static void egui_mask_round_rectangle_blend_rgb565_alpha8_middle(egui_color_int_t *dst_row, const uint16_t *src_row, const uint8_t *src_alpha_row,
                                                                 const egui_dim_t *src_x_map, egui_dim_t start_index, egui_dim_t end_index,
                                                                 egui_alpha_t canvas_alpha)
{
    for (egui_dim_t i = start_index; i < end_index; i++)
    {
        egui_dim_t src_x = src_x_map[i];
        egui_alpha_t alpha = src_alpha_row[src_x];

        if (alpha == 0)
        {
            continue;
        }

        if (canvas_alpha != EGUI_ALPHA_100)
        {
            alpha = egui_color_alpha_mix(canvas_alpha, alpha);
        }

        if (alpha == 0)
        {
            continue;
        }

        egui_image_std_blend_rgb565_src_pixel_fast(&dst_row[i], src_row[src_x], alpha);
    }
}

int egui_mask_round_rectangle_blend_rgb565_alpha8_segment(egui_mask_t *self, egui_color_int_t *dst_row, const uint16_t *src_row, const uint8_t *src_alpha_row,
                                                          const egui_dim_t *src_x_map, egui_dim_t count, egui_dim_t screen_x, egui_dim_t screen_y,
                                                          egui_alpha_t canvas_alpha)
{
    EGUI_LOCAL_INIT(egui_mask_round_rectangle_t);
    egui_dim_t radius = local->radius;
    egui_dim_t width = self->region.size.width;
    egui_dim_t height = self->region.size.height;
    egui_dim_t sel_x = self->region.location.x;
    egui_dim_t sel_y = self->region.location.y;
    egui_dim_t seg_start = EGUI_MAX(screen_x, sel_x);
    egui_dim_t seg_end = EGUI_MIN(screen_x + count, sel_x + width);
    const egui_circle_info_t *info;
    const egui_circle_item_t *items;
    egui_dim_t row_index;

    if (screen_y < sel_y || screen_y >= sel_y + height || seg_start >= seg_end)
    {
        return 1;
    }

    if (radius <= 0 || (screen_y >= sel_y + radius && screen_y < sel_y + height - radius))
    {
        egui_mask_round_rectangle_blend_rgb565_alpha8_middle(dst_row, src_row, src_alpha_row, src_x_map, seg_start - screen_x, seg_end - screen_x,
                                                             canvas_alpha);
        return 1;
    }

    info = egui_canvas_get_circle_item(radius);
    if (info == NULL)
    {
        return 0;
    }

    items = (const egui_circle_item_t *)info->items;
    row_index = (screen_y < sel_y + radius) ? (screen_y - sel_y) : (sel_y + height - 1 - screen_y);

    {
        egui_dim_t left_start = seg_start;
        egui_dim_t left_end = EGUI_MIN(seg_end, sel_x + radius);

        if (left_start < left_end)
        {
            egui_mask_round_rectangle_blend_rgb565_alpha8_range(dst_row, src_row, src_alpha_row, src_x_map, left_start - screen_x, left_end - screen_x,
                                                                left_start - sel_x, 1, info, items, row_index, canvas_alpha);
        }
    }

    {
        egui_dim_t mid_start = EGUI_MAX(seg_start, sel_x + radius);
        egui_dim_t mid_end = EGUI_MIN(seg_end, sel_x + width - radius);

        if (mid_start < mid_end)
        {
            egui_mask_round_rectangle_blend_rgb565_alpha8_middle(dst_row, src_row, src_alpha_row, src_x_map, mid_start - screen_x, mid_end - screen_x,
                                                                 canvas_alpha);
        }
    }

    {
        egui_dim_t right_start = EGUI_MAX(seg_start, sel_x + width - radius);
        egui_dim_t right_end = seg_end;

        if (right_start < right_end)
        {
            egui_mask_round_rectangle_blend_rgb565_alpha8_range(dst_row, src_row, src_alpha_row, src_x_map, right_start - screen_x, right_end - screen_x,
                                                                sel_x + width - 1 - right_start, -1, info, items, row_index, canvas_alpha);
        }
    }

    return 1;
}

// Compute the opaque boundary column for a circle corner quadrant at a given row.
// Returns the first column index (in corner coords 0..radius-1) that is guaranteed fully opaque.
static egui_dim_t egui_mask_circle_corner_get_opaque_boundary(egui_dim_t row_in_corner, const egui_circle_info_t *info)
{
    const egui_circle_item_t *items = (const egui_circle_item_t *)info->items;
    egui_dim_t item_count = (egui_dim_t)info->item_count;
    egui_dim_t left_boundary;

    // Primary half boundary (col >= row_in_corner): use items[row_in_corner]
    if (row_in_corner < item_count)
    {
        const egui_circle_item_t *item = &items[row_in_corner];
        left_boundary = (egui_dim_t)item->start_offset + (egui_dim_t)item->valid_count;
    }
    else
    {
        // Rows below the compact table still become fully opaque from the
        // reserve-rect strip that starts at item_count.
        left_boundary = item_count;
    }

    // Mirror half boundary (col < min(row_in_corner, item_count))
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

static egui_dim_t egui_mask_circle_corner_get_visible_boundary(egui_dim_t row_in_corner, const egui_circle_info_t *info)
{
    const egui_circle_item_t *items = (const egui_circle_item_t *)info->items;
    egui_dim_t item_count = (egui_dim_t)info->item_count;
    egui_dim_t left_boundary;

    if (row_in_corner < item_count)
    {
        left_boundary = (egui_dim_t)items[row_in_corner].start_offset;
    }
    else
    {
        left_boundary = item_count;
    }

    {
        egui_dim_t mirror_limit = EGUI_MIN(row_in_corner, item_count);
        if (mirror_limit > 0)
        {
            egui_dim_t low = 0;
            egui_dim_t high = mirror_limit;

            while (low < high)
            {
                egui_dim_t mid = low + ((high - low) >> 1);
                if (row_in_corner >= (egui_dim_t)items[mid].start_offset)
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
        if (*x_start >= *x_end)
        {
            return EGUI_MASK_ROW_OUTSIDE;
        }
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
    const egui_circle_info_t *info = egui_canvas_get_circle_item(radius);

    if (info != NULL)
    {
        egui_dim_t row_in_corner = radius - dy;
        egui_dim_t boundary = egui_mask_circle_corner_get_visible_boundary(row_in_corner, info);

        *x_start = EGUI_MAX(sel_x + boundary, x_min);
        *x_end = EGUI_MIN(sel_x + width - boundary, x_max);
        return (*x_start < *x_end);
    }

    {
        uint32_t visible_radius_sq = (uint32_t)(radius + 1) * (uint32_t)(radius + 1);
        egui_dim_t visible_half =
                (egui_dim_t)egui_mask_round_rectangle_isqrt((dy * (uint32_t)dy < visible_radius_sq) ? (visible_radius_sq - dy * (uint32_t)dy) : 0);
        egui_dim_t left_center_x = sel_x + radius;
        egui_dim_t right_center_x = sel_x + width - radius - 1;

        *x_start = EGUI_MAX(left_center_x - visible_half, x_min);
        *x_end = EGUI_MIN(right_center_x + visible_half + 1, x_max);
    }
    return (*x_start < *x_end);
}

// name must be _type##_api_table, it will be used by EGUI_VIEW_DEFINE to init api.
const egui_mask_api_t egui_mask_round_rectangle_t_api_table = {
        .kind = EGUI_MASK_KIND_ROUND_RECTANGLE,
        .mask_point = egui_mask_round_rectangle_mask_point,
        .mask_get_row_range = egui_mask_round_rectangle_get_row_range,
        .mask_get_row_visible_range = egui_mask_round_rectangle_get_row_visible_range,
        .mask_blend_row_color = NULL,
        .mask_get_row_overlay = NULL,
};

void egui_mask_round_rectangle_init(egui_mask_t *self)
{
    EGUI_LOCAL_INIT(egui_mask_round_rectangle_t);
    // call super init.
    egui_mask_init(self);
    // update api.
    self->api = &egui_mask_round_rectangle_t_api_table;

    // init local data.
    local->radius = 0;
}

#endif
