#include <stdio.h>
#include <assert.h>

#include "egui_view_slider.h"
#include "egui_view_group.h"
#include "egui_view_circle_dirty.h"
#include "egui_view_linear_value_helper.h"
#include "style/egui_theme.h"

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
#include "core/egui_canvas_gradient.h"
#endif

static uint8_t egui_view_slider_get_max_value(const egui_view_slider_t *local)
{
    if (local == NULL || local->max_value == 0u)
    {
        return 100u;
    }

    return local->max_value;
}

static uint8_t egui_view_slider_clamp_value(const egui_view_slider_t *local, uint8_t value)
{
    uint8_t max_value = egui_view_slider_get_max_value(local);

    return value > max_value ? max_value : value;
}

static uint8_t egui_view_slider_value_to_percent(const egui_view_slider_t *local, uint8_t value)
{
    uint8_t max_value = egui_view_slider_get_max_value(local);

    value = egui_view_slider_clamp_value(local, value);
    if (max_value >= 100u)
    {
        return value;
    }

    return (uint8_t)(((uint32_t)value * 100u + (uint32_t)max_value / 2u) / (uint32_t)max_value);
}

static uint8_t egui_view_slider_percent_to_value(const egui_view_slider_t *local, uint8_t percent)
{
    uint8_t max_value = egui_view_slider_get_max_value(local);
    uint8_t value;

    if (percent > 100u)
    {
        percent = 100u;
    }

    if (max_value >= 100u)
    {
        return percent;
    }

    value = (uint8_t)(((uint32_t)percent * (uint32_t)max_value + 50u) / 100u);
    return egui_view_slider_clamp_value(local, value);
}

void egui_view_slider_set_on_value_changed_listener(egui_view_t *self, egui_view_on_value_changed_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_slider_t);
    local->on_value_changed = listener;
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static uint8_t egui_view_slider_get_thumb_metrics(egui_view_t *self, egui_view_slider_t *local, uint8_t value, egui_dim_t *out_thumb_x,
                                                  egui_dim_t *out_thumb_y,
                                                  egui_dim_t *out_thumb_radius)
{
    egui_region_t region;
    egui_view_linear_value_metrics_t metrics;
    uint8_t percent;

    if (self->region_screen.size.width <= 0 || self->region_screen.size.height <= 0)
    {
        return 0;
    }

    egui_view_get_work_region(self, &region);

    if (!egui_view_linear_value_get_metrics(&region, 1, &metrics))
    {
        return 0;
    }

    percent = egui_view_slider_value_to_percent(local, value);

    if (out_thumb_x != NULL)
    {
        *out_thumb_x = egui_view_linear_value_get_x(&metrics, percent);
    }
    if (out_thumb_y != NULL)
    {
        *out_thumb_y = metrics.center_y;
    }
    if (out_thumb_radius != NULL)
    {
        *out_thumb_radius = metrics.knob_radius;
    }

    return 1;
}
#endif

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static uint8_t egui_view_slider_get_thumb_dirty_region(egui_view_t *self, egui_view_slider_t *local, uint8_t value, egui_region_t *dirty_region)
{
    egui_dim_t thumb_x;
    egui_dim_t thumb_y;
    egui_dim_t thumb_radius;

    if (dirty_region == NULL)
    {
        return 0;
    }

    if (!egui_view_slider_get_thumb_metrics(self, local, value, &thumb_x, &thumb_y, &thumb_radius))
    {
        egui_region_init_empty(dirty_region);
        return 0;
    }

    egui_region_init_empty(dirty_region);
    egui_view_circle_dirty_add_circle_region(dirty_region, thumb_x, thumb_y, thumb_radius, EGUI_VIEW_CIRCLE_DIRTY_AA_PAD + 1);
    return !egui_region_is_empty(dirty_region);
}
#endif

