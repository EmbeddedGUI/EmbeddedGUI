#include <stdio.h>
#include <assert.h>

#include "egui_view_scale.h"
#include "font/egui_font_std.h"

static void egui_view_scale_int_to_str(int16_t value, char *buf, int buf_size)
{
    int pos = 0;
    int16_t v = value;
    char tmp[8];
    int tmp_len = 0;

    if (buf_size < 2)
    {
        if (buf_size > 0)
        {
            buf[0] = '\0';
        }
        return;
    }

    if (v < 0)
    {
        buf[pos++] = '-';
        v = -v;
    }

    if (v == 0)
    {
        tmp[tmp_len++] = '0';
    }
    else
    {
        while (v > 0 && tmp_len < (int)sizeof(tmp))
        {
            tmp[tmp_len++] = '0' + (v % 10);
            v /= 10;
        }
    }

    for (int i = tmp_len - 1; i >= 0 && pos < buf_size - 1; i--)
    {
        buf[pos++] = tmp[i];
    }
    buf[pos] = '\0';
}

void egui_view_scale_set_range(egui_view_t *self, int16_t min, int16_t max)
{
    EGUI_LOCAL_INIT(egui_view_scale_t);
    if (local->range_min != min || local->range_max != max)
    {
        local->range_min = min;
        local->range_max = max;
        egui_view_invalidate(self);
    }
}

void egui_view_scale_set_value(egui_view_t *self, int16_t value)
{
    EGUI_LOCAL_INIT(egui_view_scale_t);
    if (value < local->range_min)
    {
        value = local->range_min;
    }
    if (value > local->range_max)
    {
        value = local->range_max;
    }
    if (local->value != value)
    {
        local->value = value;
        egui_view_invalidate(self);
    }
}

void egui_view_scale_set_ticks(egui_view_t *self, uint8_t major_count, uint8_t minor_count)
{
    EGUI_LOCAL_INIT(egui_view_scale_t);
    if (local->major_tick_count != major_count || local->minor_tick_count != minor_count)
    {
        local->major_tick_count = major_count;
        local->minor_tick_count = minor_count;
        egui_view_invalidate(self);
    }
}

void egui_view_scale_set_orientation(egui_view_t *self, uint8_t is_horizontal)
{
    EGUI_LOCAL_INIT(egui_view_scale_t);
    if (local->is_horizontal != is_horizontal)
    {
        local->is_horizontal = is_horizontal;
        egui_view_invalidate(self);
    }
}

void egui_view_scale_set_tick_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_scale_t);
    if (local->tick_color.full != color.full)
    {
        local->tick_color = color;
        egui_view_invalidate(self);
    }
}

void egui_view_scale_set_label_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_scale_t);
    if (local->label_color.full != color.full)
    {
        local->label_color = color;
        egui_view_invalidate(self);
    }
}

void egui_view_scale_set_indicator_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_scale_t);
    if (local->indicator_color.full != color.full)
    {
        local->indicator_color = color;
        egui_view_invalidate(self);
    }
}

void egui_view_scale_show_labels(egui_view_t *self, uint8_t show)
{
    EGUI_LOCAL_INIT(egui_view_scale_t);
    if (local->show_labels != show)
    {
        local->show_labels = show;
        egui_view_invalidate(self);
    }
}

void egui_view_scale_show_indicator(egui_view_t *self, uint8_t show)
{
    EGUI_LOCAL_INIT(egui_view_scale_t);
    if (local->show_indicator != show)
    {
        local->show_indicator = show;
        egui_view_invalidate(self);
    }
}

void egui_view_scale_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_scale_t);
    if (local->font != font)
    {
        local->font = font;
        egui_view_invalidate(self);
    }
}

