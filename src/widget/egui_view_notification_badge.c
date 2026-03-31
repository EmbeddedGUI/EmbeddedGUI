#include <assert.h>
#include <string.h>

#include "egui_view_notification_badge.h"
#include "egui_view_icon_font.h"
#include "utils/egui_sprintf.h"
#include "font/egui_font.h"
#include "font/egui_font_std.h"
#include "resource/egui_resource.h"

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
#include "core/egui_canvas_gradient.h"
#endif

static const egui_font_t *egui_view_notification_badge_get_icon_font(egui_view_notification_badge_t *local, egui_dim_t area_size)
{
    if (local->icon_font != NULL)
    {
        return local->icon_font;
    }

    return egui_view_icon_font_get_auto(area_size, 18, 22);
}

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

void egui_view_notification_badge_set_content_style(egui_view_t *self, egui_view_notification_badge_content_style_t style)
{
    EGUI_LOCAL_INIT(egui_view_notification_badge_t);
    if (local->content_style == (uint8_t)style)
    {
        return;
    }

    local->content_style = (uint8_t)style;
    egui_view_invalidate(self);
}

void egui_view_notification_badge_set_icon(egui_view_t *self, const char *icon)
{
    EGUI_LOCAL_INIT(egui_view_notification_badge_t);
    if (local->icon == icon)
    {
        return;
    }

    local->icon = icon;
    egui_view_invalidate(self);
}

void egui_view_notification_badge_set_icon_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_notification_badge_t);
    if (local->icon_font == font)
    {
        return;
    }

    local->icon_font = font;
    egui_view_invalidate(self);
}

void egui_view_notification_badge_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_notification_badge_t);
    const char *display_text = NULL;
    const egui_font_t *display_font = NULL;
    uint8_t use_circle = 0;

    if (local->content_style == EGUI_VIEW_NOTIFICATION_BADGE_CONTENT_STYLE_ICON)
    {
        if (local->icon == NULL)
        {
            return;
        }
        display_text = local->icon;
        use_circle = 1;
    }
    else
    {
        if (local->count == 0)
        {
            return;
        }

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
        display_text = local->text_buffer;
    }

    if (display_text == NULL)
    {
        return;
    }

    egui_region_t region;
    egui_view_get_work_region(self, &region);
    display_font = (local->content_style == EGUI_VIEW_NOTIFICATION_BADGE_CONTENT_STYLE_ICON)
                           ? egui_view_notification_badge_get_icon_font(local, EGUI_MIN(region.size.width, region.size.height))
                           : local->font;
    if (display_font == NULL)
    {
        return;
    }

    // Get text dimensions
    egui_dim_t text_w = 0;
    egui_dim_t text_h = 0;
    if (display_font->api != NULL && display_font->api->get_str_size != NULL)
    {
        display_font->api->get_str_size(display_font, display_text, 0, 0, &text_w, &text_h);
    }

    egui_dim_t w = region.size.width;
    egui_dim_t h = region.size.height;
    egui_dim_t center_x = region.location.x + w / 2;
    egui_dim_t center_y = region.location.y + h / 2;

    // Draw background: circle for small numbers, rounded rect for wider text
    egui_dim_t radius = EGUI_MIN(w, h) / 2;
    if (use_circle || text_w + 4 <= h)
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
    egui_canvas_draw_text_in_rect(display_font, display_text, &region, EGUI_ALIGN_CENTER, local->text_color, EGUI_ALPHA_100);
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
    local->content_style = EGUI_VIEW_NOTIFICATION_BADGE_CONTENT_STYLE_COUNT;
    local->icon = EGUI_ICON_MS_NOTIFICATIONS;
    local->icon_font = NULL;
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
