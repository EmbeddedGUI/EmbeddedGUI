#include <stdio.h>
#include <assert.h>

#include "egui_view_led.h"
#include "core/egui_core.h"
#include "egui_view_circle_dirty.h"

#if EGUI_CONFIG_FUNCTION_WIDGET_ENHANCED_DRAW
#include "canvas/egui_canvas_gradient.h"
#endif

/**
 * @file egui_view_led.c
 * @brief Circular status indicator with optional timer-driven blinking.
 *
 * Learning path:
 * - state changes try to invalidate only the circular lamp area,
 * - a widget-owned timer toggles the on or off state when blinking,
 * - attach and detach control whether the blink timer is actually running.
 */

/**
 * @brief Compute the smallest dirty region that fully covers the lamp circle.
 */
static uint8_t egui_view_led_get_dirty_region(egui_view_t *self, egui_region_t *dirty_region)
{
    egui_region_t region;
    egui_dim_t center_x;
    egui_dim_t center_y;
    egui_dim_t radius;

    if (dirty_region == NULL)
    {
        return 0;
    }

    egui_region_init_empty(dirty_region);
    if (self->region_screen.size.width <= 0 || self->region_screen.size.height <= 0)
    {
        return 0;
    }

    egui_view_get_work_region(self, &region);
    radius = EGUI_MIN(region.size.width, region.size.height) / 2 - 1;
    if (radius <= 0)
    {
        return 0;
    }

    center_x = region.location.x + region.size.width / 2;
    center_y = region.location.y + region.size.height / 2;
    egui_view_circle_dirty_add_circle_region(dirty_region, center_x, center_y, radius, EGUI_VIEW_CIRCLE_DIRTY_AA_PAD);
    return egui_region_is_empty(dirty_region) ? 0 : 1;
}

/**
 * @brief Invalidate only the indicator circle when possible.
 */
static void egui_view_led_invalidate_indicator(egui_view_t *self)
{
    egui_region_t dirty_region;

    if (egui_view_led_get_dirty_region(self, &dirty_region))
    {
        egui_view_invalidate_region(self, &dirty_region);
    }
    else
    {
        egui_view_invalidate(self);
    }
}

/**
 * @brief Timer callback that flips the LED state during blinking.
 */
static void egui_view_led_blink_callback(egui_timer_t *timer)
{
    egui_view_led_t *local = (egui_view_led_t *)timer->user_data;
    local->is_on = !local->is_on;
    egui_view_led_invalidate_indicator((egui_view_t *)local);
}

/**
 * @brief Start or stop the blink timer based on widget state and attachment.
 */
static void egui_view_led_update_blink_timer(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_led_t);

    if (egui_view_get_core(self) == NULL)
    {
        return;
    }

    if (local->is_blinking && self->is_attached_to_window && local->blink_period > 0)
    {
        if (!egui_view_check_timer_start(self, &local->blink_timer))
        {
            egui_view_start_timer(self, &local->blink_timer, local->blink_period, local->blink_period);
        }
    }
    else
    {
        egui_view_stop_timer(self, &local->blink_timer);
    }
}

/**
 * @brief Resume blinking when the widget becomes attached to a window.
 */
static void egui_view_led_on_attach_to_window(egui_view_t *self)
{
    egui_view_on_attach_to_window(self);
    egui_view_led_update_blink_timer(self);
}

/**
 * @brief Stop the blink timer before normal detach processing.
 */
static void egui_view_led_on_detach_from_window(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_led_t);

    egui_view_stop_timer(self, &local->blink_timer);
    egui_view_on_detach_from_window(self);
}

/**
 * @brief Force the indicator into the lit state.
 */
void egui_view_led_set_on(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_led_t);
    if (!local->is_on)
    {
        local->is_on = 1;
        egui_view_led_invalidate_indicator(self);
    }
}

/**
 * @brief Force the indicator into the unlit state.
 */
void egui_view_led_set_off(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_led_t);
    if (local->is_on)
    {
        local->is_on = 0;
        egui_view_led_invalidate_indicator(self);
    }
}

/**
 * @brief Toggle the current lit or unlit state immediately.
 */
void egui_view_led_toggle(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_led_t);
    local->is_on = !local->is_on;
    egui_view_led_invalidate_indicator(self);
}

/**
 * @brief Return whether the indicator is currently lit.
 */
int egui_view_led_get_is_on(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_led_t);
    return (int)local->is_on;
}

/**
 * @brief Enable blinking and update the timer schedule.
 */
void egui_view_led_set_blink(egui_view_t *self, uint16_t period_ms)
{
    EGUI_LOCAL_INIT(egui_view_led_t);
    local->blink_period = period_ms;
    local->is_blinking = 1;
    egui_view_led_update_blink_timer(self);
}

/**
 * @brief Disable blinking while keeping the current visual state.
 */
void egui_view_led_stop_blink(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_led_t);
    local->is_blinking = 0;
    egui_view_led_update_blink_timer(self);
}

