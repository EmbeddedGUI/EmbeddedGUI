#ifndef _EGUI_CANVAS_H_
#define _EGUI_CANVAS_H_

#include "egui_common.h"
#include "egui_region.h"
#include "mask/egui_mask.h"
#include "mask/egui_mask_circle.h"
#include "mask/egui_mask_round_rectangle.h"

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

#ifndef EGUI_CIRCLE_INFO_T_DEFINED
#define EGUI_CIRCLE_INFO_T_DEFINED
typedef struct egui_circle_info egui_circle_info_t;
#endif
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

#if EGUI_CONFIG_CANVAS_EXTRA_CLIP_ENABLE
    // Optional extra clip region in screen coordinates (used by scroll views to clip children).
    // When set, egui_canvas_calc_work_region also intersects with this region.
    const egui_region_t *extra_clip_region;
#endif

#if EGUI_CONFIG_CANVAS_SPEC_CIRCLE_INFO_ENABLE
    uint16_t res_circle_info_count_spec;
    const egui_circle_info_t *res_circle_info_spec_arr;
#endif
};

extern egui_canvas_t canvas_data;

void egui_mask_image_mask_point(egui_mask_t *self, egui_dim_t x, egui_dim_t y, egui_color_t *color, egui_alpha_t *alpha);
int egui_mask_image_fill_row_segment(egui_mask_t *self, egui_color_int_t *dst, egui_dim_t y, egui_dim_t x_start, egui_dim_t x_end, egui_color_t color,
                                     egui_alpha_t alpha);

__EGUI_STATIC_INLINE__ egui_alpha_t egui_canvas_get_circle_corner_value(egui_dim_t pos_row, egui_dim_t pos_col, const egui_circle_info_t *info);
__EGUI_STATIC_INLINE__ egui_alpha_t egui_canvas_get_circle_corner_value_fixed_row(egui_dim_t row_index, egui_dim_t col_index, const egui_circle_info_t *info,
                                                                                  const egui_circle_item_t *items);

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
#if EGUI_CONFIG_CANVAS_EXTRA_CLIP_ENABLE
    canvas_data.extra_clip_region = clip_region;
#else
    EGUI_UNUSED(clip_region);
#endif
}

__EGUI_STATIC_INLINE__ void egui_canvas_clear_extra_clip(void)
{
#if EGUI_CONFIG_CANVAS_EXTRA_CLIP_ENABLE
    canvas_data.extra_clip_region = NULL;
#endif
}

__EGUI_STATIC_INLINE__ const egui_region_t *egui_canvas_get_extra_clip(void)
{
#if EGUI_CONFIG_CANVAS_EXTRA_CLIP_ENABLE
    return canvas_data.extra_clip_region;
#else
    return NULL;
#endif
}

__EGUI_STATIC_INLINE__ void egui_canvas_set_mask(egui_mask_t *mask)
{
    egui_canvas_t *self = &canvas_data;

#if EGUI_CONFIG_FUNCTION_SUPPORT_MASK
    self->mask = mask;
#else
    EGUI_UNUSED(mask);
    self->mask = NULL;
#endif
}

__EGUI_STATIC_INLINE__ egui_mask_t *egui_canvas_get_mask(void)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_MASK
    egui_canvas_t *self = &canvas_data;

    return self->mask;
#else
    return NULL;
#endif
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

#if EGUI_CONFIG_FUNCTION_SUPPORT_MASK
    if (self->mask != NULL)
    {
        self->mask->api->mask_point(self->mask, x, y, &color, &alpha);
    }
#endif

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

__EGUI_STATIC_INLINE__ void egui_canvas_fill_color_buffer(egui_color_int_t *dst, uint32_t count, egui_color_t color)
{
    if (count == 0)
    {
        return;
    }

#if EGUI_CONFIG_COLOR_DEPTH == 16
    uint16_t pixel = color.full;

    if ((((egui_uintptr_t)dst) & 0x03U) != 0U)
    {
        *dst++ = pixel;
        count--;
    }

    if (count >= 2)
    {
        uint32_t packed = ((uint32_t)pixel << 16) | pixel;
        uint32_t *dst32 = (uint32_t *)dst;

        while (count >= 8)
        {
            dst32[0] = packed;
            dst32[1] = packed;
            dst32[2] = packed;
            dst32[3] = packed;
            dst32 += 4;
            count -= 8;
        }

        while (count >= 2)
        {
            *dst32++ = packed;
            count -= 2;
        }

        dst = (egui_color_int_t *)dst32;
    }

    if (count != 0)
    {
        *dst = pixel;
    }
#else
    egui_color_int_t pixel = color.full;

    for (uint32_t i = 0; i < count; i++)
    {
        dst[i] = pixel;
    }
#endif
}

