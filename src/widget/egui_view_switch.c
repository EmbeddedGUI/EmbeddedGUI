#include <stdio.h>
#include <assert.h>

#include "egui_view_switch.h"

void egui_view_switch_set_on_checked_listener(egui_view_t *self, egui_view_on_checked_listener_t listener)
{
    egui_view_switch_t *local = (egui_view_switch_t *)self;
    local->on_checked_changed = listener;
}

void egui_view_switch_set_checked(egui_view_t *self, uint8_t is_checked)
{
    egui_view_switch_t *local = (egui_view_switch_t *)self;
    if(is_checked != local->is_checked)
    {
        local->is_checked = is_checked;
        if(local->on_checked_changed)
        {
            local->on_checked_changed(self, is_checked);
        }

        egui_view_invalidate(self);
    }
}

static void egui_view_switch_on_click(egui_view_t *self)
{
    egui_view_switch_t *local = (egui_view_switch_t *)self;

    egui_view_switch_set_checked(self, !local->is_checked);
}

void egui_view_switch_on_draw(egui_view_t *self)
{
    egui_view_switch_t *local = (egui_view_switch_t *)self;

    egui_region_t region;
    egui_view_get_work_region(self, &region);

    egui_dim_t radius = region.size.height / 2 - 1;
    if(local->is_checked)
    {
        // draw background.
        egui_canvas_draw_round_rectangle_fill(region.location.x, region.location.y, region.size.width, region.size.height, radius, local->bk_color_on, local->alpha);
        // draw switch.
        egui_canvas_draw_circle_fill(region.location.x + region.size.width - radius - 1, region.location.y + radius + 1, radius, local->switch_color_on, local->alpha);
    }
    else
    {
        // draw background.
        egui_canvas_draw_round_rectangle_fill(region.location.x, region.location.y, region.size.width, region.size.height, radius, local->bk_color_off, local->alpha);
        // draw switch.
        egui_canvas_draw_circle_fill(region.location.x + radius, region.location.y + radius + 1, radius, local->switch_color_off, local->alpha);
    }
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_switch_t) = {
    .dispatch_touch_event = egui_view_dispatch_touch_event,
    .on_touch_event = egui_view_on_touch_event,
    .on_intercept_touch_event = egui_view_on_intercept_touch_event,
    .compute_scroll = egui_view_compute_scroll,
    .calculate_layout = egui_view_calculate_layout,
    .request_layout = egui_view_request_layout,
    .draw = egui_view_draw,
    .on_attach_to_window = egui_view_on_attach_to_window,
    .on_draw = egui_view_switch_on_draw, // changed
    .on_detach_from_window = egui_view_on_detach_from_window,
};

void egui_view_switch_init(egui_view_t *self)
{
    egui_view_switch_t *local = (egui_view_switch_t *)self;
    // call super init.
    egui_view_init(self);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_switch_t);

    // init local data.
    local->is_checked = false;
    local->bk_color_on = EGUI_COLOR_GREEN;
    local->bk_color_off = EGUI_COLOR_DARK_GREY;
    local->switch_color_on = EGUI_COLOR_WHITE;
    local->switch_color_off = EGUI_COLOR_WHITE;
    local->alpha = EGUI_ALPHA_100;
    egui_view_set_on_click_listener(self, egui_view_switch_on_click);

    egui_view_set_view_name(self, "egui_view_switch");
}
