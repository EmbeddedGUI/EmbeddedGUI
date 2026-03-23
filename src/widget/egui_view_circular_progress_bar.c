#include <stdio.h>
#include <assert.h>

#include "egui_view_circular_progress_bar.h"
#include "core/egui_common.h"
#include "font/egui_font_std.h"
#include "resource/egui_resource.h"
#include "utils/egui_sprintf.h"
#include "egui_view_circle_dirty.h"

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
#include "core/egui_canvas_gradient.h"
#endif

static int egui_view_circular_progress_bar_get_font_code_index(const egui_font_std_info_t *font, uint32_t utf8_code)
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

static const egui_font_std_char_descriptor_t *egui_view_circular_progress_bar_get_font_desc(const egui_font_t *text_font, uint32_t utf8_code)
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

    code_index = egui_view_circular_progress_bar_get_font_code_index(font_info, utf8_code);
    if (code_index < 0)
    {
        return NULL;
    }

    return &font_info->char_array[code_index];
}

static uint8_t egui_view_circular_progress_bar_measure_text(const egui_font_t *text_font, const char *text, egui_dim_t *text_w, egui_dim_t *text_h)
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

        char_desc = egui_view_circular_progress_bar_get_font_desc(text_font, utf8_code);
        width += (char_desc != NULL) ? char_desc->adv : EGUI_MAX((egui_dim_t)1, (egui_dim_t)(font_info->height / 2));
        cursor += utf8_bytes;
    }

    *text_w = width;
    *text_h = font_info->height;
    return (width > 0 && font_info->height > 0) ? 1 : 0;
}

static const egui_font_t *egui_view_circular_progress_bar_get_text_font(egui_view_circular_progress_bar_t *local, egui_dim_t inner_r)
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

static void egui_view_circular_progress_bar_format_text(uint8_t process, char *buf, uint32_t buf_size)
{
    int len = 0;

    if (buf == NULL || buf_size < 2)
    {
        return;
    }

    egui_sprintf_int(buf, buf_size - 1, (int32_t)process);
    while (buf[len] != '\0' && len < (int)(buf_size - 1))
    {
        len++;
    }
    if (len < (int)(buf_size - 2))
    {
        buf[len] = '%';
        buf[len + 1] = '\0';
    }
}