static void egui_view_slider_invalidate_value_change(egui_view_t *self, egui_view_slider_t *local, uint8_t old_value)
{
    egui_region_t region;
    egui_region_t dirty_region;
    egui_view_linear_value_metrics_t metrics;
    egui_dim_t old_thumb_x;
    egui_dim_t new_thumb_x;
    egui_dim_t diff_x;
    egui_dim_t diff_w;
    uint8_t old_percent;
    uint8_t new_percent;

    if (self->region_screen.size.width <= 0 || self->region_screen.size.height <= 0)
    {
        egui_view_invalidate(self);
        return;
    }

    if (egui_view_has_pending_dirty(self))
    {
        egui_view_invalidate_full(self);
        return;
    }

    egui_view_get_work_region(self, &region);

    if (!egui_view_linear_value_get_metrics(&region, 1, &metrics))
    {
        egui_view_invalidate(self);
        return;
    }

    old_percent = egui_view_slider_value_to_percent(local, old_value);
    new_percent = egui_view_slider_value_to_percent(local, local->value);
    old_thumb_x = egui_view_linear_value_get_x(&metrics, old_percent);
    new_thumb_x = egui_view_linear_value_get_x(&metrics, new_percent);

    egui_region_init_empty(&dirty_region);

    diff_x = EGUI_MIN(old_thumb_x, new_thumb_x) - metrics.track_radius;
    diff_w = EGUI_ABS(new_thumb_x - old_thumb_x) + metrics.track_radius * 2 + 1;
    egui_view_circle_dirty_add_rect_region(&dirty_region, diff_x, metrics.track_y, diff_w, metrics.track_height, EGUI_VIEW_CIRCLE_DIRTY_AA_PAD);

    egui_view_circle_dirty_add_circle_region(&dirty_region, old_thumb_x, metrics.center_y, metrics.knob_radius, EGUI_VIEW_CIRCLE_DIRTY_AA_PAD + 1);
    egui_view_circle_dirty_add_circle_region(&dirty_region, new_thumb_x, metrics.center_y, metrics.knob_radius, EGUI_VIEW_CIRCLE_DIRTY_AA_PAD + 1);

    if (egui_region_is_empty(&dirty_region))
    {
        return;
    }

    egui_view_invalidate_region(self, &dirty_region);
}

void egui_view_slider_set_value(egui_view_t *self, uint8_t value)
{
    EGUI_LOCAL_INIT(egui_view_slider_t);
    uint8_t old_value;

    value = egui_view_slider_clamp_value(local, value);
    if (value != local->value)
    {
        old_value = local->value;
        local->value = value;
        if (local->on_value_changed)
        {
            local->on_value_changed(self, value);
        }

        egui_view_slider_invalidate_value_change(self, local, old_value);
    }
}

uint8_t egui_view_slider_get_value(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_slider_t);
    return local->value;
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_TOUCH
static void egui_view_slider_update_value_from_touch(egui_view_t *self, egui_dim_t touch_x)
{
    EGUI_LOCAL_INIT(egui_view_slider_t);
    egui_view_linear_value_metrics_t metrics;
    uint8_t percent;

    // Convert screen touch_x to local coordinates
    egui_dim_t local_x = touch_x - self->region_screen.location.x;

    if (!egui_view_linear_value_get_metrics(&self->region, 1, &metrics))
    {
        return;
    }

    percent = egui_view_linear_value_get_value_from_local_x(&metrics, local_x);
    uint8_t new_value = egui_view_slider_percent_to_value(local, percent);
    egui_view_slider_set_value(self, new_value);
}
#endif

