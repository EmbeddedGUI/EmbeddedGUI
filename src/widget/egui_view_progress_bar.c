#include <stdio.h>
#include <assert.h>

#include "egui_view_progress_bar.h"
#if EGUI_CONFIG_FUNCTION_EVENT_LITE
#include "core/egui_event.h"
#endif
#include "core/egui_core.h"
#include "egui_view_circle_dirty.h"
#include "egui_view_linear_value_helper.h"
#include "style/egui_theme.h"

#if EGUI_CONFIG_FUNCTION_WIDGET_ENHANCED_DRAW
#include "canvas/egui_canvas_gradient.h"
#endif

/**
 * @file egui_view_progress_bar.c
 * @brief Linear progress indicator built on shared track and knob geometry helpers.
 *
 * The implementation is a good
 * reference for:
 * - clamping percentage values,
 * - using theme parts for track and fill colors,
 * - invalidating only the portion changed by progress
 * updates.
 */

/**
 * @brief Register the optional callback fired when progress changes.
 */
void egui_view_progress_bar_set_on_progress_listener(egui_view_t *self, egui_view_on_progress_changed_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_progress_bar_t);
    local->on_progress_changed = listener;
}

egui_view_on_progress_changed_listener_t egui_view_progress_bar_get_on_progress_listener(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_progress_bar_t);
    return local->on_progress_changed;
}

/**
 * @brief Invalidate only the fill span and optional knob touched by a value change.
 */
static void egui_view_progress_bar_invalidate_process_change(egui_view_t *self, egui_view_progress_bar_t *local, uint8_t old_process)
{
    egui_region_t region;
    egui_region_t dirty_region;
    egui_view_linear_value_metrics_t metrics;
    egui_dim_t old_width;
    egui_dim_t new_width;

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
    if (!egui_view_linear_value_get_metrics(&region, 0, &metrics))
    {
        egui_view_invalidate(self);
        return;
    }

    old_width = (egui_dim_t)((uint32_t)metrics.usable_width * old_process / 100);
    new_width = (egui_dim_t)((uint32_t)metrics.usable_width * local->process / 100);

    egui_region_init_empty(&dirty_region);
    egui_view_circle_dirty_add_rect_region(&dirty_region, metrics.start_x + EGUI_MIN(old_width, new_width) - metrics.track_radius, metrics.track_y,
                                           EGUI_ABS(new_width - old_width) + metrics.track_radius * 2 + 1, metrics.track_height, EGUI_VIEW_CIRCLE_DIRTY_AA_PAD);

    if (local->is_show_control)
    {
        egui_dim_t knob_x;

        knob_x = egui_view_linear_value_clamp_x(&metrics, metrics.start_x + old_width);
        egui_view_circle_dirty_add_circle_region(&dirty_region, knob_x, metrics.center_y, metrics.knob_radius, EGUI_VIEW_CIRCLE_DIRTY_AA_PAD + 1);

        knob_x = egui_view_linear_value_clamp_x(&metrics, metrics.start_x + new_width);
        egui_view_circle_dirty_add_circle_region(&dirty_region, knob_x, metrics.center_y, metrics.knob_radius, EGUI_VIEW_CIRCLE_DIRTY_AA_PAD + 1);
    }

    if (egui_region_is_empty(&dirty_region))
    {
        return;
    }

    egui_view_invalidate_region(self, &dirty_region);
}

/**
 * @brief Clamp and store the new progress value, then notify and redraw.
 */
void egui_view_progress_bar_set_process(egui_view_t *self, uint8_t process)
{
    EGUI_LOCAL_INIT(egui_view_progress_bar_t);
    uint8_t old_process;
    if (process > 100)
    {
        process = 100;
    }
    if (process != local->process)
    {
        old_process = local->process;
        local->process = process;
        if (local->on_progress_changed)
        {
            local->on_progress_changed(self, process);
        }
#if EGUI_CONFIG_FUNCTION_EVENT_LITE
        egui_view_send_event(self, EGUI_EVENT_VALUE_CHANGED, &local->process);
#endif

        egui_view_progress_bar_invalidate_process_change(self, local, old_process);
    }
}

uint8_t egui_view_progress_bar_get_process(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_progress_bar_t);
    return local->process;
}

egui_color_t egui_view_progress_bar_get_bk_color(egui_view_t *self)
{
    egui_color_t zero;
    zero.full = 0;
    if (self == NULL)
    {
        return zero;
    }
    EGUI_LOCAL_INIT(egui_view_progress_bar_t);
    return local->bk_color;
}

egui_color_t egui_view_progress_bar_get_progress_color(egui_view_t *self)
{
    egui_color_t zero;
    zero.full = 0;
    if (self == NULL)
    {
        return zero;
    }
    EGUI_LOCAL_INIT(egui_view_progress_bar_t);
    return local->progress_color;
}

egui_color_t egui_view_progress_bar_get_control_color(egui_view_t *self)
{
    egui_color_t zero;
    zero.full = 0;
    if (self == NULL)
    {
        return zero;
    }
    EGUI_LOCAL_INIT(egui_view_progress_bar_t);
    return local->control_color;
}

uint8_t egui_view_progress_bar_get_is_show_control(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_progress_bar_t);
    return local->is_show_control;
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static int egui_view_progress_bar_step_process(egui_view_t *self, egui_view_progress_bar_t *local, int8_t delta)
{
    int16_t process = local->process;

    process = (int16_t)(process + delta);
    if (process < 0)
    {
        process = 0;
    }
    if (process > 100)
    {
        process = 100;
    }
    if (process == local->process)
    {
        return 0;
    }

    egui_view_progress_bar_set_process(self, (uint8_t)process);
    return 1;
}
#endif

