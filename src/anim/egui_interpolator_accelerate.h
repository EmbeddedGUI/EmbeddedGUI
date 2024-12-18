#ifndef _EGUI_INTERPOLATOR_ACCELATE_H_
#define _EGUI_INTERPOLATOR_ACCELATE_H_

#include "egui_interpolator.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_interpolator_accelerate egui_interpolator_accelerate_t;
struct egui_interpolator_accelerate
{
    egui_interpolator_t base;

    egui_float_t factor; // Acceleration factor
};

void egui_interpolator_accelerate_factor_set(egui_interpolator_t *self, egui_float_t factor);
void egui_interpolator_accelerate_init(egui_interpolator_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_INTERPOLATOR_ACCELATE_H_ */
