#ifndef _EGUI_INTERPOLATOR_DECELERATE_H_
#define _EGUI_INTERPOLATOR_DECELERATE_H_

#include "egui_interpolator.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_interpolator_decelerate egui_interpolator_decelerate_t;
struct egui_interpolator_decelerate
{
    egui_interpolator_t base;

    egui_float_t factor; // factor Degree to which the animation should be eased. Setting factor to 1.0f produces an upside-down y=x^2 parabola. Increasing
                         // factor above 1.0f makes exaggerates the ease-out effect (i.e., it starts even faster and ends evens slower)
};

void egui_interpolator_decelerate_init(egui_interpolator_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_INTERPOLATOR_DECELERATE_H_ */
