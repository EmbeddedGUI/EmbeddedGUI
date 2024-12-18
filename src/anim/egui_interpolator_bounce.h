#ifndef _EGUI_INTERPOLATOR_BOUNCE_H_
#define _EGUI_INTERPOLATOR_BOUNCE_H_

#include "egui_interpolator.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_interpolator_bounce egui_interpolator_bounce_t;
struct egui_interpolator_bounce
{
    egui_interpolator_t base;
};

void egui_interpolator_bounce_init(egui_interpolator_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_INTERPOLATOR_BOUNCE_H_ */
