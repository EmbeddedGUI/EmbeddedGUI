#ifndef _EGUI_STYLE_TRANSITION_H_
#define _EGUI_STYLE_TRANSITION_H_

#include "egui_style.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/* Color linear interpolation: fraction 0=from, 255=to */
egui_color_t egui_color_lerp(egui_color_t from, egui_color_t to, uint8_t fraction);

/* Alpha linear interpolation */
egui_alpha_t egui_alpha_lerp(egui_alpha_t from, egui_alpha_t to, uint8_t fraction);

/* Dimension linear interpolation */
egui_dim_t egui_dim_lerp(egui_dim_t from, egui_dim_t to, uint8_t fraction);

/* Style transition context - lives in RAM only during animation */
typedef struct egui_style_transition
{
    egui_style_t current;     /* Mutable interpolated result */
    const egui_style_t *from; /* Source style (ROM) */
    const egui_style_t *to;   /* Target style (ROM) */
    uint16_t prop_mask;       /* Which properties to animate */
    uint8_t is_active;        /* Is transition in progress */
} egui_style_transition_t;

/* Initialize a transition between two styles */
void egui_style_transition_init(egui_style_transition_t *trans, const egui_style_t *from, const egui_style_t *to, uint16_t prop_mask);

/* Update transition with fraction (0-255, where 0=from, 255=to) */
void egui_style_transition_update(egui_style_transition_t *trans, uint8_t fraction);

/* Mark transition as finished */
void egui_style_transition_finish(egui_style_transition_t *trans);

/* Get current style (interpolated if active, otherwise target) */
const egui_style_t *egui_style_transition_get_current(const egui_style_transition_t *trans);

/* Check if transition is active */
static inline uint8_t egui_style_transition_is_active(const egui_style_transition_t *trans)
{
    return trans->is_active;
}

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_STYLE_TRANSITION_H_ */
