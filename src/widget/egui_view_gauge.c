#include <stdio.h>
#include <assert.h>

#include "egui_view_gauge.h"
#include "core/egui_common.h"
#include "utils/egui_fixmath.h"
#include "utils/egui_sprintf.h"
#include "font/egui_font_std.h"
#include "resource/egui_resource.h"
#include "egui_view_circle_dirty.h"

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
#include "core/egui_canvas_gradient.h"
#endif

static int egui_view_gauge_get_font_code_index(const egui_font_std_info_t *font, uint32_t utf8_code)
{
    int first = 0;
    int last;
    int middle;

    if (font == NULL || font->count == 0)
    {
        return -1;
    }

    last = font->count - 1;
    middle = (first + last) / 2;
    while (first <= last)
    {
        if (font->code_array[middle].code < utf8_code)
        {
            first = middle + 1;
        }
        else if (font->code_array[middle].code == utf8_code)
        {
            return middle;
        }
        else
        {
            last = middle - 1;
        }
        middle = (first + last) / 2;
    }

    return -1;
}

static const egui_font_std_char_descriptor_t *egui_view_gauge_get_font_desc(const egui_font_t *text_font, uint32_t utf8_code)
{
    const egui_font_std_info_t *font_info;
    int code_index;

    if (text_font == NULL || text_font->res == NULL)
    {
        return NULL;
    }

    font_info = (const egui_font_std_info_t *)text_font->res;
    if (font_info->res_type != EGUI_RESOURCE_TYPE_INTERNAL || font_info->char_array == NULL || font_info->code_array == NULL)
    {
        return NULL;
    }

    code_index = egui_view_gauge_get_font_code_index(font_info, utf8_code);
    if (code_index < 0)
    {
        return NULL;
    }

    return &font_info->char_array[code_index];
}

static uint8_t egui_view_gauge_measure_text(const egui_font_t *text_font, const char *text, egui_dim_t *text_w, egui_dim_t *text_h)
{
    const egui_font_std_info_t *font_info;
    const egui_font_std_char_descriptor_t *char_desc;
    const char *cursor;
    uint32_t utf8_code;
    int utf8_bytes;
    egui_dim_t width;

    if (text_w == NULL || text_h == NULL)
    {
        return 0;
    }

    *text_w = 0;
    *text_h = 0;

    if (text_font == NULL || text == NULL)
    {
        return 0;
    }

    if (text_font->api != NULL && text_font->api->get_str_size != NULL)
    {
        return (text_font->api->get_str_size(text_font, text, 0, 0, text_w, text_h) == 0) ? 1 : 0;
    }

    if (text_font->res == NULL)
    {
        return 0;
    }

    font_info = (const egui_font_std_info_t *)text_font->res;
    width = 0;
    cursor = text;
    while (*cursor != '\0')
    {
        utf8_bytes = egui_font_get_utf8_code(cursor, &utf8_code);
        if (utf8_bytes <= 0)
        {
            return 0;
        }

        char_desc = egui_view_gauge_get_font_desc(text_font, utf8_code);
        width += (char_desc != NULL) ? char_desc->adv : EGUI_MAX((egui_dim_t)1, (egui_dim_t)(font_info->height / 2));
        cursor += utf8_bytes;
    }

    *text_w = width;
    *text_h = font_info->height;
    return (width > 0 && font_info->height > 0) ? 1 : 0;
}

static void egui_view_gauge_format_text(uint8_t value, char *buf, uint32_t buf_size)
{
    if (buf == NULL || buf_size < 2)
    {
        return;
    }

    egui_sprintf_int(buf, buf_size - 1, (int32_t)value);
}

