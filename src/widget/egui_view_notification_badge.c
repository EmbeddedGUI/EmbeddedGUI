#include <assert.h>
#include <string.h>

#include "egui_view_notification_badge.h"
#include "utils/egui_sprintf.h"
#include "font/egui_font.h"
#include "font/egui_font_std.h"
#include "resource/egui_resource.h"

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
#include "core/egui_canvas_gradient.h"
#endif

void egui_view_notification_badge_set_count(egui_view_t *self, uint16_t count)
{
    EGUI_LOCAL_INIT(egui_view_notification_badge_t);
    if (count != local->count)
    {
        local->count = count;
        egui_view_invalidate(self);
    }
}

uint16_t egui_view_notification_badge_get_count(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_notification_badge_t);
    return local->count;
}

void egui_view_notification_badge_set_max_display(egui_view_t *self, uint8_t max)
{
    EGUI_LOCAL_INIT(egui_view_notification_badge_t);
    if (max < 1)
    {
        max = 1;
    }
    if (max != local->max_display)
    {
        local->max_display = max;
        egui_view_invalidate(self);
    }
}

void egui_view_notification_badge_set_badge_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_notification_badge_t);
    local->badge_color = color;
    egui_view_invalidate(self);
}

void egui_view_notification_badge_set_text_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_notification_badge_t);
    local->text_color = color;
    egui_view_invalidate(self);
}

void egui_view_notification_badge_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_notification_badge_t);
    local->font = font;
    egui_view_invalidate(self);
}

void egui_view_notification_badge_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_notification_badge_t);

    if (local->count == 0)
    {
        return;
    }

    egui_region_t region;
    egui_view_get_work_region(self, &region);

    // Format text
    if (local->count <= local->max_display)
    {
        egui_sprintf_int(local->text_buffer, sizeof(local->text_buffer), local->count);
    }
    else
    {
        int pos = 0;
        pos += egui_sprintf_int(&local->text_buffer[pos], (int)sizeof(local->text_buffer) - pos, local->max_display);
        pos += egui_sprintf_char(&local->text_buffer[pos], (int)sizeof(local->text_buffer) - pos, '+');
    }

    // Get text dimensions
    egui_dim_t text_w = 0;
    egui_dim_t text_h = 0;
    if (local->font && local->font->api)
    {
        local->font->api->get_str_size(local->font, local->text_buffer, 0, 0, &text_w, &text_h);
    }

    egui_dim_t w = region.size.width;
    egui_dim_t h = region.size.height;
    egui_dim_t center_x = region.location.x + w / 2;
    egui_dim_t center_y = region.location.y + h / 2;

    // Draw background: circle for small numbers, rounded rect for wider text
    egui_dim_t radius = EGUI_MIN(w, h) / 2;
    if (text_w + 4 <= h)
    {
        // Circle background
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
        {
            egui_color_t badge_light = egui_rgb_mix(local->badge_color, EGUI_COLOR_WHITE, 80);
            egui_gradient_stop_t badge_stops[2] = {
                    {.position = 0, .color = badge_light},
                    {.position = 255, .color = local->badge_color},
            };
            egui_gradient_t badge_grad = {
                    .type = EGUI_GRADIENT_TYPE_RADIAL,
                    .stop_count = 2,
                    .alpha = EGUI_ALPHA_100,
                    .stops = badge_stops,
                    .center_x = 0,
                    .center_y = 0,
                    .radius = radius - 1,
            };
            egui_canvas_draw_circle_fill_gradient(center_x, center_y, radius - 1, &badge_grad);
        }
#else
        egui_canvas_draw_circle_fill(center_x, center_y, radius - 1, local->badge_color, EGUI_ALPHA_100);
#endif
    }
    else
    {
        // Rounded rectangle background
        egui_dim_t r = h / 2;
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
        {
            egui_color_t badge_light = egui_rgb_mix(local->badge_color, EGUI_COLOR_WHITE, 80);
            egui_gradient_stop_t badge_stops[2] = {
                    {.position = 0, .color = badge_light},
                    {.position = 255, .color = local->badge_color},
            };
            egui_gradient_t badge_grad = {
                    .type = EGUI_GRADIENT_TYPE_LINEAR_VERTICAL,
                    .stop_count = 2,
                    .alpha = EGUI_ALPHA_100,
                    .stops = badge_stops,
            };
            egui_canvas_draw_round_rectangle_fill_gradient(region.location.x, region.location.y, w, h, r, &badge_grad);
        }
#else
        egui_canvas_draw_round_rectangle_fill(region.location.x, region.location.y, w, h, r, local->badge_color, EGUI_ALPHA_100);
#endif
    }

    // Draw centered text
    egui_canvas_draw_text_in_rect(local->font, local->text_buffer, &region, EGUI_ALIGN_CENTER, local->text_color, EGUI_ALPHA_100);
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_notification_badge_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_notification_badge_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_notification_badge_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_notification_badge_t);
    // call super init.
    egui_view_init(self);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_notification_badge_t);

    // init local data.
    local->count = 0;
    local->max_display = 99;
    local->badge_color = EGUI_THEME_DANGER;
    local->text_color = EGUI_THEME_TEXT;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    memset(local->text_buffer, 0, sizeof(local->text_buffer));

    egui_view_set_view_name(self, "egui_view_notification_badge");
}

void egui_view_notification_badge_apply_params(egui_view_t *self, const egui_view_notification_badge_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_notification_badge_t);

    self->region = params->region;

    local->count = params->count;

    egui_view_invalidate(self);
}

void egui_view_notification_badge_init_with_params(egui_view_t *self, const egui_view_notification_badge_params_t *params)
{
    egui_view_notification_badge_init(self);
    egui_view_notification_badge_apply_params(self, params);
}