static uint8_t egui_view_circular_progress_bar_get_text_region(const egui_font_t *text_font, egui_dim_t center_x, egui_dim_t center_y, egui_dim_t inner_r,
                                                               uint8_t process, egui_region_t *text_region)
{
    egui_region_t text_box;
    egui_dim_t text_w;
    egui_dim_t text_h;
    egui_dim_t offset_x;
    egui_dim_t offset_y;
    char text_buf[8];

    if (text_font == NULL || text_region == NULL)
    {
        return 0;
    }

    text_box.location.x = center_x - (inner_r * 2 - 4) / 2;
    text_box.location.y = center_y - ((egui_dim_t)EGUI_FONT_STD_GET_FONT_HEIGHT(text_font) + 4) / 2;
    text_box.size.width = inner_r * 2 - 4;
    text_box.size.height = (egui_dim_t)EGUI_FONT_STD_GET_FONT_HEIGHT(text_font) + 4;
    if (egui_region_is_empty(&text_box))
    {
        return 0;
    }

    egui_view_circular_progress_bar_format_text(process, text_buf, sizeof(text_buf));
    if (!egui_view_circular_progress_bar_measure_text(text_font, text_buf, &text_w, &text_h) || text_w <= 0 || text_h <= 0)
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

static void egui_view_circular_progress_bar_invalidate_process_change(egui_view_t *self, egui_view_circular_progress_bar_t *local, uint8_t old_process)
{
    egui_region_t region;
    egui_region_t arc_region;
    egui_region_t text_region;
    egui_region_t text_dirty_region;
    const egui_font_t *text_font;
    egui_dim_t center_x;
    egui_dim_t center_y;
    egui_dim_t radius;
    egui_dim_t inner_r;
    egui_dim_t mid_r;
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

    egui_region_init_empty(&arc_region);
    egui_region_init_empty(&text_dirty_region);

    old_end_angle = local->start_angle + (int16_t)((uint32_t)old_process * 360U / 100U);
    new_end_angle = local->start_angle + (int16_t)((uint32_t)local->process * 360U / 100U);
    if (old_end_angle != new_end_angle)
    {
        dirty_start_angle = EGUI_MIN(old_end_angle, new_end_angle);
        dirty_sweep = (uint16_t)EGUI_ABS(new_end_angle - old_end_angle);
        egui_view_circle_dirty_compute_arc_region(center_x, center_y, mid_r, local->stroke_width / 2 + EGUI_VIEW_CIRCLE_DIRTY_AA_PAD, dirty_start_angle,
                                                  dirty_sweep, &arc_region);
    }

    text_font = egui_view_circular_progress_bar_get_text_font(local, inner_r);
    if (text_font != NULL)
    {
        if (egui_view_circular_progress_bar_get_text_region(text_font, center_x, center_y, inner_r, old_process, &text_region))
        {
            egui_view_circle_dirty_union_region(&text_dirty_region, &text_region);
        }
        if (old_process != local->process &&
            egui_view_circular_progress_bar_get_text_region(text_font, center_x, center_y, inner_r, local->process, &text_region))
        {
            egui_view_circle_dirty_union_region(&text_dirty_region, &text_region);
        }
    }

    if (egui_region_is_empty(&arc_region) && egui_region_is_empty(&text_dirty_region))
    {
        egui_view_invalidate(self);
        return;
    }

    if (!egui_region_is_empty(&arc_region))
    {
        egui_view_invalidate_region(self, &arc_region);
    }
    if (!egui_region_is_empty(&text_dirty_region))
    {
        egui_view_invalidate_region(self, &text_dirty_region);
    }
}

void egui_view_circular_progress_bar_set_on_progress_listener(egui_view_t *self, egui_view_on_progress_changed_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_circular_progress_bar_t);
    local->on_progress_changed = listener;
}

void egui_view_circular_progress_bar_set_process(egui_view_t *self, uint8_t process)
{
    EGUI_LOCAL_INIT(egui_view_circular_progress_bar_t);
    uint8_t old_process;
    if (process > 100)
    {
        process = 100;
    }
    if (process != local->process)
    {
        old_process = local->process;
        local->process = process;
        if (local->on_progress_changed)
        {
            local->on_progress_changed(self, process);
        }

        egui_view_circular_progress_bar_invalidate_process_change(self, local, old_process);
    }
}

void egui_view_circular_progress_bar_set_stroke_width(egui_view_t *self, egui_dim_t stroke_width)
{
    EGUI_LOCAL_INIT(egui_view_circular_progress_bar_t);
    if (stroke_width != local->stroke_width)
    {
        local->stroke_width = stroke_width;
        egui_view_invalidate(self);
    }
}

void egui_view_circular_progress_bar_set_progress_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_circular_progress_bar_t);
    local->progress_color = color;
    egui_view_invalidate(self);
}

void egui_view_circular_progress_bar_set_bk_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_circular_progress_bar_t);
    local->bk_color = color;
    egui_view_invalidate(self);
}

void egui_view_circular_progress_bar_set_text_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_circular_progress_bar_t);
    local->text_color = color;
    egui_view_invalidate(self);
}

void egui_view_circular_progress_bar_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_circular_progress_bar_t);
    local->font = font;
    egui_view_invalidate(self);
}

void egui_view_circular_progress_bar_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_circular_progress_bar_t);

    egui_region_t region;
    egui_view_get_work_region(self, &region);

    // Calculate center and radius from the widget region
    egui_dim_t center_x = region.location.x + region.size.width / 2;
    egui_dim_t center_y = region.location.y + region.size.height / 2;
    egui_dim_t radius = EGUI_MIN(region.size.width, region.size.height) / 2 - local->stroke_width / 2 - 1;

    if (radius <= 0)
    {
        return;
    }

    egui_dim_t inner_r = radius - local->stroke_width;
    if (inner_r < 0)
        inner_r = 0;

    // Draw background arc (full 360 degrees)
    // In Enhanced mode use ring fill so geometry matches the progress ring fill below
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
    {
        egui_gradient_stop_t bg_stops[2] = {
                {.position = 0, .color = local->bk_color},
                {.position = 255, .color = local->bk_color},
        };
        egui_gradient_t bg_grad = {
                .type = EGUI_GRADIENT_TYPE_RADIAL,
                .stop_count = 2,
                .alpha = EGUI_ALPHA_100,
                .stops = bg_stops,
        };
        egui_canvas_draw_arc_ring_fill_gradient(center_x, center_y, radius, inner_r, 0, 360, &bg_grad);
    }