static uint8_t egui_view_gauge_get_text_region(const egui_font_t *text_font, egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, egui_dim_t inner_r,
                                               uint8_t value, egui_region_t *text_region)
{
    egui_region_t text_box;
    egui_dim_t text_w;
    egui_dim_t text_h;
    egui_dim_t offset_x;
    egui_dim_t offset_y;
    egui_dim_t font_h;
    egui_dim_t lbl_w;
    egui_dim_t lbl_h;
    char text_buf[8];

    if (text_font == NULL || text_region == NULL)
    {
        return 0;
    }

    font_h = (egui_dim_t)EGUI_FONT_STD_GET_FONT_HEIGHT(text_font);
    lbl_w = inner_r * 2 - 4;
    lbl_h = font_h + 4;
    text_box.location.x = center_x - lbl_w / 2;
    text_box.location.y = center_y + EGUI_MAX(radius / 10, 3) + 1;
    text_box.size.width = lbl_w;
    text_box.size.height = lbl_h;
    if (egui_region_is_empty(&text_box))
    {
        return 0;
    }

    egui_view_gauge_format_text(value, text_buf, sizeof(text_buf));
    if (!egui_view_gauge_measure_text(text_font, text_buf, &text_w, &text_h) || text_w <= 0 || text_h <= 0)
    {
        egui_region_copy(text_region, &text_box);
        return 1;
    }

    text_w = EGUI_MIN(text_w, text_box.size.width);
    text_h = EGUI_MIN(text_h, text_box.size.height);
    egui_common_align_get_x_y(text_box.size.width, text_box.size.height, text_w, text_h, EGUI_ALIGN_CENTER, &offset_x, &offset_y);

    text_region->location.x = text_box.location.x + offset_x;
    text_region->location.y = text_box.location.y + offset_y;
    text_region->size.width = text_w;
    text_region->size.height = text_h;

    if (text_region->location.x > text_box.location.x)
    {
        text_region->location.x--;
        text_region->size.width++;
    }
    if (text_region->location.y > text_box.location.y)
    {
        text_region->location.y--;
        text_region->size.height++;
    }
    if (text_region->location.x + text_region->size.width < text_box.location.x + text_box.size.width)
    {
        text_region->size.width++;
    }
    if (text_region->location.y + text_region->size.height < text_box.location.y + text_box.size.height)
    {
        text_region->size.height++;
    }

    egui_region_intersect(text_region, &text_box, text_region);
    return egui_region_is_empty(text_region) ? 0 : 1;
}

static const egui_font_t *egui_view_gauge_get_text_font(egui_view_gauge_t *local, egui_dim_t inner_r)
{
    if (local->font != NULL)
    {
        return local->font;
    }

    if (inner_r >= 40)
    {
        return (const egui_font_t *)&egui_res_font_montserrat_14_4;
    }
    if (inner_r >= 28)
    {
        return (const egui_font_t *)&egui_res_font_montserrat_12_4;
    }
    if (inner_r >= 18)
    {
        return (const egui_font_t *)&egui_res_font_montserrat_8_4;
    }
    return NULL;
}

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
__attribute__((optimize("Os"))) static void egui_view_gauge_draw_ring_gradient(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius, egui_dim_t inner_r,
                                                                               int16_t start_angle, int16_t end_angle,
                                                                               egui_color_t start_color, egui_color_t end_color)
{
    egui_gradient_stop_t stops[2] = {
            {.position = 0, .color = start_color},
            {.position = 255, .color = end_color},
    };
    egui_gradient_t grad = {
            .type = EGUI_GRADIENT_TYPE_RADIAL,
            .stop_count = 2,
            .alpha = EGUI_ALPHA_100,
            .stops = stops,
    };

    egui_canvas_draw_arc_ring_fill_gradient(center_x, center_y, radius, inner_r, start_angle, end_angle, &grad);
}

__attribute__((optimize("Os"))) static void egui_view_gauge_draw_center_dot_gradient(egui_dim_t center_x, egui_dim_t center_y, egui_dim_t radius,
                                                                                      egui_color_t start_color, egui_color_t end_color)
{
    egui_gradient_stop_t stops[2] = {
            {.position = 0, .color = start_color},
            {.position = 255, .color = end_color},
    };
    egui_gradient_t grad = {
            .type = EGUI_GRADIENT_TYPE_RADIAL,
            .stop_count = 2,
            .alpha = EGUI_ALPHA_100,
            .stops = stops,
    };

    egui_canvas_draw_circle_fill_gradient(center_x, center_y, radius, &grad);
}
#endif

