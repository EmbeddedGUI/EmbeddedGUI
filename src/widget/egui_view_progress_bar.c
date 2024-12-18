#include <stdio.h>
#include <assert.h>

#include "egui_view_progress_bar.h"

void egui_view_progress_bar_set_on_progress_listener(egui_view_t *self, egui_view_on_progress_changed_listener_t listener)
{
    egui_view_progress_bar_t *local = (egui_view_progress_bar_t *)self;
    local->on_progress_changed = listener;
}

void egui_view_progress_bar_set_process(egui_view_t *self, uint8_t process)
{
    egui_view_progress_bar_t *local = (egui_view_progress_bar_t *)self;
    if(process != local->process)
    {
        local->process = process;
        if(local->on_progress_changed)
        {
            local->on_progress_changed(self, process);
        }

        egui_view_invalidate(self);
    }
}

static void egui_view_progress_bar_on_click(egui_view_t *self)
{
    egui_view_progress_bar_t *local = (egui_view_progress_bar_t *)self;

    // egui_view_progress_bar_set_switch_on(self, !local->is_checked);
}

void egui_view_progress_bar_on_draw(egui_view_t *self)
{
    egui_view_progress_bar_t *local = (egui_view_progress_bar_t *)self;

    egui_region_t region;
    egui_view_get_work_region(self, &region);

    egui_dim_t radius = region.size.height / 2 - 1;
    egui_dim_t radius_control = radius;
    egui_dim_t central_y = region.location.y + region.size.height / 2;
    egui_dim_t diff_control = 0;
    if(local->is_show_control)
    {
        diff_control = 4;
    }
    radius = radius - diff_control;

    // draw background.
    egui_canvas_draw_round_rectangle_fill(region.location.x, central_y - radius, region.size.width, region.size.height - 2 * diff_control, radius, local->bk_color, EGUI_ALPHA_100);
    
    // draw progress.
    egui_dim_t progress_width = region.size.width * local->process / 100;
    // TODO: if progress_width < radius, then draw a circle instead of a rectangle.
    if(progress_width < radius)
    {
        egui_canvas_draw_circle_fill(region.location.x + progress_width / 2, central_y, progress_width / 2, local->progress_color, EGUI_ALPHA_100);
    }
    else
    {
        egui_canvas_draw_round_rectangle_fill(region.location.x, central_y - radius, progress_width, region.size.height - 2 * diff_control, radius, local->progress_color, EGUI_ALPHA_100);
    }

    if(local->is_show_control)
    {
        // draw control.
        egui_canvas_draw_circle_fill(region.location.x + progress_width, central_y + 1, radius_control, local->control_color, EGUI_ALPHA_100);
    }
}

EGUI_VIEW_API_DEFINE(egui_view_progress_bar_t, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, egui_view_progress_bar_on_draw, NULL);

void egui_view_progress_bar_init(egui_view_t *self)
{
    egui_view_progress_bar_t *local = (egui_view_progress_bar_t *)self;
#if EGUI_CONFIG_AC5	
	  EGUI_VIEW_API_INIT(egui_view_progress_bar_t, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, egui_view_progress_bar_on_draw, NULL);
#endif
    // call super init.
    egui_view_init(self);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_progress_bar_t);

    // init local data.
    local->process = 10;
    local->bk_color = EGUI_COLOR_DARK_GREY;
    local->progress_color = EGUI_COLOR_WHITE;
    local->control_color = EGUI_COLOR_WHITE;
    local->is_show_control = 1;
    // egui_view_set_on_click_listener(self, egui_view_progress_bar_on_click);

    egui_view_set_view_name(self, "egui_view_progress_bar");
}
