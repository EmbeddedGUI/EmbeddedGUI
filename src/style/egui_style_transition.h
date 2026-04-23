#ifndef _EGUI_STYLE_TRANSITION_H_
#define _EGUI_STYLE_TRANSITION_H_

#include "egui_style.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Linearly interpolate two colors. Fraction `0` keeps `from`, `255` reaches `to`.
 */
egui_color_t egui_color_lerp(egui_color_t from, egui_color_t to, uint8_t fraction);

/**
 * @brief Linearly interpolate two alpha values.
 */
egui_alpha_t egui_alpha_lerp(egui_alpha_t from, egui_alpha_t to, uint8_t fraction);

/**
 * @brief Linearly interpolate two dimensions.
 */
egui_dim_t egui_dim_lerp(egui_dim_t from, egui_dim_t to, uint8_t fraction);

/**
 * @brief Mutable RAM-side transition context between two immutable styles.
 */
typedef struct egui_style_transition
{
    egui_style_t current;     /* Mutable interpolated result */
    const egui_style_t *from; /* Source style (ROM) */
    const egui_style_t *to;   /* Target style (ROM) */
    uint16_t prop_mask;       /* Which properties to animate */
    uint8_t is_active;        /* Is transition in progress */
} egui_style_transition_t;

/**
 * @brief Initialize a transition between two style records.
 */
void egui_style_transition_init(egui_style_transition_t *trans, const egui_style_t *from, const egui_style_t *to, uint16_t prop_mask);

/**
 * @brief Update the transition with a 0..255 fraction.
 */
void egui_style_transition_update(egui_style_transition_t *trans, uint8_t fraction);

/**
 * @brief Mark the transition as finished and snap to the target style.
 */
void egui_style_transition_finish(egui_style_transition_t *trans);

/**
 * @brief Get the style currently visible to the renderer.
 */
const egui_style_t *egui_style_transition_get_current(const egui_style_transition_t *trans);

/**
 * @brief Return whether the transition is still active.
 */
static inline uint8_t egui_style_transition_is_active(const egui_style_transition_t *trans)
{
    return trans->is_active;
}

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_STYLE_TRANSITION_H_ */