__attribute__((optimize("Os"))) static void egui_view_gauge_draw_value_text(egui_view_gauge_t *local, egui_dim_t center_x, egui_dim_t center_y,
                                                                             egui_dim_t inner_r, egui_dim_t center_dot_r)
{
    const egui_font_t *text_font;
    char val_buf[8];
    egui_region_t text_rect;
    egui_dim_t font_h;
    egui_dim_t lbl_w;
    egui_dim_t lbl_h;

    text_font = egui_view_gauge_get_text_font(local, inner_r);
    if (text_font == NULL)
    {
        return;
    }

    egui_sprintf_int(val_buf, sizeof(val_buf), (int32_t)local->value);
    font_h = (egui_dim_t)EGUI_FONT_STD_GET_FONT_HEIGHT(text_font);
    lbl_w = inner_r * 2 - 4;
    lbl_h = font_h + 4;
    text_rect.location.x = center_x - lbl_w / 2;
    text_rect.location.y = center_y + center_dot_r + 1;
    text_rect.size.width = lbl_w;
    text_rect.size.height = lbl_h;
    egui_canvas_draw_text_in_rect(text_font, val_buf, &text_rect, EGUI_ALIGN_CENTER, local->text_color, EGUI_ALPHA_100);
}

static void egui_view_gauge_invalidate_value_change(egui_view_t *self, egui_view_gauge_t *local, uint8_t old_value)
{
    egui_region_t region;
    egui_region_t indicator_dirty_region;
    egui_region_t arc_region;
    egui_region_t text_region;
    egui_region_t text_dirty_region;
    const egui_font_t *text_font;
    egui_dim_t center_x;
    egui_dim_t center_y;
    egui_dim_t radius;
    egui_dim_t inner_r;
    egui_dim_t mid_r;
    egui_dim_t needle_len;
    egui_dim_t tip_x;
    egui_dim_t tip_y;
    int16_t old_end_angle;
    int16_t new_end_angle;
    int16_t dirty_start_angle;
    uint16_t dirty_sweep;

    if (self->region_screen.size.width <= 0 || self->region_screen.size.height <= 0)
    {
        egui_view_invalidate(self);
        return;
    }

    egui_view_get_work_region(self, &region);
    center_x = region.location.x + region.size.width / 2;
    center_y = region.location.y + region.size.height / 2;
    radius = EGUI_MIN(region.size.width, region.size.height) / 2 - local->stroke_width / 2 - 1;
    if (radius <= 0)
    {
        egui_view_invalidate(self);
        return;
    }

    inner_r = radius - local->stroke_width;
    if (inner_r < 0)
    {
        inner_r = 0;
    }
    mid_r = radius - local->stroke_width / 2;
    if (mid_r < 0)
    {
        mid_r = 0;
    }
    needle_len = radius - local->stroke_width / 2 - 4;
    if (needle_len < 0)
    {
        needle_len = 0;
    }

    egui_region_init_empty(&indicator_dirty_region);
    egui_region_init_empty(&text_dirty_region);

    old_end_angle = local->start_angle + (int16_t)((int32_t)local->sweep_angle * old_value / 100);
    new_end_angle = local->start_angle + (int16_t)((int32_t)local->sweep_angle * local->value / 100);
    if (old_end_angle != new_end_angle)
    {
        dirty_start_angle = EGUI_MIN(old_end_angle, new_end_angle);
        dirty_sweep = (uint16_t)EGUI_ABS(new_end_angle - old_end_angle);
        if (egui_view_circle_dirty_compute_arc_region(center_x, center_y, mid_r, local->stroke_width / 2 + EGUI_VIEW_CIRCLE_DIRTY_AA_PAD, dirty_start_angle,
                                                      dirty_sweep, &arc_region))
        {
            egui_view_circle_dirty_union_region(&indicator_dirty_region, &arc_region);
        }
    }

    egui_view_circle_dirty_get_circle_point(center_x, center_y, needle_len, old_end_angle, &tip_x, &tip_y);
    egui_view_circle_dirty_add_line_region(&indicator_dirty_region, center_x, center_y, tip_x, tip_y, local->needle_width, EGUI_VIEW_CIRCLE_DIRTY_AA_PAD);

    egui_view_circle_dirty_get_circle_point(center_x, center_y, needle_len, new_end_angle, &tip_x, &tip_y);
    egui_view_circle_dirty_add_line_region(&indicator_dirty_region, center_x, center_y, tip_x, tip_y, local->needle_width, EGUI_VIEW_CIRCLE_DIRTY_AA_PAD);

    text_font = egui_view_gauge_get_text_font(local, inner_r);
    if (text_font != NULL)
    {
        if (egui_view_gauge_get_text_region(text_font, center_x, center_y, radius, inner_r, old_value, &text_region))
        {
            egui_view_circle_dirty_union_region(&text_dirty_region, &text_region);
        }
        if (old_value != local->value && egui_view_gauge_get_text_region(text_font, center_x, center_y, radius, inner_r, local->value, &text_region))
        {
            egui_view_circle_dirty_union_region(&text_dirty_region, &text_region);
        }
    }

    if (egui_region_is_empty(&indicator_dirty_region) && egui_region_is_empty(&text_dirty_region))
    {
        egui_view_invalidate(self);
        return;
    }

    if (!egui_region_is_empty(&indicator_dirty_region))
    {
        egui_view_invalidate_region(self, &indicator_dirty_region);
    }
    if (!egui_region_is_empty(&text_dirty_region))
    {
        egui_view_invalidate_region(self, &text_dirty_region);
    }
}

