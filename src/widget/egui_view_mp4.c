#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "egui_view_mp4.h"
#include "egui.h"

#include "uicode.h"

void egui_view_mp4_on_draw(egui_view_t *self)
{
    egui_view_mp4_t *local = (egui_view_mp4_t *)self;

    if(!local->mp4_image_list || local->mp4_image_count == 0)
    {
        return;
    }
    const egui_image_t *image = (const egui_image_t *)local->mp4_image_list[local->mp4_image_index];

    // printf("on_draw, mp4_image_index=%d, image: %p\n", local->mp4_image_index, image);

    egui_canvas_draw_image(image, 0, 0);
    
    // local->mp4_image_index ++;
    // if(local->mp4_image_index >= local->mp4_image_count)
    // {
    //     // local->mp4_image_index = 0;
    //     local->mp4_image_index = local->mp4_image_count - 1;
    // }
    // else
    // {
    //     egui_view_invalidate(self);
    // }
}

static void anim_timer_callback(egui_timer_t *timer)
{
    // EGUI_LOG_INF("anim_timer_callback\r\n");
    egui_view_mp4_t *local = timer->user_data;

    if (local->mp4_image_index >= local->mp4_image_count - 1)
    {
        egui_timer_stop_timer(timer);
        if(local->callback)
        {
            local->callback(local, 1);
        }
    }
    else
    {
        local->mp4_image_index++;

        egui_view_invalidate((egui_view_t *)local);
    }
}

void egui_view_mp4_start_work(egui_view_t *self, int interval_ms)
{
    egui_view_mp4_t *local = (egui_view_mp4_t *)self;

    local->mp4_image_index = 0;
    egui_timer_start_timer(&local->anim_timer, 0, interval_ms);
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_mp4_t) = {
    .dispatch_touch_event = egui_view_dispatch_touch_event,
    .on_touch_event = egui_view_on_touch_event,
    .on_intercept_touch_event = egui_view_on_intercept_touch_event,
    .compute_scroll = egui_view_compute_scroll,
    .calculate_layout = egui_view_calculate_layout,
    .request_layout = egui_view_request_layout,
    .draw = egui_view_draw,
    .on_attach_to_window = egui_view_on_attach_to_window,
    .on_draw = egui_view_mp4_on_draw, // changed
    .on_detach_from_window = egui_view_on_detach_from_window,
};

void egui_view_mp4_init(egui_view_t *self)
{
    egui_view_mp4_t *local = (egui_view_mp4_t *)self;
    // call super init.
    egui_view_init(self);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_mp4_t);

    // init local data.
    local->mp4_image_list = NULL;
    local->mp4_image_count = 0;
    local->mp4_image_index = 0;

    
    local->anim_timer.callback = anim_timer_callback;
    local->anim_timer.user_data = self;
    egui_timer_stop_timer(&local->anim_timer);
}
