#ifndef _EGUI_INTERPOLATOR_CYCLE_H_
#define _EGUI_INTERPOLATOR_CYCLE_H_

#include "egui_interpolator.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_interpolator_cycle egui_interpolator_cycle_t;
struct egui_interpolator_cycle
{
    egui_interpolator_t base;

    egui_float_t cycles; // number of sine cycles completed while input goes from 0 to 1
};

/** Set how many sine-wave cycles are completed while the input moves from `0` to `1`. */
void egui_interpolator_cycle_tension_set(egui_interpolator_t *self, egui_float_t cycles);
/** Initialize a cycle interpolator that oscillates around zero. */
void egui_interpolator_cycle_init(egui_interpolator_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_INTERPOLATOR_CYCLE_H_ */
