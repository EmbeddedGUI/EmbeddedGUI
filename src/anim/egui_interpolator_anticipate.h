#ifndef _EGUI_INTERPOLATOR_ANTICPATE_H_
#define _EGUI_INTERPOLATOR_ANTICPATE_H_

#include "egui_interpolator.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_interpolator_anticipate egui_interpolator_anticipate_t;
struct egui_interpolator_anticipate
{
    egui_interpolator_t base;

    egui_float_t tension; // Amount of anticipation. When tension equals 0.0f, there is no anticipation and the interpolator becomes a simple acceleration
                          // interpolator.
};

void egui_interpolator_anticipate_tension_set(egui_interpolator_t *self, egui_float_t tension);
void egui_interpolator_anticipate_init(egui_interpolator_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_INTERPOLATOR_ANTICPATE_H_ */
