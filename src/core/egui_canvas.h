#ifndef _EGUI_CANVAS_H_
#define _EGUI_CANVAS_H_

#include "egui_common.h"
#include "egui_region.h"
#include "mask/egui_mask.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_circle_item egui_circle_item_t;
struct egui_circle_item
{
    uint16_t start_offset : 10;
    uint16_t valid_count : 6; // In radius 999, max valid count is 42.
    uint16_t data_offset;
};

typedef struct egui_circle_info egui_circle_info_t;
struct egui_circle_info
{
    uint16_t radius;
    uint16_t item_count;
    const void *items;
    const uint8_t *data; // point to data buffer for circle drawing
};

struct egui_canvas
{
    egui_color_int_t *pfb;    // pointer to frame buffer
    egui_region_t pfb_region; // location of the region in the frame buffer that can be used for drawing

    egui_region_t base_view_work_region; // pfb region intersect with the base view region. For fast drawing. in base view coordinates.

    egui_location_t pfb_location_in_base_view; // pfb base location in base view coordinates.

    egui_alpha_t alpha; // global alpha value for all drawing operations

    egui_mask_t *mask; // current mask for alpha blending

    // Optional extra clip region in screen coordinates (used by scroll views to clip children).
    // When set, egui_canvas_calc_work_region also intersects with this region.
    const egui_region_t *extra_clip_region;

    uint16_t res_circle_info_count_spec;
    const egui_circle_info_t *res_circle_info_spec_arr;
};

extern egui_canvas_t canvas_data;

__EGUI_STATIC_INLINE__ void egui_canvas_set_alpha(egui_alpha_t alpha)
{
    egui_canvas_t *self = &canvas_data;

    self->alpha = alpha;
}

__EGUI_STATIC_INLINE__ egui_alpha_t egui_canvas_get_alpha(void)
{
    egui_canvas_t *self = &canvas_data;

    return self->alpha;
}

__EGUI_STATIC_INLINE__ void egui_canvas_mix_alpha(egui_alpha_t alpha)
{
    egui_canvas_t *self = &canvas_data;

    self->alpha = egui_color_alpha_mix(self->alpha, alpha);
}

__EGUI_STATIC_INLINE__ void egui_canvas_clear_mask(void)
{
    egui_canvas_t *self = &canvas_data;

    self->mask = NULL;
}

/**
 * Set an extra clip region for restricting child view drawing within a scroll viewport.
 * Pass NULL to disable. Used by egui_view_scroll to prevent items rendering outside bounds.
 */
__EGUI_STATIC_INLINE__ void egui_canvas_set_extra_clip(const egui_region_t *clip_region)
{
    canvas_data.extra_clip_region = clip_region;
}

__EGUI_STATIC_INLINE__ void egui_canvas_clear_extra_clip(void)
{
    canvas_data.extra_clip_region = NULL;
}

__EGUI_STATIC_INLINE__ const egui_region_t *egui_canvas_get_extra_clip(void)
{
    return canvas_data.extra_clip_region;
}

__EGUI_STATIC_INLINE__ void egui_canvas_set_mask(egui_mask_t *mask)
{
    egui_canvas_t *self = &canvas_data;

    self->mask = mask;
}

__EGUI_STATIC_INLINE__ egui_mask_t *egui_canvas_get_mask(void)
{
    egui_canvas_t *self = &canvas_data;

    return self->mask;
}

__EGUI_STATIC_INLINE__ egui_region_t *egui_canvas_get_base_view_work_region(void)
{
    egui_canvas_t *self = &canvas_data;

    return &self->base_view_work_region;
}

__EGUI_STATIC_INLINE__ egui_region_t *egui_canvas_get_pfb_region(void)
{
    egui_canvas_t *self = &canvas_data;

    return &self->pfb_region;
}

__EGUI_STATIC_INLINE__ int egui_canvas_is_region_active(const egui_region_t *region)
{
    egui_canvas_t *self = &canvas_data;
    egui_region_t active_region;

    if (region == NULL)
    {
        return 0;
    }

    active_region = self->base_view_work_region;
    active_region.location.x += self->pfb_region.location.x - self->pfb_location_in_base_view.x;
    active_region.location.y += self->pfb_region.location.y - self->pfb_location_in_base_view.y;

    return egui_region_is_intersect(&active_region, region);
}