__EGUI_STATIC_INLINE__ void egui_canvas_blend_color_buffer_alpha(egui_color_int_t *dst, uint32_t count, egui_color_t color, egui_alpha_t alpha)
{
    if (count == 0 || alpha == 0)
    {
        return;
    }

    if (alpha == EGUI_ALPHA_100)
    {
        egui_canvas_fill_color_buffer(dst, count, color);
        return;
    }

#if EGUI_CONFIG_COLOR_DEPTH == 16
    if (alpha > 251)
    {
        egui_canvas_fill_color_buffer(dst, count, color);
        return;
    }

    if (alpha < 4)
    {
        return;
    }

    {
        uint32_t fg_rb_g = (color.full | ((uint32_t)color.full << 16)) & 0x07E0F81FUL;

        for (uint32_t i = 0; i < count; i++)
        {
            uint16_t bg = dst[i];
            uint32_t bg_rb_g = (bg | ((uint32_t)bg << 16)) & 0x07E0F81FUL;
            uint32_t result = (bg_rb_g + ((fg_rb_g - bg_rb_g) * ((uint32_t)alpha >> 3) >> 5)) & 0x07E0F81FUL;
            dst[i] = (uint16_t)(result | (result >> 16));
        }
    }
#else
    for (uint32_t i = 0; i < count; i++)
    {
        egui_rgb_mix_ptr((egui_color_t *)&dst[i], &color, (egui_color_t *)&dst[i], alpha);
    }
#endif
}

__EGUI_STATIC_INLINE__ void egui_canvas_fill_masked_row_segment(egui_canvas_t *self, egui_dim_t y, egui_dim_t x_start, egui_dim_t x_end, egui_color_t color,
                                                                egui_alpha_t alpha)
{
    if (x_start >= x_end)
    {
        return;
    }

    egui_dim_t pfb_width = self->pfb_region.size.width;
    egui_dim_t pfb_x = x_start - self->pfb_location_in_base_view.x;
    egui_dim_t pfb_y = y - self->pfb_location_in_base_view.y;
    egui_color_int_t *dst = &self->pfb[pfb_y * pfb_width + pfb_x];

    if (self->mask->api->kind == EGUI_MASK_KIND_CIRCLE)
    {
        egui_mask_circle_t *circle_mask = (egui_mask_circle_t *)self->mask;
        egui_dim_t row_index;
        egui_dim_t center_x = circle_mask->center_x;
        egui_dim_t radius = circle_mask->radius;
        const egui_circle_info_t *info = circle_mask->info;
        const egui_circle_item_t *items;

        if (y == circle_mask->point_cached_y)
        {
            if (!circle_mask->point_cached_row_valid || info == NULL)
            {
                return;
            }
            row_index = circle_mask->point_cached_row_index;
        }
        else
        {
            egui_dim_t dy = (y > circle_mask->center_y) ? (y - circle_mask->center_y) : (circle_mask->center_y - y);
            if (dy > radius || info == NULL)
            {
                circle_mask->point_cached_y = y;
                circle_mask->point_cached_row_valid = 0;
                return;
            }
            row_index = radius - dy;
            circle_mask->point_cached_y = y;
            circle_mask->point_cached_row_index = row_index;
            circle_mask->point_cached_row_valid = 1;
        }

        items = (const egui_circle_item_t *)info->items;
        for (egui_dim_t xp = x_start; xp < x_end; xp++, dst++)
        {
            egui_dim_t dx = (xp > center_x) ? (xp - center_x) : (center_x - xp);
            egui_alpha_t pixel_alpha;

            if (dx > radius)
            {
                continue;
            }

            pixel_alpha = egui_color_alpha_mix(egui_canvas_get_circle_corner_value_fixed_row(row_index, radius - dx, info, items), alpha);
            if (pixel_alpha == 0)
            {
                continue;
            }

            if (pixel_alpha == EGUI_ALPHA_100)
            {
                *dst = color.full;
            }
            else
            {
                egui_rgb_mix_ptr((egui_color_t *)dst, &color, (egui_color_t *)dst, pixel_alpha);
            }
        }
        return;
    }

    if (self->mask->api->kind == EGUI_MASK_KIND_ROUND_RECTANGLE)
    {
        if (egui_mask_round_rectangle_fill_row_segment(self->mask, dst, y, x_start, x_end, color, alpha))
        {
            return;
        }
    }

    if (self->mask->api->kind == EGUI_MASK_KIND_IMAGE)
    {
        if (egui_mask_image_fill_row_segment(self->mask, dst, y, x_start, x_end, color, alpha))
        {
            return;
        }
    }

    for (egui_dim_t xp = x_start; xp < x_end; xp++, dst++)
    {
        egui_color_t pixel_color = color;
        egui_alpha_t pixel_alpha = alpha;
        self->mask->api->mask_point(self->mask, xp, y, &pixel_color, &pixel_alpha);

        if (pixel_alpha == 0)
        {
            continue;
        }

        if (pixel_alpha == EGUI_ALPHA_100)
        {
            *dst = pixel_color.full;
        }
        else
        {
            egui_rgb_mix_ptr((egui_color_t *)dst, &pixel_color, (egui_color_t *)dst, pixel_alpha);
        }
    }
}

