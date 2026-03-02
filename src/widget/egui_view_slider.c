#include <stdio.h>
#include <assert.h>

#include "egui_view_slider.h"
#include "egui_view_group.h"
#include "style/egui_theme.h"

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
#include "core/egui_canvas_gradient.h"
#endif

void egui_view_slider_set_on_value_changed_listener(egui_view_t *self, egui_view_on_value_changed_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_slider_t);
    local->on_value_changed = listener;
}

void egui_view_slider_set_value(egui_view_t *self, uint8_t value)
{
    EGUI_LOCAL_INIT(egui_view_slider_t);
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

uint8_t egui_view_slider_get_value(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_slider_t);
    return local->value;
}

static void egui_view_slider_update_value_from_touch(egui_view_t *self, egui_dim_t touch_x)
{
    EGUI_LOCAL_INIT(egui_view_slider_t);

    // Convert screen touch_x to local coordinates
    egui_dim_t local_x = touch_x - self->region_screen.location.x;

    // Use consistent max thumb size logic
    egui_dim_t thumb_radius = (self->region.size.height / 2) - 1;
    if (thumb_radius > EGUI_THEME_RADIUS_LG)
        thumb_radius = EGUI_THEME_RADIUS_LG;
    if (thumb_radius < EGUI_THEME_RADIUS_SM)
        thumb_radius = EGUI_THEME_RADIUS_SM;

    egui_dim_t thumb_margin = thumb_radius + 1;
    egui_dim_t usable_width = self->region.size.width - 2 * thumb_margin;

    if (usable_width <= 0)
    {
        return;
    }

    // Clamp to valid range
    egui_dim_t offset = local_x - thumb_margin;
    if (offset < 0)
    {
        offset = 0;
    }
    if (offset > usable_width)
    {
        offset = usable_width;
    }

    // Map to 0-100
    uint8_t new_value = (uint8_t)((uint32_t)offset * 100 / usable_width);
    egui_view_slider_set_value(self, new_value);
}

void egui_view_slider_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_slider_t);

    egui_region_t region;
    egui_view_get_work_region(self, &region);

    // Fixed thin track for modern look, centered vertically
    egui_dim_t track_height = EGUI_THEME_TRACK_THICKNESS;
    egui_dim_t track_y = region.location.y + (region.size.height - track_height) / 2;
    egui_dim_t track_radius = track_height / 2;

    // Thumb size based on height but limited to max radius
    egui_dim_t thumb_radius = (region.size.height / 2) - 1;
    if (thumb_radius > EGUI_THEME_RADIUS_LG)
        thumb_radius = EGUI_THEME_RADIUS_LG;
    if (thumb_radius < EGUI_THEME_RADIUS_SM)
        thumb_radius = EGUI_THEME_RADIUS_SM;

    // Add 1-pixel margin beyond thumb_radius to prevent circle edge clipping
    egui_dim_t thumb_margin = thumb_radius + 1;

    // Ensure we have enough width
    if (region.size.width <= 2 * thumb_margin)
    {
        return;
    }

    egui_dim_t usable_width = region.size.width - 2 * thumb_margin;
    egui_dim_t start_x = region.location.x + thumb_margin;
    egui_dim_t thumb_x = start_x + (egui_dim_t)((uint32_t)usable_width * local->value / 100);
    egui_dim_t thumb_y = region.location.y + region.size.height / 2;

    egui_dim_t track_draw_x = region.location.x + thumb_margin;
    egui_dim_t track_draw_width = usable_width;

    /* Query theme styles for 3 parts */
    const egui_widget_style_desc_t *desc = egui_current_theme ? egui_current_theme->slider : NULL;
    egui_state_t state = egui_style_get_view_state(self);

    const egui_style_t *track_style = desc ? egui_style_get(desc, EGUI_PART_MAIN, state) : NULL;
    const egui_style_t *fill_style = desc ? egui_style_get(desc, EGUI_PART_INDICATOR, state) : NULL;
    const egui_style_t *knob_style = desc ? egui_style_get(desc, EGUI_PART_KNOB, state) : NULL;

    egui_color_t track_color = track_style ? track_style->bg_color : local->track_color;
    egui_color_t fill_color = fill_style ? fill_style->bg_color : local->active_color;
    egui_color_t knob_color = knob_style ? knob_style->bg_color : local->thumb_color;

    // Background (Grey)
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
    {
        egui_canvas_draw_round_rectangle_fill(track_draw_x, track_y, track_draw_width, track_height, track_radius, track_color, EGUI_ALPHA_100);
    }
#else
    egui_canvas_draw_round_rectangle_fill(track_draw_x, track_y, track_draw_width, track_height, track_radius, track_color, EGUI_ALPHA_100);
#endif

    // Active Track (Blue)
    egui_dim_t active_width = thumb_x - track_draw_x;
    if (active_width > 0)
    {
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
        egui_canvas_draw_round_rectangle_fill(track_draw_x, track_y, active_width, track_height, track_radius, fill_color, EGUI_ALPHA_100);
#else
        egui_canvas_draw_round_rectangle_fill(track_draw_x, track_y, active_width, track_height, track_radius, fill_color, EGUI_ALPHA_100);
#endif
    }

    // Draw thumb circle
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
    {
        egui_canvas_draw_circle_fill_hq(thumb_x, thumb_y, thumb_radius, knob_color, EGUI_ALPHA_100);
        // Draw a subtle border around the knob for visibility on same-color backgrounds
        egui_canvas_draw_circle(thumb_x, thumb_y, thumb_radius, 1, EGUI_THEME_BORDER, 80);
    }
#else
    egui_canvas_draw_circle_fill(thumb_x, thumb_y, thumb_radius, knob_color, EGUI_ALPHA_100);
    egui_canvas_draw_circle(thumb_x, thumb_y, thumb_radius, 1, EGUI_THEME_BORDER, EGUI_ALPHA_100);
#endif
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
int egui_view_slider_on_touch_event(egui_view_t *self, egui_motion_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_slider_t);

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

        // Update value from touch position
        egui_view_slider_update_value_from_touch(self, event->location.x);
        break;
    }
    case EGUI_MOTION_EVENT_ACTION_MOVE:
    {
        if (local->is_dragging)
        {
            egui_view_slider_update_value_from_touch(self, event->location.x);
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

    return 1; // consume event
}
#endif // EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_slider_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
        .on_touch_event = egui_view_slider_on_touch_event,
#else
        .on_touch_event = egui_view_on_touch_event,
#endif
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_slider_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_slider_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_slider_t);
    // call super init.
    egui_view_init(self);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_slider_t);

    // init local data.
    local->on_value_changed = NULL;
    local->value = 0;
    local->is_dragging = 0;
    local->track_color = EGUI_THEME_TRACK_BG;
    local->active_color = EGUI_THEME_PRIMARY;
    local->thumb_color = EGUI_THEME_THUMB;

    egui_view_set_view_name(self, "egui_view_slider");
}

void egui_view_slider_apply_params(egui_view_t *self, const egui_view_slider_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_slider_t);

    self->region = params->region;

    local->value = params->value;

    egui_view_invalidate(self);
}

void egui_view_slider_init_with_params(egui_view_t *self, const egui_view_slider_params_t *params)
{
    egui_view_slider_init(self);
    egui_view_slider_apply_params(self, params);
}