// For speed, we check if the point is within the base_view_work_region before calling egui_canvas_draw_point_limit.
__EGUI_STATIC_INLINE__ void egui_canvas_draw_point_limit_skip_mask(egui_dim_t x, egui_dim_t y, egui_color_t color, egui_alpha_t alpha)
{
    egui_canvas_t *self = &canvas_data;

    // mix alpha
    alpha = egui_color_alpha_mix(self->alpha, alpha);
    // if color is fully transparent, do not draw anything.
    if (alpha == 0)
    {
        return;
    }

    // skip pt check.
    // if (egui_region_pt_in_rect(&self->base_view_work_region, x, y))
    {
        egui_dim_t pos_x = x - self->pfb_location_in_base_view.x;
        egui_dim_t pos_y = y - self->pfb_location_in_base_view.y;

        egui_color_t *back_color = (egui_color_t *)&self->pfb[pos_y * self->pfb_region.size.width + pos_x];
#if TEST_CANVAS_TEST_WOKR
        back_color->full = alpha; // For test
#else
        if (alpha == EGUI_ALPHA_100)
        {
            *back_color = color;
        }
        else
        {
            egui_rgb_mix_ptr(back_color, &color, back_color, alpha);
        }
#endif
    }
}

// For speed, we check if the point is within the base_view_work_region before calling egui_canvas_draw_point_limit.
__EGUI_STATIC_INLINE__ void egui_canvas_draw_point_limit(egui_dim_t x, egui_dim_t y, egui_color_t color, egui_alpha_t alpha)
{
    egui_canvas_t *self = &canvas_data;

    if (self->mask != NULL)
    {
        self->mask->api->mask_point(self->mask, x, y, &color, &alpha);
    }

    // mix alpha
    alpha = egui_color_alpha_mix(self->alpha, alpha);
    // if color is fully transparent, do not draw anything.
    if (alpha == 0)
    {
        return;
    }

    // skip pt check.
    // if (egui_region_pt_in_rect(&self->base_view_work_region, x, y))
    {
        egui_dim_t pos_x = x - self->pfb_location_in_base_view.x;
        egui_dim_t pos_y = y - self->pfb_location_in_base_view.y;

        egui_color_t *back_color = (egui_color_t *)&self->pfb[pos_y * self->pfb_region.size.width + pos_x];
#if TEST_CANVAS_TEST_WOKR
        back_color->full = alpha; // For test
#else
        if (alpha == EGUI_ALPHA_100)
        {
            *back_color = color;
        }
        else
        {
            egui_rgb_mix_ptr(back_color, &color, back_color, alpha);
        }
#endif
    }
}

// In pfb coordinates
__EGUI_STATIC_INLINE__ void egui_canvas_set_point_color_raw(egui_canvas_t *self, egui_dim_t x, egui_dim_t y, egui_color_t color)
{
    *(egui_color_t *)&self->pfb[y * self->pfb_region.size.width + x] = color;
}

// In pfb coordinates
__EGUI_STATIC_INLINE__ void egui_canvas_set_point_color_with_alpha_raw(egui_canvas_t *self, egui_dim_t x, egui_dim_t y, egui_color_t color, egui_alpha_t alpha)
{
    egui_color_t *back_color = (egui_color_t *)&self->pfb[y * self->pfb_region.size.width + x];

    egui_rgb_mix_ptr(back_color, &color, back_color, alpha);
}

// In screen coordinates
__EGUI_STATIC_INLINE__ void egui_canvas_set_point_color(egui_canvas_t *self, egui_dim_t x, egui_dim_t y, egui_color_t color)
{
    egui_dim_t pos_x = x - self->pfb_location_in_base_view.x;
    egui_dim_t pos_y = y - self->pfb_location_in_base_view.y;
    egui_canvas_set_point_color_raw(self, pos_x, pos_y, color);
}

// In screen coordinates
__EGUI_STATIC_INLINE__ void egui_canvas_set_point_color_with_alpha(egui_canvas_t *self, egui_dim_t x, egui_dim_t y, egui_color_t color, egui_alpha_t alpha)
{
    egui_dim_t pos_x = x - self->pfb_location_in_base_view.x;
    egui_dim_t pos_y = y - self->pfb_location_in_base_view.y;
    egui_canvas_set_point_color_with_alpha_raw(self, pos_x, pos_y, color, alpha);
}

