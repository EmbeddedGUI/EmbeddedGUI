#ifndef _EGUI_VIEW_ANIMATED_IMAGE_H_
#define _EGUI_VIEW_ANIMATED_IMAGE_H_

#include "egui_view.h"
#include "image/egui_image.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_animated_image egui_view_animated_image_t;
/**
 * @brief Image widget that cycles through a borrowed frame array.
 *
 * This widget does not allocate images or own a timer. Callers provide the
 * frame table, choose the playback interval, and periodically call `update()`
 * with elapsed time from their own tick source.
 */
struct egui_view_animated_image
{
    egui_view_t base;

    const egui_image_t **frames;
    uint8_t frame_count;
    uint8_t current_frame;
    uint8_t is_playing;
    uint8_t is_loop;
    uint16_t frame_interval_ms;
    uint16_t elapsed_ms;
};

// ============== AnimatedImage Params ==============
typedef struct egui_view_animated_image_params egui_view_animated_image_params_t;
/**
 * @brief Construction-time parameter block for one animated-image widget.
 */
struct egui_view_animated_image_params
{
    egui_region_t region;
    uint16_t frame_interval_ms;
};

/** Build an animated-image parameter block with region and frame interval. */
#define EGUI_VIEW_ANIMATED_IMAGE_PARAMS_INIT(_name, _x, _y, _w, _h, _interval_ms)                                                                              \
    static const egui_view_animated_image_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .frame_interval_ms = (_interval_ms)}

/** Apply the region and frame interval from one parameter block. */
void egui_view_animated_image_apply_params(egui_view_t *self, const egui_view_animated_image_params_t *params);
/** Initialize an animated-image view and immediately apply its parameter block. */
void egui_view_animated_image_init_with_params(egui_view_t *self, egui_core_t *core, const egui_view_animated_image_params_t *params);

/** Borrow an external frame array. The widget does not copy the array or image objects. */
void egui_view_animated_image_set_frames(egui_view_t *self, const egui_image_t **frames, uint8_t count);
/** Return the borrowed frame array pointer, or NULL when no frame array is set. */
const egui_image_t **egui_view_animated_image_get_frames(egui_view_t *self);
/** Return the number of frames in the borrowed frame array. */
uint8_t egui_view_animated_image_get_frame_count(egui_view_t *self);
/** Set the per-frame interval used by `update()`. A value of 0 pauses frame advancement. */
void egui_view_animated_image_set_interval(egui_view_t *self, uint16_t ms);
/** Return the per-frame interval in milliseconds. */
uint16_t egui_view_animated_image_get_interval(egui_view_t *self);
/** Mark the animation as playing and reset accumulated time. Call `update()` periodically to advance frames. */
void egui_view_animated_image_play(egui_view_t *self);
/** Stop frame advancement while keeping the current frame visible. */
void egui_view_animated_image_stop(egui_view_t *self);
/** Return whether playback is currently enabled. */
uint8_t egui_view_animated_image_is_playing(egui_view_t *self);
/** Enable or disable looping when playback reaches the last frame. */
void egui_view_animated_image_set_loop(egui_view_t *self, uint8_t enable);
/** Return whether playback wraps back to the first frame. */
uint8_t egui_view_animated_image_get_loop(egui_view_t *self);
/** Switch to one frame immediately. Out-of-range indices are ignored. */
void egui_view_animated_image_set_current_frame(egui_view_t *self, uint8_t index);
/** Return the current frame index. */
uint8_t egui_view_animated_image_get_current_frame(egui_view_t *self);
/** Return the accumulated elapsed time since the last frame step. */
uint16_t egui_view_animated_image_get_elapsed(egui_view_t *self);
/** Advance playback by elapsed milliseconds. This widget has no internal timer, so callers must drive it externally. */
void egui_view_animated_image_update(egui_view_t *self, uint16_t elapsed_ms);

/** Initialize the animated-image view with looping enabled and a 100 ms default interval. */
void egui_view_animated_image_init(egui_view_t *self, egui_core_t *core);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_ANIMATED_IMAGE_H_ */