void egui_view_gauge_set_value(egui_view_t *self, uint8_t value)
{
    EGUI_LOCAL_INIT(egui_view_gauge_t);
    uint8_t old_value;
    if (value > 100)
    {
        value = 100;
    }
    if (value != local->value)
    {
        old_value = local->value;
        local->value = value;
        egui_view_gauge_invalidate_value_change(self, local, old_value);
    }
}

void egui_view_gauge_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_gauge_t);
    local->font = font;
    egui_view_invalidate(self);
}

void egui_view_gauge_set_text_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_gauge_t);
    local->text_color = color;
    egui_view_invalidate(self);
}

__attribute__((optimize("Os"))) void egui_view_gauge_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_gauge_t);

    egui_region_t region;
    egui_view_get_work_region(self, &region);

    egui_dim_t w = region.size.width;
    egui_dim_t h = region.size.height;
    egui_dim_t center_x = region.location.x + w / 2;
    egui_dim_t center_y = region.location.y + h / 2;
    egui_dim_t radius = EGUI_MIN(w, h) / 2 - local->stroke_width / 2 - 1;

    if (radius <= 0)
    {
        return;
    }

    egui_dim_t inner_r = radius - local->stroke_width;
    if (inner_r < 0)
        inner_r = 0;

    // Background arc (full sweep)
    int16_t bg_start = local->start_angle;
    int16_t bg_end = local->start_angle + local->sweep_angle;
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
    egui_view_gauge_draw_ring_gradient(center_x, center_y, radius, inner_r, bg_start, bg_end, local->bk_color, local->bk_color);
#else
    egui_canvas_draw_arc(center_x, center_y, radius, bg_start, bg_end, local->stroke_width, local->bk_color, EGUI_ALPHA_100);