// In screen coordinates
__EGUI_STATIC_INLINE__ void egui_canvas_set_point_color_with_mask(egui_canvas_t *self, egui_dim_t x, egui_dim_t y, egui_color_t color, egui_alpha_t alpha)
{
    self->mask->api->mask_point(self->mask, x, y, &color, &alpha);

    // if color is fully transparent, do not draw anything.
    if (alpha == 0)
    {
        return;
    }

    egui_dim_t pos_x = x - self->pfb_location_in_base_view.x;
    egui_dim_t pos_y = y - self->pfb_location_in_base_view.y;

    // egui_color_t *back_color = (egui_color_t *)&self->pfb[pos_y * self->pfb_region.size.width + pos_x];

    if (alpha == EGUI_ALPHA_100)
    {
        egui_canvas_set_point_color_raw(self, pos_x, pos_y, color);
    }
    else
    {
        egui_canvas_set_point_color_with_alpha_raw(self, pos_x, pos_y, color, alpha);
    }
}

// In screen coordinates
__EGUI_STATIC_INLINE__ void egui_canvas_set_point_color_check(egui_canvas_t *self, egui_dim_t x, egui_dim_t y, egui_color_t color)
{
    if (egui_region_pt_in_rect(&self->base_view_work_region, x, y))
    {
        egui_canvas_set_point_color(self, x, y, color);
    }
}

// In screen coordinates
__EGUI_STATIC_INLINE__ void egui_canvas_set_point_color_with_alpha_check(egui_canvas_t *self, egui_dim_t x, egui_dim_t y, egui_color_t color,
                                                                         egui_alpha_t alpha)
{
    if (egui_region_pt_in_rect(&self->base_view_work_region, x, y))
    {
        egui_canvas_set_point_color_with_alpha(self, x, y, color, alpha);
    }
}

// In screen coordinates
__EGUI_STATIC_INLINE__ void egui_canvas_set_point_color_with_mask_check(egui_canvas_t *self, egui_dim_t x, egui_dim_t y, egui_color_t color, egui_alpha_t alpha)
{
    if (egui_region_pt_in_rect(&self->base_view_work_region, x, y))
    {
        egui_canvas_set_point_color_with_mask(self, x, y, color, alpha);
    }
}

