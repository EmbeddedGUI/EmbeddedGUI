#include <stdio.h>
#include <assert.h>

#include "egui_view_toggle_button.h"
#include "resource/egui_resource.h"

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
#include "core/egui_canvas_gradient.h"
#endif

static const egui_font_t *egui_view_toggle_button_get_icon_font(egui_view_toggle_button_t *local, egui_dim_t area_size)
{
    if (local->icon_font != NULL)
    {
        return local->icon_font;
    }

    if (area_size <= 20)
    {
        return EGUI_FONT_ICON_MS_16;
    }
    if (area_size <= 26)
    {
        return EGUI_FONT_ICON_MS_20;
    }
    return EGUI_FONT_ICON_MS_24;
}

static void egui_view_toggle_button_draw_content(egui_view_toggle_button_t *local, const egui_region_t *region, egui_color_t text_color)
{
    const char *icon = local->icon;
    const char *text = local->text;
    const egui_font_t *text_font = (local->font != NULL) ? local->font : (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    egui_region_t draw_region = *region;

    if ((icon == NULL || icon[0] == '\0') && (text == NULL || text[0] == '\0'))
    {
        return;
    }

    if (icon == NULL || icon[0] == '\0')
    {
        egui_canvas_draw_text_in_rect(text_font, text, &draw_region, EGUI_ALIGN_CENTER, text_color, EGUI_ALPHA_100);
        return;
    }

    if (text == NULL || text[0] == '\0')
    {
        egui_canvas_draw_text_in_rect(egui_view_toggle_button_get_icon_font(local, EGUI_MIN(region->size.width, region->size.height)), icon, &draw_region,
                                      EGUI_ALIGN_CENTER, text_color, EGUI_ALPHA_100);
        return;
    }

    {
        const egui_font_t *icon_font = egui_view_toggle_button_get_icon_font(local, region->size.height);
        egui_dim_t icon_width = 0;
        egui_dim_t icon_height = 0;
        egui_dim_t text_width = 0;
        egui_dim_t text_height = 0;
        egui_dim_t content_width;
        egui_dim_t start_x;
        egui_dim_t gap = local->icon_text_gap;
        egui_region_t icon_region;
        egui_region_t text_region;

        if (icon_font != NULL && icon_font->api != NULL && icon_font->api->get_str_size != NULL)
        {
            icon_font->api->get_str_size(icon_font, icon, 0, 0, &icon_width, &icon_height);
        }
        if (text_font != NULL && text_font->api != NULL && text_font->api->get_str_size != NULL)
        {
            text_font->api->get_str_size(text_font, text, 0, 0, &text_width, &text_height);
        }
        (void)icon_height;
        (void)text_height;

        if (icon_width <= 0)
        {
            icon_width = EGUI_MIN(region->size.height, 20);
        }
        if (text_width <= 0)
        {
            gap = 0;
        }
        if (gap < 0)
        {
            gap = 0;
        }

        content_width = icon_width + gap + text_width;
        if (content_width < 0)
        {
            content_width = 0;
        }

        start_x = region->location.x + (region->size.width - content_width) / 2;
        if (start_x < region->location.x)
        {
            start_x = region->location.x;
        }

        icon_region.location.x = start_x;
        icon_region.location.y = region->location.y;
        icon_region.size.width = EGUI_MIN(icon_width, region->size.width);
        icon_region.size.height = region->size.height;

        text_region.location.x = icon_region.location.x + icon_region.size.width + gap;
        text_region.location.y = region->location.y;
        text_region.size.width = region->location.x + region->size.width - text_region.location.x;
        text_region.size.height = region->size.height;

        egui_canvas_draw_text_in_rect(icon_font, icon, &icon_region, EGUI_ALIGN_CENTER, text_color, EGUI_ALPHA_100);
        if (text_region.size.width > 0)
        {
            egui_canvas_draw_text_in_rect(text_font, text, &text_region, EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER, text_color, EGUI_ALPHA_100);
        }
    }
}

void egui_view_toggle_button_set_on_toggled_listener(egui_view_t *self, egui_view_on_toggled_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_toggle_button_t);
    local->on_toggled = listener;
}

void egui_view_toggle_button_set_toggled(egui_view_t *self, uint8_t is_toggled)
{
    EGUI_LOCAL_INIT(egui_view_toggle_button_t);
    if (is_toggled != local->is_toggled)
    {
        local->is_toggled = is_toggled;
        if (local->on_toggled)
        {
            local->on_toggled(self, is_toggled);
        }
        egui_view_invalidate(self);
    }
}

uint8_t egui_view_toggle_button_is_toggled(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_toggle_button_t);
    return local->is_toggled;
}

void egui_view_toggle_button_set_icon(egui_view_t *self, const char *icon)
{
    EGUI_LOCAL_INIT(egui_view_toggle_button_t);

    if (local->icon == icon)
    {
        return;
    }

    local->icon = icon;
    egui_view_invalidate(self);
}

void egui_view_toggle_button_set_text(egui_view_t *self, const char *text)
{
    EGUI_LOCAL_INIT(egui_view_toggle_button_t);
    local->text = text;
    egui_view_invalidate(self);
}

void egui_view_toggle_button_set_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_toggle_button_t);
    local->font = font;
    egui_view_invalidate(self);
}