#endif

    // Progress arc
    int16_t progress_end = local->start_angle + (int16_t)((int32_t)local->sweep_angle * local->value / 100);
    if (local->value > 0)
    {
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
        egui_color_t color_light = egui_rgb_mix(local->progress_color, EGUI_COLOR_WHITE, 80);
        egui_view_gauge_draw_ring_gradient(center_x, center_y, radius, inner_r, bg_start, progress_end, color_light, local->progress_color);
#else
        egui_canvas_draw_arc(center_x, center_y, radius, bg_start, progress_end, local->stroke_width, local->progress_color, EGUI_ALPHA_100);
#endif
    }

    // Needle - analog clock style hand
    egui_dim_t needle_len = radius - local->stroke_width / 2 - 4;
    egui_dim_t center_dot_r = EGUI_MAX(radius / 10, 3);

    // Tick marks: 10 divisions, minor every 10%, major at 0%/50%/100%
    {
        egui_dim_t tick_base = inner_r - 3; // just inside the arc ring
        for (int i = 0; i <= 10; i++)
        {
            int16_t tick_deg = local->start_angle + (int16_t)((int32_t)local->sweep_angle * i / 10);
            egui_float_t a = EGUI_FLOAT_DIV(EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(tick_deg), EGUI_FLOAT_PI), EGUI_FLOAT_VALUE_INT(180));
            egui_float_t c = EGUI_FLOAT_COS(a);
            egui_float_t s = EGUI_FLOAT_SIN(a);
            int is_major = (i % 5 == 0);
            egui_dim_t tlen = is_major ? 9 : 5;
            egui_alpha_t ta = is_major ? 200 : 110;
            egui_dim_t x1 = center_x + (egui_dim_t)EGUI_FLOAT_INT_PART(EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(tick_base), c));
            egui_dim_t y1 = center_y + (egui_dim_t)EGUI_FLOAT_INT_PART(EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(tick_base), s));
            egui_dim_t x2 = center_x + (egui_dim_t)EGUI_FLOAT_INT_PART(EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(tick_base - tlen), c));
            egui_dim_t y2 = center_y + (egui_dim_t)EGUI_FLOAT_INT_PART(EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(tick_base - tlen), s));
            egui_canvas_draw_line(x1, y1, x2, y2, is_major ? 2 : 1, local->needle_color, ta);
        }
    }
    if (needle_len > 0)
    {
        // angle_deg = start_angle + value * sweep_angle / 100
        int16_t needle_deg = local->start_angle + (int16_t)((int32_t)local->sweep_angle * local->value / 100);
        // Convert degrees to fixed-point radians: rad = deg * PI / 180
        egui_float_t angle_rad = EGUI_FLOAT_DIV(EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(needle_deg), EGUI_FLOAT_PI), EGUI_FLOAT_VALUE_INT(180));

        egui_float_t cos_val = EGUI_FLOAT_COS(angle_rad);
        egui_float_t sin_val = EGUI_FLOAT_SIN(angle_rad);

        egui_dim_t tip_x = center_x + (egui_dim_t)EGUI_FLOAT_INT_PART(EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(needle_len), cos_val));
        egui_dim_t tip_y = center_y + (egui_dim_t)EGUI_FLOAT_INT_PART(EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(needle_len), sin_val));
        egui_dim_t hand_w = local->needle_width;
        egui_canvas_draw_line_round_cap_hq(center_x, center_y, tip_x, tip_y, hand_w, local->needle_color, EGUI_ALPHA_100);
    }

    // Center dot decoration
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
    egui_color_t color_light = egui_rgb_mix(local->needle_color, EGUI_COLOR_WHITE, 80);
    egui_view_gauge_draw_center_dot_gradient(center_x, center_y, center_dot_r, color_light, local->needle_color);
#else
    egui_canvas_draw_circle_fill(center_x, center_y, center_dot_r, local->needle_color, EGUI_ALPHA_100);
#endif

    // Center value text below the pivot dot
    // Auto-select font based on inner_r when user has not explicitly set one
    egui_view_gauge_draw_value_text(local, center_x, center_y, inner_r, center_dot_r);
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_gauge_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_gauge_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_gauge_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_gauge_t);
    // call super init.
    egui_view_init(self);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_gauge_t);

    // init local data.
    local->value = 0;
    local->stroke_width = EGUI_THEME_TRACK_THICKNESS;
    local->start_angle = 150; // ~7 o'clock (0=3 o'clock, clockwise)
    local->sweep_angle = 240; // sweep 240 degrees, gap at bottom
    local->bk_color = EGUI_THEME_TRACK_BG;
    local->progress_color = EGUI_THEME_PRIMARY;
    local->needle_color = EGUI_THEME_DANGER;
    local->needle_width = 3;
    local->text_color = EGUI_THEME_TEXT_PRIMARY;
    local->font = NULL; // NULL means auto-select based on gauge size

    egui_view_set_view_name(self, "egui_view_gauge");
}

void egui_view_gauge_apply_params(egui_view_t *self, const egui_view_gauge_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_gauge_t);

    self->region = params->region;

    local->value = params->value;

    egui_view_invalidate(self);
}

void egui_view_gauge_init_with_params(egui_view_t *self, const egui_view_gauge_params_t *params)
{
    egui_view_gauge_init(self);
    egui_view_gauge_apply_params(self, params);
}