__EGUI_STATIC_INLINE__ void egui_canvas_set_rect_color_with_mask(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_color_t color,
                                                                 egui_alpha_t alpha)
{
    egui_canvas_t *self = &canvas_data;

    egui_dim_t xp, yp;
    egui_dim_t x_total, y_total;

    // for speed, calculate total positions outside of the loop
    x_total = x + width;
    y_total = y + height;

    // Check if row-range optimization is available
    if (self->mask->api->mask_get_row_range != NULL)
    {
        egui_dim_t x_start, x_end;
        egui_dim_t pfb_x_offset = self->pfb_location_in_base_view.x;
        egui_dim_t pfb_y_offset = self->pfb_location_in_base_view.y;

        for (yp = y; yp < y_total; yp++)
        {
            int result = self->mask->api->mask_get_row_range(self->mask, yp, x, x_total, &x_start, &x_end);
            if (result == EGUI_MASK_ROW_OUTSIDE)
            {
                continue; // Skip entire row
            }

            egui_dim_t pfb_y = yp - pfb_y_offset;

            if (result == EGUI_MASK_ROW_INSIDE)
            {
                // Fast fill: entire row is fully opaque through mask - use direct pointer
                egui_dim_t pfb_xs = x_start - pfb_x_offset;
                egui_dim_t pfb_xe = x_end - pfb_x_offset;
                egui_color_int_t *dst = &self->pfb[pfb_y * self->pfb_region.size.width + pfb_xs];
                egui_dim_t count = pfb_xe - pfb_xs;

                if (alpha == EGUI_ALPHA_100)
                {
                    for (xp = 0; xp < count; xp++)
                    {
                        dst[xp] = color.full;
                    }
                }
                else
                {
                    for (xp = 0; xp < count; xp++)
                    {
                        egui_rgb_mix_ptr((egui_color_t *)&dst[xp], &color, (egui_color_t *)&dst[xp], alpha);
                    }
                }
            }
            else // EGUI_MASK_ROW_PARTIAL
            {
                egui_dim_t visible_x_start = x;
                egui_dim_t visible_x_end = x_total;
                if (self->mask->api->mask_get_row_visible_range != NULL &&
                    !self->mask->api->mask_get_row_visible_range(self->mask, yp, x, x_total, &visible_x_start, &visible_x_end))
                {
                    continue;
                }

                // Left edge: per-pixel with mask
                for (xp = visible_x_start; xp < EGUI_MIN(x_start, visible_x_end); xp++)
                {
                    egui_canvas_set_point_color_with_mask(self, xp, yp, color, alpha);
                }
                // Middle: fast fill (guaranteed fully opaque through mask) - use direct pointer
                if (x_start < x_end)
                {
                    egui_dim_t pfb_xs = x_start - pfb_x_offset;
                    egui_dim_t pfb_xe = x_end - pfb_x_offset;
                    egui_color_int_t *dst = &self->pfb[pfb_y * self->pfb_region.size.width + pfb_xs];
                    egui_dim_t count = pfb_xe - pfb_xs;

                    if (alpha == EGUI_ALPHA_100)
                    {
                        for (xp = 0; xp < count; xp++)
                        {
                            dst[xp] = color.full;
                        }
                    }
                    else
                    {
                        for (xp = 0; xp < count; xp++)
                        {
                            egui_rgb_mix_ptr((egui_color_t *)&dst[xp], &color, (egui_color_t *)&dst[xp], alpha);
                        }
                    }
                }
                // Right edge: per-pixel with mask
                for (xp = EGUI_MAX(x_end, visible_x_start); xp < visible_x_end; xp++)
                {
                    egui_canvas_set_point_color_with_mask(self, xp, yp, color, alpha);
                }
            }
        }
    }
    else
    {
        // Fallback: original per-pixel path
        for (yp = y; yp < y_total; yp++)
        {
            for (xp = x; xp < x_total; xp++)
            {
                egui_canvas_set_point_color_with_mask(self, xp, yp, color, alpha);
            }
        }
    }
}

__EGUI_STATIC_INLINE__ void egui_canvas_set_rect_color_with_alpha(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_color_t color,
                                                                  egui_alpha_t alpha)
{
    egui_canvas_t *self = &canvas_data;

    egui_dim_t pfb_width = self->pfb_region.size.width;
    egui_dim_t x_start = x - self->pfb_location_in_base_view.x;
    egui_dim_t y_start = y - self->pfb_location_in_base_view.y;
    egui_dim_t y_total = y_start + height;

    // Bounds guard: prevent PFB overflow
    if (x_start < 0 || y_start < 0 || x_start + width > pfb_width || y_total > self->pfb_region.size.height)
    {
        return;
    }

    for (egui_dim_t yp = y_start; yp < y_total; yp++)
    {
        egui_color_int_t *dst = &self->pfb[yp * pfb_width + x_start];
        for (egui_dim_t i = 0; i < width; i++)
        {
            egui_rgb_mix_ptr((egui_color_t *)&dst[i], &color, (egui_color_t *)&dst[i], alpha);
        }
    }
}

__EGUI_STATIC_INLINE__ void egui_canvas_set_rect_color(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_color_t color)
{
    egui_canvas_t *self = &canvas_data;

    egui_dim_t yp;
    egui_dim_t pfb_width = self->pfb_region.size.width;

    // for speed, calculate total positions outside of the loop
    egui_dim_t x_start = x - self->pfb_location_in_base_view.x;
    egui_dim_t y_start = y - self->pfb_location_in_base_view.y;
    egui_dim_t y_total = y_start + height;

    // Bounds guard: prevent PFB overflow
    if (x_start < 0 || y_start < 0 || x_start + width > pfb_width || y_total > self->pfb_region.size.height)
    {
        return;
    }

    for (yp = y_start; yp < y_total; yp++)
    {
        egui_color_int_t *dst = &self->pfb[yp * pfb_width + x_start];
        for (egui_dim_t i = 0; i < width; i++)
        {
            dst[i] = color.full;
        }
    }
}

