#ifndef _EGUI_INTERPOLATOR_ACCELERATE_DECELERATE_H_
#define _EGUI_INTERPOLATOR_ACCELERATE_DECELERATE_H_

#include "egui_interpolator.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_interpolator_accelerate_decelerate egui_interpolator_accelerate_decelerate_t;
struct egui_interpolator_accelerate_decelerate
{
    egui_interpolator_t base;
};

void egui_interpolator_accelerate_decelerate_init(egui_interpolator_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_INTERPOLATOR_ACCELERATE_DECELERATE_H_ */
