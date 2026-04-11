#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_canvas.h"
#include "font/egui_font.h"
#include "image/egui_image.h"
#include "image/egui_image_std.h"
#include "egui_api.h"
#include "egui_trig_lut.h"

#define TEST_CANVAS_TEST_WOKR 0

egui_canvas_t canvas_data;

enum
{
    EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP,
    EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM,
    EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP,
    EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM,
};

extern const egui_circle_info_t egui_res_circle_info_arr[];

#define ARC_INT_Q8_SHIFT           8
#define ARC_AA_HALF_TRANSITION_Q15 24576

static const egui_float_t arc_sin_lut[91] = {EGUI_TRIG_SIN_LUT_VALUES};

static const uint16_t arc_cot_q8_lut[91] = {
        0,   14666, 7331, 4885, 3661, 2926, 2436, 2085, 1822, 1616, 1452, 1317, 1204, 1109, 1027, 955, 893, 837, 788, 743, 703, 667, 634,
        603, 575,   549,  525,  502,  481,  462,  443,  426,  410,  394,  380,  366,  352,  340,  328, 316, 305, 294, 284, 275, 265, 256,
        247, 239,   231,  223,  215,  207,  200,  193,  186,  179,  173,  166,  160,  154,  148,  142, 136, 130, 125, 119, 114, 109, 103,
        98,  93,    88,   83,   78,   73,   69,   64,   59,   54,   50,   45,   41,   36,   31,   27,  22,  18,  13,  9,   4,   0,
};

__EGUI_STATIC_INLINE__ uint16_t egui_canvas_arc_get_sin_q15(int16_t angle)
{
    return (uint16_t)egui_trig_float_to_q15(arc_sin_lut[angle]);
}

__EGUI_STATIC_INLINE__ uint16_t egui_canvas_arc_get_cos_q15(int16_t angle)
{
    return (uint16_t)egui_trig_float_to_q15(arc_sin_lut[90 - angle]);
}

__EGUI_STATIC_INLINE__ egui_dim_t egui_canvas_arc_mul_cot_limit(egui_dim_t value, int16_t angle)
{
    if (angle <= 0)
    {
        return EGUI_DIM_MAX;
    }
    if (angle >= 90)
    {
        return 0;
    }

    return (egui_dim_t)(((int32_t)value * arc_cot_q8_lut[angle]) >> ARC_INT_Q8_SHIFT);
}

__EGUI_STATIC_INLINE__ int egui_canvas_arc_mul_cot_q8_nonnegative(egui_dim_t value, int16_t angle, int32_t *result_q8)
{
    if (result_q8 == NULL)
    {
        return 0;
    }

    if (angle <= 0)
    {
        return 0;
    }
    if (angle >= 90)
    {
        *result_q8 = 0;
        return 1;
    }

    *result_q8 = (int32_t)value * arc_cot_q8_lut[angle];
    return 1;
}

__EGUI_STATIC_INLINE__ int32_t egui_canvas_arc_get_transition_over_cos_q8(int16_t angle)
{
    uint16_t cos_q15 = egui_canvas_arc_get_cos_q15(angle);

    return (int32_t)(((ARC_AA_HALF_TRANSITION_Q15 << ARC_INT_Q8_SHIFT) + (cos_q15 >> 1)) / cos_q15);
}

void egui_canvas_draw_point(egui_dim_t x, egui_dim_t y, egui_color_t color, egui_alpha_t alpha)
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

    if (egui_region_pt_in_rect(&self->base_view_work_region, x, y))
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

void egui_canvas_fill_color_buffer(egui_color_int_t *dst, uint32_t count, egui_color_t color)
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

void egui_canvas_draw_fillrect(egui_dim_t x, egui_dim_t y, egui_dim_t xSize, egui_dim_t ySize, egui_color_t color, egui_alpha_t alpha)
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

const egui_circle_info_t *egui_canvas_get_circle_item(egui_dim_t r)
{
    egui_canvas_t *self = &canvas_data;

    if (r < EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE)
    {
        const egui_circle_info_t *info = &egui_res_circle_info_arr[r];
        if (info->radius == (uint16_t)r)
        {
            return info;
        }
        return NULL;
    }

#if EGUI_CONFIG_CANVAS_SPEC_CIRCLE_INFO_ENABLE
    for (int i = 0; i < self->res_circle_info_count_spec; i++)
    {
        if (self->res_circle_info_spec_arr[i].radius == r)
        {
            return &self->res_circle_info_spec_arr[i];
        }
    }
#else
    EGUI_UNUSED(self);
#endif

    return NULL;
}

static egui_dim_t egui_canvas_clamp_round_radius(egui_dim_t radius, egui_dim_t width, egui_dim_t height)
{
    egui_dim_t max_radius;

    if (radius <= 0 || width <= 2 || height <= 2)
    {
        return 0;
    }

    max_radius = EGUI_MIN((egui_dim_t)(width >> 1), (egui_dim_t)(height >> 1)) - 1;
    if (max_radius <= 0)
    {
        return 0;
    }

    if (radius > max_radius)
    {
        radius = max_radius;
    }

    return radius;
}

__EGUI_STATIC_INLINE__ int egui_canvas_round_rect_is_circle_case(egui_dim_t width, egui_dim_t height, egui_dim_t radius)
{
    return (radius > 0 && width == height && (((radius << 1) + 2) == width));
}

__EGUI_STATIC_INLINE__ int egui_canvas_circle_corner_is_left(int type)
{
    return (type == EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP || type == EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM);
}

__EGUI_STATIC_INLINE__ int egui_canvas_circle_corner_is_top(int type)
{
    return (type == EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP || type == EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP);
}

__EGUI_STATIC_INLINE__ egui_dim_t egui_canvas_circle_corner_get_row_index(int type, const egui_region_t *region, egui_dim_t radius, egui_dim_t screen_y)
{
    if (egui_canvas_circle_corner_is_top(type))
    {
        return screen_y - region->location.y;
    }

    return region->location.y + radius - 1 - screen_y;
}

__EGUI_STATIC_INLINE__ void egui_canvas_circle_corner_col_range_to_screen_x(egui_dim_t center_x, egui_dim_t radius, int type, egui_dim_t col_start,
                                                                            egui_dim_t col_end, egui_dim_t *screen_x_start, egui_dim_t *screen_x_end)
{
    if (egui_canvas_circle_corner_is_left(type))
    {
        *screen_x_start = center_x - radius + col_start;
        *screen_x_end = center_x - radius + col_end;
    }
    else
    {
        *screen_x_start = center_x + radius - col_end + 1;
        *screen_x_end = center_x + radius - col_start + 1;
    }
}

__EGUI_STATIC_INLINE__ egui_dim_t egui_canvas_circle_corner_screen_x_to_col(egui_dim_t center_x, egui_dim_t radius, int type, egui_dim_t screen_x)
{
    if (egui_canvas_circle_corner_is_left(type))
    {
        return screen_x - (center_x - radius);
    }

    return center_x + radius - screen_x;
}

__EGUI_STATIC_INLINE__ egui_dim_t egui_canvas_circle_corner_get_visible_boundary(egui_dim_t row_in_corner, const egui_circle_info_t *info,
                                                                                 const egui_circle_item_t *items)
{
    egui_dim_t item_count = (egui_dim_t)info->item_count;
    egui_dim_t left_boundary = (row_in_corner < item_count) ? (egui_dim_t)items[row_in_corner].start_offset : item_count;
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

    return left_boundary;
}

