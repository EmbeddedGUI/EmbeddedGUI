#include <stdio.h>
#include <assert.h>

#include "egui_view_progress_bar.h"
#include "egui_view_circle_dirty.h"
#include "egui_view_linear_value_helper.h"
#include "style/egui_theme.h"

#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
#include "core/egui_canvas_gradient.h"
#endif

void egui_view_progress_bar_set_on_progress_listener(egui_view_t *self, egui_view_on_progress_changed_listener_t listener)
{
    EGUI_LOCAL_INIT(egui_view_progress_bar_t);
    local->on_progress_changed = listener;
}

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

        egui_view_progress_bar_invalidate_process_change(self, local, old_process);
    }
}

// static void egui_view_progress_bar_on_click(egui_view_t *self)
// {
//     egui_view_progress_bar_t *local = (egui_view_progress_bar_t *)self;
//     EGUI_UNUSED(local);

//     // egui_view_progress_bar_set_switch_on(self, !local->is_checked);
// }

void egui_view_progress_bar_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_progress_bar_t);

    egui_region_t region;
    egui_view_linear_value_metrics_t metrics;
    egui_dim_t progress_width;
    egui_view_get_work_region(self, &region);

    if (!egui_view_linear_value_get_metrics(&region, 0, &metrics))
    {
        return;
    }

    /* Query theme styles for 2 parts */
    const egui_widget_style_desc_t *desc = egui_current_theme ? egui_current_theme->progress_bar : NULL;
    egui_state_t state = egui_style_get_view_state(self);
    const egui_style_t *track_style = desc ? egui_style_get(desc, EGUI_PART_MAIN, state) : NULL;
    const egui_style_t *fill_style = desc ? egui_style_get(desc, EGUI_PART_INDICATOR, state) : NULL;

    egui_color_t bk = track_style ? track_style->bg_color : local->bk_color;
    egui_color_t prog = fill_style ? fill_style->bg_color : local->progress_color;

    // draw background (thin rounded track)
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
    {
        egui_canvas_draw_round_rectangle_fill(metrics.start_x, metrics.track_y, metrics.usable_width, metrics.track_height, metrics.track_radius, bk,
                                              EGUI_ALPHA_100);
    }
#else
    egui_canvas_draw_round_rectangle_fill(metrics.start_x, metrics.track_y, metrics.usable_width, metrics.track_height, metrics.track_radius, bk,
                                          EGUI_ALPHA_100);
#endif

    // draw progress
    progress_width = (egui_dim_t)((uint32_t)metrics.usable_width * local->process / 100);
    if (progress_width > 0)
    {
#if EGUI_CONFIG_WIDGET_ENHANCED_DRAW
        egui_canvas_draw_round_rectangle_fill(metrics.start_x, metrics.track_y, progress_width, metrics.track_height, metrics.track_radius, prog,
                                              EGUI_ALPHA_100);
#else
        egui_canvas_draw_round_rectangle_fill(metrics.start_x, metrics.track_y, progress_width, metrics.track_height, metrics.track_radius, prog,
                                              EGUI_ALPHA_100);
#endif
    }

    if (local->is_show_control)
    {
        egui_dim_t knob_x = egui_view_linear_value_clamp_x(&metrics, metrics.start_x + progress_width);

        egui_canvas_draw_circle_fill_basic(knob_x, metrics.center_y, metrics.knob_radius, local->control_color, EGUI_ALPHA_100);
        egui_canvas_draw_circle_basic(knob_x, metrics.center_y, metrics.knob_radius, 1, EGUI_THEME_BORDER, EGUI_ALPHA_100);
    }
}

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
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_progress_bar_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_progress_bar_t);

    // call super init.
    egui_view_init(self);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_progress_bar_t);

    // init local data.
    local->on_progress_changed = NULL;

    local->process = 10;
    local->bk_color = EGUI_THEME_TRACK_BG;
    local->progress_color = EGUI_THEME_PRIMARY;
    local->control_color = EGUI_THEME_THUMB;
    local->is_show_control = 0; // Default off for standard progress bar
    // egui_view_set_on_click_listener(self, egui_view_progress_bar_on_click);

    egui_view_set_view_name(self, "egui_view_progress_bar");
}

void egui_view_progress_bar_apply_params(egui_view_t *self, const egui_view_progress_bar_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_progress_bar_t);

    self->region = params->region;

    local->process = params->process;

    egui_view_invalidate(self);
}

void egui_view_progress_bar_init_with_params(egui_view_t *self, const egui_view_progress_bar_params_t *params)
{
    egui_view_progress_bar_init(self);
    egui_view_progress_bar_apply_params(self, params);
}