void egui_view_scale_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_scale_t);

    egui_region_t region;
    egui_view_get_work_region(self, &region);

    egui_dim_t w = region.size.width;
    egui_dim_t h = region.size.height;

    if (local->major_tick_count < 2)
    {
        return;
    }

    int16_t range = local->range_max - local->range_min;
    if (range <= 0)
    {
        return;
    }

    uint8_t major_count = local->major_tick_count;
    uint8_t minor_count = local->minor_tick_count;

    const egui_font_t *font = local->font;
    egui_dim_t font_h = 0;
    if (font != NULL && local->show_labels)
    {
        font_h = EGUI_FONT_STD_GET_FONT_HEIGHT(font);
    }

    if (local->is_horizontal)
    {
        // Horizontal scale: ticks drawn from bottom edge upward
        egui_dim_t tick_area_w = w;
        egui_dim_t base_y = region.location.y + h - 1;
        if (font != NULL && local->show_labels && font_h > 0)
        {
            base_y = region.location.y + h - font_h - 2;
        }

        for (uint8_t i = 0; i < major_count; i++)
        {
            // Major tick position
            egui_dim_t px = region.location.x + (egui_dim_t)((int32_t)tick_area_w * i / (major_count - 1));
            egui_canvas_draw_line(px, base_y, px, base_y - local->major_tick_len + 1, 1, local->tick_color, EGUI_ALPHA_100);

            // Label below tick
            if (font != NULL && local->show_labels && font_h > 0)
            {
                int16_t label_val = local->range_min + (int16_t)((int32_t)range * i / (major_count - 1));
                char buf[8];
                egui_view_scale_int_to_str(label_val, buf, sizeof(buf));
                EGUI_REGION_DEFINE(label_rect, px - 15, base_y + 2, 30, font_h);
                egui_canvas_draw_text_in_rect(font, buf, &label_rect, EGUI_ALIGN_HCENTER | EGUI_ALIGN_TOP, local->label_color, EGUI_ALPHA_100);
            }

            // Minor ticks between this major and next
            if (i < major_count - 1 && minor_count > 0)
            {
                egui_dim_t next_px = region.location.x + (egui_dim_t)((int32_t)tick_area_w * (i + 1) / (major_count - 1));
                for (uint8_t j = 1; j <= minor_count; j++)
                {
                    egui_dim_t mx = px + (egui_dim_t)((int32_t)(next_px - px) * j / (minor_count + 1));
                    egui_canvas_draw_line(mx, base_y, mx, base_y - local->minor_tick_len + 1, 1, local->tick_color, EGUI_ALPHA_100);
                }
            }
        }

        // Indicator line
        if (local->show_indicator)
        {
            int32_t val_offset = (int32_t)(local->value - local->range_min) * tick_area_w / range;
            egui_dim_t ind_x = region.location.x + (egui_dim_t)val_offset;
            egui_canvas_draw_line(ind_x, region.location.y, ind_x, base_y, 2, local->indicator_color, EGUI_ALPHA_100);
        }
    }
    else
    {
        // Vertical scale: ticks drawn from left edge rightward
        egui_dim_t tick_area_h = h;
        egui_dim_t base_x = region.location.x;

        for (uint8_t i = 0; i < major_count; i++)
        {
            // Major tick position (bottom = range_min, top = range_max)
            egui_dim_t py = region.location.y + h - 1 - (egui_dim_t)((int32_t)tick_area_h * i / (major_count - 1));
            egui_canvas_draw_line(base_x, py, base_x + local->major_tick_len - 1, py, 1, local->tick_color, EGUI_ALPHA_100);

            // Label to the right of tick
            if (font != NULL && local->show_labels && font_h > 0)
            {
                int16_t label_val = local->range_min + (int16_t)((int32_t)range * i / (major_count - 1));
                char buf[8];
                egui_view_scale_int_to_str(label_val, buf, sizeof(buf));
                EGUI_REGION_DEFINE(label_rect, base_x + local->major_tick_len + 2, py - font_h / 2, 30, font_h);
                egui_canvas_draw_text_in_rect(font, buf, &label_rect, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, local->label_color, EGUI_ALPHA_100);
            }

            // Minor ticks
            if (i < major_count - 1 && minor_count > 0)
            {
                egui_dim_t next_py = region.location.y + h - 1 - (egui_dim_t)((int32_t)tick_area_h * (i + 1) / (major_count - 1));
                for (uint8_t j = 1; j <= minor_count; j++)
                {
                    egui_dim_t my = py - (egui_dim_t)((int32_t)(py - next_py) * j / (minor_count + 1));
                    egui_canvas_draw_line(base_x, my, base_x + local->minor_tick_len - 1, my, 1, local->tick_color, EGUI_ALPHA_100);
                }
            }
        }

        // Indicator line
        if (local->show_indicator)
        {
            int32_t val_offset = (int32_t)(local->value - local->range_min) * tick_area_h / range;
            egui_dim_t ind_y = region.location.y + h - 1 - (egui_dim_t)val_offset;
            egui_canvas_draw_line(base_x, ind_y, region.location.x + w - 1, ind_y, 2, local->indicator_color, EGUI_ALPHA_100);
        }
    }
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_scale_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_scale_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_scale_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_scale_t);
    // call super init.
    egui_view_init(self);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_scale_t);

    // init local data.
    local->range_min = 0;
    local->range_max = 100;
    local->value = 0;
    local->major_tick_count = 6;
    local->minor_tick_count = 4;
    local->major_tick_len = 10;
    local->minor_tick_len = 5;
    local->is_horizontal = 1;
    local->show_labels = 1;
    local->show_indicator = 0;
    local->tick_color = EGUI_THEME_TRACK_BG;
    local->label_color = EGUI_THEME_TEXT;
    local->indicator_color = EGUI_THEME_PRIMARY;
    local->font = NULL;

    egui_view_set_view_name(self, "egui_view_scale");
}

void egui_view_scale_apply_params(egui_view_t *self, const egui_view_scale_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_scale_t);

    self->region = params->region;

    local->range_min = params->range_min;
    local->range_max = params->range_max;
    local->major_tick_count = params->major_tick_count;

    egui_view_invalidate(self);
}

void egui_view_scale_init_with_params(egui_view_t *self, const egui_view_scale_params_t *params)
{
    egui_view_scale_init(self);
    egui_view_scale_apply_params(self, params);
}