void egui_view_slider_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_slider_t);
    egui_region_t region;
    egui_view_linear_value_metrics_t metrics;
    egui_dim_t thumb_x;
    egui_dim_t thumb_y;
    egui_dim_t track_draw_x;
    egui_dim_t track_draw_width;
    egui_dim_t active_width;
    uint8_t percent;

    egui_view_get_work_region(self, &region);

    if (!egui_view_linear_value_get_metrics(&region, 1, &metrics))
    {
        return;
    }

    percent = egui_view_slider_value_to_percent(local, local->value);
    thumb_x = egui_view_linear_value_get_x(&metrics, percent);
    thumb_y = metrics.center_y;
    track_draw_x = metrics.start_x;
    track_draw_width = metrics.usable_width;

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
        egui_canvas_draw_round_rectangle_fill(track_draw_x, metrics.track_y, track_draw_width, metrics.track_height, metrics.track_radius, track_color,
                                              EGUI_ALPHA_100);
    }
#else
    egui_canvas_draw_round_rectangle_fill(track_draw_x, metrics.track_y, track_draw_width, metrics.track_height, metrics.track_radius, track_color,
                                          EGUI_ALPHA_100);
#endif

    // Active Track (Blue)
    active_width = thumb_x - track_draw_x;
    if (active_width > 0)
    {
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
        egui_canvas_draw_round_rectangle_fill(track_draw_x, metrics.track_y, active_width, metrics.track_height, metrics.track_radius, fill_color,
                                              EGUI_ALPHA_100);
#else
        egui_canvas_draw_round_rectangle_fill(track_draw_x, metrics.track_y, active_width, metrics.track_height, metrics.track_radius, fill_color,
                                              EGUI_ALPHA_100);
#endif
    }

    // Draw thumb circle
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
    {
        egui_canvas_draw_circle_fill_hq(thumb_x, thumb_y, metrics.knob_radius, knob_color, EGUI_ALPHA_100);
        // Draw a subtle border around the knob for visibility on same-color backgrounds
        egui_canvas_draw_circle(thumb_x, thumb_y, metrics.knob_radius, 1, EGUI_THEME_BORDER, 80);
    }
#else
    egui_canvas_draw_circle_fill_basic(thumb_x, thumb_y, metrics.knob_radius, knob_color, EGUI_ALPHA_100);
    egui_canvas_draw_circle_basic(thumb_x, thumb_y, metrics.knob_radius, 1, EGUI_THEME_BORDER, EGUI_ALPHA_100);
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
        egui_region_t dirty_region;
        const egui_region_t *dirty_ptr = NULL;
        uint8_t old_value = local->value;

        local->is_dragging = 1;

        // Request parent to not intercept touch events
        if (self->parent != NULL)
        {
            egui_view_group_request_disallow_intercept_touch_event((egui_view_t *)self->parent, 1);
        }

        // Update value from touch position
        egui_view_slider_update_value_from_touch(self, event->location.x);

        if (old_value == local->value && egui_view_slider_get_thumb_dirty_region(self, local, local->value, &dirty_region))
        {
            dirty_ptr = &dirty_region;
        }

        egui_view_set_pressed_with_region(self, true, dirty_ptr);
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
        egui_region_t dirty_region;
        const egui_region_t *dirty_ptr = NULL;

        if (egui_view_slider_get_thumb_dirty_region(self, local, local->value, &dirty_region))
        {
            dirty_ptr = &dirty_region;
        }

        local->is_dragging = 0;
        egui_view_set_pressed_with_region(self, false, dirty_ptr);
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
    local->max_value = 100u;
    local->track_color = EGUI_THEME_TRACK_BG;
    local->active_color = EGUI_THEME_PRIMARY;
    local->thumb_color = EGUI_THEME_THUMB;

    egui_view_set_view_name(self, "egui_view_slider");
}

void egui_view_slider_apply_params(egui_view_t *self, const egui_view_slider_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_slider_t);

    self->region = params->region;

    local->value = egui_view_slider_clamp_value(local, params->value);

    egui_view_invalidate(self);
}

void egui_view_slider_init_with_params(egui_view_t *self, const egui_view_slider_params_t *params)
{
    egui_view_slider_init(self);
    egui_view_slider_apply_params(self, params);
}