__EGUI_STATIC_INLINE__ void egui_canvas_set_rect_color_with_mask_intersect(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_color_t color,
                                                                           egui_alpha_t alpha)
{
    egui_canvas_t *self = &canvas_data;

    // only work within intersection of base_view_work_region and the rectangle to be drawn
    EGUI_REGION_DEFINE(region, x, y, width, height);
    egui_region_intersect(&region, &self->base_view_work_region, &region);
    if (egui_region_is_empty(&region))
    {
        return;
    }

    egui_canvas_set_rect_color_with_mask(region.location.x, region.location.y, region.size.width, region.size.height, color, alpha);
}

__EGUI_STATIC_INLINE__ void egui_canvas_set_rect_color_with_alpha_intersect(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_color_t color,
                                                                            egui_alpha_t alpha)
{
    egui_canvas_t *self = &canvas_data;

    // only work within intersection of base_view_work_region and the rectangle to be drawn
    EGUI_REGION_DEFINE(region, x, y, width, height);
    egui_region_intersect(&region, &self->base_view_work_region, &region);
    if (egui_region_is_empty(&region))
    {
        return;
    }

    egui_canvas_set_rect_color_with_alpha(region.location.x, region.location.y, region.size.width, region.size.height, color, alpha);
}

__EGUI_STATIC_INLINE__ void egui_canvas_set_rect_color_intersect(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_color_t color)
{
    egui_canvas_t *self = &canvas_data;

    // only work within intersection of base_view_work_region and the rectangle to be drawn
    EGUI_REGION_DEFINE(region, x, y, width, height);
    egui_region_intersect(&region, &self->base_view_work_region, &region);
    if (egui_region_is_empty(&region))
    {
        return;
    }
    egui_canvas_set_rect_color(region.location.x, region.location.y, region.size.width, region.size.height, color);
}

__EGUI_STATIC_INLINE__ egui_canvas_t *egui_canvas_get_canvas(void)
{
    return &canvas_data;
}

void egui_canvas_set_alpha(egui_alpha_t alpha);
egui_alpha_t egui_canvas_get_alpha(void);
void egui_canvas_mix_alpha(egui_alpha_t alpha);

const egui_circle_info_t *egui_canvas_get_circle_item(egui_dim_t r);

__EGUI_STATIC_INLINE__ egui_alpha_t egui_canvas_get_circle_corner_value(egui_dim_t pos_row, egui_dim_t pos_col, const egui_circle_info_t *info)
{
    egui_dim_t offset;
    const egui_circle_item_t *ptr;
    uint16_t start_offset;
    uint16_t valid_count;
    uint16_t data_value_offset;

    // get min pos_row and pos_col
    egui_dim_t min_pos;
    egui_dim_t max_pos;
    if (pos_row < pos_col)
    {
        min_pos = pos_row;
        max_pos = pos_col;
    }
    else
    {
        min_pos = pos_col;
        max_pos = pos_row;
    }

    // if in middle of circle, return 0xFF
    if (min_pos >= info->item_count)
    {
        return EGUI_ALPHA_100;
    }

    int item_index = min_pos;

    ptr = &((const egui_circle_item_t *)info->items)[item_index];
    start_offset = ptr->start_offset;

    // get the offset
    offset = max_pos;

    // if out of circle, return 0
    if (offset < start_offset)
    {
        return EGUI_ALPHA_0;
    }

    valid_count = ptr->valid_count;
    // if in circle, return 0xFF
    if (offset >= start_offset + valid_count)
    {
        return EGUI_ALPHA_100;
    }

    data_value_offset = ptr->data_offset;
    return info->data[data_value_offset + offset - start_offset];
}

// void egui_canvas_draw_point_limit(egui_dim_t x, egui_dim_t y, egui_color_t color, egui_alpha_t alpha);
void egui_canvas_draw_point(egui_dim_t x, egui_dim_t y, egui_color_t color, egui_alpha_t alpha);