void egui_view_toggle_button_set_icon_font(egui_view_t *self, const egui_font_t *font)
{
    EGUI_LOCAL_INIT(egui_view_toggle_button_t);

    if (local->icon_font == font)
    {
        return;
    }

    local->icon_font = font;
    egui_view_invalidate(self);
}

void egui_view_toggle_button_set_icon_text_gap(egui_view_t *self, egui_dim_t gap)
{
    EGUI_LOCAL_INIT(egui_view_toggle_button_t);

    if (local->icon_text_gap == gap)
    {
        return;
    }

    local->icon_text_gap = gap;
    egui_view_invalidate(self);
}

void egui_view_toggle_button_set_text_color(egui_view_t *self, egui_color_t color)
{
    EGUI_LOCAL_INIT(egui_view_toggle_button_t);
    local->text_color = color;
    egui_view_invalidate(self);
}

void egui_view_toggle_button_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_toggle_button_t);

    egui_region_t region;
    egui_view_get_work_region(self, &region);

    egui_color_t bg_color = local->is_toggled ? local->on_color : local->off_color;
    if (!egui_view_get_enable(self))
    {
        bg_color = EGUI_THEME_DISABLED;
    }

    // Draw background rounded rectangle
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
    {
        egui_color_t color_light = egui_rgb_mix(bg_color, EGUI_COLOR_WHITE, 80);
        egui_gradient_stop_t tb_stops[2] = {
                {.position = 0, .color = color_light},
                {.position = 255, .color = bg_color},
        };
        egui_gradient_t tb_grad = {
                .type = EGUI_GRADIENT_TYPE_LINEAR_VERTICAL,
                .stop_count = 2,
                .alpha = EGUI_ALPHA_100,
                .stops = tb_stops,
        };
        egui_canvas_draw_round_rectangle_fill_gradient(region.location.x, region.location.y, region.size.width, region.size.height, local->corner_radius,
                                                       &tb_grad);
    }
#else
    egui_canvas_draw_round_rectangle_fill(region.location.x, region.location.y, region.size.width, region.size.height, local->corner_radius, bg_color,
                                          EGUI_ALPHA_100);
#endif

    // Draw pressed overlay
    if (self->is_pressed)
    {
        egui_canvas_draw_round_rectangle_fill(region.location.x, region.location.y, region.size.width, region.size.height, local->corner_radius,
                                              EGUI_THEME_PRESS_OVERLAY, EGUI_THEME_PRESS_OVERLAY_ALPHA);
    }

    if ((local->icon != NULL && local->icon[0] != '\0') || (local->text != NULL && local->text[0] != '\0'))
    {
        egui_color_t text_color;
        if (!egui_view_get_enable(self))
        {
            text_color = EGUI_THEME_TEXT_SECONDARY;
        }
        else if (local->is_toggled)
        {
            text_color = local->text_color;
        }
        else
        {
            text_color = EGUI_THEME_TEXT_PRIMARY;
        }

        egui_view_toggle_button_draw_content(local, &region, text_color);
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
int egui_view_toggle_button_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_toggle_button_t);
    int is_inside = egui_region_pt_in_rect(&self->region_screen, event->location.x, event->location.y);

    if (self->is_enable == false)
    {
        return 0;
    }

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
    {
        egui_view_set_pressed(self, is_inside);
        break;
    }
    case EGUI_MOTION_EVENT_ACTION_MOVE:
    {
        if (self->is_pressed != is_inside)
        {
            egui_view_set_pressed(self, is_inside);
        }
        break;
    }
    case EGUI_MOTION_EVENT_ACTION_UP:
    {
        int was_pressed = self->is_pressed;
        egui_view_set_pressed(self, false);
        if (was_pressed && is_inside)
        {
            egui_view_toggle_button_set_toggled(self, !local->is_toggled);
        }
        break;
    }
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
    {
        egui_view_set_pressed(self, false);
        break;
    }
    default:
        break;
    }

    return 1;
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_toggle_button_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_toggle_button_on_touch_event,
#else
        .on_touch_event = egui_view_on_touch_event,
#endif
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_toggle_button_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_toggle_button_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_toggle_button_t);
    // call super init.
    egui_view_init(self);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_toggle_button_t);

    // init local data.
    local->on_toggled = NULL;
    local->is_toggled = 0;
    local->icon = NULL;
    local->text = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->icon_font = NULL;
    local->text_color = EGUI_THEME_TEXT;
    local->on_color = EGUI_THEME_PRIMARY;
    local->off_color = EGUI_THEME_TRACK_OFF;
    local->corner_radius = EGUI_THEME_RADIUS_MD;
    local->icon_text_gap = 6;

    egui_view_set_view_name(self, "egui_view_toggle_button");
}

void egui_view_toggle_button_apply_params(egui_view_t *self, const egui_view_toggle_button_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_toggle_button_t);

    self->region = params->region;

    local->text = params->text;
    local->is_toggled = params->is_toggled;

    egui_view_invalidate(self);
}

void egui_view_toggle_button_init_with_params(egui_view_t *self, const egui_view_toggle_button_params_t *params)
{
    egui_view_toggle_button_init(self);
    egui_view_toggle_button_apply_params(self, params);
}