/**
 * @brief Return whether the indicator is currently blinking.
 */
int egui_view_led_get_is_blinking(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_led_t);
    return (int)local->is_blinking;
}

/**
 * @brief Return the blink period in milliseconds.
 */
uint16_t egui_view_led_get_blink_period(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_led_t);
    return local->blink_period;
}

egui_color_t egui_view_led_get_on_color(egui_view_t *self)
{
    egui_color_t zero;
    zero.full = 0;
    if (self == NULL)
    {
        return zero;
    }
    EGUI_LOCAL_INIT(egui_view_led_t);
    return local->on_color;
}

egui_color_t egui_view_led_get_off_color(egui_view_t *self)
{
    egui_color_t zero;
    zero.full = 0;
    if (self == NULL)
    {
        return zero;
    }
    EGUI_LOCAL_INIT(egui_view_led_t);
    return local->off_color;
}

egui_color_t egui_view_led_get_border_color(egui_view_t *self)
{
    egui_color_t zero;
    zero.full = 0;
    if (self == NULL)
    {
        return zero;
    }
    EGUI_LOCAL_INIT(egui_view_led_t);
    return local->border_color;
}

egui_dim_t egui_view_led_get_border_width(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_led_t);
    return local->border_width;
}

/**
 * @brief Replace the colors used by the lit and unlit fill paths.
 */
void egui_view_led_set_colors(egui_view_t *self, egui_color_t on_color, egui_color_t off_color)
{
    EGUI_LOCAL_INIT(egui_view_led_t);
    local->on_color = on_color;
    local->off_color = off_color;
    egui_view_led_invalidate_indicator(self);
}

/**
 * @brief Draw the LED border and inner fill circle.
 */
void egui_view_led_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_led_t);
    egui_canvas_t *canvas = egui_view_get_canvas(self);

    egui_region_t region;
    egui_view_get_work_region(self, &region);

    egui_dim_t w = region.size.width;
    egui_dim_t h = region.size.height;
    egui_dim_t center_x = region.location.x + w / 2;
    egui_dim_t center_y = region.location.y + h / 2;
    egui_dim_t radius = EGUI_MIN(w, h) / 2 - 1;

    if (radius <= 0)
    {
        return;
    }

    egui_color_t fill_color = local->is_on ? local->on_color : local->off_color;

    // Draw border circle
    if (local->border_width > 0)
    {
        egui_canvas_draw_circle(canvas, center_x, center_y, radius, local->border_width, local->border_color, EGUI_ALPHA_100);
    }

    // Draw inner filled circle
    egui_dim_t inner_radius = radius - local->border_width;
    if (inner_radius > 0)
    {
#if EGUI_CONFIG_FUNCTION_WIDGET_ENHANCED_DRAW
        {
            egui_color_t fill_light = egui_rgb_mix(fill_color, EGUI_COLOR_WHITE, 100);
            egui_gradient_stop_t led_stops[2] = {
                    {.position = 0, .color = fill_light},
                    {.position = 255, .color = fill_color},
            };
            egui_gradient_t led_grad = {
                    .type = EGUI_GRADIENT_TYPE_RADIAL,
                    .stop_count = 2,
                    .alpha = EGUI_ALPHA_100,
                    .stops = led_stops,
                    .center_x = 0,
                    .center_y = 0,
                    .radius = inner_radius,
            };
            egui_canvas_draw_circle_fill_gradient(canvas, center_x, center_y, inner_radius, &led_grad);
        }
#else
        egui_canvas_draw_circle_fill(canvas, center_x, center_y, inner_radius, fill_color, EGUI_ALPHA_100);
#endif
    }
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_led_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_led_on_attach_to_window,
        .on_draw = egui_view_led_on_draw,
        .on_detach_from_window = egui_view_led_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

/**
 * @brief Initialize the LED with default theme colors and a blink timer.
 */
void egui_view_led_init(egui_view_t *self, egui_core_t *core)
{
    EGUI_INIT_LOCAL(egui_view_led_t);
    // call super init.
    egui_view_init(self, core);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_led_t);

    // init local data.
    local->is_on = 0;
    local->is_blinking = 0;
    local->blink_period = 500;
    local->on_color = EGUI_THEME_PRIMARY;
    local->off_color = EGUI_THEME_TRACK_OFF;
    local->border_color = EGUI_THEME_BORDER;
    local->border_width = 2;

    egui_timer_init_timer(&local->blink_timer, local, egui_view_led_blink_callback);

    egui_view_set_view_name(self, "egui_view_led");
}

/**
 * @brief Apply geometry and initial on or off state from one parameter block.
 */
void egui_view_led_apply_params(egui_view_t *self, const egui_view_led_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_led_t);

    self->region = params->region;

    local->is_on = params->is_on;

    egui_view_invalidate(self);
}

/**
 * @brief Convenience initializer that chains LED init and params.
 */
void egui_view_led_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_led_params_t *params)
{
    egui_view_led_init(self, core);
    egui_view_led_apply_params(self, params);
}
