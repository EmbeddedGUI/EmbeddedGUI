#include "egui_view_notification_badge.h"
#include "core/egui_api.h"
#include "egui_view_icon_font.h"
#include "egui_view_notification_badge_internal.h"
#include "resource/egui_resource.h"

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
#include "core/egui_canvas_gradient.h"
#endif

static char *egui_view_notification_badge_append_uint(char *cursor, char *end, uint16_t value)
{
    char tmp[5];
    uint8_t len = 0;

    if (cursor == NULL || end == NULL || cursor >= end)
    {
        return cursor;
    }

    do
    {
        tmp[len++] = (char)('0' + (value % 10));
        value /= 10;
    } while (value > 0 && len < sizeof(tmp));

    while (len > 0 && cursor < end)
    {
        *cursor++ = tmp[--len];
    }

    *cursor = '\0';
    return cursor;
}

static const egui_font_t *egui_view_notification_badge_get_default_font(void)
{
    return (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
}

uint8_t egui_view_notification_badge_get_text_region(const egui_region_t *region, egui_region_t *text_region)
{
    if (region == NULL || text_region == NULL || region->size.width <= 2 || region->size.height <= 2)
    {
        return 0;
    }

    *text_region = *region;
    text_region->location.x++;
    text_region->location.y++;
    text_region->size.width -= 2;
    text_region->size.height -= 2;
    return (uint8_t)(!egui_region_is_empty(text_region));
}

void egui_view_notification_badge_format_count_text(egui_view_notification_badge_t *local)
{
    char *cursor = local->text_buffer;
    char *end = local->text_buffer + sizeof(local->text_buffer) - 1;

    if (local->count <= local->max_display)
    {
        egui_view_notification_badge_append_uint(cursor, end, local->count);
        return;
    }

    cursor = egui_view_notification_badge_append_uint(cursor, end, local->max_display);
    if (cursor < end)
    {
        *cursor++ = '+';
        *cursor = '\0';
    }
}

void egui_view_notification_badge_draw_background(const egui_region_t *region, egui_color_t badge_color, uint8_t use_circle)
{
    egui_dim_t w = region->size.width;
    egui_dim_t h = region->size.height;
    egui_dim_t center_x = region->location.x + w / 2;
    egui_dim_t center_y = region->location.y + h / 2;
    egui_dim_t radius = EGUI_MIN(w, h) / 2;

    if (use_circle)
    {
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
        {
            egui_color_t badge_light = egui_rgb_mix(badge_color, EGUI_COLOR_WHITE, 80);
            egui_gradient_stop_t badge_stops[2] = {
                    {.position = 0, .color = badge_light},
                    {.position = 255, .color = badge_color},
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
        egui_canvas_draw_circle_fill_basic(center_x, center_y, radius - 1, badge_color, EGUI_ALPHA_100);
#endif
        return;
    }

    {
        egui_dim_t round_radius = h / 2;
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
        {
            egui_color_t badge_light = egui_rgb_mix(badge_color, EGUI_COLOR_WHITE, 80);
            egui_gradient_stop_t badge_stops[2] = {
                    {.position = 0, .color = badge_light},
                    {.position = 255, .color = badge_color},
            };
            egui_gradient_t badge_grad = {
                    .type = EGUI_GRADIENT_TYPE_LINEAR_VERTICAL,
                    .stop_count = 2,
                    .alpha = EGUI_ALPHA_100,
                    .stops = badge_stops,
            };
            egui_canvas_draw_round_rectangle_fill_gradient(region->location.x, region->location.y, w, h, round_radius, &badge_grad);
        }
#else
        egui_canvas_draw_round_rectangle_fill(region->location.x, region->location.y, w, h, round_radius, badge_color, EGUI_ALPHA_100);
#endif
    }
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
    if (local->font == font)
    {
        return;
    }

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
    egui_region_t region;

    egui_view_get_work_region(self, &region);

    if (local->content_style == EGUI_VIEW_NOTIFICATION_BADGE_CONTENT_STYLE_ICON)
    {
        const egui_font_t *icon_font;

        if (!EGUI_VIEW_ICON_TEXT_VALID(local->icon))
        {
            return;
        }

        icon_font = EGUI_VIEW_ICON_FONT_RESOLVE(local->icon_font, EGUI_MIN(region.size.width, region.size.height), 18, 22);
        if (icon_font == NULL)
        {
            return;
        }

        egui_view_notification_badge_draw_background(&region, local->badge_color, 1);
        egui_canvas_draw_text_in_rect(icon_font, local->icon, &region, EGUI_ALIGN_CENTER, local->text_color, EGUI_ALPHA_100);
        return;
    }

    {
        const egui_font_t *font = local->font ? local->font : egui_view_notification_badge_get_default_font();
        egui_region_t text_region;
        egui_dim_t text_width = 0;
        egui_dim_t text_height = 0;
        uint8_t use_circle;

        if (local->count == 0 || font == NULL)
        {
            return;
        }

        egui_view_notification_badge_format_count_text(local);
        if (!egui_view_notification_badge_get_text_region(&region, &text_region))
        {
            return;
        }

        if (font->api != NULL && font->api->get_str_size != NULL)
        {
            font->api->get_str_size(font, local->text_buffer, 0, 0, &text_width, &text_height);
        }
        EGUI_UNUSED(text_height);

        use_circle = (uint8_t)(text_width + 4 <= region.size.height);
        egui_view_notification_badge_draw_background(&region, local->badge_color, use_circle);
        egui_canvas_draw_text_in_rect(font, local->text_buffer, &text_region, EGUI_ALIGN_CENTER, local->text_color, EGUI_ALPHA_100);
    }
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
    local->font = NULL;
    local->content_style = EGUI_VIEW_NOTIFICATION_BADGE_CONTENT_STYLE_COUNT;
    local->icon = EGUI_ICON_MS_NOTIFICATIONS;
    local->icon_font = NULL;
    egui_api_memset(local->text_buffer, 0, sizeof(local->text_buffer));

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