/**
 * @brief Draw the track, filled progress span, and optional control knob.
 */
void egui_view_progress_bar_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_progress_bar_t);
    egui_canvas_t *canvas = egui_view_get_canvas(self);

    egui_region_t region;
    egui_view_linear_value_metrics_t metrics;
    egui_dim_t progress_width;
    egui_view_get_work_region(self, &region);

    if (!egui_view_linear_value_get_metrics(&region, 0, &metrics))
    {
        return;
    }

    /* Query theme styles for 2 parts */
    const egui_theme_t *theme = egui_theme_get(egui_view_get_core(self));
    const egui_widget_style_desc_t *desc = theme ? theme->progress_bar : NULL;
    egui_state_t state = egui_style_get_view_state(self);
    const egui_style_t *track_style = desc ? egui_style_get(desc, EGUI_PART_MAIN, state) : NULL;
    const egui_style_t *fill_style = desc ? egui_style_get(desc, EGUI_PART_INDICATOR, state) : NULL;

    egui_color_t bk = track_style ? track_style->bg_color : local->bk_color;
    egui_color_t prog = fill_style ? fill_style->bg_color : local->progress_color;

    // draw background (thin rounded track)
#if EGUI_CONFIG_FUNCTION_WIDGET_ENHANCED_DRAW
    {
        egui_canvas_draw_round_rectangle_fill(canvas, metrics.start_x, metrics.track_y, metrics.usable_width, metrics.track_height, metrics.track_radius, bk,
                                              EGUI_ALPHA_100);
    }
#else
    egui_canvas_draw_round_rectangle_fill(canvas, metrics.start_x, metrics.track_y, metrics.usable_width, metrics.track_height, metrics.track_radius, bk,
                                          EGUI_ALPHA_100);
#endif

    // draw progress
    progress_width = (egui_dim_t)((uint32_t)metrics.usable_width * local->process / 100);
    if (progress_width > 0)
    {
#if EGUI_CONFIG_FUNCTION_WIDGET_ENHANCED_DRAW
        egui_canvas_draw_round_rectangle_fill(canvas, metrics.start_x, metrics.track_y, progress_width, metrics.track_height, metrics.track_radius, prog,
                                              EGUI_ALPHA_100);
#else
        egui_canvas_draw_round_rectangle_fill(canvas, metrics.start_x, metrics.track_y, progress_width, metrics.track_height, metrics.track_radius, prog,
                                              EGUI_ALPHA_100);
#endif
    }

    if (local->is_show_control)
    {
        egui_dim_t knob_x = egui_view_linear_value_clamp_x(&metrics, metrics.start_x + progress_width);

        egui_canvas_draw_circle_fill_basic(canvas, knob_x, metrics.center_y, metrics.knob_radius, local->control_color, EGUI_ALPHA_100);
        egui_canvas_draw_circle_basic(canvas, knob_x, metrics.center_y, metrics.knob_radius, 1, EGUI_THEME_BORDER, EGUI_ALPHA_100);
    }
}

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static int egui_view_progress_bar_on_key_event(egui_view_t *self, egui_key_event_t *event)
{
    EGUI_LOCAL_INIT(egui_view_progress_bar_t);

    if (self->is_enable == false || event == NULL)
    {
        return 0;
    }

    if (event->key_code != EGUI_KEY_CODE_LEFT && event->key_code != EGUI_KEY_CODE_RIGHT)
    {
        return egui_view_on_key_event(self, event);
    }

    if (event->type == EGUI_KEY_EVENT_ACTION_DOWN)
    {
        egui_view_set_pressed(self, true);
        egui_view_progress_bar_step_process(self, local, (event->key_code == EGUI_KEY_CODE_RIGHT) ? 1 : -1);
        return 1;
    }

    if (event->type == EGUI_KEY_EVENT_ACTION_LONG_PRESS || event->type == EGUI_KEY_EVENT_ACTION_REPEAT)
    {
        egui_view_progress_bar_step_process(self, local, (event->key_code == EGUI_KEY_CODE_RIGHT) ? 1 : -1);
        return 1;
    }

    if (event->type == EGUI_KEY_EVENT_ACTION_UP)
    {
        egui_view_set_pressed(self, false);
        return 1;
    }

    return 1;
}
#endif

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_progress_bar_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_progress_bar_on_draw, // changed
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_progress_bar_on_key_event,
#endif
};

/**
 * @brief Initialize the progress bar with theme-based default colors.
 */
void egui_view_progress_bar_init(egui_view_t *self, egui_core_t *core)
{
    EGUI_INIT_LOCAL(egui_view_progress_bar_t);

    // call super init.
    egui_view_init(self, core);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_progress_bar_t);

    // init local data.
    local->on_progress_changed = NULL;

    local->process = 10;
    local->bk_color = EGUI_THEME_TRACK_BG;
    local->progress_color = EGUI_THEME_PRIMARY;
    local->control_color = EGUI_THEME_THUMB;
    local->is_show_control = 0; // Default off for standard progress bar
#if EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
    self->is_focusable = true;
#endif

    egui_view_set_view_name(self, "egui_view_progress_bar");
}

/**
 * @brief Apply geometry and initial percentage from one parameter block.
 */
void egui_view_progress_bar_apply_params(egui_view_t *self, const egui_view_progress_bar_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_progress_bar_t);

    self->region = params->region;

    local->process = params->process;

    egui_view_invalidate(self);
}

/**
 * @brief Convenience initializer that chains progress-bar init and params.
 */
void egui_view_progress_bar_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_progress_bar_params_t *params)
{
    egui_view_progress_bar_init(self, core);
    egui_view_progress_bar_apply_params(self, params);
}
