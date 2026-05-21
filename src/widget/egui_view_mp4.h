#ifndef _EGUI_VIEW_MP4_H_
#define _EGUI_VIEW_MP4_H_

#include "widget/egui_view.h"

#include "core/egui_timer.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_mp4 egui_view_mp4_t;

/** Playback callback. `is_end` is nonzero when the frame sequence reaches its last image. */
typedef void (*egui_view_mp4_callback_func)(egui_view_mp4_t *, int is_end);

struct egui_view_mp4
{
    egui_view_t base;

    egui_timer_t anim_timer;

    egui_view_mp4_callback_func callback;

    uint8_t align_type;
    uint8_t is_playing;

    const egui_image_t **mp4_image_list;
    uint16_t mp4_image_index;
    uint16_t mp4_image_count;
    uint16_t frame_interval_ms;
};

// ============== MP4 Params ==============
typedef struct egui_view_mp4_params egui_view_mp4_params_t;
struct egui_view_mp4_params
{
    egui_region_t region;
};

#define EGUI_VIEW_MP4_PARAMS_INIT(_name, _x, _y, _w, _h) static const egui_view_mp4_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}}

/** Apply the region from one parameter block. */
void egui_view_mp4_apply_params(egui_view_t *self, const egui_view_mp4_params_t *params);
/** Initialize an MP4-style frame player and immediately apply its parameter block. */
void egui_view_mp4_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_mp4_params_t *params);

/** Set how the current frame is aligned inside the widget bounds. */
void egui_view_mp4_set_align_type(egui_view_t *self, uint8_t align_type);
/** Return how the current frame is aligned inside the widget bounds. Returns 0 when self is NULL. */
uint8_t egui_view_mp4_get_align_type(egui_view_t *self);
/** Register the callback fired when playback reaches the end of the frame list. */
void egui_view_mp4_set_callback(egui_view_t *self, egui_view_mp4_callback_func callback);
/** Return the callback fired when playback reaches the end of the frame list. */
egui_view_mp4_callback_func egui_view_mp4_get_callback(egui_view_t *self);
/** Borrow an external image-frame array. The widget does not copy the frame list or image objects. */
void egui_view_mp4_set_mp4_image_list(egui_view_t *self, const egui_image_t **mp4_image_list, uint16_t mp4_image_count);
/** Return the borrowed external image-frame array pointer. */
const egui_image_t **egui_view_mp4_get_mp4_image_list(egui_view_t *self);
/** Return the number of frames in the borrowed image-frame array. */
uint16_t egui_view_mp4_get_mp4_image_count(egui_view_t *self);
/** Return the current frame index. */
uint16_t egui_view_mp4_get_mp4_image_index(egui_view_t *self);
/** Return the configured playback interval in milliseconds. */
uint16_t egui_view_mp4_get_frame_interval_ms(egui_view_t *self);
/** Return whether timer-driven playback is currently marked active. */
uint8_t egui_view_mp4_is_playing(egui_view_t *self);
/** Start playback from frame 0 and schedule the internal timer when the widget is attached and the interval is valid. */
void egui_view_mp4_start_work(egui_view_t *self, int interval_ms);
/** Stop playback and cancel the internal frame timer. */
void egui_view_mp4_stop_work(egui_view_t *self);
/** Initialize the timer-driven frame-sequence player. */
void egui_view_mp4_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_MP4_H_ */
