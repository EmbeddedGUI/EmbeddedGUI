#ifndef _EGUI_VIEW_ANIMATED_IMAGE_H_
#define _EGUI_VIEW_ANIMATED_IMAGE_H_

#include "egui_view.h"
#include "image/egui_image.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_view_animated_image egui_view_animated_image_t;
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
struct egui_view_animated_image_params
{
    egui_region_t region;
    uint16_t frame_interval_ms;
};

#define EGUI_VIEW_ANIMATED_IMAGE_PARAMS_INIT(_name, _x, _y, _w, _h, _interval_ms)                                                                              \
    static const egui_view_animated_image_params_t _name = {.region = {{(_x), (_y)}, {(_w), (_h)}}, .frame_interval_ms = (_interval_ms)}

void egui_view_animated_image_apply_params(egui_view_t *self, const egui_view_animated_image_params_t *params);
void egui_view_animated_image_init_with_params(egui_view_t *self, const egui_view_animated_image_params_t *params);

void egui_view_animated_image_set_frames(egui_view_t *self, const egui_image_t **frames, uint8_t count);
void egui_view_animated_image_set_interval(egui_view_t *self, uint16_t ms);
void egui_view_animated_image_play(egui_view_t *self);
void egui_view_animated_image_stop(egui_view_t *self);
void egui_view_animated_image_set_loop(egui_view_t *self, uint8_t enable);
void egui_view_animated_image_set_current_frame(egui_view_t *self, uint8_t index);
void egui_view_animated_image_update(egui_view_t *self, uint16_t elapsed_ms);

void egui_view_animated_image_init(egui_view_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_ANIMATED_IMAGE_H_ */
