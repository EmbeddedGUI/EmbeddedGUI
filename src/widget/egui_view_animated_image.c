#include <stdio.h>
#include <assert.h>

#include "egui_view_animated_image.h"
#include "image/egui_image.h"

void egui_view_animated_image_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_animated_image_t);

    egui_region_t region;
    egui_view_get_work_region(self, &region);

    if (local->frames == NULL || local->frame_count == 0)
    {
        return;
    }

    const egui_image_t *image = local->frames[local->current_frame];
    if (image == NULL)
    {
        return;
    }

    egui_canvas_draw_image(image, region.location.x, region.location.y);
}

void egui_view_animated_image_set_frames(egui_view_t *self, const egui_image_t **frames, uint8_t count)
{
    EGUI_LOCAL_INIT(egui_view_animated_image_t);

    local->frames = frames;
    local->frame_count = count;
    local->current_frame = 0;
    local->elapsed_ms = 0;
    egui_view_invalidate(self);
}

void egui_view_animated_image_set_interval(egui_view_t *self, uint16_t ms)
{
    EGUI_LOCAL_INIT(egui_view_animated_image_t);

    local->frame_interval_ms = ms;
}

void egui_view_animated_image_play(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_animated_image_t);

    if (local->is_playing)
    {
        return;
    }
    local->is_playing = 1;
    local->elapsed_ms = 0;
}

void egui_view_animated_image_stop(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_animated_image_t);

    local->is_playing = 0;
}

void egui_view_animated_image_set_loop(egui_view_t *self, uint8_t enable)
{
    EGUI_LOCAL_INIT(egui_view_animated_image_t);

    local->is_loop = enable ? 1 : 0;
}

void egui_view_animated_image_set_current_frame(egui_view_t *self, uint8_t index)
{
    EGUI_LOCAL_INIT(egui_view_animated_image_t);

    if (index >= local->frame_count)
    {
        return;
    }
    if (local->current_frame == index)
    {
        return;
    }
    local->current_frame = index;
    egui_view_invalidate(self);
}

void egui_view_animated_image_update(egui_view_t *self, uint16_t elapsed_ms)
{
    EGUI_LOCAL_INIT(egui_view_animated_image_t);

    if (!local->is_playing || local->frames == NULL || local->frame_count == 0 || local->frame_interval_ms == 0)
    {
        return;
    }

    uint8_t old_frame = local->current_frame;
    local->elapsed_ms += elapsed_ms;

    while (local->elapsed_ms >= local->frame_interval_ms)
    {
        local->elapsed_ms -= local->frame_interval_ms;
        local->current_frame++;

        if (local->current_frame >= local->frame_count)
        {
            if (local->is_loop)
            {
                local->current_frame = 0;
            }
            else
            {
                local->current_frame = local->frame_count - 1;
                local->is_playing = 0;
                local->elapsed_ms = 0;
                break;
            }
        }
    }

    if (local->current_frame != old_frame)
    {
        egui_view_invalidate(self);
    }
}

const egui_view_api_t EGUI_VIEW_API_TABLE_NAME(egui_view_animated_image_t) = {
        .dispatch_touch_event = egui_view_dispatch_touch_event,
        .on_touch_event = egui_view_on_touch_event,
        .on_intercept_touch_event = egui_view_on_intercept_touch_event,
        .compute_scroll = egui_view_compute_scroll,
        .calculate_layout = egui_view_calculate_layout,
        .request_layout = egui_view_request_layout,
        .draw = egui_view_draw,
        .on_attach_to_window = egui_view_on_attach_to_window,
        .on_draw = egui_view_animated_image_on_draw,
        .on_detach_from_window = egui_view_on_detach_from_window,
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
        .dispatch_key_event = egui_view_dispatch_key_event,
        .on_key_event = egui_view_on_key_event,
#endif
};

void egui_view_animated_image_init(egui_view_t *self)
{
    EGUI_INIT_LOCAL(egui_view_animated_image_t);

    // call super init.
    egui_view_init(self);
    // update api.
    self->api = &EGUI_VIEW_API_TABLE_NAME(egui_view_animated_image_t);

    // init local data.
    egui_view_set_view_name(self, "egui_view_animated_image");

    local->frames = NULL;
    local->frame_count = 0;
    local->current_frame = 0;
    local->is_playing = 0;
    local->is_loop = 1;
    local->frame_interval_ms = 100;
    local->elapsed_ms = 0;
}

void egui_view_animated_image_apply_params(egui_view_t *self, const egui_view_animated_image_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_animated_image_t);

    self->region = params->region;
    local->frame_interval_ms = params->frame_interval_ms;

    egui_view_invalidate(self);
}

void egui_view_animated_image_init_with_params(egui_view_t *self, const egui_view_animated_image_params_t *params)
{
    egui_view_animated_image_init(self);
    egui_view_animated_image_apply_params(self, params);
}