__EGUI_STATIC_INLINE__ void egui_canvas_draw_fillrect(egui_dim_t x, egui_dim_t y, egui_dim_t xSize, egui_dim_t ySize, egui_color_t color, egui_alpha_t alpha)
{
    egui_canvas_t *self = &canvas_data;

    // only work within intersection of base_view_work_region and the rectangle to be drawn
    EGUI_REGION_DEFINE(region, x, y, xSize, ySize);
    egui_region_intersect(&region, &self->base_view_work_region, &region);
    if (egui_region_is_empty(&region))
    {
        return;
    }

    // step1: mix alpha
    alpha = egui_color_alpha_mix(self->alpha, alpha);
    // if color is fully transparent, do not draw anything.
    if (alpha == 0)
    {
        return;
    }

    if (self->mask != NULL)
    {
        egui_canvas_set_rect_color_with_mask(region.location.x, region.location.y, region.size.width, region.size.height, color, alpha);
    }
    else
    {
        if (alpha == EGUI_ALPHA_100)
        {
            egui_canvas_set_rect_color(region.location.x, region.location.y, region.size.width, region.size.height, color);
        }
        else
        {
            egui_canvas_set_rect_color_with_alpha(region.location.x, region.location.y, region.size.width, region.size.height, color, alpha);
        }
    }
}

__EGUI_STATIC_INLINE__ void egui_canvas_draw_vline(egui_dim_t x, egui_dim_t y, egui_dim_t length, egui_color_t color, egui_alpha_t alpha)
{
    egui_canvas_draw_fillrect(x, y, 1, length, color, alpha);
}

__EGUI_STATIC_INLINE__ void egui_canvas_draw_hline(egui_dim_t x, egui_dim_t y, egui_dim_t length, egui_color_t color, egui_alpha_t alpha)
{
    egui_canvas_draw_fillrect(x, y, length, 1, color, alpha);
}
void egui_canvas_draw_line(egui_dim_t x1, egui_dim_t y1, egui_dim_t x2, egui_dim_t y2, egui_dim_t stroke_width, egui_color_t color, egui_alpha_t alpha);
void egui_canvas_draw_line_segment(egui_dim_t x1, egui_dim_t y1, egui_dim_t x2, egui_dim_t y2, egui_dim_t stroke_width, egui_color_t color, egui_alpha_t alpha);
void egui_canvas_draw_rectangle(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_dim_t stroke_width, egui_color_t color,
                                egui_alpha_t alpha);
void egui_canvas_draw_rectangle_fill(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_color_t color, egui_alpha_t alpha);
void egui_canvas_draw_round_rectangle_corners_fill(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_dim_t radius_left_top,
                                                   egui_dim_t radius_left_bottom, egui_dim_t radius_right_top, egui_dim_t radius_right_bottom,
                                                   egui_color_t color, egui_alpha_t alpha);
void egui_canvas_draw_round_rectangle_corners(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_dim_t radius_left_top,
                                              egui_dim_t radius_left_bottom, egui_dim_t radius_right_top, egui_dim_t radius_right_bottom,
                                              egui_dim_t stroke_width, egui_color_t color, egui_alpha_t alpha);
void egui_canvas_draw_round_rectangle(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_dim_t radius, egui_dim_t stroke_width,
                                      egui_color_t color, egui_alpha_t alpha);
void egui_canvas_draw_round_rectangle_fill(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_dim_t r, egui_color_t color,
                                           egui_alpha_t alpha);

// Basic (performance) circle/arc - always available, uses precomputed lookup tables
void egui_canvas_draw_circle_basic(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, egui_dim_t stroke_width, egui_color_t color,
                                   egui_alpha_t alpha);
void egui_canvas_draw_circle_fill_basic(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, egui_color_t color, egui_alpha_t alpha);
void egui_canvas_draw_arc_basic(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, int16_t start_angle, int16_t end_angle, egui_dim_t stroke_width,
                                egui_color_t color, egui_alpha_t alpha);
void egui_canvas_draw_arc_fill_basic(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, int16_t start_angle, int16_t end_angle, egui_color_t color,
                                     egui_alpha_t alpha);

