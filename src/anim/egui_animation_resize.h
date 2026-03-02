#ifndef _EGUI_ANIMATION_RESIZE_H_
#define _EGUI_ANIMATION_RESIZE_H_

#include "egui_animation.h"
#include "core/egui_canvas.h"
#include "core/egui_region.h"
#include "core/egui_timer.h"
#include "utils/egui_slist.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

enum
{
    EGUI_ANIMATION_RESIZE_MODE_LEFT = 0,   // Anchor left/top edge, grow right/down
    EGUI_ANIMATION_RESIZE_MODE_CENTER = 1, // Anchor center, grow both directions
    EGUI_ANIMATION_RESIZE_MODE_RIGHT = 2,  // Anchor right/bottom edge, grow left/up
};

#define EGUI_ANIMATION_RESIZE_PARAMS_INIT(_name, _from_w, _to_w, _from_h, _to_h, _mode)                                                                        \
    static const egui_animation_resize_params_t _name = {                                                                                                      \
            .from_width_ratio = _from_w,                                                                                                                       \
            .to_width_ratio = _to_w,                                                                                                                           \
            .from_height_ratio = _from_h,                                                                                                                      \
            .to_height_ratio = _to_h,                                                                                                                          \
            .mode = _mode,                                                                                                                                     \
    }

typedef struct egui_animation_resize_params egui_animation_resize_params_t;
struct egui_animation_resize_params
{
    egui_float_t from_width_ratio;  // start width ratio (Q16.16, 1.0 = original)
    egui_float_t to_width_ratio;    // end width ratio
    egui_float_t from_height_ratio; // start height ratio
    egui_float_t to_height_ratio;   // end height ratio
    uint8_t mode;                   // resize anchor mode
};

typedef struct egui_animation_resize egui_animation_resize_t;
struct egui_animation_resize
{
    egui_animation_t base;

    egui_region_t origin_region; // original region saved on start

    const egui_animation_resize_params_t *params;
};

void egui_animation_resize_params_set(egui_animation_resize_t *self, const egui_animation_resize_params_t *params);
void egui_animation_resize_init(egui_animation_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_ANIMATION_RESIZE_H_ */
