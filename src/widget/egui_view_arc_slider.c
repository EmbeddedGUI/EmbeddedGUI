#include <stdio.h>
#include <assert.h>

#include "egui_view_arc_slider.h"
#include "widget/egui_view_group.h"
#include "utils/egui_fixmath.h"

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
#include "core/egui_canvas_gradient.h"
#endif

// Integer atan2 approximation returning degrees 0-359
static int16_t integer_atan2_deg(int32_t dy, int32_t dx)
{
    if (dx == 0 && dy == 0)
    {
        return 0;
    }
    if (dx == 0)
    {
        return (dy > 0) ? 90 : 270;
    }
    if (dy == 0)
    {
        return (dx > 0) ? 0 : 180;
    }

    int32_t abs_y = (dy < 0) ? -dy : dy;
    int32_t abs_x = (dx < 0) ? -dx : dx;
    int16_t angle;

    if (abs_x >= abs_y)
    {
        angle = (int16_t)((int32_t)45 * abs_y / abs_x);
    }
    else
    {
        angle = 90 - (int16_t)((int32_t)45 * abs_x / abs_y);
    }

    if (dx < 0)
    {
        angle = 180 - angle;
    }
    if (dy < 0)
    {
        angle = 360 - angle;
    }

    return angle % 360;
}

void egui_view_arc_slider_set_on_value_changed_listener(egui_view_t *self, egui_view_on_arc_value_changed_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_arc_slider_t);
    local->on_value_changed = listener;
}

void egui_view_arc_slider_set_value(egui_view_t *self, uint8_t value)
{
    EGUI_LOCAL_INIT(egui_view_arc_slider_t);
    if (value > 100)
    {
        value = 100;
    }
    if (value != local->value)
    {
        local->value = value;
        if (local->on_value_changed)
        {
            local->on_value_changed(self, value);
        }
        egui_view_invalidate(self);
    }
}

uint8_t egui_view_arc_slider_get_value(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_arc_slider_t);
    return local->value;
}

void egui_view_arc_slider_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_arc_slider_t);

    egui_region_t region;
    egui_view_get_work_region(self, &region);

    egui_dim_t w = region.size.width;
    egui_dim_t h = region.size.height;
    egui_dim_t center_x = region.location.x + w / 2;
    egui_dim_t center_y = region.location.y + h / 2;
    egui_dim_t radius = EGUI_MIN(w, h) / 2 - local->thumb_radius - 1;

    if (radius <= 0)
    {
        return;
    }

    // Background track arc
    int16_t bg_start = local->start_angle;
    int16_t bg_end = local->start_angle + local->sweep_angle;
    egui_canvas_draw_arc(center_x, center_y, radius, bg_start, bg_end, local->stroke_width, local->track_color, EGUI_ALPHA_100);

    // Active progress arc
    int16_t progress_end = local->start_angle + (int16_t)((int32_t)local->sweep_angle * local->value / 100);
    if (local->value > 0)
    {
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
        {
            egui_color_t color_light = egui_rgb_mix(local->active_color, EGUI_COLOR_WHITE, 80);
            egui_gradient_stop_t stops[2] = {
                    {.position = 0, .color = color_light},
                    {.position = 255, .color = local->active_color},
            };
            egui_gradient_t grad = {
                    .type = EGUI_GRADIENT_TYPE_RADIAL,
                    .stop_count = 2,
                    .alpha = EGUI_ALPHA_100,
                    .stops = stops,
            };
            egui_canvas_draw_arc_ring_fill_gradient(center_x, center_y, radius, radius - local->stroke_width, bg_start, progress_end, &grad);
        }
#else
        egui_canvas_draw_arc(center_x, center_y, radius, bg_start, progress_end, local->stroke_width, local->active_color, EGUI_ALPHA_100);
#endif
    }

    // Thumb circle at current value position (centered on arc visual center)
    // Arc stroke extends inward from radius to radius-stroke_width, so center is at radius - stroke_width/2
    egui_dim_t thumb_track_radius = radius - local->stroke_width / 2;
    int16_t thumb_deg = progress_end;
    egui_float_t angle_rad = EGUI_FLOAT_DIV(EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(thumb_deg), EGUI_FLOAT_PI), EGUI_FLOAT_VALUE_INT(180));
    egui_float_t cos_val = EGUI_FLOAT_COS(angle_rad);
    egui_float_t sin_val = EGUI_FLOAT_SIN(angle_rad);

    egui_dim_t thumb_x = center_x + (egui_dim_t)EGUI_FLOAT_INT_PART(EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(thumb_track_radius), cos_val));
    egui_dim_t thumb_y = center_y + (egui_dim_t)EGUI_FLOAT_INT_PART(EGUI_FLOAT_MULT(EGUI_FLOAT_VALUE_INT(thumb_track_radius), sin_val));

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
    {
        egui_color_t color_light = egui_rgb_mix(local->thumb_color, EGUI_COLOR_WHITE, 80);
        egui_gradient_stop_t stops[2] = {
                {.position = 0, .color = color_light},
                {.position = 255, .color = local->thumb_color},
        };
        egui_gradient_t grad = {
                .type = EGUI_GRADIENT_TYPE_RADIAL,
                .stop_count = 2,
                .alpha = EGUI_ALPHA_100,
                .stops = stops,
        };
        egui_canvas_draw_circle_fill_gradient(thumb_x, thumb_y, local->thumb_radius, &grad);
    }