#else
    egui_canvas_draw_arc(center_x, center_y, radius, 0, 360, local->stroke_width, local->bk_color, EGUI_ALPHA_100);
#endif

    // Draw progress arc
    if (local->process > 0)
    {
        int16_t end_angle = local->start_angle + (int16_t)((uint32_t)local->process * 360 / 100);
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
        {
            egui_color_t color_light = egui_rgb_mix(local->progress_color, EGUI_COLOR_WHITE, 80);
            egui_gradient_stop_t stops[2] = {
                    {.position = 0, .color = color_light},
                    {.position = 255, .color = local->progress_color},
            };
            egui_gradient_t grad = {
                    .type = EGUI_GRADIENT_TYPE_RADIAL,
                    .stop_count = 2,
                    .alpha = EGUI_ALPHA_100,
                    .stops = stops,
            };
            egui_canvas_draw_arc_ring_fill_gradient_round_cap(center_x, center_y, radius, inner_r, local->start_angle, end_angle, &grad, EGUI_ARC_CAP_BOTH);
        }
#else
        egui_canvas_draw_arc(center_x, center_y, radius, local->start_angle, end_angle, local->stroke_width, local->progress_color, EGUI_ALPHA_100);
#endif
    }

    // Center percentage text — auto-select font by inner_r
    {
        const egui_font_t *text_font = egui_view_circular_progress_bar_get_text_font(local, inner_r);
        if (text_font != NULL)
        {
            char val_buf[8];
            egui_view_circular_progress_bar_format_text(local->process, val_buf, sizeof(val_buf));
            egui_dim_t font_h = (egui_dim_t)EGUI_FONT_STD_GET_FONT_HEIGHT(text_font);
            egui_dim_t lbl_w = inner_r * 2 - 4;
            egui_dim_t lbl_h = font_h + 4;
            egui_region_t text_rect;
            text_rect.location.x = center_x - lbl_w / 2;
            text_rect.location.y = center_y - lbl_h / 2;
            text_rect.size.width = lbl_w;
            text_rect.size.height = lbl_h;
            egui_canvas_draw_text_in_rect(text_font, val_buf, &text_rect, EGUI_ALIGN_CENTER, local->text_color, EGUI_ALPHA_100);
        }
    }
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_circular_progress_bar_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_circular_progress_bar_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_circular_progress_bar_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_circular_progress_bar_t);
    // call super init.
    egui_view_init(self);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_circular_progress_bar_t);

    // init local data.
    local->on_progress_changed = NULL;
    local->process = 0;
    local->stroke_width = EGUI_THEME_TRACK_THICKNESS;
    local->start_angle = -90;
    local->bk_color = EGUI_THEME_TRACK_BG;
    local->progress_color = EGUI_THEME_PRIMARY;
    local->text_color = EGUI_THEME_TEXT_PRIMARY;
    local->font = NULL; // NULL = auto-select based on inner_r

    egui_view_set_view_name(self, "egui_view_circular_progress_bar");
}

void egui_view_circular_progress_bar_apply_params(egui_view_t *self, const egui_view_circular_progress_bar_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_circular_progress_bar_t);

    self->region = params->region;

    local->process = params->process;

    egui_view_invalidate(self);
}

void egui_view_circular_progress_bar_init_with_params(egui_view_t *self, const egui_view_circular_progress_bar_params_t *params)
{
    egui_view_circular_progress_bar_init(self);
    egui_view_circular_progress_bar_apply_params(self, params);
}