__EGUI_STATIC_INLINE__ void egui_canvas_set_rect_color_with_mask(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_color_t color,
                                                                 egui_alpha_t alpha)
{
    egui_canvas_t *self = &canvas_data;

    egui_dim_t yp;
    egui_dim_t x_total, y_total;

    // for speed, calculate total positions outside of the loop
    x_total = x + width;
    y_total = y + height;

    // Check if mask has uniform row-level color blend (e.g., LINEAR_VERTICAL gradient)
    if (self->mask->api->mask_blend_row_color != NULL)
    {
        egui_dim_t pfb_x_offset = self->pfb_location_in_base_view.x;
        egui_dim_t pfb_y_offset = self->pfb_location_in_base_view.y;
        egui_dim_t pfb_width = self->pfb_region.size.width;

        for (yp = y; yp < y_total; yp++)
        {
            egui_color_t row_color = color;
            if (self->mask->api->mask_blend_row_color(self->mask, yp, &row_color))
            {
                egui_dim_t pfb_y = yp - pfb_y_offset;
                egui_color_int_t *dst = &self->pfb[pfb_y * pfb_width + (x - pfb_x_offset)];

                if (alpha == EGUI_ALPHA_100)
                {
                    egui_canvas_fill_color_buffer(dst, width, row_color);
                }
                else
                {
                    egui_canvas_blend_color_buffer_alpha(dst, width, row_color, alpha);
                }
            }
            else
            {
                egui_canvas_fill_masked_row_segment(self, yp, x, x_total, color, alpha);
            }
        }
        return;
    }

    if (self->mask->api->kind == EGUI_MASK_KIND_CIRCLE || self->mask->api->kind == EGUI_MASK_KIND_ROUND_RECTANGLE)
    {
        egui_dim_t pfb_x_offset = self->pfb_location_in_base_view.x;
        egui_dim_t pfb_y_offset = self->pfb_location_in_base_view.y;
        egui_dim_t pfb_width = self->pfb_region.size.width;

        for (yp = y; yp < y_total; yp++)
        {
            egui_color_int_t *dst = &self->pfb[(yp - pfb_y_offset) * pfb_width + (x - pfb_x_offset)];

            if (self->mask->api->kind == EGUI_MASK_KIND_CIRCLE)
            {
                egui_mask_circle_fill_row_segment(self->mask, dst, yp, x, x_total, color, alpha);
            }
            else
            {
                egui_mask_round_rectangle_fill_row_segment(self->mask, dst, yp, x, x_total, color, alpha);
            }
        }
        return;
    }

    if (self->mask->api->kind == EGUI_MASK_KIND_IMAGE)
    {
        egui_dim_t pfb_x_offset = self->pfb_location_in_base_view.x;
        egui_dim_t pfb_y_offset = self->pfb_location_in_base_view.y;
        egui_dim_t pfb_width = self->pfb_region.size.width;

        for (yp = y; yp < y_total; yp++)
        {
            egui_color_int_t *dst = &self->pfb[(yp - pfb_y_offset) * pfb_width + (x - pfb_x_offset)];

            if (!egui_mask_image_fill_row_segment(self->mask, dst, yp, x, x_total, color, alpha))
            {
                break;
            }
        }

        if (yp == y_total)
        {
            return;
        }
    }

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
                    egui_canvas_fill_color_buffer(dst, count, color);
                }
                else
                {
                    egui_canvas_blend_color_buffer_alpha(dst, count, color, alpha);
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
                egui_canvas_fill_masked_row_segment(self, yp, visible_x_start, EGUI_MIN(x_start, visible_x_end), color, alpha);
                // Middle: fast fill (guaranteed fully opaque through mask) - use direct pointer
                if (x_start < x_end)
                {
                    egui_dim_t pfb_xs = x_start - pfb_x_offset;
                    egui_dim_t pfb_xe = x_end - pfb_x_offset;
                    egui_color_int_t *dst = &self->pfb[pfb_y * self->pfb_region.size.width + pfb_xs];
                    egui_dim_t count = pfb_xe - pfb_xs;

                    if (alpha == EGUI_ALPHA_100)
                    {
                        egui_canvas_fill_color_buffer(dst, count, color);
                    }
                    else
                    {
                        egui_canvas_blend_color_buffer_alpha(dst, count, color, alpha);
                    }
                }
                // Right edge: per-pixel with mask
                egui_canvas_fill_masked_row_segment(self, yp, EGUI_MAX(x_end, visible_x_start), visible_x_end, color, alpha);
            }
        }
    }
    else
    {
        // Fallback: original per-pixel path
        for (yp = y; yp < y_total; yp++)
        {
            egui_canvas_fill_masked_row_segment(self, yp, x, x_total, color, alpha);
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
        egui_canvas_blend_color_buffer_alpha(dst, width, color, alpha);
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

    if (x_start == 0 && width == pfb_width)
    {
        egui_canvas_fill_color_buffer(&self->pfb[y_start * pfb_width], (uint32_t)width * (uint32_t)height, color);
        return;
    }

    for (yp = y_start; yp < y_total; yp++)
    {
        egui_canvas_fill_color_buffer(&self->pfb[yp * pfb_width + x_start], width, color);
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

__EGUI_STATIC_INLINE__ egui_alpha_t egui_canvas_get_circle_corner_value_fixed_row(egui_dim_t row_index, egui_dim_t col_index, const egui_circle_info_t *info,
                                                                                  const egui_circle_item_t *items)
{
    if (row_index <= col_index)
    {
        egui_dim_t item_count = (egui_dim_t)info->item_count;

        if (row_index >= item_count)
        {
            return EGUI_ALPHA_100;
        }

        const egui_circle_item_t *row_item = &items[row_index];
        egui_dim_t start_offset = row_item->start_offset;
        egui_dim_t valid_count = row_item->valid_count;

        if (col_index < start_offset)
        {
            return EGUI_ALPHA_0;
        }

        if (col_index >= start_offset + valid_count)
        {
            return EGUI_ALPHA_100;
        }

        return info->data[row_item->data_offset + col_index - start_offset];
    }

    {
        egui_dim_t item_count = (egui_dim_t)info->item_count;

        if (col_index >= item_count)
        {
            return EGUI_ALPHA_100;
        }

        const egui_circle_item_t *col_item = &items[col_index];
        egui_dim_t start_offset = col_item->start_offset;
        egui_dim_t valid_count = col_item->valid_count;

        if (row_index < start_offset)
        {
            return EGUI_ALPHA_0;
        }

        if (row_index >= start_offset + valid_count)
        {
            return EGUI_ALPHA_100;
        }

        return info->data[col_item->data_offset + row_index - start_offset];
    }
}

__EGUI_STATIC_INLINE__ int egui_canvas_get_circle_fill_basic_row_layout(egui_dim_t radius, egui_dim_t row_index, egui_dim_t *start_offset,
                                                                        egui_dim_t *valid_count, egui_dim_t *fill_start, egui_dim_t *fill_end)
{
    const egui_circle_info_t *info = egui_canvas_get_circle_item(radius);
    const egui_circle_item_t *items;
    egui_dim_t edge_start = radius + 1;
    egui_dim_t edge_count = 0;
    egui_dim_t fill_offset = radius + 1;

    if (info == NULL || row_index < 0 || row_index > radius)
    {
        return 0;
    }

    items = (const egui_circle_item_t *)info->items;

    {
        egui_dim_t first_visible = radius + 1;
        egui_dim_t first_opaque = radius + 1;

        for (egui_dim_t col_index = 0; col_index <= radius; col_index++)
        {
            egui_alpha_t pixel_alpha = egui_canvas_get_circle_corner_value_fixed_row(row_index, col_index, info, items);

            if (first_visible > radius && pixel_alpha != EGUI_ALPHA_0)
            {
                first_visible = col_index;
            }

            if (first_opaque > radius && pixel_alpha == EGUI_ALPHA_100)
            {
                first_opaque = col_index;
                break;
            }
        }

        if (first_visible <= radius)
        {
            edge_start = first_visible;
            fill_offset = first_opaque;

            if (fill_offset > radius)
            {
                fill_offset = radius + 1;
            }

            edge_count = fill_offset - edge_start;
        }
    }

    if (start_offset != NULL)
    {
        *start_offset = edge_start;
    }

    if (valid_count != NULL)
    {
        *valid_count = edge_count;
    }

    if (fill_start != NULL)
    {
        *fill_start = fill_offset;
    }

    if (fill_end != NULL)
    {
        *fill_end = ((radius << 1) + 1) - fill_offset;
    }

    return 1;
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

    if (EGUI_CONFIG_FUNCTION_SUPPORT_MASK && self->mask != NULL)
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
void egui_canvas_draw_circle_hq(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, egui_dim_t stroke_width, egui_color_t color, egui_alpha_t alpha);
void egui_canvas_draw_circle_fill_hq(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, egui_color_t color, egui_alpha_t alpha);
void egui_canvas_draw_arc_hq(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, int16_t start_angle, int16_t end_angle, egui_dim_t stroke_width,
                             egui_color_t color, egui_alpha_t alpha);
void egui_canvas_draw_arc_fill_hq(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, int16_t start_angle, int16_t end_angle, egui_color_t color,
                                  egui_alpha_t alpha);

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

void egui_canvas_draw_ellipse(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius_x, egui_dim_t radius_y, egui_dim_t stroke_width, egui_color_t color,
                              egui_alpha_t alpha);
void egui_canvas_draw_ellipse_fill(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius_x, egui_dim_t radius_y, egui_color_t color, egui_alpha_t alpha);

void egui_canvas_draw_polygon(const egui_dim_t *points, uint8_t count, egui_dim_t stroke_width, egui_color_t color, egui_alpha_t alpha);
void egui_canvas_draw_polygon_fill(const egui_dim_t *points, uint8_t count, egui_color_t color, egui_alpha_t alpha);
void egui_canvas_draw_polyline(const egui_dim_t *points, uint8_t count, egui_dim_t stroke_width, egui_color_t color, egui_alpha_t alpha);

// HQ (quality) line/polyline - runtime sub-pixel sampling, better anti-aliasing
void egui_canvas_draw_line_hq(egui_dim_t x1, egui_dim_t y1, egui_dim_t x2, egui_dim_t y2, egui_dim_t stroke_width, egui_color_t color, egui_alpha_t alpha);
void egui_canvas_draw_line_segment_hq(egui_dim_t x1, egui_dim_t y1, egui_dim_t x2, egui_dim_t y2, egui_dim_t stroke_width, egui_color_t color,
                                      egui_alpha_t alpha);
void egui_canvas_draw_line_round_cap_hq(egui_dim_t x1, egui_dim_t y1, egui_dim_t x2, egui_dim_t y2, egui_dim_t stroke_width, egui_color_t color,
                                        egui_alpha_t alpha);
void egui_canvas_draw_polyline_hq(const egui_dim_t *points, uint8_t count, egui_dim_t stroke_width, egui_color_t color, egui_alpha_t alpha);
void egui_canvas_draw_polyline_round_cap_hq(const egui_dim_t *points, uint8_t count, egui_dim_t stroke_width, egui_color_t color, egui_alpha_t alpha);
void egui_canvas_draw_arc_round_cap_hq(egui_dim_t cx, egui_dim_t cy, egui_dim_t radius, int16_t start_angle, int16_t end_angle, egui_dim_t stroke_width,
                                       egui_color_t color, egui_alpha_t alpha);

void egui_canvas_draw_bezier_quad(egui_dim_t x0, egui_dim_t y0, egui_dim_t cx, egui_dim_t cy, egui_dim_t x1, egui_dim_t y1, egui_dim_t stroke_width,
                                  egui_color_t color, egui_alpha_t alpha);
void egui_canvas_draw_bezier_cubic(egui_dim_t x0, egui_dim_t y0, egui_dim_t cx0, egui_dim_t cy0, egui_dim_t cx1, egui_dim_t cy1, egui_dim_t x1, egui_dim_t y1,
                                   egui_dim_t stroke_width, egui_color_t color, egui_alpha_t alpha);

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

/* Convenience arc helpers that accept start angle + sweep angle.
 * Negative sweep is supported and will be converted to the corresponding [start, end] range.
 */
void egui_canvas_draw_arc_sweep(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, int16_t start_angle, int16_t sweep_angle, egui_dim_t stroke_width,
                                egui_color_t color, egui_alpha_t alpha);

__EGUI_STATIC_INLINE__ int egui_canvas_should_use_circle_fill_basic_legacy_clip(egui_dim_t radius, egui_dim_t clip_width, egui_dim_t clip_height)
{
    egui_dim_t circle_diameter;
    egui_dim_t compact_tile_limit;

    if (radius < 0 || clip_width <= 0 || clip_height <= 0)
    {
        return 0;
    }

    if (clip_width <= 1)
    {
        return 1;
    }

    if (clip_height <= 1)
    {
        return 0;
    }

    // Keep only compact 2-D clips on the legacy path; long strips stay on the row-wise path.
    circle_diameter = (radius << 1) + 1;
    compact_tile_limit = EGUI_MAX(2, (circle_diameter + 7) >> 3);

    return EGUI_MAX(clip_width, clip_height) <= compact_tile_limit;
}
void egui_canvas_draw_arc_fill_sweep(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, int16_t start_angle, int16_t sweep_angle, egui_color_t color,
                                     egui_alpha_t alpha);
void egui_canvas_draw_arc_round_cap_sweep_hq(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, int16_t start_angle, int16_t sweep_angle,
                                             egui_dim_t stroke_width, egui_color_t color, egui_alpha_t alpha);

void egui_canvas_draw_image_transform(const egui_image_t *img, egui_dim_t x, egui_dim_t y, int16_t angle_deg, int16_t scale_q8);
/* Convenience image rotation helpers.
 * x/y use the same top-left coordinates as egui_canvas_draw_image().
 * pivot_x/pivot_y are relative to the unrotated
 * image top-left, and that pivot stays fixed in screen coordinates. */
void egui_canvas_draw_image_rotate(const egui_image_t *img, egui_dim_t x, egui_dim_t y, int16_t angle_deg);
void egui_canvas_draw_image_rotate_scale(const egui_image_t *img, egui_dim_t x, egui_dim_t y, int16_t angle_deg, int16_t scale_q8);
void egui_canvas_draw_image_rotate_pivot(const egui_image_t *img, egui_dim_t x, egui_dim_t y, egui_dim_t pivot_x, egui_dim_t pivot_y, int16_t angle_deg,
                                         int16_t scale_q8);

void egui_canvas_draw_text_transform(const egui_font_t *font, const void *string, egui_dim_t x, egui_dim_t y, int16_t angle_deg, int16_t scale_q8,
                                     egui_color_t color, egui_alpha_t alpha);
/* Convenience text rotation helpers.
 * x/y use the same top-left coordinates as egui_canvas_draw_text().
 * pivot_x/pivot_y are relative to the measured
 * unrotated text bounding box, and that pivot stays fixed in screen coordinates. */
void egui_canvas_draw_text_rotate(const egui_font_t *font, const void *string, egui_dim_t x, egui_dim_t y, int16_t angle_deg, egui_color_t color,
                                  egui_alpha_t alpha);
void egui_canvas_draw_text_rotate_scale(const egui_font_t *font, const void *string, egui_dim_t x, egui_dim_t y, int16_t angle_deg, int16_t scale_q8,
                                        egui_color_t color, egui_alpha_t alpha);
void egui_canvas_draw_text_rotate_pivot(const egui_font_t *font, const void *string, egui_dim_t x, egui_dim_t y, egui_dim_t pivot_x, egui_dim_t pivot_y,
                                        int16_t angle_deg, int16_t scale_q8, egui_color_t color, egui_alpha_t alpha);
void egui_canvas_transform_release_frame_cache(void);

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