// HQ (quality) circle/arc - runtime sub-pixel sampling, better anti-aliasing, no radius limit
#if EGUI_CONFIG_FUNCTION_CANVAS_DRAW_CIRCLE_HQ
void egui_canvas_draw_circle_hq(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, egui_dim_t stroke_width, egui_color_t color, egui_alpha_t alpha);
void egui_canvas_draw_circle_fill_hq(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, egui_color_t color, egui_alpha_t alpha);
void egui_canvas_draw_arc_hq(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, int16_t start_angle, int16_t end_angle, egui_dim_t stroke_width,
                             egui_color_t color, egui_alpha_t alpha);
void egui_canvas_draw_arc_fill_hq(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, int16_t start_angle, int16_t end_angle, egui_color_t color,
                                  egui_alpha_t alpha);
#endif

// Default circle/arc API - controlled by EGUI_CONFIG_CIRCLE_DEFAULT_ALGO_HQ
#if EGUI_CONFIG_CIRCLE_DEFAULT_ALGO_HQ
#define egui_canvas_draw_circle      egui_canvas_draw_circle_hq
#define egui_canvas_draw_circle_fill egui_canvas_draw_circle_fill_hq
#define egui_canvas_draw_arc         egui_canvas_draw_arc_hq
#define egui_canvas_draw_arc_fill    egui_canvas_draw_arc_fill_hq
#else
#define egui_canvas_draw_circle      egui_canvas_draw_circle_basic
#define egui_canvas_draw_circle_fill egui_canvas_draw_circle_fill_basic
#define egui_canvas_draw_arc         egui_canvas_draw_arc_basic
#define egui_canvas_draw_arc_fill    egui_canvas_draw_arc_fill_basic
#endif
void egui_canvas_draw_triangle(egui_dim_t x1, egui_dim_t y1, egui_dim_t x2, egui_dim_t y2, egui_dim_t x3, egui_dim_t y3, egui_color_t color,
                               egui_alpha_t alpha);
void egui_canvas_draw_triangle_fill(egui_dim_t x1, egui_dim_t y1, egui_dim_t x2, egui_dim_t y2, egui_dim_t x3, egui_dim_t y3, egui_color_t color,
                                    egui_alpha_t alpha);

#if EGUI_CONFIG_FUNCTION_CANVAS_DRAW_ELLIPSE
void egui_canvas_draw_ellipse(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius_x, egui_dim_t radius_y, egui_dim_t stroke_width, egui_color_t color,
                              egui_alpha_t alpha);
void egui_canvas_draw_ellipse_fill(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius_x, egui_dim_t radius_y, egui_color_t color, egui_alpha_t alpha);
#endif

#if EGUI_CONFIG_FUNCTION_CANVAS_DRAW_POLYGON
void egui_canvas_draw_polygon(const egui_dim_t *points, uint8_t count, egui_dim_t stroke_width, egui_color_t color, egui_alpha_t alpha);
void egui_canvas_draw_polygon_fill(const egui_dim_t *points, uint8_t count, egui_color_t color, egui_alpha_t alpha);
void egui_canvas_draw_polyline(const egui_dim_t *points, uint8_t count, egui_dim_t stroke_width, egui_color_t color, egui_alpha_t alpha);
#endif

// HQ (quality) line/polyline - runtime sub-pixel sampling, better anti-aliasing
#if EGUI_CONFIG_FUNCTION_CANVAS_DRAW_LINE_HQ
void egui_canvas_draw_line_hq(egui_dim_t x1, egui_dim_t y1, egui_dim_t x2, egui_dim_t y2, egui_dim_t stroke_width, egui_color_t color, egui_alpha_t alpha);
void egui_canvas_draw_line_segment_hq(egui_dim_t x1, egui_dim_t y1, egui_dim_t x2, egui_dim_t y2, egui_dim_t stroke_width, egui_color_t color,
                                      egui_alpha_t alpha);
void egui_canvas_draw_line_round_cap_hq(egui_dim_t x1, egui_dim_t y1, egui_dim_t x2, egui_dim_t y2, egui_dim_t stroke_width, egui_color_t color,
                                        egui_alpha_t alpha);
