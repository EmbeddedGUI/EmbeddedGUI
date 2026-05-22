#include <stdio.h>
#include <assert.h>

#include "egui_view_animated_image.h"
#include "core/egui_core.h"
#include "image/egui_image.h"

/**
 * @file egui_view_animated_image.c
 * @brief Minimal frame-player widget driven by external elapsed time.
 *
 * Learning path:
 * - the widget only stores frame pointers and playback state,
 * - drawing always shows the current frame at the work-region origin,
 * - playback advances only when outside code calls `update()`.
 */

/**
 * @brief Draw the current frame without scaling.
 */
void egui_view_animated_image_on_draw(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_animated_image_t);
    egui_canvas_t *canvas = egui_view_get_canvas(self);

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

    egui_canvas_draw_image(canvas, image, region.location.x, region.location.y);
}

/**
 * @brief Replace the borrowed frame array and restart playback position.
 */
void egui_view_animated_image_set_frames(egui_view_t *self, const egui_image_t **frames, uint8_t count)
{
    EGUI_LOCAL_INIT(egui_view_animated_image_t);

    local->frames = frames;
    local->frame_count = count;
    local->current_frame = 0;
    local->elapsed_ms = 0;
    egui_view_invalidate(self);
}

const egui_image_t **egui_view_animated_image_get_frames(egui_view_t *self)
{
    if (self == NULL)
    {
        return NULL;
    }
    EGUI_LOCAL_INIT(egui_view_animated_image_t);
    return local->frames;
}

uint8_t egui_view_animated_image_get_frame_count(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_animated_image_t);
    return local->frame_count;
}

/**
 * @brief Change the frame interval used by future `update()` calls.
 */
void egui_view_animated_image_set_interval(egui_view_t *self, uint16_t ms)
{
    EGUI_LOCAL_INIT(egui_view_animated_image_t);

    local->frame_interval_ms = ms;
}

uint16_t egui_view_animated_image_get_interval(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_animated_image_t);
    return local->frame_interval_ms;
}

/**
 * @brief Start playback from the current frame and clear accumulated time.
 */
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

/**
 * @brief Stop frame advancement while keeping the current frame on screen.
 */
void egui_view_animated_image_stop(egui_view_t *self)
{
    EGUI_LOCAL_INIT(egui_view_animated_image_t);

    local->is_playing = 0;
}

uint8_t egui_view_animated_image_is_playing(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_animated_image_t);
    return local->is_playing;
}

/**
 * @brief Control whether playback wraps back to frame zero.
 */
void egui_view_animated_image_set_loop(egui_view_t *self, uint8_t enable)
{
    EGUI_LOCAL_INIT(egui_view_animated_image_t);

    local->is_loop = enable ? 1 : 0;
}

uint8_t egui_view_animated_image_get_loop(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_animated_image_t);
    return local->is_loop;
}

/**
 * @brief Jump to one frame immediately and request redraw on real changes.
 */
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

uint8_t egui_view_animated_image_get_current_frame(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_animated_image_t);
    return local->current_frame;
}

uint16_t egui_view_animated_image_get_elapsed(egui_view_t *self)
{
    if (self == NULL)
    {
        return 0;
    }
    EGUI_LOCAL_INIT(egui_view_animated_image_t);
    return local->elapsed_ms;
}

/**
 * @brief Advance playback state using externally supplied elapsed milliseconds.
 *
 * This helper intentionally contains no platform timer logic, so
 * examples can
 * drive it from app loops, widget timers, or any custom animation scheduler.
 */
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

/**
 * @brief Initialize the animated-image widget with loop playback enabled.
 */
void egui_view_animated_image_init(egui_view_t *self, egui_core_t *core)
{
    EGUI_INIT_LOCAL(egui_view_animated_image_t);

    // call super init.
    egui_view_init(self, core);
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

/**
 * @brief Apply geometry and initial frame interval from one parameter block.
 */
void egui_view_animated_image_apply_params(egui_view_t *self, const egui_view_animated_image_params_t *params)
{
    EGUI_LOCAL_INIT(egui_view_animated_image_t);

    self->region = params->region;
    local->frame_interval_ms = params->frame_interval_ms;

    egui_view_invalidate(self);
}

/**
 * @brief Convenience initializer that chains animated-image init and params.
 */
void egui_view_animated_image_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_animated_image_params_t *params)
{
    egui_view_animated_image_init(self, core);
    egui_view_animated_image_apply_params(self, params);
}
