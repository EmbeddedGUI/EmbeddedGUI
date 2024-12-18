#ifndef _EGUI_INTERPOLATOR_H_
#define _EGUI_INTERPOLATOR_H_

#include "core/egui_canvas.h"
#include "core/egui_region.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

typedef struct egui_interpolator_api egui_interpolator_api_t;
struct egui_interpolator_api
{
    /**
     * Maps a value representing the elapsed fraction of an animation to a value that represents
     * the interpolated fraction. This interpolated value is then multiplied by the change in
     * value of an animation to derive the animated value at the current elapsed animation time.
     *
     * @param input A value between 0 and 1.0 indicating our current point
     *        in the animation where 0 represents the start and 1.0 represents
     *        the end
     * @return The interpolation value. This value can be more than 1.0 for
     *         interpolators which overshoot their targets, or less than 0 for
     *         interpolators that undershoot their targets.
     */
    egui_float_t (*get_interpolation)(egui_interpolator_t *self, egui_float_t input); //
};

struct egui_interpolator
{
    const egui_interpolator_api_t *api; // api of the view
};

void egui_interpolator_init(egui_interpolator_t *self);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_INTERPOLATOR_H_ */
