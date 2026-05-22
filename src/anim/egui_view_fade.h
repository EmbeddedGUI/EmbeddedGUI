#ifndef _EGUI_VIEW_FADE_H_
#define _EGUI_VIEW_FADE_H_

#include "widget/egui_view.h"
#include "anim/egui_animation_alpha.h"

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * Handle struct for one fade animation.
 * The caller is responsible for the lifetime of this object (stack or static).
 * It must remain valid until the
 * animation finishes.
 */
typedef struct egui_view_fade egui_view_fade_t;
struct egui_view_fade
{
    egui_animation_alpha_t anim;          /**< alpha animation instance */
    egui_animation_alpha_params_t params; /**< alpha range (mutable) */
    uint8_t is_fade_out : 1;              /**< 1 = hide view on complete */
};

/**
 * Start a fade-in animation on @p view.
 *
 * @param handle       Caller-owned handle struct (must outlive the animation).
 * @param view         Target view.
 * @param duration_ms  Duration of the animation in milliseconds.
 * @param delay_ms     Delay before the animation starts (requires ANIM_DELAY).
 */
void egui_view_fade_in(egui_view_fade_t *handle, egui_view_t *view, uint16_t duration_ms, uint16_t delay_ms);

/**
 * Start a fade-out animation on @p view.
 * The view will be hidden (is_visible = 0) when the animation completes.
 *
 * @param handle       Caller-owned handle struct (must outlive the animation).
 * @param view         Target view.
 * @param duration_ms  Duration of the animation in milliseconds.
 * @param delay_ms     Delay before the animation starts (requires ANIM_DELAY).
 */
void egui_view_fade_out(egui_view_fade_t *handle, egui_view_t *view, uint16_t duration_ms, uint16_t delay_ms);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
}
#endif

#endif /* _EGUI_VIEW_FADE_H_ */