__EGUI_STATIC_INLINE__ egui_dim_t egui_canvas_circle_corner_get_opaque_boundary(egui_dim_t row_in_corner, const egui_circle_info_t *info,
                                                                                const egui_circle_item_t *items)
{
    egui_dim_t item_count = (egui_dim_t)info->item_count;
    egui_dim_t left_boundary;
    egui_dim_t mirror_limit;

    if (row_in_corner < item_count)
    {
        const egui_circle_item_t *item = &items[row_in_corner];

        left_boundary = (egui_dim_t)item->start_offset + (egui_dim_t)item->valid_count;
    }
    else
    {
        left_boundary = item_count;
    }

    mirror_limit = EGUI_MIN(row_in_corner, item_count);
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

__EGUI_STATIC_INLINE__ egui_dim_t egui_canvas_circle_corner_get_opaque_threshold(const egui_circle_item_t *items, egui_dim_t index)
{
    return (egui_dim_t)items[index].start_offset + (egui_dim_t)items[index].valid_count;
}

typedef struct
{
    egui_region_t region;
    egui_region_t region_intersect;
    egui_dim_t row_index_start;
    egui_dim_t row_index_end;
    egui_dim_t col_index_start;
    egui_dim_t col_index_end;
    int sign_x;
    int sign_y;
} egui_canvas_corner_bounds_t;

static void egui_canvas_circle_corner_init_region(egui_canvas_corner_bounds_t *bounds, egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, int type)
{
    switch (type)
    {
    case EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP:
        egui_region_init(&bounds->region, center_x - radius, center_y - radius, radius, radius);
        break;
    case EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM:
        egui_region_init(&bounds->region, center_x - radius, center_y + 1, radius, radius);
        break;
    case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP:
        egui_region_init(&bounds->region, center_x + 1, center_y - radius, radius, radius);
        break;
    case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM:
        egui_region_init(&bounds->region, center_x + 1, center_y + 1, radius, radius);
        break;
    }
}

static int egui_canvas_circle_corner_get_bounds(egui_canvas_t *self, egui_dim_t radius, int type, egui_canvas_corner_bounds_t *bounds)
{
    egui_dim_t diff_x;
    egui_dim_t diff_y;

    egui_region_intersect(&bounds->region, &self->base_view_work_region, &bounds->region_intersect);
    if (egui_region_is_empty(&bounds->region_intersect))
    {
        return 0;
    }

    bounds->row_index_start = 0;
    bounds->row_index_end = radius;
    bounds->col_index_start = 0;
    bounds->col_index_end = radius;

    diff_x = bounds->region_intersect.location.x - bounds->region.location.x;
    diff_y = bounds->region_intersect.location.y - bounds->region.location.y;

    switch (type)
    {
    case EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP:
        bounds->row_index_start = diff_y;
        bounds->row_index_end = bounds->row_index_start + bounds->region_intersect.size.height;
        bounds->col_index_start = diff_x;
        bounds->col_index_end = bounds->col_index_start + bounds->region_intersect.size.width;
        break;
    case EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM:
        bounds->row_index_end = radius - diff_y;
        bounds->row_index_start = bounds->row_index_end - bounds->region_intersect.size.height;
        bounds->col_index_start = diff_x;
        bounds->col_index_end = bounds->col_index_start + bounds->region_intersect.size.width;
        break;
    case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP:
        bounds->row_index_start = diff_y;
        bounds->row_index_end = bounds->row_index_start + bounds->region_intersect.size.height;
        bounds->col_index_end = radius - diff_x;
        bounds->col_index_start = bounds->col_index_end - bounds->region_intersect.size.width;
        break;
    case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM:
        bounds->row_index_end = radius - diff_y;
        bounds->row_index_start = bounds->row_index_end - bounds->region_intersect.size.height;
        bounds->col_index_end = radius - diff_x;
        bounds->col_index_start = bounds->col_index_end - bounds->region_intersect.size.width;
        break;
    }

    bounds->sign_x = egui_canvas_circle_corner_is_left(type) ? -1 : 1;
    bounds->sign_y = egui_canvas_circle_corner_is_top(type) ? -1 : 1;
    return 1;
}

__EGUI_STATIC_INLINE__ void egui_canvas_draw_direct_row_span(egui_color_t *dst_row, egui_dim_t pfb_ofs_x, egui_dim_t clip_x_start, egui_dim_t clip_x_end,
                                                             egui_dim_t seg_x_start, egui_dim_t seg_x_end, egui_color_t color, egui_alpha_t alpha)
{
    if (alpha == 0)
    {
        return;
    }

    seg_x_start = EGUI_MAX(seg_x_start, clip_x_start);
    seg_x_end = EGUI_MIN(seg_x_end, clip_x_end);
    if (seg_x_start >= seg_x_end)
    {
        return;
    }

    if (alpha == EGUI_ALPHA_100)
    {
        egui_canvas_fill_color_buffer((egui_color_int_t *)&dst_row[seg_x_start - pfb_ofs_x], seg_x_end - seg_x_start, color);
    }
    else
    {
        egui_canvas_blend_color_buffer_alpha((egui_color_int_t *)&dst_row[seg_x_start - pfb_ofs_x], seg_x_end - seg_x_start, color, alpha);
    }
}

__EGUI_STATIC_INLINE__ void egui_canvas_draw_direct_pixel(egui_color_t *dst, egui_color_t color, egui_alpha_t alpha)
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

__EGUI_STATIC_INLINE__ void egui_canvas_draw_direct_vertical_span(egui_color_t *pfb, egui_dim_t pfb_width, egui_dim_t pfb_ofs_x, egui_dim_t pfb_ofs_y,
                                                                  egui_dim_t clip_x_start, egui_dim_t clip_x_end, egui_dim_t clip_y_start,
                                                                  egui_dim_t clip_y_end, egui_dim_t screen_x, egui_dim_t seg_y_start, egui_dim_t seg_y_end,
                                                                  egui_color_t color, egui_alpha_t alpha)
{
    egui_dim_t local_x;

    if (alpha == 0)
    {
        return;
    }

    if (screen_x < clip_x_start || screen_x >= clip_x_end)
    {
        return;
    }

    seg_y_start = EGUI_MAX(seg_y_start, clip_y_start);
    seg_y_end = EGUI_MIN(seg_y_end, clip_y_end);
    if (seg_y_start >= seg_y_end)
    {
        return;
    }

    local_x = screen_x - pfb_ofs_x;
    if (alpha == EGUI_ALPHA_100)
    {
        for (egui_dim_t screen_y = seg_y_start; screen_y < seg_y_end; screen_y++)
        {
            pfb[(screen_y - pfb_ofs_y) * pfb_width + local_x] = color;
        }
    }
    else
    {
        for (egui_dim_t screen_y = seg_y_start; screen_y < seg_y_end; screen_y++)
        {
            egui_color_t *dst = &pfb[(screen_y - pfb_ofs_y) * pfb_width + local_x];
            egui_rgb_mix_ptr(dst, &color, dst, alpha);
        }
    }
}

__EGUI_STATIC_INLINE__ void egui_canvas_draw_direct_rect(egui_color_t *pfb, egui_dim_t pfb_width, egui_dim_t pfb_ofs_x, egui_dim_t pfb_ofs_y,
                                                         egui_dim_t clip_x_start, egui_dim_t clip_x_end, egui_dim_t clip_y_start, egui_dim_t clip_y_end,
                                                         egui_dim_t rect_x, egui_dim_t rect_y, egui_dim_t rect_width, egui_dim_t rect_height,
                                                         egui_color_t color, egui_alpha_t alpha)
{
    egui_dim_t rect_y_start;
    egui_dim_t rect_y_end;

    if (alpha == 0 || rect_width <= 0 || rect_height <= 0)
    {
        return;
    }

    rect_y_start = EGUI_MAX(rect_y, clip_y_start);
    rect_y_end = EGUI_MIN(rect_y + rect_height, clip_y_end);
    if (rect_y_start >= rect_y_end)
    {
        return;
    }

    for (egui_dim_t screen_y = rect_y_start; screen_y < rect_y_end; screen_y++)
    {
        egui_color_t *dst_row = &pfb[(screen_y - pfb_ofs_y) * pfb_width];
        egui_canvas_draw_direct_row_span(dst_row, pfb_ofs_x, clip_x_start, clip_x_end, rect_x, rect_x + rect_width, color, alpha);
    }
}

static void egui_canvas_draw_circle_corner_stroke_edge_direct_row(egui_color_t *dst_row, egui_dim_t pfb_ofs_x, egui_dim_t clip_x_start, egui_dim_t clip_x_end,
                                                                  egui_dim_t center_x, egui_dim_t radius, int type, egui_dim_t row_index,
                                                                  egui_dim_t stroke_width, egui_dim_t col_start, egui_dim_t col_end,
                                                                  const egui_circle_info_t *info, const egui_circle_item_t *items,
                                                                  const egui_circle_info_t *info_inner, const egui_circle_item_t *items_inner,
                                                                  egui_color_t color, egui_alpha_t alpha, egui_alpha_t canvas_alpha)
{
    egui_dim_t seg_x_start;
    egui_dim_t seg_x_end;
    egui_dim_t col_index;
    egui_color_t *dst;
    egui_dim_t col_step;
    int check_inner = (row_index >= stroke_width);
    int apply_draw_alpha = (alpha != EGUI_ALPHA_100);
    int apply_canvas_alpha = (canvas_alpha != EGUI_ALPHA_100);

    egui_canvas_circle_corner_col_range_to_screen_x(center_x, radius, type, col_start, col_end, &seg_x_start, &seg_x_end);
    seg_x_start = EGUI_MAX(seg_x_start, clip_x_start);
    seg_x_end = EGUI_MIN(seg_x_end, clip_x_end);
    if (seg_x_start >= seg_x_end)
    {
        return;
    }

    col_index = egui_canvas_circle_corner_screen_x_to_col(center_x, radius, type, seg_x_start);
    col_step = egui_canvas_circle_corner_is_left(type) ? 1 : -1;
    dst = &dst_row[seg_x_start - pfb_ofs_x];

    for (egui_dim_t screen_x = seg_x_start; screen_x < seg_x_end; screen_x++, col_index += col_step, dst++)
    {
        egui_alpha_t mix_alpha = egui_canvas_get_circle_corner_value_fixed_row(row_index, col_index, info, items);

        if (mix_alpha == 0)
        {
            continue;
        }

        if (apply_draw_alpha)
        {
            mix_alpha = egui_color_alpha_mix(mix_alpha, alpha);
        }
        if (check_inner && col_index >= stroke_width)
        {
            egui_alpha_t alpha_inner =
                    egui_canvas_get_circle_corner_value_fixed_row(row_index - stroke_width, col_index - stroke_width, info_inner, items_inner);

            if (alpha_inner == EGUI_ALPHA_100)
            {
                continue;
            }

            if (alpha_inner != 0)
            {
                mix_alpha = egui_color_alpha_mix(mix_alpha, EGUI_ALPHA_100 - alpha_inner);
            }
        }

        if (apply_canvas_alpha)
        {
            mix_alpha = egui_color_alpha_mix(canvas_alpha, mix_alpha);
            if (mix_alpha == 0)
            {
                continue;
            }
        }

        if (mix_alpha == EGUI_ALPHA_100)
        {
            *dst = color;
        }
        else
        {
            egui_rgb_mix_ptr(dst, &color, dst, mix_alpha);
        }
    }
}

static void egui_canvas_draw_circle_corner_direct_stroke(egui_canvas_t *self, egui_dim_t center_x, egui_dim_t radius, egui_dim_t stroke_width, int type,
                                                         egui_color_t color, egui_alpha_t alpha, const egui_region_t *region,
                                                         const egui_region_t *region_intersect, const egui_circle_info_t *info,
                                                         const egui_circle_info_t *info_inner)
{
    const egui_circle_item_t *items = (const egui_circle_item_t *)info->items;
    const egui_circle_item_t *items_inner = (const egui_circle_item_t *)info_inner->items;
    egui_dim_t outer_item_count = (egui_dim_t)info->item_count;
    egui_dim_t inner_item_count = (egui_dim_t)info_inner->item_count;
    egui_dim_t clip_x_start = region_intersect->location.x;
    egui_dim_t clip_x_end = clip_x_start + region_intersect->size.width;
    egui_dim_t clip_y_end = region_intersect->location.y + region_intersect->size.height;
    egui_dim_t pfb_width = self->pfb_region.size.width;
    egui_dim_t pfb_ofs_x = self->pfb_location_in_base_view.x;
    egui_dim_t pfb_ofs_y = self->pfb_location_in_base_view.y;
    egui_alpha_t solid_alpha = egui_color_alpha_mix(self->alpha, alpha);
    egui_dim_t outer_mirror_visible = outer_item_count;
    egui_dim_t outer_mirror_opaque = outer_item_count;
    egui_dim_t inner_mirror_visible = inner_item_count;
    egui_dim_t inner_mirror_opaque = inner_item_count;
    egui_dim_t prev_row_index = EGUI_DIM_MAX;
    egui_dim_t prev_inner_row_index = EGUI_DIM_MAX;
    int outer_state_ready = 0;
    int inner_state_ready = 0;

    if (solid_alpha == 0)
    {
        return;
    }

    for (egui_dim_t screen_y = region_intersect->location.y; screen_y < clip_y_end; screen_y++)
    {
        egui_dim_t row_index = egui_canvas_circle_corner_get_row_index(type, region, radius, screen_y);
        egui_dim_t outer_visible;
        egui_dim_t outer_opaque;
        egui_dim_t inner_visible = radius;
        egui_dim_t inner_opaque = radius;
        egui_dim_t end_col;
        egui_dim_t solid_col_start;
        egui_dim_t solid_col_end;
        egui_dim_t seg_x_start;
        egui_dim_t seg_x_end;
        egui_color_t *dst_row;
        egui_dim_t row_limit;

        if (!outer_state_ready)
        {
            while (outer_mirror_visible > 0 && row_index >= (egui_dim_t)items[outer_mirror_visible - 1].start_offset)
            {
                outer_mirror_visible--;
            }
            while (outer_mirror_opaque > 0 && row_index >= egui_canvas_circle_corner_get_opaque_threshold(items, outer_mirror_opaque - 1))
            {
                outer_mirror_opaque--;
            }
            outer_state_ready = 1;
        }
        else if (row_index > prev_row_index)
        {
            while (outer_mirror_visible > 0 && row_index >= (egui_dim_t)items[outer_mirror_visible - 1].start_offset)
            {
                outer_mirror_visible--;
            }
            while (outer_mirror_opaque > 0 && row_index >= egui_canvas_circle_corner_get_opaque_threshold(items, outer_mirror_opaque - 1))
            {
                outer_mirror_opaque--;
            }
        }
        else if (row_index < prev_row_index)
        {
            while (outer_mirror_visible < outer_item_count && row_index < (egui_dim_t)items[outer_mirror_visible].start_offset)
            {
                outer_mirror_visible++;
            }
            while (outer_mirror_opaque < outer_item_count && row_index < egui_canvas_circle_corner_get_opaque_threshold(items, outer_mirror_opaque))
            {
                outer_mirror_opaque++;
            }
        }
        prev_row_index = row_index;

        if (row_index < outer_item_count)
        {
            outer_visible = (egui_dim_t)items[row_index].start_offset;
            outer_opaque = egui_canvas_circle_corner_get_opaque_threshold(items, row_index);
        }
        else
        {
            outer_visible = outer_item_count;
            outer_opaque = row_index;
        }

        row_limit = EGUI_MIN(row_index, outer_item_count);
        if (outer_mirror_visible < row_limit)
        {
            outer_visible = EGUI_MIN(outer_visible, outer_mirror_visible);
        }
        if (outer_mirror_opaque < row_limit)
        {
            outer_opaque = EGUI_MIN(outer_opaque, outer_mirror_opaque);
        }

        if (outer_visible >= radius)
        {
            continue;
        }

        if (outer_opaque < outer_visible)
        {
            outer_opaque = outer_visible;
        }
        if (outer_opaque > radius)
        {
            outer_opaque = radius;
        }

        if (row_index >= stroke_width)
        {
            egui_dim_t inner_row_index = row_index - stroke_width;
            egui_dim_t inner_row_limit;

            if (!inner_state_ready)
            {
                while (inner_mirror_visible > 0 && inner_row_index >= (egui_dim_t)items_inner[inner_mirror_visible - 1].start_offset)
                {
                    inner_mirror_visible--;
                }
                while (inner_mirror_opaque > 0 && inner_row_index >= egui_canvas_circle_corner_get_opaque_threshold(items_inner, inner_mirror_opaque - 1))
                {
                    inner_mirror_opaque--;
                }
                inner_state_ready = 1;
            }
            else if (inner_row_index > prev_inner_row_index)
            {
                while (inner_mirror_visible > 0 && inner_row_index >= (egui_dim_t)items_inner[inner_mirror_visible - 1].start_offset)
                {
                    inner_mirror_visible--;
                }
                while (inner_mirror_opaque > 0 && inner_row_index >= egui_canvas_circle_corner_get_opaque_threshold(items_inner, inner_mirror_opaque - 1))
                {
                    inner_mirror_opaque--;
                }
            }
            else if (inner_row_index < prev_inner_row_index)
            {
                while (inner_mirror_visible < inner_item_count && inner_row_index < (egui_dim_t)items_inner[inner_mirror_visible].start_offset)
                {
                    inner_mirror_visible++;
                }
                while (inner_mirror_opaque < inner_item_count &&
                       inner_row_index < egui_canvas_circle_corner_get_opaque_threshold(items_inner, inner_mirror_opaque))
                {
                    inner_mirror_opaque++;
                }
            }
            prev_inner_row_index = inner_row_index;

            if (inner_row_index < inner_item_count)
            {
                inner_visible = stroke_width + (egui_dim_t)items_inner[inner_row_index].start_offset;
                inner_opaque = stroke_width + egui_canvas_circle_corner_get_opaque_threshold(items_inner, inner_row_index);
            }
            else
            {
                inner_visible = stroke_width + inner_item_count;
                inner_opaque = stroke_width + inner_row_index;
            }

            inner_row_limit = EGUI_MIN(inner_row_index, inner_item_count);
            if (inner_mirror_visible < inner_row_limit)
            {
                inner_visible = stroke_width + EGUI_MIN((egui_dim_t)(inner_visible - stroke_width), inner_mirror_visible);
            }
            if (inner_mirror_opaque < inner_row_limit)
            {
                inner_opaque = stroke_width + EGUI_MIN((egui_dim_t)(inner_opaque - stroke_width), inner_mirror_opaque);
            }

            if (inner_visible > radius)
            {
                inner_visible = radius;
            }
            if (inner_opaque > radius)
            {
                inner_opaque = radius;
            }
        }

        end_col = inner_opaque;
        if (end_col <= outer_visible)
        {
            continue;
        }

        solid_col_start = EGUI_MIN(outer_opaque, end_col);
        solid_col_end = EGUI_MIN(inner_visible, end_col);
        if (solid_col_end < solid_col_start)
        {
            solid_col_end = solid_col_start;
        }

        dst_row = (egui_color_t *)&self->pfb[(screen_y - pfb_ofs_y) * pfb_width];

        egui_canvas_draw_circle_corner_stroke_edge_direct_row(dst_row, pfb_ofs_x, clip_x_start, clip_x_end, center_x, radius, type, row_index, stroke_width,
                                                              outer_visible, solid_col_start, info, items, info_inner, items_inner, color, alpha, self->alpha);

        egui_canvas_circle_corner_col_range_to_screen_x(center_x, radius, type, solid_col_start, solid_col_end, &seg_x_start, &seg_x_end);
        egui_canvas_draw_direct_row_span(dst_row, pfb_ofs_x, clip_x_start, clip_x_end, seg_x_start, seg_x_end, color, solid_alpha);

        egui_canvas_draw_circle_corner_stroke_edge_direct_row(dst_row, pfb_ofs_x, clip_x_start, clip_x_end, center_x, radius, type, row_index, stroke_width,
                                                              solid_col_end, end_col, info, items, info_inner, items_inner, color, alpha, self->alpha);
    }
}

#if defined(EGUI_CONFIG_CIRCLE_FILL_BASIC) && EGUI_CONFIG_CIRCLE_FILL_BASIC
static void egui_canvas_draw_circle_corner_fill_direct(egui_canvas_t *self, egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, int type,
                                                       egui_color_t color, egui_alpha_t alpha, const egui_circle_info_t *info, egui_dim_t row_start,
                                                       egui_dim_t row_end, egui_dim_t col_start, egui_dim_t col_end, egui_dim_t iter_start, egui_dim_t iter_end,
                                                       const egui_region_t *region_intersect)
{
    egui_dim_t row_index;
    egui_dim_t col_index;
    egui_alpha_t mix_alpha;
    egui_dim_t sel_x;
    egui_dim_t sel_y;
    egui_dim_t len;
    int apply_draw_alpha = (alpha != EGUI_ALPHA_100);
    int apply_canvas_alpha = (self->alpha != EGUI_ALPHA_100);
    egui_dim_t clip_x_start = region_intersect->location.x;
    egui_dim_t clip_x_end = clip_x_start + region_intersect->size.width;
    egui_dim_t clip_y_start = region_intersect->location.y;
    egui_dim_t clip_y_end = clip_y_start + region_intersect->size.height;
    egui_color_t *pfb = (egui_color_t *)self->pfb;
    egui_dim_t pfb_width = self->pfb_region.size.width;
    egui_dim_t pfb_ofs_x = self->pfb_location_in_base_view.x;
    egui_dim_t pfb_ofs_y = self->pfb_location_in_base_view.y;
    egui_alpha_t solid_alpha = egui_color_alpha_mix(self->alpha, alpha);

    for (row_index = iter_start; row_index < iter_end; row_index++)
    {
        const egui_circle_item_t *ptr = &((const egui_circle_item_t *)info->items)[row_index];
        uint16_t start_offset = ptr->start_offset;
        uint16_t valid_count = ptr->valid_count;
        uint16_t data_value_offset = ptr->data_offset;

        sel_y = radius - row_index;

        int primary_visible = (row_index >= row_start && row_index < row_end);
        int mirror_visible = (row_index >= col_start && row_index < col_end);

        for (int i = 0; i < valid_count; i++)
        {
            col_index = start_offset + i;
            sel_x = radius - col_index;
            mix_alpha = info->data[data_value_offset + i];

            if (apply_draw_alpha)
            {
                mix_alpha = egui_color_alpha_mix(alpha, mix_alpha);
            }
            if (apply_canvas_alpha)
            {
                mix_alpha = egui_color_alpha_mix(self->alpha, mix_alpha);
                if (mix_alpha == 0)
                {
                    continue;
                }
            }

            if (primary_visible && col_index >= col_start && col_index < col_end)
            {
                egui_dim_t screen_x;
                egui_dim_t screen_y;

                switch (type)
                {
                case EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP:
                    screen_x = center_x + (-sel_x);
                    screen_y = center_y + (-sel_y);
                    break;
                case EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM:
                    screen_x = center_x + (-sel_x);
                    screen_y = center_y + (sel_y);
                    break;
                case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP:
                    screen_x = center_x + (sel_x);
                    screen_y = center_y + (-sel_y);
                    break;
                default:
                    screen_x = center_x + (sel_x);
                    screen_y = center_y + (sel_y);
                    break;
                }

                egui_canvas_draw_direct_pixel(&pfb[(screen_y - pfb_ofs_y) * pfb_width + (screen_x - pfb_ofs_x)], color, mix_alpha);
            }
            if (sel_x == sel_y)
            {
                continue;
            }
            if (mirror_visible && col_index >= row_start && col_index < row_end)
            {
                egui_dim_t screen_x;
                egui_dim_t screen_y;

                switch (type)
                {
                case EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP:
                    screen_x = center_x + (-sel_y);
                    screen_y = center_y + (-sel_x);
                    break;
                case EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM:
                    screen_x = center_x + (-sel_y);
                    screen_y = center_y + (sel_x);
                    break;
                case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP:
                    screen_x = center_x + (sel_y);
                    screen_y = center_y + (-sel_x);
                    break;
                default:
                    screen_x = center_x + (sel_y);
                    screen_y = center_y + (sel_x);
                    break;
                }

                egui_canvas_draw_direct_pixel(&pfb[(screen_y - pfb_ofs_y) * pfb_width + (screen_x - pfb_ofs_x)], color, mix_alpha);
            }
        }

        egui_dim_t offset = start_offset + valid_count;
        len = radius - offset;
        if (len > 0)
        {
            if (primary_visible)
            {
                egui_dim_t screen_x;
                egui_dim_t screen_y;
                egui_color_t *dst_row;

                switch (type)
                {
                case EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP:
                    screen_x = center_x + (-(radius - offset));
                    screen_y = center_y + (-sel_y);
                    break;
                case EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM:
                    screen_x = center_x + (-(radius - offset));
                    screen_y = center_y + (sel_y);
                    break;
                case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP:
                    screen_x = center_x + 1;
                    screen_y = center_y + (-sel_y);
                    break;
                default:
                    screen_x = center_x + 1;
                    screen_y = center_y + (sel_y);
                    break;
                }

                dst_row = &pfb[(screen_y - pfb_ofs_y) * pfb_width];
                egui_canvas_draw_direct_row_span(dst_row, pfb_ofs_x, clip_x_start, clip_x_end, screen_x, screen_x + len, color, solid_alpha);
            }
            if (mirror_visible)
            {
                egui_dim_t screen_x;
                egui_dim_t screen_y;

                switch (type)
                {
                case EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP:
                    screen_x = center_x + (-sel_y);
                    screen_y = center_y + (-(radius - offset));
                    break;
                case EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM:
                    screen_x = center_x + (-sel_y);
                    screen_y = center_y + 1;
                    break;
                case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP:
                    screen_x = center_x + (sel_y);
                    screen_y = center_y + (-(radius - offset));
                    break;
                default:
                    screen_x = center_x + (sel_y);
                    screen_y = center_y + 1;
                    break;
                }

                egui_canvas_draw_direct_vertical_span(pfb, pfb_width, pfb_ofs_x, pfb_ofs_y, clip_x_start, clip_x_end, clip_y_start, clip_y_end, screen_x,
                                                      screen_y, screen_y + len, color, solid_alpha);
            }
        }
    }

    len = radius - info->item_count;
    if (len > 0)
    {
        egui_dim_t rect_x;
        egui_dim_t rect_y;

        switch (type)
        {
        case EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP:
            rect_x = center_x + (-len);
            rect_y = center_y + (-len);
            break;
        case EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM:
            rect_x = center_x + (-len);
            rect_y = center_y + 1;
            break;
        case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP:
            rect_x = center_x + 1;
            rect_y = center_y + (-len);
            break;
        default:
            rect_x = center_x + 1;
            rect_y = center_y + 1;
            break;
        }

        egui_canvas_draw_direct_rect(pfb, pfb_width, pfb_ofs_x, pfb_ofs_y, clip_x_start, clip_x_end, clip_y_start, clip_y_end, rect_x, rect_y, len, len, color,
                                     solid_alpha);
    }
}
#endif

void egui_canvas_draw_circle_corner_fill(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, int type, egui_color_t color, egui_alpha_t alpha)
{
    egui_canvas_t *self = &canvas_data;

    egui_dim_t row_index;
    egui_dim_t col_index;
    egui_alpha_t mix_alpha;
    egui_dim_t sel_x;
    egui_dim_t sel_y;
    egui_dim_t len;

    if (radius <= 0)
    {
        return;
    }

    // only work within intersection of base_view_work_region and the rectangle to be drawn
    egui_canvas_corner_bounds_t bounds;

    egui_canvas_circle_corner_init_region(&bounds, center_x, center_y, radius, type);
    if (!egui_canvas_circle_corner_get_bounds(self, radius, type, &bounds))
    {
        return;
    }
    const egui_circle_info_t *info = egui_canvas_get_circle_item(radius);
    if (info == NULL)
    {
        EGUI_LOG_WRN("Circle radius %d not supported, increase EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE or register spec circle.\n", radius);
        return;
    }

#if !defined(EGUI_CONFIG_CIRCLE_FILL_BASIC) || !EGUI_CONFIG_CIRCLE_FILL_BASIC
    // Legacy corner-fill path: smaller code size, slower on fill-heavy benchmarks.
    egui_dim_t row_start = bounds.row_index_start;
    egui_dim_t row_end = bounds.row_index_end;
    egui_dim_t col_start = bounds.col_index_start;
    egui_dim_t col_end = bounds.col_index_end;
    int apply_draw_alpha = (alpha != EGUI_ALPHA_100);

    egui_dim_t iter_start = EGUI_MAX(EGUI_MIN(row_start, col_start), 0);
    egui_dim_t iter_end = EGUI_MIN(EGUI_MAX(row_end, col_end), (egui_dim_t)info->item_count);

    for (row_index = iter_start; row_index < iter_end; row_index++)
    {
        const egui_circle_item_t *ptr = &((const egui_circle_item_t *)info->items)[row_index];
        uint16_t start_offset = ptr->start_offset;
        uint16_t valid_count = ptr->valid_count;
        uint16_t data_value_offset = ptr->data_offset;

        sel_y = radius - row_index;

        // Primary row visible: row_index acts as y-offset in corner coords
        int primary_visible = (row_index >= row_start && row_index < row_end);
        // Mirror column visible: row_index acts as x-offset in corner coords (mirrored)
        int mirror_visible = (row_index >= col_start && row_index < col_end);

        // Edge pixels
        for (int i = 0; i < valid_count; i++)
        {
            col_index = start_offset + i;
            sel_x = radius - col_index;
            mix_alpha = info->data[data_value_offset + i];

            if (apply_draw_alpha)
            {
                mix_alpha = egui_color_alpha_mix(alpha, mix_alpha);
            }

            // Primary pixel: visible when row_index in Y-range AND col_index in X-range
            if (primary_visible && col_index >= col_start && col_index < col_end)
            {
                switch (type)
                {
                case EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP:
                    egui_canvas_draw_point_limit(center_x + (-sel_x), center_y + (-sel_y), color, mix_alpha);
                    break;
                case EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM:
                    egui_canvas_draw_point_limit(center_x + (-sel_x), center_y + (sel_y), color, mix_alpha);
                    break;
                case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP:
                    egui_canvas_draw_point_limit(center_x + (sel_x), center_y + (-sel_y), color, mix_alpha);
                    break;
                case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM:
                    egui_canvas_draw_point_limit(center_x + (sel_x), center_y + (sel_y), color, mix_alpha);
                    break;
                }
            }
            // skip the diagonal write twice
            if (sel_x == sel_y)
            {
                continue;
            }
            // Mirror pixel: visible when row_index in X-range AND col_index in Y-range
            if (mirror_visible && col_index >= row_start && col_index < row_end)
            {
                switch (type)
                {
                case EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP:
                    egui_canvas_draw_point_limit(center_x + (-sel_y), center_y + (-sel_x), color, mix_alpha);
                    break;
                case EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM:
                    egui_canvas_draw_point_limit(center_x + (-sel_y), center_y + (sel_x), color, mix_alpha);
                    break;
                case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP:
                    egui_canvas_draw_point_limit(center_x + (sel_y), center_y + (-sel_x), color, mix_alpha);
                    break;
                case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM:
                    egui_canvas_draw_point_limit(center_x + (sel_y), center_y + (sel_x), color, mix_alpha);
                    break;
                }
            }
        }

        // write the reserve value to reserve the space, here can use write line or write column
        {
            egui_dim_t offset = start_offset + valid_count;
            len = radius - offset;
        }
        if (len > 0)
        {
            // hline: at primary y position, visible when row_index in Y-range
            if (primary_visible)
            {
                switch (type)
                {
                case EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP:
                    egui_canvas_draw_hline(center_x + (-(radius - (start_offset + valid_count))), center_y + (-sel_y), len, color, alpha);
                    break;
                case EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM:
                    egui_canvas_draw_hline(center_x + (-(radius - (start_offset + valid_count))), center_y + (sel_y), len, color, alpha);
                    break;
                case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP:
                    egui_canvas_draw_hline(center_x + 1, center_y + (-sel_y), len, color, alpha);
                    break;
                case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM:
                    egui_canvas_draw_hline(center_x + 1, center_y + (sel_y), len, color, alpha);
                    break;
                }
            }
            // vline: at mirror x position, visible when row_index in X-range
            if (mirror_visible)
            {
                switch (type)
                {
                case EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP:
                    egui_canvas_draw_vline(center_x + (-sel_y), center_y + (-(radius - (start_offset + valid_count))), len, color, alpha);
                    break;
                case EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM:
                    egui_canvas_draw_vline(center_x + (-sel_y), center_y + 1, len, color, alpha);
                    break;
                case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP:
                    egui_canvas_draw_vline(center_x + (sel_y), center_y + (-(radius - (start_offset + valid_count))), len, color, alpha);
                    break;
                case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM:
                    egui_canvas_draw_vline(center_x + (sel_y), center_y + 1, len, color, alpha);
                    break;
                }
            }
        }
    }

    // write reserve value with rect
    len = radius - info->item_count;
    if (len > 0)
    {
        switch (type)
        {
        case EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP:
            egui_canvas_draw_fillrect(center_x + (-len), center_y + (-len), len, len, color, alpha);
            break;
        case EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM:
            egui_canvas_draw_fillrect(center_x + (-len), center_y + 1, len, len, color, alpha);
            break;
        case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP:
            egui_canvas_draw_fillrect(center_x + 1, center_y + (-len), len, len, color, alpha);
            break;
        case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM:
            egui_canvas_draw_fillrect(center_x + 1, center_y + 1, len, len, color, alpha);
            break;
        }
    }
    return;
#else
    egui_region_t region_intersect = bounds.region_intersect;

    // Compute visible row/col ranges from intersection (same as circle_corner outline)
    egui_dim_t row_start = bounds.row_index_start;
    egui_dim_t row_end = bounds.row_index_end;
    egui_dim_t col_start = bounds.col_index_start;
    egui_dim_t col_end = bounds.col_index_end;

    // Effective iteration range: union of rows producing visible primary [row_start, row_end)
    // and rows producing visible mirror output [col_start, col_end)
    egui_dim_t iter_start = EGUI_MAX(EGUI_MIN(row_start, col_start), 0);
    egui_dim_t iter_end = EGUI_MIN(EGUI_MAX(row_end, col_end), (egui_dim_t)info->item_count);
    int apply_draw_alpha = (alpha != EGUI_ALPHA_100);

    if (self->mask == NULL && region_intersect.size.width > 1)
    {
        egui_canvas_draw_circle_corner_fill_direct(self, center_x, center_y, radius, type, color, alpha, info, row_start, row_end, col_start, col_end,
                                                   iter_start, iter_end, &region_intersect);
        return;
    }

    const int apply_canvas_alpha = 0;
    const int use_direct_pfb = 0;
    egui_dim_t clip_x_start = 0;
    egui_dim_t clip_x_end = 0;
    egui_dim_t clip_y_start = 0;
    egui_dim_t clip_y_end = 0;
    egui_color_t *pfb = NULL;
    egui_dim_t pfb_width = 0;
    egui_dim_t pfb_ofs_x = 0;
    egui_dim_t pfb_ofs_y = 0;
    egui_alpha_t solid_alpha = 0;

    for (row_index = iter_start; row_index < iter_end; row_index++)
    {
        const egui_circle_item_t *ptr = &((const egui_circle_item_t *)info->items)[row_index];
        uint16_t start_offset = ptr->start_offset;
        uint16_t valid_count = ptr->valid_count;
        uint16_t data_value_offset = ptr->data_offset;

        sel_y = radius - row_index;

        // Primary row visible: row_index acts as y-offset in corner coords
        int primary_visible = (row_index >= row_start && row_index < row_end);
        // Mirror column visible: row_index acts as x-offset in corner coords (mirrored)
        int mirror_visible = (row_index >= col_start && row_index < col_end);

        // Edge pixels
        for (int i = 0; i < valid_count; i++)
        {
            col_index = start_offset + i;
            sel_x = radius - col_index;
            mix_alpha = info->data[data_value_offset + i];

            if (apply_draw_alpha)
            {
                mix_alpha = egui_color_alpha_mix(alpha, mix_alpha);
            }

            if (use_direct_pfb && apply_canvas_alpha)
            {
                mix_alpha = egui_color_alpha_mix(self->alpha, mix_alpha);
                if (mix_alpha == 0)
                {
                    continue;
                }
            }

            // Primary pixel: visible when row_index in Y-range AND col_index in X-range
            if (primary_visible && col_index >= col_start && col_index < col_end)
            {
                if (use_direct_pfb)
                {
                    egui_dim_t screen_x;
                    egui_dim_t screen_y;

                    switch (type)
                    {
                    case EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP:
                        screen_x = center_x + (-sel_x);
                        screen_y = center_y + (-sel_y);
                        break;
                    case EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM:
                        screen_x = center_x + (-sel_x);
                        screen_y = center_y + (sel_y);
                        break;
                    case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP:
                        screen_x = center_x + (sel_x);
                        screen_y = center_y + (-sel_y);
                        break;
                    default:
                        screen_x = center_x + (sel_x);
                        screen_y = center_y + (sel_y);
                        break;
                    }

                    egui_canvas_draw_direct_pixel(&pfb[(screen_y - pfb_ofs_y) * pfb_width + (screen_x - pfb_ofs_x)], color, mix_alpha);
                }
                else
                {
                    switch (type)
                    {
                    case EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP:
                        egui_canvas_draw_point_limit(center_x + (-sel_x), center_y + (-sel_y), color, mix_alpha);
                        break;
                    case EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM:
                        egui_canvas_draw_point_limit(center_x + (-sel_x), center_y + (sel_y), color, mix_alpha);
                        break;
                    case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP:
                        egui_canvas_draw_point_limit(center_x + (sel_x), center_y + (-sel_y), color, mix_alpha);
                        break;
                    case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM:
                        egui_canvas_draw_point_limit(center_x + (sel_x), center_y + (sel_y), color, mix_alpha);
                        break;
                    }
                }
            }
            // skip the diagonal write twice
            if (sel_x == sel_y)
            {
                continue;
            }
            // Mirror pixel: visible when row_index in X-range AND col_index in Y-range
            if (mirror_visible && col_index >= row_start && col_index < row_end)
            {
                if (use_direct_pfb)
                {
                    egui_dim_t screen_x;
                    egui_dim_t screen_y;

                    switch (type)
                    {
                    case EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP:
                        screen_x = center_x + (-sel_y);
                        screen_y = center_y + (-sel_x);
                        break;
                    case EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM:
                        screen_x = center_x + (-sel_y);
                        screen_y = center_y + (sel_x);
                        break;
                    case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP:
                        screen_x = center_x + (sel_y);
                        screen_y = center_y + (-sel_x);
                        break;
                    default:
                        screen_x = center_x + (sel_y);
                        screen_y = center_y + (sel_x);
                        break;
                    }

                    egui_canvas_draw_direct_pixel(&pfb[(screen_y - pfb_ofs_y) * pfb_width + (screen_x - pfb_ofs_x)], color, mix_alpha);
                }
                else
                {
                    switch (type)
                    {
                    case EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP:
                        egui_canvas_draw_point_limit(center_x + (-sel_y), center_y + (-sel_x), color, mix_alpha);
                        break;
                    case EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM:
                        egui_canvas_draw_point_limit(center_x + (-sel_y), center_y + (sel_x), color, mix_alpha);
                        break;
                    case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP:
                        egui_canvas_draw_point_limit(center_x + (sel_y), center_y + (-sel_x), color, mix_alpha);
                        break;
                    case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM:
                        egui_canvas_draw_point_limit(center_x + (sel_y), center_y + (sel_x), color, mix_alpha);
                        break;
                    }
                }
            }
        }

        // write the reserve value to reserve the space, here can use write line or write column
        egui_dim_t offset = start_offset + valid_count;
        len = radius - offset;
        if (len > 0)
        {
            // hline: at primary y position, visible when row_index in Y-range
            if (primary_visible)
            {
                if (use_direct_pfb)
                {
                    egui_dim_t screen_x;
                    egui_dim_t screen_y;
                    egui_color_t *dst_row;

                    switch (type)
                    {
                    case EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP:
                        screen_x = center_x + (-(radius - offset));
                        screen_y = center_y + (-sel_y);
                        break;
                    case EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM:
                        screen_x = center_x + (-(radius - offset));
                        screen_y = center_y + (sel_y);
                        break;
                    case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP:
                        screen_x = center_x + 1;
                        screen_y = center_y + (-sel_y);
                        break;
                    default:
                        screen_x = center_x + 1;
                        screen_y = center_y + (sel_y);
                        break;
                    }

                    dst_row = &pfb[(screen_y - pfb_ofs_y) * pfb_width];
                    egui_canvas_draw_direct_row_span(dst_row, pfb_ofs_x, clip_x_start, clip_x_end, screen_x, screen_x + len, color, solid_alpha);
                }
                else
                {
                    switch (type)
                    {
                    case EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP:
                        egui_canvas_draw_hline(center_x + (-(radius - offset)), center_y + (-sel_y), len, color, alpha);
                        break;
                    case EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM:
                        egui_canvas_draw_hline(center_x + (-(radius - offset)), center_y + (sel_y), len, color, alpha);
                        break;
                    case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP:
                        egui_canvas_draw_hline(center_x + 1, center_y + (-sel_y), len, color, alpha);
                        break;
                    case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM:
                        egui_canvas_draw_hline(center_x + 1, center_y + (sel_y), len, color, alpha);
                        break;
                    }
                }
            }
            // vline: at mirror x position, visible when row_index in X-range
            if (mirror_visible)
            {
                if (use_direct_pfb)
                {
                    egui_dim_t screen_x;
                    egui_dim_t screen_y;

                    switch (type)
                    {
                    case EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP:
                        screen_x = center_x + (-sel_y);
                        screen_y = center_y + (-(radius - offset));
                        break;
                    case EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM:
                        screen_x = center_x + (-sel_y);
                        screen_y = center_y + 1;
                        break;
                    case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP:
                        screen_x = center_x + (sel_y);
                        screen_y = center_y + (-(radius - offset));
                        break;
                    default:
                        screen_x = center_x + (sel_y);
                        screen_y = center_y + 1;
                        break;
                    }

                    egui_canvas_draw_direct_vertical_span(pfb, pfb_width, pfb_ofs_x, pfb_ofs_y, clip_x_start, clip_x_end, clip_y_start, clip_y_end, screen_x,
                                                          screen_y, screen_y + len, color, solid_alpha);
                }
                else
                {
                    switch (type)
                    {
                    case EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP:
                        egui_canvas_draw_vline(center_x + (-sel_y), center_y + (-(radius - offset)), len, color, alpha);
                        break;
                    case EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM:
                        egui_canvas_draw_vline(center_x + (-sel_y), center_y + 1, len, color, alpha);
                        break;
                    case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP:
                        egui_canvas_draw_vline(center_x + (sel_y), center_y + (-(radius - offset)), len, color, alpha);
                        break;
                    case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM:
                        egui_canvas_draw_vline(center_x + (sel_y), center_y + 1, len, color, alpha);
                        break;
                    }
                }
            }
        }
    }

    // write reserve value with rect
    len = radius - info->item_count;
    if (len > 0)
    {
        if (use_direct_pfb)
        {
            egui_dim_t rect_x;
            egui_dim_t rect_y;

            switch (type)
            {
            case EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP:
                rect_x = center_x + (-len);
                rect_y = center_y + (-len);
                break;
            case EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM:
                rect_x = center_x + (-len);
                rect_y = center_y + 1;
                break;
            case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP:
                rect_x = center_x + 1;
                rect_y = center_y + (-len);
                break;
            default:
                rect_x = center_x + 1;
                rect_y = center_y + 1;
                break;
            }

            egui_canvas_draw_direct_rect(pfb, pfb_width, pfb_ofs_x, pfb_ofs_y, clip_x_start, clip_x_end, clip_y_start, clip_y_end, rect_x, rect_y, len, len,
                                         color, solid_alpha);
        }
        else
        {
            switch (type)
            {
            case EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP:
                egui_canvas_draw_fillrect(center_x + (-len), center_y + (-len), len, len, color, alpha);
                break;
            case EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM:
                egui_canvas_draw_fillrect(center_x + (-len), center_y + 1, len, len, color, alpha);
                break;
            case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP:
                egui_canvas_draw_fillrect(center_x + 1, center_y + (-len), len, len, color, alpha);
                break;
            case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM:
                egui_canvas_draw_fillrect(center_x + 1, center_y + 1, len, len, color, alpha);
                break;
            }
        }
    }
#endif
}

#if defined(EGUI_CONFIG_CIRCLE_FILL_BASIC) && EGUI_CONFIG_CIRCLE_FILL_BASIC
__EGUI_STATIC_INLINE__ void egui_canvas_draw_circle_fill_basic_edge_direct_row(egui_color_t *dst_row, egui_dim_t pfb_ofs_x, egui_dim_t clip_x_start,
                                                                               egui_dim_t clip_x_end, egui_dim_t base_x, egui_dim_t screen_x_start,
                                                                               egui_dim_t screen_x_end, egui_dim_t row_index, egui_dim_t total_width,
                                                                               int mirrored, const egui_circle_info_t *info, const egui_circle_item_t *items,
                                                                               egui_color_t color, egui_alpha_t alpha)
{
    egui_color_t *dst;
    egui_dim_t col_index;
    egui_dim_t col_step;
    int apply_alpha;

    if (alpha == 0)
    {
        return;
    }

    screen_x_start = EGUI_MAX(screen_x_start, clip_x_start);
    screen_x_end = EGUI_MIN(screen_x_end, clip_x_end);
    if (screen_x_start >= screen_x_end)
    {
        return;
    }

    dst = &dst_row[screen_x_start - pfb_ofs_x];
    col_index = mirrored ? (total_width - 1 - (screen_x_start - base_x)) : (screen_x_start - base_x);
    col_step = mirrored ? -1 : 1;
    apply_alpha = (alpha != EGUI_ALPHA_100);

    for (egui_dim_t screen_x = screen_x_start; screen_x < screen_x_end; screen_x++, col_index += col_step, dst++)
    {
        egui_alpha_t pixel_alpha = egui_canvas_get_circle_corner_value_fixed_row(row_index, col_index, info, items);

        if (apply_alpha)
        {
            pixel_alpha = egui_color_alpha_mix(alpha, pixel_alpha);
        }

        egui_canvas_draw_direct_pixel(dst, color, pixel_alpha);
    }
}

__EGUI_STATIC_INLINE__ void egui_canvas_draw_circle_fill_basic_edge_row(egui_dim_t screen_y, egui_dim_t clip_x_start, egui_dim_t clip_x_end, egui_dim_t base_x,
                                                                        egui_dim_t screen_x_start, egui_dim_t screen_x_end, egui_dim_t row_index,
                                                                        egui_dim_t total_width, int mirrored, const egui_circle_info_t *info,
                                                                        const egui_circle_item_t *items, egui_color_t color, egui_alpha_t alpha)
{
    egui_dim_t col_index;
    egui_dim_t col_step;
    int apply_alpha;

    screen_x_start = EGUI_MAX(screen_x_start, clip_x_start);
    screen_x_end = EGUI_MIN(screen_x_end, clip_x_end);
    if (screen_x_start >= screen_x_end)
    {
        return;
    }

    col_index = mirrored ? (total_width - 1 - (screen_x_start - base_x)) : (screen_x_start - base_x);
    col_step = mirrored ? -1 : 1;
    apply_alpha = (alpha != EGUI_ALPHA_100);

    for (egui_dim_t screen_x = screen_x_start; screen_x < screen_x_end; screen_x++, col_index += col_step)
    {
        egui_alpha_t pixel_alpha = egui_canvas_get_circle_corner_value_fixed_row(row_index, col_index, info, items);

        if (apply_alpha)
        {
            pixel_alpha = egui_color_alpha_mix(alpha, pixel_alpha);
        }
        if (pixel_alpha != 0)
        {
            egui_canvas_draw_point_limit(screen_x, screen_y, color, pixel_alpha);
        }
    }
}

__EGUI_STATIC_INLINE__ void egui_canvas_get_circle_fill_basic_row_layout_fast(egui_dim_t radius, egui_dim_t row_index, const egui_circle_info_t *info,
                                                                              const egui_circle_item_t *items, egui_dim_t *start_offset, egui_dim_t *fill_start,
                                                                              egui_dim_t *fill_end)
{
    egui_dim_t visible_boundary = egui_canvas_circle_corner_get_visible_boundary(row_index, info, items);
    egui_dim_t opaque_boundary = egui_canvas_circle_corner_get_opaque_boundary(row_index, info, items);

    if (opaque_boundary < visible_boundary)
    {
        opaque_boundary = visible_boundary;
    }
    if (opaque_boundary > radius)
    {
        opaque_boundary = radius;
    }

    if (start_offset != NULL)
    {
        *start_offset = visible_boundary;
    }

    if (fill_start != NULL)
    {
        *fill_start = opaque_boundary;
    }

    if (fill_end != NULL)
    {
        *fill_end = ((radius << 1) + 1) - opaque_boundary;
    }
}
#endif

static void egui_canvas_draw_circle_fill_basic_legacy(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, egui_color_t color, egui_alpha_t alpha)
{
    egui_canvas_draw_circle_corner_fill(center_x, center_y, radius, EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP, color, alpha);
    egui_canvas_draw_circle_corner_fill(center_x, center_y, radius, EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM, color, alpha);
    egui_canvas_draw_circle_corner_fill(center_x, center_y, radius, EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP, color, alpha);
    egui_canvas_draw_circle_corner_fill(center_x, center_y, radius, EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM, color, alpha);

    egui_canvas_draw_hline(center_x - radius, center_y, radius, color, alpha);
    egui_canvas_draw_hline(center_x + 1, center_y, radius, color, alpha);
    egui_canvas_draw_vline(center_x, center_y - radius, (radius << 1) + 1, color, alpha);
}

/* egui_canvas_get_circle_corner_value moved to egui_canvas.h for reuse by gradient code */

int egui_canvas_get_circle_left_top(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, egui_dim_t x, egui_dim_t y, egui_alpha_t *alpha)
{
    egui_canvas_t *self = &canvas_data;

    egui_dim_t row_index;
    egui_dim_t col_index;
    // int info_type;
    // int info_type_inner;
    egui_alpha_t mix_alpha;
    egui_dim_t sel_x;
    egui_dim_t sel_y;
    // egui_dim_t len;
    // uint16_t start_offset;
    // uint16_t valid_count;
    // uint16_t data_value_offset;
    EGUI_UNUSED(self);

    // only work within intersection of base_view_work_region and the rectangle to be drawn
    EGUI_REGION_DEFINE(region, center_x - radius, center_y - radius, radius, radius);
    if (!egui_region_pt_in_rect(&region, x, y))
    {
        return 0;
    }

    const egui_circle_info_t *info = egui_canvas_get_circle_item(radius);
    if (info == NULL)
    {
        EGUI_LOG_WRN("Circle radius %d not supported, increase EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE or register spec circle.\n", radius);
        return 0;
    }

    sel_x = (-x) + center_x;
    sel_y = (-y) + center_y;

    col_index = radius - sel_x;
    row_index = radius - sel_y;

    // get the mix alpha value
    mix_alpha = egui_canvas_get_circle_corner_value(row_index, col_index, info);
    if (mix_alpha == 0)
    {
        *alpha = 0;
        return 1;
    }

    *alpha = egui_color_alpha_mix(mix_alpha, *alpha);

    return 1;
}

int egui_canvas_get_circle_left_bottom(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, egui_dim_t x, egui_dim_t y, egui_alpha_t *alpha)
{
    // egui_canvas_t *self = &canvas_data;

    egui_dim_t row_index;
    egui_dim_t col_index;
    // int info_type_inner;
    egui_alpha_t mix_alpha;
    egui_dim_t sel_x;
    egui_dim_t sel_y;
    // egui_dim_t len;
    // uint16_t start_offset;
    // uint16_t valid_count;
    // uint16_t data_value_offset;

    // only work within intersection of base_view_work_region and the rectangle to be drawn
    EGUI_REGION_DEFINE(region, center_x - radius, center_y + 1, radius, radius);
    if (!egui_region_pt_in_rect(&region, x, y))
    {
        return 0;
    }

    const egui_circle_info_t *info = egui_canvas_get_circle_item(radius);
    if (info == NULL)
    {
        EGUI_LOG_WRN("Circle radius %d not supported, increase EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE or register spec circle.\n", radius);
        return 0;
    }

    sel_x = (-x) + center_x;
    sel_y = y - center_y;

    col_index = radius - sel_x;
    row_index = radius - sel_y;

    // get the mix alpha value
    mix_alpha = egui_canvas_get_circle_corner_value(row_index, col_index, info);
    if (mix_alpha == 0)
    {
        *alpha = 0;
        return 1;
    }

    *alpha = egui_color_alpha_mix(mix_alpha, *alpha);

    return 1;
}

int egui_canvas_get_circle_right_top(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, egui_dim_t x, egui_dim_t y, egui_alpha_t *alpha)
{
    // egui_canvas_t *self = &canvas_data;

    egui_dim_t row_index;
    egui_dim_t col_index;
    // int info_type;
    //  int info_type_inner;
    egui_alpha_t mix_alpha;
    egui_dim_t sel_x;
    egui_dim_t sel_y;
    // egui_dim_t len;
    // uint16_t start_offset;
    // uint16_t valid_count;
    // uint16_t data_value_offset;

    // only work within intersection of base_view_work_region and the rectangle to be drawn
    EGUI_REGION_DEFINE(region, center_x + 1, center_y - radius, radius, radius);
    if (!egui_region_pt_in_rect(&region, x, y))
    {
        return 0;
    }

    const egui_circle_info_t *info = egui_canvas_get_circle_item(radius);
    if (info == NULL)
    {
        EGUI_LOG_WRN("Circle radius %d not supported, increase EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE or register spec circle.\n", radius);
        return 0;
    }

    sel_x = x - center_x;
    sel_y = (-y) + center_y;

    col_index = radius - sel_x;
    row_index = radius - sel_y;

    // get the mix alpha value
    mix_alpha = egui_canvas_get_circle_corner_value(row_index, col_index, info);
    if (mix_alpha == 0)
    {
        *alpha = 0;
        return 1;
    }

    *alpha = egui_color_alpha_mix(mix_alpha, *alpha);

    return 1;
}

int egui_canvas_get_circle_right_bottom(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, egui_dim_t x, egui_dim_t y, egui_alpha_t *alpha)
{
    // egui_canvas_t *self = &canvas_data;

    egui_dim_t row_index;
    egui_dim_t col_index;
    // int info_type;
    // int info_type_inner;
    egui_alpha_t mix_alpha;
    egui_dim_t sel_x;
    egui_dim_t sel_y;
    // egui_dim_t len;
    // uint16_t start_offset;
    // uint16_t valid_count;
    // uint16_t data_value_offset;

    // only work within intersection of base_view_work_region and the rectangle to be drawn
    EGUI_REGION_DEFINE(region, center_x + 1, center_y + 1, radius, radius);
    if (!egui_region_pt_in_rect(&region, x, y))
    {
        return 0;
    }

    const egui_circle_info_t *info = egui_canvas_get_circle_item(radius);
    if (info == NULL)
    {
        EGUI_LOG_WRN("Circle radius %d not supported, increase EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE or register spec circle.\n", radius);
        return 0;
    }

    sel_x = x - center_x;
    sel_y = y - center_y;

    col_index = radius - sel_x;
    row_index = radius - sel_y;

    // get the mix alpha value
    mix_alpha = egui_canvas_get_circle_corner_value(row_index, col_index, info);
    if (mix_alpha == 0)
    {
        *alpha = 0;
        return 1;
    }

    *alpha = egui_color_alpha_mix(mix_alpha, *alpha);

    return 1;
}

void egui_canvas_draw_circle_corner(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, egui_dim_t stroke_width, int type, egui_color_t color,
                                    egui_alpha_t alpha)
{
    egui_canvas_t *self = &canvas_data;

    egui_dim_t row_index;
    egui_dim_t col_index;
    egui_alpha_t mix_alpha;
    egui_dim_t sel_x;
    egui_dim_t sel_y;
    egui_alpha_t circle_alpha;

    if (radius <= 0 || stroke_width <= 0)
    {
        return;
    }

    // only work within intersection of base_view_work_region and the rectangle to be drawn
    egui_canvas_corner_bounds_t bounds;
    egui_region_t region;
    egui_region_t region_intersect;

    egui_canvas_circle_corner_init_region(&bounds, center_x, center_y, radius, type);
    if (!egui_canvas_circle_corner_get_bounds(self, radius, type, &bounds))
    {
        return;
    }
    region = bounds.region;
    region_intersect = bounds.region_intersect;

    // if radius <= stroke_width, draw a filled circle
    if (radius <= stroke_width)
    {
        egui_canvas_draw_circle_corner_fill(center_x, center_y, radius, type, color, alpha);
        return;
    }

    const egui_circle_info_t *info = egui_canvas_get_circle_item(radius);
    if (info == NULL)
    {
        EGUI_LOG_WRN("Circle radius %d not supported, increase EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE or register spec circle.\n", radius);
        return;
    }

    egui_dim_t radius_inner = radius - stroke_width;
    const egui_circle_info_t *info_inner = egui_canvas_get_circle_item(radius_inner);
    if (info_inner == NULL)
    {
        EGUI_LOG_WRN("Circle radius %d not supported, increase EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE or register spec circle.\n", radius_inner);
        return;
    }
    const egui_circle_item_t *items = (const egui_circle_item_t *)info->items;

    if (self->mask == NULL)
    {
        egui_canvas_draw_circle_corner_direct_stroke(self, center_x, radius, stroke_width, type, color, alpha, &region, &region_intersect, info, info_inner);
        return;
    }

    // Get the start and end row/col index of the arc
    egui_dim_t row_index_start = bounds.row_index_start;
    egui_dim_t row_index_end = bounds.row_index_end;
    egui_dim_t col_index_start = bounds.col_index_start;
    egui_dim_t col_index_end = bounds.col_index_end;
    int sign_x = bounds.sign_x;
    int sign_y = bounds.sign_y;

    // Direct PFB write setup
    int use_direct_pfb = (self->mask == NULL);
    egui_dim_t pfb_width = self->pfb_region.size.width;
    egui_dim_t pfb_ofs_x = self->pfb_location_in_base_view.x;
    egui_dim_t pfb_ofs_y = self->pfb_location_in_base_view.y;
    egui_alpha_t self_alpha = self->alpha;

    for (row_index = row_index_start; row_index < row_index_end; row_index++)
    {
        sel_y = radius - row_index;

        circle_alpha = EGUI_ALPHA_0;
        col_index = 0;

        // get the start index of the valid data
        if (row_index < info->item_count)
        {
            col_index = items[row_index].start_offset;
        }

        if (use_direct_pfb)
        {
            egui_dim_t py = center_y + sign_y * sel_y;
            egui_color_t *dst_row = (egui_color_t *)&self->pfb[(py - pfb_ofs_y) * pfb_width];

            for (col_index = EGUI_MAX(col_index, col_index_start); col_index < col_index_end; col_index++)
            {
                sel_x = radius - col_index;

                // get the alpha value
                if (circle_alpha != EGUI_ALPHA_100)
                {
                    circle_alpha = egui_canvas_get_circle_corner_value(row_index, col_index, info);
                    if (circle_alpha == 0)
                    {
                        continue;
                    }
                }

                mix_alpha = egui_color_alpha_mix(circle_alpha, alpha);
                // check inner circle
                if (row_index >= stroke_width && col_index >= stroke_width)
                {
                    egui_alpha_t alpha_inner = egui_canvas_get_circle_corner_value(row_index - stroke_width, col_index - stroke_width, info_inner);
                    if (alpha_inner != 0)
                    {
                        if (alpha_inner == EGUI_ALPHA_100)
                        {
                            break;
                        }
                        else
                        {
                            mix_alpha = egui_color_alpha_mix(mix_alpha, EGUI_ALPHA_100 - alpha_inner);
                        }
                    }
                }

                egui_alpha_t eff_alpha = egui_color_alpha_mix(self_alpha, mix_alpha);
                if (eff_alpha == 0)
                {
                    continue;
                }

                egui_dim_t px = center_x + sign_x * sel_x;
                egui_color_t *dst = &dst_row[px - pfb_ofs_x];
                if (eff_alpha == EGUI_ALPHA_100)
                {
                    *dst = color;
                }
                else
                {
                    egui_rgb_mix_ptr(dst, &color, dst, eff_alpha);
                }
            }
        }
        else
        {
            for (col_index = EGUI_MAX(col_index, col_index_start); col_index < col_index_end; col_index++)
            {
                sel_x = radius - col_index;

                // get the alpha value
                if (circle_alpha != EGUI_ALPHA_100)
                {
                    circle_alpha = egui_canvas_get_circle_corner_value(row_index, col_index, info);
                    if (circle_alpha == 0)
                    {
                        continue;
                    }
                }

                mix_alpha = egui_color_alpha_mix(circle_alpha, alpha);
                // check inner circle
                if (row_index >= stroke_width && col_index >= stroke_width)
                {
                    egui_alpha_t alpha_inner = egui_canvas_get_circle_corner_value(row_index - stroke_width, col_index - stroke_width, info_inner);
                    if (alpha_inner != 0)
                    {
                        if (alpha_inner == EGUI_ALPHA_100)
                        {
                            break;
                        }
                        else
                        {
                            mix_alpha = egui_color_alpha_mix(mix_alpha, EGUI_ALPHA_100 - alpha_inner);
                        }
                    }
                }

                egui_canvas_draw_point_limit(center_x + sign_x * sel_x, center_y + sign_y * sel_y, color, mix_alpha);
            }
        }
    }
}

#define ARC_AA_HALF_TRANSITION ARC_AA_HALF_TRANSITION_Q15

typedef struct
{
    egui_dim_t radius;
    int16_t start_angle;
    int16_t end_angle;
    uint16_t start_sin_q15;
    uint16_t start_cos_q15;
    uint16_t end_sin_q15;
    uint16_t end_cos_q15;
    egui_dim_t last_start_x;
    egui_dim_t last_end_x;
    egui_dim_t cur_start_x;
    egui_dim_t cur_end_x;
    egui_dim_t next_start_x;
    egui_dim_t next_end_x;
    egui_dim_t x_allow_min;
    egui_dim_t x_allow_max;
    egui_dim_t x_arc_allow_min;
    egui_dim_t x_arc_allow_max;
} egui_canvas_arc_scan_state_t;

__EGUI_STATIC_INLINE__ egui_dim_t egui_canvas_arc_fill_basic_qx_ceil_nonnegative(int32_t value_q8)
{
    if (value_q8 <= 0)
    {
        return 0;
    }

    return (egui_dim_t)((value_q8 + ((1 << ARC_INT_Q8_SHIFT) - 1)) >> ARC_INT_Q8_SHIFT);
}

__EGUI_STATIC_INLINE__ int egui_canvas_arc_fill_basic_qx_floor_nonnegative(int32_t value_q8, egui_dim_t *result)
{
    if (value_q8 < 0)
    {
        return 0;
    }

    *result = (egui_dim_t)(value_q8 >> ARC_INT_Q8_SHIFT);
    return 1;
}

static void egui_canvas_arc_prepare_scan_state(egui_canvas_arc_scan_state_t *scan_state, egui_dim_t radius, int16_t start_angle, int16_t end_angle)
{
    scan_state->radius = radius;
    scan_state->start_angle = start_angle;
    scan_state->end_angle = end_angle;
    scan_state->start_sin_q15 = (start_angle > 0) ? egui_canvas_arc_get_sin_q15(start_angle) : 0;
    scan_state->start_cos_q15 = (start_angle > 0) ? egui_canvas_arc_get_cos_q15(start_angle) : 0;
    scan_state->end_sin_q15 = (end_angle < 90) ? egui_canvas_arc_get_sin_q15(end_angle) : 0;
    scan_state->end_cos_q15 = (end_angle < 90) ? egui_canvas_arc_get_cos_q15(end_angle) : 0;
    scan_state->last_start_x = egui_canvas_arc_mul_cot_limit(radius, start_angle);
    scan_state->last_end_x = egui_canvas_arc_mul_cot_limit(radius, end_angle);
    scan_state->cur_start_x = scan_state->last_start_x;
    scan_state->cur_end_x = scan_state->last_end_x;
    scan_state->next_start_x = scan_state->last_start_x;
    scan_state->next_end_x = scan_state->last_end_x;
    scan_state->x_allow_min = 0;
    scan_state->x_allow_max = EGUI_DIM_MAX;
    scan_state->x_arc_allow_min = 0;
    scan_state->x_arc_allow_max = EGUI_DIM_MAX;
}

int egui_canvas_get_arc_fill_basic_row_angle_opaque_range(egui_dim_t radius, egui_dim_t qy, int16_t start_angle, int16_t end_angle, egui_dim_t *qx_min,
                                                          egui_dim_t *qx_max)
{
    egui_dim_t local_min = 0;
    egui_dim_t local_max = radius;

    if (qx_min == NULL || qx_max == NULL)
    {
        return 0;
    }

    if (radius < 0 || qy < 0 || qy > radius)
    {
        return 0;
    }

    if (start_angle < 0 || end_angle > 90 || start_angle >= end_angle)
    {
        return 0;
    }

    if (start_angle > 0)
    {
        int32_t max_bound_q8;

        if (!egui_canvas_arc_mul_cot_q8_nonnegative(qy, start_angle, &max_bound_q8))
        {
            return 0;
        }
        max_bound_q8 -= egui_canvas_arc_get_transition_over_cos_q8(start_angle);

        if (!egui_canvas_arc_fill_basic_qx_floor_nonnegative(max_bound_q8, &local_max))
        {
            return 0;
        }
    }

    if (end_angle < 90)
    {
        int32_t min_bound_q8;

        if (!egui_canvas_arc_mul_cot_q8_nonnegative(qy, end_angle, &min_bound_q8))
        {
            return 0;
        }
        min_bound_q8 += egui_canvas_arc_get_transition_over_cos_q8(end_angle);

        local_min = egui_canvas_arc_fill_basic_qx_ceil_nonnegative(min_bound_q8);
    }

    if (local_min > radius)
    {
        return 0;
    }

    local_max = EGUI_MIN(local_max, radius);
    if (local_min > local_max)
    {
        return 0;
    }

    *qx_min = local_min;
    *qx_max = local_max;
    return 1;
}

static egui_alpha_t arc_edge_smoothstep_alpha(int32_t signed_dist_q15)
{
    // 1.5-pixel wide AA transition zone with smoothstep for smoother radial edges
    // half_transition = 0.75 in Q16.16 = 49152

    if (signed_dist_q15 >= ARC_AA_HALF_TRANSITION)
    {
        return EGUI_ALPHA_100;
    }
    if (signed_dist_q15 <= -ARC_AA_HALF_TRANSITION)
    {
        return EGUI_ALPHA_0;
    }

    // Smoothstep interpolation: map [-0.75, 0.75] to [0, 255]
    int32_t coverage_q15 = signed_dist_q15 + ARC_AA_HALF_TRANSITION;
    int32_t t_q15 = (int32_t)(((int64_t)coverage_q15 * 21845) >> 15);
    if (t_q15 < 0)
    {
        t_q15 = 0;
    }
    else if (t_q15 > (1 << 15))
    {
        t_q15 = (1 << 15);
    }
    int32_t t_sq_q15 = (int32_t)(((int64_t)t_q15 * t_q15) >> 15);
    int32_t smooth_q15 = (int32_t)(((int64_t)t_sq_q15 * ((3 << 15) - (t_q15 << 1))) >> 15);
    egui_alpha_t alpha = (egui_alpha_t)((smooth_q15 * EGUI_ALPHA_100) >> 15);
    if (alpha > EGUI_ALPHA_100)
    {
        alpha = EGUI_ALPHA_100;
    }
    return alpha;
}

static egui_alpha_t arc_get_point_alpha(egui_dim_t x, egui_dim_t y, const egui_canvas_arc_scan_state_t *scan_state)
{
    egui_alpha_t alpha_start = EGUI_ALPHA_100;
    egui_alpha_t alpha_end = EGUI_ALPHA_100;

    if (scan_state->start_angle != 0)
    {
        int32_t signed_dist_q15 = ((int32_t)y * scan_state->start_cos_q15) - ((int32_t)x * scan_state->start_sin_q15);

        alpha_start = arc_edge_smoothstep_alpha(signed_dist_q15);
        if (alpha_start == EGUI_ALPHA_0)
        {
            return EGUI_ALPHA_0;
        }
    }

    if (scan_state->end_angle != 90)
    {
        int32_t signed_dist_q15 = ((int32_t)x * scan_state->end_sin_q15) - ((int32_t)y * scan_state->end_cos_q15);

        alpha_end = arc_edge_smoothstep_alpha(signed_dist_q15);
        if (alpha_end == EGUI_ALPHA_0)
        {
            return EGUI_ALPHA_0;
        }
    }

    return egui_color_alpha_mix(alpha_start, alpha_end);
}

static int egui_canvas_arc_try_apply_edge_alpha(const egui_canvas_arc_scan_state_t *scan_state, egui_dim_t sel_x, egui_dim_t sel_y, egui_alpha_t *mix_alpha)
{
    egui_alpha_t point_alpha;

    if ((sel_x < scan_state->x_allow_min) || (sel_x > scan_state->x_allow_max))
    {
        return 0;
    }

    if ((sel_x > scan_state->x_arc_allow_max) && ((scan_state->next_start_x > scan_state->radius) || (sel_x < scan_state->x_arc_allow_min)))
    {
        return 1;
    }

    point_alpha = arc_get_point_alpha(sel_x, sel_y, scan_state);
    if (point_alpha == EGUI_ALPHA_0)
    {
        return 0;
    }

    if (point_alpha != EGUI_ALPHA_100)
    {
        *mix_alpha = egui_color_alpha_mix(*mix_alpha, point_alpha);
    }

    return 1;
}

void egui_canvas_draw_arc_corner_fill(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, int16_t start_angle, int16_t end_angle, int type,
                                      egui_color_t color, egui_alpha_t alpha)
{
    egui_canvas_t *self = &canvas_data;

    egui_dim_t row_index;
    egui_dim_t col_index;
    egui_alpha_t mix_alpha;
    egui_dim_t sel_x;
    egui_dim_t sel_y;
    egui_alpha_t circle_alpha;

    // only work within intersection of base_view_work_region and the rectangle to be drawn
    egui_canvas_corner_bounds_t bounds;
    egui_region_t region_intersect;

    egui_canvas_circle_corner_init_region(&bounds, center_x, center_y, radius, type);
    if (!egui_canvas_circle_corner_get_bounds(self, radius, type, &bounds))
    {
        return;
    }
    region_intersect = bounds.region_intersect;
    const egui_circle_info_t *info = egui_canvas_get_circle_item(radius);
    if (info == NULL)
    {
        EGUI_LOG_WRN("Circle radius %d not supported, increase EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE or register spec circle.\n", radius);
        return;
    }
    const egui_circle_item_t *items = (const egui_circle_item_t *)info->items;

    if (end_angle > 90)
    {
        end_angle = 90;
    }
    if (start_angle < 0)
    {
        start_angle = 0;
    }

    if (start_angle >= end_angle)
    {
        return;
    }

    if (start_angle == 0 && end_angle == 90)
    {
        switch (type)
        {
        case EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP:
            egui_canvas_draw_circle_corner_fill(center_x, center_y, radius, EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP, color, alpha);
            break;
        case EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM:
            egui_canvas_draw_circle_corner_fill(center_x, center_y, radius, EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM, color, alpha);
            break;
        case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP:
            egui_canvas_draw_circle_corner_fill(center_x, center_y, radius, EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP, color, alpha);
            break;
        case EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM:
            egui_canvas_draw_circle_corner_fill(center_x, center_y, radius, EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM, color, alpha);
            break;
        }

        return;
    }

    // Get the start and end row/col index of the arc
    egui_dim_t row_index_start = bounds.row_index_start;
    egui_dim_t row_index_end = bounds.row_index_end;
    egui_dim_t col_index_start = bounds.col_index_start;
    egui_dim_t col_index_end = bounds.col_index_end;
    egui_canvas_arc_scan_state_t scan_state;
    int sign_x = bounds.sign_x;
    int sign_y = bounds.sign_y;

    egui_canvas_arc_prepare_scan_state(&scan_state, radius, start_angle, end_angle);

    // Direct PFB write setup
    int use_direct_pfb = (self->mask == NULL);
    egui_dim_t pfb_width = self->pfb_region.size.width;
    egui_dim_t pfb_ofs_x = self->pfb_location_in_base_view.x;
    egui_dim_t pfb_ofs_y = self->pfb_location_in_base_view.y;
    egui_alpha_t self_alpha = self->alpha;
    egui_dim_t item_count = (egui_dim_t)info->item_count;
    egui_dim_t outer_mirror_visible = item_count;
    egui_dim_t outer_mirror_opaque = item_count;
    int apply_draw_alpha = (alpha != EGUI_ALPHA_100);
    int apply_canvas_alpha = (self_alpha != EGUI_ALPHA_100);

    for (row_index = row_index_start; row_index < row_index_end; row_index++)
    {
        egui_dim_t circle_opaque;
        egui_dim_t row_limit;

        sel_y = radius - row_index;

        // for speed, calculate the start and end x of the arc
        if (scan_state.start_angle != 0)
        {
            scan_state.cur_start_x = scan_state.next_start_x;
            scan_state.next_start_x = egui_canvas_arc_mul_cot_limit((sel_y - 1), scan_state.start_angle);
        }

        if (scan_state.end_angle != 0)
        {
            scan_state.cur_end_x = scan_state.next_end_x;
            scan_state.next_end_x = egui_canvas_arc_mul_cot_limit((sel_y - 1), scan_state.end_angle);
        }

        scan_state.x_allow_min = scan_state.next_end_x;
        scan_state.x_allow_max = scan_state.last_start_x;
        scan_state.x_arc_allow_min = scan_state.next_start_x;
        scan_state.x_arc_allow_max = scan_state.last_end_x;

        while (outer_mirror_visible > 0 && row_index >= (egui_dim_t)items[outer_mirror_visible - 1].start_offset)
        {
            outer_mirror_visible--;
        }
        while (outer_mirror_opaque > 0 && row_index >= egui_canvas_circle_corner_get_opaque_threshold(items, outer_mirror_opaque - 1))
        {
            outer_mirror_opaque--;
        }

        circle_alpha = EGUI_ALPHA_0;
        if (row_index < item_count)
        {
            col_index = items[row_index].start_offset;
            circle_opaque = egui_canvas_circle_corner_get_opaque_threshold(items, row_index);
        }
        else
        {
            col_index = item_count;
            circle_opaque = row_index;
        }

        row_limit = EGUI_MIN(row_index, item_count);
        if (outer_mirror_visible < row_limit)
        {
            col_index = EGUI_MIN(col_index, outer_mirror_visible);
        }
        if (outer_mirror_opaque < row_limit)
        {
            circle_opaque = EGUI_MIN(circle_opaque, outer_mirror_opaque);
        }
        if (circle_opaque < col_index)
        {
            circle_opaque = col_index;
        }

        if (use_direct_pfb)
        {
            egui_dim_t py = center_y + sign_y * sel_y;
            egui_color_t *dst_row = (egui_color_t *)&self->pfb[(py - pfb_ofs_y) * pfb_width];
            egui_dim_t col_begin = EGUI_MAX(col_index, col_index_start);
            egui_dim_t col_end = col_index_end;
            egui_dim_t opaque_qx_min = 0;
            egui_dim_t opaque_qx_max = 0;
            egui_dim_t opaque_col_start = 0;
            egui_dim_t opaque_col_end = 0;
            egui_dim_t opaque_screen_start = 0;
            egui_dim_t opaque_screen_end = 0;
            egui_alpha_t opaque_alpha = apply_canvas_alpha ? egui_color_alpha_mix(self_alpha, apply_draw_alpha ? alpha : EGUI_ALPHA_100)
                                                           : (apply_draw_alpha ? alpha : EGUI_ALPHA_100);
            int has_opaque_span = 0;

            if (scan_state.x_allow_min > 0 || scan_state.x_allow_max < radius)
            {
                egui_dim_t visible_qx_min = EGUI_MIN(scan_state.x_allow_min, radius);
                egui_dim_t visible_qx_max = EGUI_MIN(scan_state.x_allow_max, radius);
                egui_dim_t visible_col_start = radius - visible_qx_max;
                egui_dim_t visible_col_end = radius - visible_qx_min + 1;

                col_begin = EGUI_MAX(col_begin, visible_col_start);
                col_end = EGUI_MIN(col_end, visible_col_end);
                if (col_begin >= col_end)
                {
                    scan_state.last_start_x = scan_state.cur_start_x;
                    scan_state.last_end_x = scan_state.cur_end_x;
                    continue;
                }
            }

            if (egui_canvas_get_arc_fill_basic_row_angle_opaque_range(radius, sel_y, start_angle, end_angle, &opaque_qx_min, &opaque_qx_max))
            {
                opaque_col_start = radius - opaque_qx_max;
                opaque_col_end = radius - opaque_qx_min + 1;
                opaque_col_start = EGUI_MAX(opaque_col_start, EGUI_MAX(circle_opaque, col_begin));
                opaque_col_end = EGUI_MIN(opaque_col_end, col_end);
                if (opaque_col_start < opaque_col_end)
                {
                    egui_dim_t span_qx_max = radius - opaque_col_start;
                    egui_dim_t span_qx_min = radius - (opaque_col_end - 1);

                    if (sign_x > 0)
                    {
                        opaque_screen_start = center_x + span_qx_min;
                        opaque_screen_end = center_x + span_qx_max + 1;
                    }
                    else
                    {
                        opaque_screen_start = center_x - span_qx_max;
                        opaque_screen_end = center_x - span_qx_min + 1;
                    }

                    has_opaque_span = (opaque_screen_start < opaque_screen_end);
                }
            }

            for (col_index = col_begin; col_index < col_end; col_index++)
            {
                if (has_opaque_span && col_index == opaque_col_start)
                {
                    egui_canvas_draw_direct_row_span(dst_row, pfb_ofs_x, region_intersect.location.x, region_intersect.location.x + region_intersect.size.width,
                                                     opaque_screen_start, opaque_screen_end, color, opaque_alpha);
                    circle_alpha = EGUI_ALPHA_100;
                    col_index = opaque_col_end - 1;
                    continue;
                }

                sel_x = radius - col_index;

                // get the alpha value
                if (circle_alpha != EGUI_ALPHA_100)
                {
                    if (col_index >= circle_opaque)
                    {
                        circle_alpha = EGUI_ALPHA_100;
                    }
                    else
                    {
                        circle_alpha = egui_canvas_get_circle_corner_value_fixed_row(row_index, col_index, info, items);
                        if (circle_alpha == 0)
                        {
                            continue;
                        }
                    }
                }

                mix_alpha = apply_draw_alpha ? egui_color_alpha_mix(circle_alpha, alpha) : circle_alpha;
                if (!egui_canvas_arc_try_apply_edge_alpha(&scan_state, sel_x, sel_y, &mix_alpha))
                {
                    continue;
                }

                egui_alpha_t eff_alpha = apply_canvas_alpha ? egui_color_alpha_mix(self_alpha, mix_alpha) : mix_alpha;
                if (eff_alpha == 0)
                {
                    continue;
                }

                egui_dim_t px = center_x + sign_x * sel_x;
                egui_color_t *dst = &dst_row[px - pfb_ofs_x];
                if (eff_alpha == EGUI_ALPHA_100)
                {
                    *dst = color;
                }
                else
                {
                    egui_rgb_mix_ptr(dst, &color, dst, eff_alpha);
                }
            }
        }
        else
        {
            egui_dim_t col_begin = EGUI_MAX(col_index, col_index_start);
            egui_dim_t col_end = col_index_end;

            if (scan_state.x_allow_min > 0 || scan_state.x_allow_max < radius)
            {
                egui_dim_t visible_qx_min = EGUI_MIN(scan_state.x_allow_min, radius);
                egui_dim_t visible_qx_max = EGUI_MIN(scan_state.x_allow_max, radius);
                egui_dim_t visible_col_start = radius - visible_qx_max;
                egui_dim_t visible_col_end = radius - visible_qx_min + 1;

                col_begin = EGUI_MAX(col_begin, visible_col_start);
                col_end = EGUI_MIN(col_end, visible_col_end);
                if (col_begin >= col_end)
                {
                    scan_state.last_start_x = scan_state.cur_start_x;
                    scan_state.last_end_x = scan_state.cur_end_x;
                    continue;
                }
            }

            for (col_index = col_begin; col_index < col_end; col_index++)
            {
                sel_x = radius - col_index;

                // get the alpha value
                if (circle_alpha != EGUI_ALPHA_100)
                {
                    if (col_index >= circle_opaque)
                    {
                        circle_alpha = EGUI_ALPHA_100;
                    }
                    else
                    {
                        circle_alpha = egui_canvas_get_circle_corner_value_fixed_row(row_index, col_index, info, items);
                        if (circle_alpha == 0)
                        {
                            continue;
                        }
                    }
                }

                mix_alpha = apply_draw_alpha ? egui_color_alpha_mix(circle_alpha, alpha) : circle_alpha;
                if (!egui_canvas_arc_try_apply_edge_alpha(&scan_state, sel_x, sel_y, &mix_alpha))
                {
                    continue;
                }

                egui_canvas_draw_point_limit(center_x + sign_x * sel_x, center_y + sign_y * sel_y, color, mix_alpha);
            }
        }

        scan_state.last_start_x = scan_state.cur_start_x;
        scan_state.last_end_x = scan_state.cur_end_x;
    }
}

/**
 * \brief           Draw filled rectangle
 * \param[in,out]   self: Pointer to \ref egui_canvas_t structure for display operations
 * \param[in]       x: Top left X position
 * \param[in]       y: Top left Y position
 * \param[in]       width: Rectangle width
 * \param[in]       height: Rectangle height
 * \param[in]       color: Color used for drawing operation
 * \sa              egui_canvas_draw_rectangle, egui_canvas_draw_round_rectangle, egui_canvas_draw_round_rectangle_fill
 */
void egui_canvas_draw_rectangle_fill(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_color_t color, egui_alpha_t alpha)
{
    // egui_canvas_t *self = &canvas_data;

    if (width <= 0 || height <= 0)
    {
        return;
    }

    egui_canvas_draw_fillrect(x, y, width, height, color, alpha);
}

/**
 * \brief           Draw rectangle
 * \param[in,out]   self: Pointer to \ref egui_canvas_t structure for display operations
 * \param[in]       x: Top left X position
 * \param[in]       y: Top left Y position
 * \param[in]       width: Rectangle width
 * \param[in]       height: Rectangle height
 * \param[in]       color: Color used for drawing operation
 * \sa              egui_canvas_draw_rectangle_fill, egui_canvas_draw_round_rectangle, egui_canvas_draw_round_rectangle_fill
 */
void egui_canvas_draw_rectangle(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_dim_t stroke_width, egui_color_t color,
                                egui_alpha_t alpha)
{
    // egui_canvas_t *self = &canvas_data;

    if (width == 0 || height == 0 || stroke_width == 0)
    {
        return;
    }
    if (stroke_width >= (width >> 1) || stroke_width >= (height >> 1))
    {
        egui_canvas_draw_rectangle_fill(x, y, width, height, color, alpha);
        return;
    }

    // draw the left and right rectangles
    egui_canvas_draw_fillrect(x, y, stroke_width, height, color, alpha);
    egui_canvas_draw_fillrect(x + width - stroke_width, y, stroke_width, height, color, alpha);

    // draw the top and bottom rectangles
    egui_canvas_draw_fillrect(x + stroke_width, y, width - (stroke_width << 1), stroke_width, color, alpha);
    egui_canvas_draw_fillrect(x + stroke_width, y + height - stroke_width, width - (stroke_width << 1), stroke_width, color, alpha);
}

/**
 * \brief           Draw filled rectangle with rounded corners
 * \param[in,out]   self: Pointer to \ref egui_canvas_t structure for display operations
 * \param[in]       x: Top left X position
 * \param[in]       y: Top left Y position
 * \param[in]       width: Rectangle width
 * \param[in]       height: Rectangle height
 * \param[in]       r: Corner radius, max value can be r = MIN(width, height) / 2
 * \param[in]       color: Color used for drawing operation
 * \sa              egui_canvas_draw_rectangle, egui_canvas_draw_rectangle_fill, egui_canvas_draw_round_rectangle
 */
void egui_canvas_draw_round_rectangle_fill(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_dim_t radius, egui_color_t color,
                                           egui_alpha_t alpha)
{
    // egui_canvas_t *self = &canvas_data;

    if (width <= 0 || height <= 0)
    {
        return;
    }

    radius = egui_canvas_clamp_round_radius(radius, width, height);
    if (radius > 0)
    {
        if (egui_canvas_round_rect_is_circle_case(width, height, radius))
        {
            egui_canvas_draw_circle_fill(x + (width >> 1), y + (height >> 1), radius, color, alpha);
            return;
        }

        // draw the middle rectangle
        egui_canvas_draw_rectangle_fill(x + radius, y, width - (radius << 1), height, color, alpha);

        // draw the left and right rectangles
        egui_canvas_draw_rectangle_fill(x, y + radius, radius, height - (radius << 1), color, alpha);
        egui_canvas_draw_rectangle_fill(x + width - radius, y + radius, radius, height - (radius << 1), color, alpha);

        // draw the four corners
        egui_canvas_draw_circle_corner_fill(x + radius, y + radius, radius, EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP, color, alpha);
        egui_canvas_draw_circle_corner_fill(x + radius, y + height - radius - 1, radius, EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM, color, alpha);
        egui_canvas_draw_circle_corner_fill(x + width - radius - 1, y + radius, radius, EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP, color, alpha);
        egui_canvas_draw_circle_corner_fill(x + width - radius - 1, y + height - radius - 1, radius, EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM, color, alpha);
    }
    else
    {
        egui_canvas_draw_rectangle_fill(x, y, width, height, color, alpha);
    }
}

/**
 * \brief           Draw rectangle with rounded corners
 * \param[in,out]   self: Pointer to \ref egui_canvas_t structure for display operations
 * \param[in]       x: Top left X position
 * \param[in]       y: Top left Y position
 * \param[in]       width: Rectangle width
 * \param[in]       height: Rectangle height
 * \param[in]       r: Corner radius, max value can be r = MIN(width, height) / 2
 * \param[in]       color: Color used for drawing operation
 * \sa              egui_canvas_draw_rectangle, egui_canvas_draw_rectangle_fill, egui_canvas_draw_round_rectangle_fill
 */
void egui_canvas_draw_round_rectangle(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_dim_t radius, egui_dim_t stroke_width,
                                      egui_color_t color, egui_alpha_t alpha)
{
    // egui_canvas_t *self = &canvas_data;

    if (width <= 0 || height <= 0 || stroke_width <= 0)
    {
        return;
    }
    if (stroke_width >= (width >> 1) || stroke_width >= (height >> 1))
    {
        egui_canvas_draw_round_rectangle_fill(x, y, width, height, stroke_width, color, alpha);
        return;
    }
    radius = egui_canvas_clamp_round_radius(radius, width, height);
    if (radius > 0)
    {
        if (egui_canvas_round_rect_is_circle_case(width, height, radius))
        {
            egui_canvas_draw_circle(x + (width >> 1), y + (height >> 1), radius, stroke_width, color, alpha);
            return;
        }

        // Think stroke_width is bigger than radius, in this case, we should adjust stroke_width to radius.
        stroke_width = EGUI_MIN(radius, stroke_width);

        // draw the left and right rectangles
        egui_canvas_draw_rectangle_fill(x, y + radius, stroke_width, height - (radius << 1), color, alpha);
        egui_canvas_draw_rectangle_fill(x + width - stroke_width, y + radius, stroke_width, height - (radius << 1), color, alpha);

        // draw the top and bottom rectangles
        egui_canvas_draw_fillrect(x + radius, y, width - (radius << 1), stroke_width, color, alpha);
        egui_canvas_draw_fillrect(x + radius, y + height - stroke_width, width - (radius << 1), stroke_width, color, alpha);

        // draw the four corners
        egui_canvas_draw_circle_corner(x + radius, y + radius, radius, stroke_width, EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP, color, alpha);
        egui_canvas_draw_circle_corner(x + radius, y + height - radius - 1, radius, stroke_width, EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM, color, alpha);
        egui_canvas_draw_circle_corner(x + width - radius - 1, y + radius, radius, stroke_width, EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP, color, alpha);
        egui_canvas_draw_circle_corner(x + width - radius - 1, y + height - radius - 1, radius, stroke_width, EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM, color,
                                       alpha);
    }
    else
    {
        egui_canvas_draw_rectangle_fill(x, y, width, height, color, alpha);
    }
}

/**
 * \brief           Draw filled rectangle with rounded corners
 * \param[in,out]   self: Pointer to \ref egui_canvas_t structure for display operations
 * \param[in]       x: Top left X position
 * \param[in]       y: Top left Y position
 * \param[in]       width: Rectangle width
 * \param[in]       height: Rectangle height
 * \param[in]       r: Corner radius, max value can be r = MIN(width, height) / 2
 * \param[in]       color: Color used for drawing operation
 * \sa              egui_canvas_draw_rectangle, egui_canvas_draw_rectangle_fill, egui_canvas_draw_round_rectangle
 */
void egui_canvas_draw_round_rectangle_corners_fill(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_dim_t radius_left_top,
                                                   egui_dim_t radius_left_bottom, egui_dim_t radius_right_top, egui_dim_t radius_right_bottom,
                                                   egui_color_t color, egui_alpha_t alpha)
{
    // egui_canvas_t *self = &canvas_data;

    if (width <= 0 || height <= 0)
    {
        return;
    }

    radius_left_top = egui_canvas_clamp_round_radius(radius_left_top, width, height);
    radius_left_bottom = egui_canvas_clamp_round_radius(radius_left_bottom, width, height);
    radius_right_top = egui_canvas_clamp_round_radius(radius_right_top, width, height);
    radius_right_bottom = egui_canvas_clamp_round_radius(radius_right_bottom, width, height);

    // left top corner
    if (radius_left_top > 0)
    {
        // draw the corners
        egui_canvas_draw_circle_corner_fill(x + radius_left_top, y + radius_left_top, radius_left_top, EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP, color, alpha);
    }

    // left bottom corner
    if (radius_left_bottom > 0)
    {
        // draw the corners
        egui_canvas_draw_circle_corner_fill(x + radius_left_bottom, y + height - radius_left_bottom - 1, radius_left_bottom,
                                            EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM, color, alpha);
    }

    // right top corner
    if (radius_right_top > 0)
    {
        // draw the corners
        egui_canvas_draw_circle_corner_fill(x + width - radius_right_top - 1, y + radius_right_top, radius_right_top, EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP, color,
                                            alpha);
    }

    // right bottom corner
    if (radius_right_bottom > 0)
    {
        // draw the corners
        egui_canvas_draw_circle_corner_fill(x + width - radius_right_bottom - 1, y + height - radius_right_bottom - 1, radius_right_bottom,
                                            EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM, color, alpha);
    }

    // draw the middle rectangle
    egui_dim_t left_width = EGUI_MAX(radius_left_top, radius_left_bottom);
    egui_dim_t right_width = EGUI_MAX(radius_right_top, radius_right_bottom);
    egui_dim_t middle_width = width - left_width - right_width;

    egui_canvas_draw_rectangle_fill(x + left_width, y, middle_width, height, color, alpha);

    // draw the left rectangles
    egui_canvas_draw_rectangle_fill(x, y + radius_left_top, left_width, height - (radius_left_top + radius_left_bottom), color, alpha);
    // draw the left small rectangles
    if (radius_left_top < radius_left_bottom)
    {
        egui_canvas_draw_rectangle_fill(x + radius_left_top, y, radius_left_bottom - radius_left_top, radius_left_top, color, alpha);
    }
    else if (radius_left_bottom < radius_left_top)
    {
        egui_canvas_draw_rectangle_fill(x + radius_left_bottom, y + height - radius_left_bottom, radius_left_top - radius_left_bottom, radius_left_bottom,
                                        color, alpha);
    }

    // draw the right rectangles
    egui_canvas_draw_rectangle_fill(x + width - right_width, y + radius_right_top, right_width, height - (radius_right_top + radius_right_bottom), color,
                                    alpha);
    // draw the right small rectangles
    if (radius_right_top < radius_right_bottom)
    {
        egui_canvas_draw_rectangle_fill(x + width - right_width, y, radius_right_bottom - radius_right_top, radius_right_top, color, alpha);
    }
    else if (radius_right_bottom < radius_right_top)
    {
        egui_canvas_draw_rectangle_fill(x + width - right_width, y + height - radius_right_bottom, radius_right_top - radius_right_bottom, radius_right_bottom,
                                        color, alpha);
    }
}

/**
 * \brief           Draw filled rectangle with rounded corners
 * \param[in,out]   self: Pointer to \ref egui_canvas_t structure for display operations
 * \param[in]       x: Top left X position
 * \param[in]       y: Top left Y position
 * \param[in]       width: Rectangle width
 * \param[in]       height: Rectangle height
 * \param[in]       r: Corner radius, max value can be r = MIN(width, height) / 2
 * \param[in]       color: Color used for drawing operation
 * \sa              egui_canvas_draw_rectangle, egui_canvas_draw_rectangle_fill, egui_canvas_draw_round_rectangle
 */
void egui_canvas_draw_round_rectangle_corners(egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_dim_t radius_left_top,
                                              egui_dim_t radius_left_bottom, egui_dim_t radius_right_top, egui_dim_t radius_right_bottom,
                                              egui_dim_t stroke_width, egui_color_t color, egui_alpha_t alpha)
{
    // egui_canvas_t *self = &canvas_data;

    if (width <= 0 || height <= 0 || stroke_width <= 0)
    {
        return;
    }
    if (stroke_width >= (width >> 1) || stroke_width >= (height >> 1))
    {
        egui_canvas_draw_round_rectangle_corners_fill(x, y, width, height, radius_left_top, radius_left_bottom, radius_right_top, radius_right_bottom, color,
                                                      alpha);
        return;
    }
    radius_left_top = egui_canvas_clamp_round_radius(radius_left_top, width, height);
    radius_left_bottom = egui_canvas_clamp_round_radius(radius_left_bottom, width, height);
    radius_right_top = egui_canvas_clamp_round_radius(radius_right_top, width, height);
    radius_right_bottom = egui_canvas_clamp_round_radius(radius_right_bottom, width, height);

    // left top corner
    if (radius_left_top > 0)
    {
        // draw the corners
        egui_canvas_draw_circle_corner(x + radius_left_top, y + radius_left_top, radius_left_top, stroke_width, EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP, color, alpha);
    }

    // left bottom corner
    if (radius_left_bottom > 0)
    {
        // draw the corners
        egui_canvas_draw_circle_corner(x + radius_left_bottom, y + height - radius_left_bottom - 1, radius_left_bottom, stroke_width,
                                       EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM, color, alpha);
    }

    // right top corner
    if (radius_right_top > 0)
    {
        // draw the corners
        egui_canvas_draw_circle_corner(x + width - radius_right_top - 1, y + radius_right_top, radius_right_top, stroke_width,
                                       EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP, color, alpha);
    }

    // right bottom corner
    if (radius_right_bottom > 0)
    {
        // draw the corners
        egui_canvas_draw_circle_corner(x + width - radius_right_bottom - 1, y + height - radius_right_bottom - 1, radius_right_bottom, stroke_width,
                                       EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM, color, alpha);
    }

    // draw the left and right rectangles
    egui_canvas_draw_rectangle_fill(x, y + radius_left_top, stroke_width, height - (radius_left_top + radius_left_bottom), color, alpha);
    egui_canvas_draw_rectangle_fill(x + width - stroke_width, y + radius_right_top, stroke_width, height - (radius_right_top + radius_right_bottom), color,
                                    alpha);

    // draw the top and bottom rectangles
    // Think stroke_width is bigger than radius_left_top, radius_left_bottom, radius_right_top, radius_right_bottom
    egui_canvas_draw_fillrect(x + EGUI_MAX(radius_left_top, stroke_width), y,
                              width - (EGUI_MAX(radius_left_top, stroke_width) + EGUI_MAX(radius_right_top, stroke_width)), stroke_width, color, alpha);
    if (radius_left_top < stroke_width)
    {
        egui_canvas_draw_fillrect(x + radius_left_top, y, stroke_width - radius_left_top, radius_left_top, color, alpha);
    }

    if (radius_right_top < stroke_width)
    {
        egui_canvas_draw_fillrect(x + width - stroke_width, y, stroke_width - radius_right_top, radius_right_top, color, alpha);
    }

    egui_canvas_draw_fillrect(x + EGUI_MAX(radius_left_bottom, stroke_width), y + height - stroke_width,
                              width - (EGUI_MAX(radius_left_bottom, stroke_width) + EGUI_MAX(radius_right_bottom, stroke_width)), stroke_width, color, alpha);
    if (radius_left_bottom < stroke_width)
    {
        egui_canvas_draw_fillrect(x + radius_left_bottom, y + height - radius_left_bottom, stroke_width - radius_left_bottom, radius_left_bottom, color, alpha);
    }

    if (radius_right_bottom < stroke_width)
    {
        egui_canvas_draw_fillrect(x + width - stroke_width, y + height - radius_right_bottom, stroke_width - radius_right_bottom, radius_right_bottom, color,
                                  alpha);
    }

    // egui_canvas_draw_fillrect(x + radius_left_top,    y,                      width - (radius_left_top + radius_right_top), stroke_width, color,
    // alpha); egui_canvas_draw_fillrect(x + radius_left_bottom, y + height - stroke_width, width - (radius_left_bottom + radius_right_bottom),
    // stroke_width, color, alpha);
}

/**
 * \brief           Draw filled circle
 * \param[in,out]   self: Pointer to \ref egui_canvas_t structure for display operations
 * \param[in]       x: X position of circle center
 * \param[in]       y: X position of circle center
 * \param[in]       r: Circle radius
 * \param[in]       color: Color used for drawing operation
 * \sa              egui_canvas_draw_circle_basic, egui_canvas_draw_circle_corner, egui_canvas_draw_circle_corner_fill
 */
void egui_canvas_draw_circle_fill_basic(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, egui_color_t color, egui_alpha_t alpha)
{
    egui_canvas_t *self = &canvas_data;
    egui_region_t region_intersect;

    if (radius < 0)
    {
        return;
    }

    if (radius == 0)
    {
        egui_canvas_draw_point(center_x, center_y, color, alpha);
        return;
    }

    EGUI_REGION_DEFINE(region, center_x - radius, center_y - radius, (radius << 1) + 1, (radius << 1) + 1);
    egui_region_intersect(&region, &self->base_view_work_region, &region_intersect);
    if (egui_region_is_empty(&region_intersect))
    {
        return;
    }

#if !defined(EGUI_CONFIG_CIRCLE_FILL_BASIC) || !EGUI_CONFIG_CIRCLE_FILL_BASIC
    egui_canvas_draw_circle_fill_basic_legacy(center_x, center_y, radius, color, alpha);
    return;
#else
    const egui_circle_info_t *info;
    const egui_circle_item_t *items;
    egui_dim_t total_width;
    egui_dim_t clip_x_start;
    egui_dim_t clip_x_end;
    egui_dim_t clip_y_end;
    egui_dim_t base_x;
    egui_alpha_t solid_alpha;
    int use_direct_pfb;

    if (egui_canvas_should_use_circle_fill_basic_legacy_clip(radius, region_intersect.size.width, region_intersect.size.height))
    {
        egui_canvas_draw_circle_fill_basic_legacy(center_x, center_y, radius, color, alpha);
        return;
    }

    info = egui_canvas_get_circle_item(radius);
    if (info == NULL)
    {
        EGUI_LOG_WRN("Circle radius %d not supported, increase EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE or register spec circle.\n", radius);
        return;
    }

    items = (const egui_circle_item_t *)info->items;
    total_width = (radius << 1) + 1;
    clip_x_start = region_intersect.location.x;
    clip_x_end = clip_x_start + region_intersect.size.width;
    clip_y_end = region_intersect.location.y + region_intersect.size.height;
    base_x = center_x - radius;
    solid_alpha = egui_color_alpha_mix(self->alpha, alpha);
    if (solid_alpha == 0)
    {
        return;
    }

    use_direct_pfb = (self->mask == NULL) ? 1 : 0;

    for (egui_dim_t screen_y = region_intersect.location.y; screen_y < clip_y_end; screen_y++)
    {
        egui_dim_t dy = (screen_y > center_y) ? (screen_y - center_y) : (center_y - screen_y);
        egui_dim_t row_index = radius - dy;
        egui_dim_t start_offset;
        egui_dim_t fill_start;
        egui_dim_t fill_end;
        egui_dim_t left_edge_start_x;
        egui_dim_t left_edge_end_x;
        egui_dim_t solid_start_x;
        egui_dim_t solid_end_x;
        egui_dim_t right_edge_start_x;
        egui_dim_t right_edge_end_x;

        egui_canvas_get_circle_fill_basic_row_layout_fast(radius, row_index, info, items, &start_offset, &fill_start, &fill_end);

        left_edge_start_x = base_x + start_offset;
        left_edge_end_x = base_x + fill_start;
        solid_start_x = left_edge_end_x;
        solid_end_x = base_x + fill_end;
        right_edge_start_x = solid_end_x;
        right_edge_end_x = base_x + total_width - start_offset;

        if (use_direct_pfb)
        {
            egui_dim_t pfb_width = self->pfb_region.size.width;
            egui_dim_t pfb_ofs_x = self->pfb_location_in_base_view.x;
            egui_dim_t pfb_ofs_y = self->pfb_location_in_base_view.y;
            egui_color_t *dst_row = (egui_color_t *)&self->pfb[(screen_y - pfb_ofs_y) * pfb_width];

            egui_canvas_draw_circle_fill_basic_edge_direct_row(dst_row, pfb_ofs_x, clip_x_start, clip_x_end, base_x, left_edge_start_x, left_edge_end_x,
                                                               row_index, total_width, 0, info, items, color, solid_alpha);
            egui_canvas_draw_direct_row_span(dst_row, pfb_ofs_x, clip_x_start, clip_x_end, solid_start_x, solid_end_x, color, solid_alpha);
            egui_canvas_draw_circle_fill_basic_edge_direct_row(dst_row, pfb_ofs_x, clip_x_start, clip_x_end, base_x, right_edge_start_x, right_edge_end_x,
                                                               row_index, total_width, 1, info, items, color, solid_alpha);
        }
        else
        {
            egui_dim_t solid_width;

            egui_canvas_draw_circle_fill_basic_edge_row(screen_y, clip_x_start, clip_x_end, base_x, left_edge_start_x, left_edge_end_x, row_index, total_width,
                                                        0, info, items, color, alpha);

            solid_start_x = EGUI_MAX(solid_start_x, clip_x_start);
            solid_end_x = EGUI_MIN(solid_end_x, clip_x_end);
            solid_width = solid_end_x - solid_start_x;
            if (solid_width > 0)
            {
                egui_canvas_draw_fillrect(solid_start_x, screen_y, solid_width, 1, color, alpha);
            }

            egui_canvas_draw_circle_fill_basic_edge_row(screen_y, clip_x_start, clip_x_end, base_x, right_edge_start_x, right_edge_end_x, row_index,
                                                        total_width, 1, info, items, color, alpha);
        }
    }
#endif
}

static int egui_canvas_arc_try_get_single_quadrant(int16_t start_angle, int16_t end_angle, int *type, int16_t *start_angle_local, int16_t *end_angle_local)
{
    int16_t quadrant;

    if (start_angle < 0 || end_angle <= start_angle || end_angle > 360)
    {
        return 0;
    }

    quadrant = start_angle / 90;
    if (((end_angle - 1) / 90) != quadrant)
    {
        return 0;
    }

    switch (quadrant)
    {
    case 0:
        *type = EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM;
        *start_angle_local = start_angle;
        *end_angle_local = end_angle;
        return 1;
    case 1:
        *type = EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM;
        *start_angle_local = 180 - end_angle;
        *end_angle_local = 180 - start_angle;
        return 1;
    case 2:
        *type = EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP;
        *start_angle_local = start_angle - 180;
        *end_angle_local = end_angle - 180;
        return 1;
    case 3:
        *type = EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP;
        *start_angle_local = 360 - end_angle;
        *end_angle_local = 360 - start_angle;
        return 1;
    default:
        return 0;
    }
}

/**
 * \brief           Draw filled circle
 * \param[in,out]   self: Pointer to \ref egui_canvas_t structure for display operations
 * \param[in]       x: X position of circle center
 * \param[in]       y: X position of circle center
 * \param[in]       r: Circle radius
 * \param[in]       color: Color used for drawing operation
 * \sa              egui_canvas_draw_circle_basic, egui_canvas_draw_circle_corner, egui_canvas_draw_circle_corner_fill
 */
void egui_canvas_draw_arc_fill_basic(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, int16_t start_angle, int16_t end_angle, egui_color_t color,
                                     egui_alpha_t alpha)
{
    // egui_canvas_t *self = &canvas_data;

    int16_t start_angle_tmp;
    int16_t end_angle_tmp;
    int is_need_middle_point = 0;
    int type;

    if (start_angle < 0 || end_angle < 0)
    {
        // not support
        return;
    }

    if (start_angle > end_angle)
    {
        // not support
        return;
    }

    if (start_angle == end_angle)
    {
        // do nothing
        return;
    }

    // for some case, arc need over flow 0. can use 360 to fix it.
    if (start_angle > 720 || end_angle > 720)
    {
        // not support
        return;
    }

    if (egui_canvas_arc_try_get_single_quadrant(start_angle, end_angle, &type, &start_angle_tmp, &end_angle_tmp))
    {
        egui_canvas_draw_arc_corner_fill(center_x, center_y, radius, start_angle_tmp, end_angle_tmp, type, color, alpha);

        if (start_angle <= 0 && end_angle >= 0)
        {
            egui_canvas_draw_hline(center_x + 1, center_y, radius, color, alpha);
            is_need_middle_point = 1;
        }
        if (start_angle <= 90 && end_angle >= 90)
        {
            egui_canvas_draw_vline(center_x, center_y + 1, radius, color, alpha);
            is_need_middle_point = 1;
        }
        if (start_angle <= 180 && end_angle >= 180)
        {
            egui_canvas_draw_hline(center_x - radius, center_y, radius, color, alpha);
            is_need_middle_point = 1;
        }
        if (start_angle <= 270 && end_angle >= 270)
        {
            egui_canvas_draw_vline(center_x, center_y - radius, radius, color, alpha);
            is_need_middle_point = 1;
        }

        if (is_need_middle_point)
        {
            egui_canvas_draw_point(center_x, center_y, color, alpha);
        }
        return;
    }

    // First draw area
    do
    {
        if (start_angle < 90)
        {
            start_angle_tmp = start_angle;
            end_angle_tmp = end_angle;
            egui_canvas_draw_arc_corner_fill(center_x, center_y, radius, start_angle_tmp, end_angle_tmp, EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM, color, alpha);
        }
        if (start_angle < 180)
        {
            start_angle_tmp = start_angle - 90;
            end_angle_tmp = end_angle - 90;
            egui_canvas_draw_arc_corner_fill(center_x, center_y, radius, 90 - end_angle_tmp, 90 - start_angle_tmp, EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM, color,
                                             alpha);
        }

        if (start_angle < 270)
        {
            start_angle_tmp = start_angle - 180;
            end_angle_tmp = end_angle - 180;
            egui_canvas_draw_arc_corner_fill(center_x, center_y, radius, start_angle_tmp, end_angle_tmp, EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP, color, alpha);
        }

        if (start_angle < 360)
        {
            start_angle_tmp = start_angle - 270;
            end_angle_tmp = end_angle - 270;
            egui_canvas_draw_arc_corner_fill(center_x, center_y, radius, 90 - end_angle_tmp, 90 - start_angle_tmp, EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP, color,
                                             alpha);
        }

        // draw the line
        if (start_angle <= 0 && end_angle >= 0)
        {
            egui_canvas_draw_hline(center_x + 1, center_y, radius, color, alpha);
            is_need_middle_point = 1;
        }
        if (start_angle <= 90 && end_angle >= 90)
        {
            egui_canvas_draw_vline(center_x, center_y + 1, radius, color, alpha);
            is_need_middle_point = 1;
        }
        if (start_angle <= 180 && end_angle >= 180)
        {
            egui_canvas_draw_hline(center_x - radius, center_y, radius, color, alpha);
            is_need_middle_point = 1;
        }
        if (start_angle <= 270 && end_angle >= 270)
        {
            egui_canvas_draw_vline(center_x, center_y - radius, radius, color, alpha);
            is_need_middle_point = 1;
        }

        start_angle = start_angle - 360;
        if (start_angle < 0)
        {
            start_angle = 0;
        }
        end_angle = end_angle - 360;
    } while (end_angle > 0);

    if (is_need_middle_point)
    {
        egui_canvas_draw_point(center_x, center_y, color, alpha);
    }
}

/**
 * \brief           Draw circle
 * \param[in,out]   self: Pointer to \ref egui_canvas_t structure for display operations
 * \param[in]       x: X position of circle center
 * \param[in]       y: X position of circle center
 * \param[in]       r: Circle radius
 * \param[in]       color: Color used for drawing operation
 * \sa              egui_canvas_draw_circle_fill_basic, egui_canvas_draw_circle_corner, egui_canvas_draw_circle_corner_fill
 */
void egui_canvas_draw_circle_basic(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, egui_dim_t stroke_width, egui_color_t color, egui_alpha_t alpha)
{
    // egui_canvas_t *self = &canvas_data;

    // if radius <= stroke_width, draw a filled circle
    if (radius <= stroke_width)
    {
        egui_canvas_draw_circle_fill_basic(center_x, center_y, radius, color, alpha);
        return;
    }

    egui_canvas_draw_circle_corner(center_x, center_y, radius, stroke_width, EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP, color, alpha);
    egui_canvas_draw_circle_corner(center_x, center_y, radius, stroke_width, EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM, color, alpha);
    egui_canvas_draw_circle_corner(center_x, center_y, radius, stroke_width, EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP, color, alpha);
    egui_canvas_draw_circle_corner(center_x, center_y, radius, stroke_width, EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM, color, alpha);

    // draw the center line
    egui_canvas_draw_hline(center_x - radius, center_y, stroke_width, color, alpha);
    egui_canvas_draw_hline(center_x + radius - stroke_width + 1, center_y, stroke_width, color, alpha);

    egui_canvas_draw_vline(center_x, center_y - radius, stroke_width, color, alpha);
    egui_canvas_draw_vline(center_x, center_y + radius - stroke_width + 1, stroke_width, color, alpha);
}

void egui_canvas_draw_arc_corner(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, int16_t start_angle, int16_t end_angle, int stroke_width,
                                 int type, egui_color_t color, egui_alpha_t alpha)
{
    egui_canvas_t *self = &canvas_data;

    egui_dim_t row_index;
    egui_dim_t col_index;
    egui_alpha_t mix_alpha;
    egui_dim_t sel_x;
    egui_dim_t sel_y;
    // egui_dim_t len;
    // uint16_t start_offset;
    // uint16_t valid_count;
    // uint16_t data_value_offset;
    egui_alpha_t circle_alpha;

    // only work within intersection of base_view_work_region and the rectangle to be drawn
    egui_canvas_corner_bounds_t bounds;

    egui_canvas_circle_corner_init_region(&bounds, center_x, center_y, radius, type);
    if (!egui_canvas_circle_corner_get_bounds(self, radius, type, &bounds))
    {
        return;
    }
    const egui_circle_info_t *info = egui_canvas_get_circle_item(radius);
    if (info == NULL)
    {
        EGUI_LOG_WRN("Circle radius %d not supported, increase EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE or register spec circle.\n", radius);
        return;
    }
    const egui_circle_item_t *items = (const egui_circle_item_t *)info->items;

    egui_dim_t radius_inner = radius - stroke_width;
    const egui_circle_info_t *info_inner = egui_canvas_get_circle_item(radius_inner);
    if (info_inner == NULL)
    {
        EGUI_LOG_WRN("Circle radius %d not supported, increase EGUI_CONFIG_CIRCLE_SUPPORT_RADIUS_BASIC_RANGE or register spec circle.\n", radius_inner);
        return;
    }
    const egui_circle_item_t *items_inner = (const egui_circle_item_t *)info_inner->items;

    if (end_angle > 90)
    {
        end_angle = 90;
    }
    if (start_angle < 0)
    {
        start_angle = 0;
    }

    if (start_angle >= end_angle)
    {
        return;
    }

    if (start_angle == 0 && end_angle == 90)
    {
        egui_canvas_draw_circle_corner(center_x, center_y, radius, stroke_width, type, color, alpha);

        return;
    }

    // Get the start and end row/col index of the arc
    egui_dim_t row_index_start = bounds.row_index_start;
    egui_dim_t row_index_end = bounds.row_index_end;
    egui_dim_t col_index_start = bounds.col_index_start;
    egui_dim_t col_index_end = bounds.col_index_end;
    egui_canvas_arc_scan_state_t scan_state;
    int sign_x = bounds.sign_x;
    int sign_y = bounds.sign_y;

    egui_canvas_arc_prepare_scan_state(&scan_state, radius, start_angle, end_angle);

    // Direct PFB write setup
    int use_direct_pfb = (self->mask == NULL);
    egui_dim_t pfb_width = self->pfb_region.size.width;
    egui_dim_t pfb_ofs_x = self->pfb_location_in_base_view.x;
    egui_dim_t pfb_ofs_y = self->pfb_location_in_base_view.y;
    egui_alpha_t self_alpha = self->alpha;
    egui_dim_t outer_item_count = (egui_dim_t)info->item_count;
    egui_dim_t inner_item_count = (egui_dim_t)info_inner->item_count;
    egui_dim_t outer_mirror_visible = outer_item_count;
    egui_dim_t outer_mirror_opaque = outer_item_count;
    egui_dim_t inner_mirror_visible = inner_item_count;
    egui_dim_t inner_mirror_opaque = inner_item_count;
    int apply_draw_alpha = (alpha != EGUI_ALPHA_100);
    int apply_canvas_alpha = (self_alpha != EGUI_ALPHA_100);

    for (row_index = row_index_start; row_index < row_index_end; row_index++)
    {
        egui_dim_t outer_visible;
        egui_dim_t outer_opaque;
        egui_dim_t inner_visible = radius;
        egui_dim_t inner_opaque = radius;
        egui_dim_t row_limit;

        sel_y = radius - row_index;

        // for speed, calculate the start and end x of the arc
        if (scan_state.start_angle != 0)
        {
            scan_state.cur_start_x = scan_state.next_start_x;
            scan_state.next_start_x = egui_canvas_arc_mul_cot_limit((sel_y - 1), scan_state.start_angle);
        }

        if (scan_state.end_angle != 0)
        {
            scan_state.cur_end_x = scan_state.next_end_x;
            scan_state.next_end_x = egui_canvas_arc_mul_cot_limit((sel_y - 1), scan_state.end_angle);
        }

        scan_state.x_allow_min = scan_state.next_end_x;
        scan_state.x_allow_max = scan_state.last_start_x;
        scan_state.x_arc_allow_min = scan_state.next_start_x;
        scan_state.x_arc_allow_max = scan_state.last_end_x;

        while (outer_mirror_visible > 0 && row_index >= (egui_dim_t)items[outer_mirror_visible - 1].start_offset)
        {
            outer_mirror_visible--;
        }
        while (outer_mirror_opaque > 0 && row_index >= egui_canvas_circle_corner_get_opaque_threshold(items, outer_mirror_opaque - 1))
        {
            outer_mirror_opaque--;
        }

        circle_alpha = EGUI_ALPHA_0;
        if (row_index < outer_item_count)
        {
            outer_visible = items[row_index].start_offset;
            outer_opaque = egui_canvas_circle_corner_get_opaque_threshold(items, row_index);
        }
        else
        {
            outer_visible = outer_item_count;
            outer_opaque = row_index;
        }

        row_limit = EGUI_MIN(row_index, outer_item_count);
        if (outer_mirror_visible < row_limit)
        {
            outer_visible = EGUI_MIN(outer_visible, outer_mirror_visible);
        }
        if (outer_mirror_opaque < row_limit)
        {
            outer_opaque = EGUI_MIN(outer_opaque, outer_mirror_opaque);
        }
        if (outer_opaque < outer_visible)
        {
            outer_opaque = outer_visible;
        }

        col_index = outer_visible;

        if (row_index >= stroke_width)
        {
            egui_dim_t inner_row_index = row_index - stroke_width;
            egui_dim_t inner_row_limit;

            while (inner_mirror_visible > 0 && inner_row_index >= (egui_dim_t)items_inner[inner_mirror_visible - 1].start_offset)
            {
                inner_mirror_visible--;
            }
            while (inner_mirror_opaque > 0 && inner_row_index >= egui_canvas_circle_corner_get_opaque_threshold(items_inner, inner_mirror_opaque - 1))
            {
                inner_mirror_opaque--;
            }

            if (inner_row_index < inner_item_count)
            {
                inner_visible = stroke_width + (egui_dim_t)items_inner[inner_row_index].start_offset;
                inner_opaque = stroke_width + egui_canvas_circle_corner_get_opaque_threshold(items_inner, inner_row_index);
            }
            else
            {
                inner_visible = stroke_width + inner_item_count;
                inner_opaque = stroke_width + inner_row_index;
            }

            inner_row_limit = EGUI_MIN(inner_row_index, inner_item_count);
            if (inner_mirror_visible < inner_row_limit)
            {
                inner_visible = stroke_width + EGUI_MIN((egui_dim_t)(inner_visible - stroke_width), inner_mirror_visible);
            }
            if (inner_mirror_opaque < inner_row_limit)
            {
                inner_opaque = stroke_width + EGUI_MIN((egui_dim_t)(inner_opaque - stroke_width), inner_mirror_opaque);
            }
            if (inner_visible > radius)
            {
                inner_visible = radius;
            }
            if (inner_opaque > radius)
            {
                inner_opaque = radius;
            }
        }

        if (use_direct_pfb)
        {
            egui_dim_t py = center_y + sign_y * sel_y;
            egui_color_t *dst_row = (egui_color_t *)&self->pfb[(py - pfb_ofs_y) * pfb_width];

            for (col_index = EGUI_MAX(col_index, col_index_start); col_index < col_index_end; col_index++)
            {
                sel_x = radius - col_index;

                // get the alpha value
                if (circle_alpha != EGUI_ALPHA_100)
                {
                    if (col_index >= outer_opaque)
                    {
                        circle_alpha = EGUI_ALPHA_100;
                    }
                    else
                    {
                        circle_alpha = egui_canvas_get_circle_corner_value_fixed_row(row_index, col_index, info, items);
                        if (circle_alpha == 0)
                        {
                            continue;
                        }
                    }
                }

                mix_alpha = apply_draw_alpha ? egui_color_alpha_mix(circle_alpha, alpha) : circle_alpha;
                // check inner circle
                if (col_index >= inner_visible)
                {
                    if (col_index >= inner_opaque)
                    {
                        break;
                    }
                    else
                    {
                        egui_alpha_t alpha_inner =
                                egui_canvas_get_circle_corner_value_fixed_row(row_index - stroke_width, col_index - stroke_width, info_inner, items_inner);
                        if (alpha_inner != 0)
                        {
                            mix_alpha = egui_color_alpha_mix(mix_alpha, EGUI_ALPHA_100 - alpha_inner);
                        }
                    }
                }

                if (!egui_canvas_arc_try_apply_edge_alpha(&scan_state, sel_x, sel_y, &mix_alpha))
                {
                    continue;
                }

                egui_alpha_t eff_alpha = apply_canvas_alpha ? egui_color_alpha_mix(self_alpha, mix_alpha) : mix_alpha;
                if (eff_alpha == 0)
                {
                    continue;
                }

                egui_dim_t px = center_x + sign_x * sel_x;
                egui_color_t *dst = &dst_row[px - pfb_ofs_x];
                if (eff_alpha == EGUI_ALPHA_100)
                {
                    *dst = color;
                }
                else
                {
                    egui_rgb_mix_ptr(dst, &color, dst, eff_alpha);
                }
            }
        }
        else
        {
            for (col_index = EGUI_MAX(col_index, col_index_start); col_index < col_index_end; col_index++)
            {
                sel_x = radius - col_index;

                // get the alpha value
                if (circle_alpha != EGUI_ALPHA_100)
                {
                    if (col_index >= outer_opaque)
                    {
                        circle_alpha = EGUI_ALPHA_100;
                    }
                    else
                    {
                        circle_alpha = egui_canvas_get_circle_corner_value_fixed_row(row_index, col_index, info, items);
                        if (circle_alpha == 0)
                        {
                            continue;
                        }
                    }
                }

                mix_alpha = apply_draw_alpha ? egui_color_alpha_mix(circle_alpha, alpha) : circle_alpha;
                // check inner circle
                if (col_index >= inner_visible)
                {
                    if (col_index >= inner_opaque)
                    {
                        break;
                    }
                    else
                    {
                        egui_alpha_t alpha_inner =
                                egui_canvas_get_circle_corner_value_fixed_row(row_index - stroke_width, col_index - stroke_width, info_inner, items_inner);
                        if (alpha_inner != 0)
                        {
                            mix_alpha = egui_color_alpha_mix(mix_alpha, EGUI_ALPHA_100 - alpha_inner);
                        }
                    }
                }

                if (!egui_canvas_arc_try_apply_edge_alpha(&scan_state, sel_x, sel_y, &mix_alpha))
                {
                    continue;
                }

                egui_canvas_draw_point_limit(center_x + sign_x * sel_x, center_y + sign_y * sel_y, color, mix_alpha);
            }
        }

        scan_state.last_start_x = scan_state.cur_start_x;
        scan_state.last_end_x = scan_state.cur_end_x;
    }
}

/**
 * \brief           Draw filled circle
 * \param[in,out]   self: Pointer to \ref egui_canvas_t structure for display operations
 * \param[in]       x: X position of circle center
 * \param[in]       y: X position of circle center
 * \param[in]       r: Circle radius
 * \param[in]       color: Color used for drawing operation
 * \sa              egui_canvas_draw_circle_basic, egui_canvas_draw_circle_corner, egui_canvas_draw_circle_corner_fill
 */
void egui_canvas_draw_arc_basic(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, int16_t start_angle, int16_t end_angle, egui_dim_t stroke_width,
                                egui_color_t color, egui_alpha_t alpha)
{
    // egui_canvas_t *self = &canvas_data;

    int16_t start_angle_tmp;
    int16_t end_angle_tmp;
    int type;

    // if radius <= stroke_width, draw a filled circle
    if (radius <= stroke_width)
    {
        egui_canvas_draw_arc_fill_basic(center_x, center_y, radius, start_angle, end_angle, color, alpha);
        return;
    }

    if (start_angle < 0 || end_angle < 0)
    {
        // not support
        return;
    }

    if (start_angle > end_angle)
    {
        // not support
        return;
    }

    if (start_angle == end_angle)
    {
        // do nothing
        return;
    }

    // for some case, arc need over flow 0. can use 360 to fix it.
    if (start_angle > 720 || end_angle > 720)
    {
        // not support
        return;
    }

    if (egui_canvas_arc_try_get_single_quadrant(start_angle, end_angle, &type, &start_angle_tmp, &end_angle_tmp))
    {
        egui_canvas_draw_arc_corner(center_x, center_y, radius, start_angle_tmp, end_angle_tmp, stroke_width, type, color, alpha);

        if (start_angle <= 0 && end_angle >= 0)
        {
            egui_canvas_draw_hline(center_x + radius - stroke_width + 1, center_y, stroke_width, color, alpha);
        }
        if (start_angle <= 90 && end_angle >= 90)
        {
            egui_canvas_draw_vline(center_x, center_y + radius - stroke_width + 1, stroke_width, color, alpha);
        }
        if (start_angle <= 180 && end_angle >= 180)
        {
            egui_canvas_draw_hline(center_x - radius, center_y, stroke_width, color, alpha);
        }
        if (start_angle <= 270 && end_angle >= 270)
        {
            egui_canvas_draw_vline(center_x, center_y - radius, stroke_width, color, alpha);
        }
        return;
    }

    // First draw area
    do
    {
        if (start_angle < 90)
        {
            start_angle_tmp = start_angle;
            end_angle_tmp = end_angle;
            egui_canvas_draw_arc_corner(center_x, center_y, radius, start_angle_tmp, end_angle_tmp, stroke_width, EGUI_CANVAS_CIRCLE_TYPE_RIGHT_BOTTOM, color,
                                        alpha);
        }
        if (start_angle < 180)
        {
            start_angle_tmp = start_angle - 90;
            end_angle_tmp = end_angle - 90;
            egui_canvas_draw_arc_corner(center_x, center_y, radius, 90 - end_angle_tmp, 90 - start_angle_tmp, stroke_width, EGUI_CANVAS_CIRCLE_TYPE_LEFT_BOTTOM,
                                        color, alpha);
        }

        if (start_angle < 270)
        {
            start_angle_tmp = start_angle - 180;
            end_angle_tmp = end_angle - 180;
            egui_canvas_draw_arc_corner(center_x, center_y, radius, start_angle_tmp, end_angle_tmp, stroke_width, EGUI_CANVAS_CIRCLE_TYPE_LEFT_TOP, color,
                                        alpha);
        }

        if (start_angle < 360)
        {
            start_angle_tmp = start_angle - 270;
            end_angle_tmp = end_angle - 270;
            egui_canvas_draw_arc_corner(center_x, center_y, radius, 90 - end_angle_tmp, 90 - start_angle_tmp, stroke_width, EGUI_CANVAS_CIRCLE_TYPE_RIGHT_TOP,
                                        color, alpha);
        }

        // draw the line
        if (start_angle <= 0 && end_angle >= 0)
        {
            egui_canvas_draw_hline(center_x + radius - stroke_width + 1, center_y, stroke_width, color, alpha);
        }
        if (start_angle <= 90 && end_angle >= 90)
        {
            egui_canvas_draw_vline(center_x, center_y + radius - stroke_width + 1, stroke_width, color, alpha);
        }
        if (start_angle <= 180 && end_angle >= 180)
        {
            egui_canvas_draw_hline(center_x - radius, center_y, stroke_width, color, alpha);
        }
        if (start_angle <= 270 && end_angle >= 270)
        {
            egui_canvas_draw_vline(center_x, center_y - radius, stroke_width, color, alpha);
        }

        start_angle = start_angle - 360;
        if (start_angle < 0)
        {
            start_angle = 0;
        }
        end_angle = end_angle - 360;
    } while (end_angle > 0);
}

#if TEST_CANVAS_TEST_WOKR
void egui_canvas_debug_print_data_array(void)
{
    EGUI_LOG_DBG("      ");
    for (int i = 0; i < self->pfb_region.size.width; i++)
    {
        EGUI_LOG_DBG("[%02d] ", i);
    }
    // next line
    EGUI_LOG_DBG("\n");

    // print data array
    for (int j = 0; j < self->pfb_region.size.height; j++)
    {
        EGUI_LOG_DBG("[%02d] ", j);
        for (int i = 0; i < self->pfb_region.size.width; i++)
        {
            EGUI_LOG_DBG(" %03d ", self->pfb[j * self->pfb_region.size.width + i] & 0xff);
        }
        // next line
        EGUI_LOG_DBG("\n");
    }
}

void egui_canvas_test_circle(void)
{
    egui_canvas_t canvas;
    egui_color_int_t pfb_arr[30 * 30];

    egui_api_memset(pfb_arr, 0, sizeof(pfb_arr));

    canvas.pfb = pfb_arr;
    canvas.pfb_region.size.width = 30;
    canvas.pfb_region.size.height = 30;
    canvas.pfb_region.location.x = 0;
    canvas.pfb_region.location.y = 0;

    canvas.base_view_work_region.size.width = 30;
    canvas.base_view_work_region.size.height = 30;
    canvas.base_view_work_region.location.x = 0;
    canvas.base_view_work_region.location.y = 0;

    canvas.pfb_location_in_base_view.x = 0;
    canvas.pfb_location_in_base_view.y = 0;

    canvas.alpha = 0xff;

    canvas.mask = NULL;

    egui_canvas_debug_print_data_array(&canvas);

    egui_dim_t radius = 12;
    // egui_dim_t central_x = 12;
    // egui_dim_t central_y = 12;

    egui_dim_t central_x = radius;
    egui_dim_t central_y = radius;

    // egui_canvas_draw_circle_corner_left_top(&canvas, central_x, central_y, radius, radius, EGUI_COLOR_WHITE, 0xff);
    // egui_canvas_draw_circle_corner_left_bottom(&canvas, central_x, central_y, radius, 3, EGUI_COLOR_WHITE, 0xff);
    // egui_canvas_draw_circle_corner_right_top(&canvas, central_x, central_y, radius, 3, EGUI_COLOR_WHITE, 0xff);
    // egui_canvas_draw_circle_corner_right_bottom(&canvas, central_x, central_y, radius, 3, EGUI_COLOR_WHITE, 0xff);

    // draw the center line
    // egui_canvas_draw_hline(&canvas, central_x - radius, central_y,          2 * radius + 1, EGUI_COLOR_WHITE, 0xFF);
    // egui_canvas_draw_vline(&canvas, central_x,          central_y - radius, 2 * radius + 1, EGUI_COLOR_WHITE, 0xFF);

    egui_canvas_draw_arc_corner_fill_right_bottom(&canvas, central_x, central_y, radius, 0, 30, EGUI_COLOR_WHITE, 0xff);
    // egui_canvas_draw_arc_corner_fill_right_bottom(&canvas, central_x, central_y, radius, 0, 45, EGUI_COLOR_WHITE, 0xff);

    egui_canvas_debug_print_data_array(&canvas);
    // while(1) {
    //     egui_api_delay(1000);
    // };
}
#endif

void egui_canvas_draw_text(const egui_font_t *font, const void *string, egui_dim_t x, egui_dim_t y, egui_color_t color, egui_alpha_t alpha)
{
    font->api->draw_string(font, string, x, y, color, alpha);
}

void egui_canvas_draw_text_in_rect_with_line_space(const egui_font_t *font, const void *string, egui_region_t *rect, uint8_t align_type, egui_dim_t line_space,
                                                   egui_color_t color, egui_alpha_t alpha)
{
    egui_font_draw_string_in_rect(font, string, rect, align_type, line_space, color, alpha);
}

void egui_canvas_draw_text_in_rect(const egui_font_t *font, const void *string, egui_region_t *rect, uint8_t align_type, egui_color_t color, egui_alpha_t alpha)
{
    egui_font_draw_string_in_rect(font, string, rect, align_type, 0, color, alpha);
}

void egui_canvas_draw_image(const egui_image_t *img, egui_dim_t x, egui_dim_t y)
{
    egui_image_draw_image(img, x, y);
}

void egui_canvas_draw_image_resize(const egui_image_t *img, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height)
{
    egui_image_draw_image_resize(img, x, y, width, height);
}

void egui_canvas_draw_image_color(const egui_image_t *img, egui_dim_t x, egui_dim_t y, egui_color_t color, egui_alpha_t alpha)
{
    egui_image_draw_image_color(img, x, y, color, alpha);
}

void egui_canvas_draw_image_resize_color(const egui_image_t *img, egui_dim_t x, egui_dim_t y, egui_dim_t width, egui_dim_t height, egui_color_t color,
                                         egui_alpha_t alpha)
{
    egui_image_draw_image_resize_color(img, x, y, width, height, color, alpha);
}

void egui_canvas_calc_work_region(egui_region_t *base_region)
{
    egui_canvas_t *self = &canvas_data;

    egui_region_t *region = &self->base_view_work_region;

    // Intersect base view region with PFB tile in screen coordinates.
    // Use fast version: both regions are guaranteed non-empty in draw path.
    egui_region_intersect_fast(base_region, &self->pfb_region, region);

    // If an extra clip is set (e.g. scroll view viewport), also intersect with it.
    // This prevents children of scroll views from rendering outside the visible viewport.
#if EGUI_CONFIG_CANVAS_EXTRA_CLIP_ENABLE
    if (self->extra_clip_region != NULL)
    {
        egui_region_t clipped;
        egui_region_intersect_fast(region, self->extra_clip_region, &clipped);
        *region = clipped;
    }
#endif

    // change to base_region coordinate.
    region->location.x -= base_region->location.x;
    region->location.y -= base_region->location.y;

    // calculate pfb_location_in_base_view in base_region coordinate.
    self->pfb_location_in_base_view.x = self->pfb_region.location.x - base_region->location.x;
    self->pfb_location_in_base_view.y = self->pfb_region.location.y - base_region->location.y;
}

void egui_canvas_register_spec_circle_info(uint16_t res_circle_info_count_spec, const egui_circle_info_t *res_circle_info_spec_arr)
{
#if EGUI_CONFIG_CANVAS_SPEC_CIRCLE_INFO_ENABLE
    egui_canvas_t *self = &canvas_data;

    self->res_circle_info_count_spec = res_circle_info_count_spec;
    self->res_circle_info_spec_arr = res_circle_info_spec_arr;
#else
    EGUI_UNUSED(res_circle_info_count_spec);
    EGUI_UNUSED(res_circle_info_spec_arr);
#endif
}

void egui_canvas_init(egui_color_int_t *pfb, egui_region_t *region)
{
    egui_canvas_t *self = &canvas_data;

    self->pfb = pfb;
    egui_region_copy(&self->pfb_region, region);

    self->alpha = EGUI_ALPHA_100;
#if EGUI_CONFIG_CANVAS_EXTRA_CLIP_ENABLE
    self->extra_clip_region = NULL;
#endif
}