#else
    egui_canvas_draw_circle_fill(thumb_x, thumb_y, local->thumb_radius, local->thumb_color, EGUI_ALPHA_100);
#endif
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static void egui_view_arc_slider_update_value_from_touch(egui_view_t *self, egui_dim_t touch_x, egui_dim_t touch_y)
{
    EGUI_LOCAL_INIT(egui_view_arc_slider_t);

    egui_dim_t w = self->region.size.width;
    egui_dim_t h = self->region.size.height;
    egui_dim_t center_x = self->region_screen.location.x + w / 2;
    egui_dim_t center_y = self->region_screen.location.y + h / 2;

    int32_t dx = touch_x - center_x;
    int32_t dy = touch_y - center_y;

    // Get touch angle in degrees (0-359, 0=right, clockwise)
    int16_t touch_angle = integer_atan2_deg(dy, dx);

    // Normalize touch_angle relative to start_angle
    int16_t start = local->start_angle % 360;
    int16_t relative_angle = touch_angle - start;

    // Handle wrap-around
    if (relative_angle < 0)
    {
        relative_angle += 360;
    }

    // Check if the angle is within the sweep range
    if (relative_angle <= local->sweep_angle)
    {
        uint8_t new_value = (uint8_t)((int32_t)relative_angle * 100 / local->sweep_angle);
        egui_view_arc_slider_set_value(self, new_value);
    }
    else
    {
        // Outside the arc range - snap to closest end
        if (360 - relative_angle < relative_angle - local->sweep_angle)
        {
            egui_view_arc_slider_set_value(self, 0);
        }
        else
        {
            egui_view_arc_slider_set_value(self, 100);
        }
    }
}

int egui_view_arc_slider_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_arc_slider_t);

    if (self->is_enable == false)
    {
        return 0;
    }

    switch (event->type)
    {
    case EGUI_MOTION_EVENT_ACTION_DOWN:
    {
        local->is_dragging = 1;
        egui_view_set_pressed(self, true);

        // Request parent to not intercept touch events
        if (self->parent != NULL)
        {
            egui_view_group_request_disallow_intercept_touch_event((egui_view_t *)self->parent, 1);
        }

        egui_view_arc_slider_update_value_from_touch(self, event->location.x, event->location.y);
        break;
    }
    case EGUI_MOTION_EVENT_ACTION_MOVE:
    {
        if (local->is_dragging)
        {
            egui_view_arc_slider_update_value_from_touch(self, event->location.x, event->location.y);
        }
        break;
    }
    case EGUI_MOTION_EVENT_ACTION_UP:
    case EGUI_MOTION_EVENT_ACTION_CANCEL:
    {
        local->is_dragging = 0;
        egui_view_set_pressed(self, false);
        break;
    }
    default:
        break;
    }

    return 1;
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_arc_slider_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_arc_slider_on_touch_event,
#else
        .on_touch_event = egui_view_on_touch_event,
#endif
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_arc_slider_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_arc_slider_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_arc_slider_t);
    // call super init.
    egui_view_init(self);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_arc_slider_t);

    // init local data.
    local->on_value_changed = NULL;
    local->value = 0;
    local->start_angle = 150;
    local->sweep_angle = 240;
    local->stroke_width = 8;
    local->track_color = EGUI_COLOR_DARK_GREY;
    local->active_color = EGUI_COLOR_BLUE;
    local->thumb_color = EGUI_COLOR_WHITE;
    local->thumb_radius = 6;
    local->is_dragging = 0;

    egui_view_set_view_name(self, "egui_view_arc_slider");
}

void egui_view_arc_slider_apply_params(egui_view_t *self, const egui_view_arc_slider_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_arc_slider_t);

    self->region = params->region;

    local->value = params->value;

    egui_view_invalidate(self);
}

void egui_view_arc_slider_init_with_params(egui_view_t *self, const egui_view_arc_slider_params_t *params)
{
    egui_view_arc_slider_init(self);
    egui_view_arc_slider_apply_params(self, params);
}
