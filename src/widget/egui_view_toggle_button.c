#include <stdio.h>
#include <assert.h>

#include "egui_view_toggle_button.h"
#include "resource/egui_resource.h"

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
#include "core/egui_canvas_gradient.h"
#endif

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

void egui_view_toggle_button_set_text(egui_view_t *self, const char *text)
{
    EGUI_LOCAL_INIT(egui_view_toggle_button_t);
    local->text = text;
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

    // Draw centered text
    if (local->text && local->font)
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
        egui_canvas_draw_text_in_rect(local->font, local->text, &region, EGUI_ALIGN_CENTER, text_color, EGUI_ALPHA_100);
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
int egui_view_toggle_button_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_toggle_button_t);

    if (self->is_enable == false)
    {
        return 0;
    }

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
    {
        egui_view_set_pressed(self, true);
        break;
    }
    case EGUI_MOTION_EVENT_ACTION_UP:
    {
        egui_view_set_pressed(self, false);
        egui_view_toggle_button_set_toggled(self, !local->is_toggled);
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
    local->text = NULL;
    local->font = (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT;
    local->text_color = EGUI_THEME_TEXT;
    local->on_color = EGUI_THEME_PRIMARY;
    local->off_color = EGUI_THEME_TRACK_OFF;
    local->corner_radius = EGUI_THEME_RADIUS_MD;

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
