#ifndef _EGUI_INTERPOLATOR_ANTICIPATE_OVERSHOOT_H_
#define _EGUI_INTERPOLATOR_ANTICIPATE_OVERSHOOT_H_

#include "egui_interpolator.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_interpolator_anticipate_overshoot egui_interpolator_anticipate_overshoot_t;
struct egui_interpolator_anticipate_overshoot
{
    egui_interpolator_t base;

    egui_float_t tension; // Amount of anticipation/overshoot. When tension equals 0.0f, there is no anticipation/overshoot and the interpolator becomes a
                          // simple acceleration/deceleration interpolator.
};

void egui_interpolator_anticipate_overshoot_tension_set(egui_interpolator_t *self, egui_float_t tension);
void egui_interpolator_anticipate_overshoot_init(egui_interpolator_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_INTERPOLATOR_ANTICIPATE_OVERSHOOT_H_ */