void egui_canvas_draw_polyline_hq(const egui_dim_t *points, uint8_t count, egui_dim_t stroke_width, egui_color_t color, egui_alpha_t alpha);
void egui_canvas_draw_polyline_round_cap_hq(const egui_dim_t *points, uint8_t count, egui_dim_t stroke_width, egui_color_t color, egui_alpha_t alpha);
#if EGUI_CONFIG_FUNCTION_CANVAS_DRAW_CIRCLE_HQ
void egui_canvas_draw_arc_round_cap_hq(egui_dim_t cx, egui_dim_t cy, egui_dim_t radius, int16_t start_angle, int16_t end_angle, egui_dim_t stroke_width,
                                       egui_color_t color, egui_alpha_t alpha);
#endif
#endif

#if EGUI_CONFIG_FUNCTION_CANVAS_DRAW_BEZIER
void egui_canvas_draw_bezier_quad(egui_dim_t x0, egui_dim_t y0, egui_dim_t cx, egui_dim_t cy, egui_dim_t x1, egui_dim_t y1, egui_dim_t stroke_width,
                                  egui_color_t color, egui_alpha_t alpha);
void egui_canvas_draw_bezier_cubic(egui_dim_t x0, egui_dim_t y0, egui_dim_t cx0, egui_dim_t cy0, egui_dim_t cx1, egui_dim_t cy1, egui_dim_t x1, egui_dim_t y1,
                                   egui_dim_t stroke_width, egui_color_t color, egui_alpha_t alpha);
#endif

void egui_canvas_draw_text(const egui_font_t *font, const void *string, egui_dim_t x, egui_dim_t y, egui_color_t color, egui_alpha_t alpha);
void egui_canvas_draw_text_in_rect_with_line_space(const egui_font_t *font, const void *string, egui_region_t *rect, uint8_t align_type, egui_dim_t line_space,
                                                   egui_color_t color, egui_alpha_t alpha);
void egui_canvas_draw_text_in_rect(const egui_font_t *font, const void *string, egui_region_t *rect, uint8_t align_type, egui_color_t color,
                                   egui_alpha_t alpha);
void egui_canvas_draw_image(const egui_image_t *img, egui_dim_t x, egui_dim_t y);
void egui_canvas_draw_image_resize(const egui_image_t *img, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height);
void egui_canvas_draw_image_color(const egui_image_t *img, egui_dim_t x, egui_dim_t y, egui_color_t color, egui_alpha_t alpha);
void egui_canvas_draw_image_resize_color(const egui_image_t *img, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_color_t color,
                                         egui_alpha_t alpha);

void egui_canvas_draw_image_transform(const egui_image_t *img, egui_dim_t x, egui_dim_t y, int16_t angle_deg, int16_t scale_q8);
void egui_canvas_draw_text_transform(const egui_font_t *font, const void *string, egui_dim_t x, egui_dim_t y, int16_t angle_deg, int16_t scale_q8,
                                     egui_color_t color, egui_alpha_t alpha);
void egui_canvas_draw_text_transform_buffered(const egui_font_t *font, const void *string, egui_dim_t x, egui_dim_t y, int16_t angle_deg, int16_t scale_q8,
                                              egui_color_t color, egui_alpha_t alpha);

void egui_canvas_calc_work_region(egui_region_t *base_region);
void egui_canvas_register_spec_circle_info(uint16_t res_circle_info_count_spec, const egui_circle_info_t *res_circle_info_spec_arr);
void egui_canvas_init(egui_color_int_t *pfb, egui_region_t *region);

int egui_canvas_get_circle_left_top(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, egui_dim_t x, egui_dim_t y, egui_alpha_t *alpha);
int egui_canvas_get_circle_left_bottom(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, egui_dim_t x, egui_dim_t y, egui_alpha_t *alpha);
int egui_canvas_get_circle_right_top(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, egui_dim_t x, egui_dim_t y, egui_alpha_t *alpha);
int egui_canvas_get_circle_right_bottom(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, egui_dim_t x, egui_dim_t y, egui_alpha_t *alpha);

void egui_canvas_clear_mask(void);
void egui_canvas_set_mask(egui_mask_t *mask);
egui_mask_t *egui_canvas_get_mask(void);

egui_region_t *egui_canvas_get_base_view_work_region(void);
egui_region_t *egui_canvas_get_pfb_region(void);
int egui_canvas_is_region_active(const egui_region_t *region);

#include "egui_canvas_gradient.h"

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_CANVAS_H_ */
