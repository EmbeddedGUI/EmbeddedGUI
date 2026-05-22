#include "egui_view_fade.h"

#include "widget/egui_view.h"
#include "anim/egui_animation_alpha.h"

/** Internal end callback: hide the view after fade-out finishes. */
static void fade_on_end(egui_animation_t *anim)
{
    if (anim != NULL && anim->target_view != NULL)
    {
        egui_view_set_visible(anim->target_view, 0);
    }
}

static const egui_animation_handle_t fade_out_handle = {
        .start = NULL,
        .repeat = NULL,
        .end = fade_on_end,
};

/** Common setup shared by fade_in and fade_out. */
static void fade_start(egui_view_fade_t *handle, egui_view_t *view, egui_alpha_t from_alpha, egui_alpha_t to_alpha, uint16_t duration_ms, uint16_t delay_ms)
{
    egui_animation_alpha_init(EGUI_ANIM_OF(&handle->anim));

    handle->params.from_alpha = from_alpha;
    handle->params.to_alpha = to_alpha;

    egui_animation_alpha_params_set(&handle->anim, &handle->params);
    egui_animation_target_view_set(EGUI_ANIM_OF(&handle->anim), view);
    egui_animation_duration_set(EGUI_ANIM_OF(&handle->anim), duration_ms);
    egui_animation_is_fill_after_set(EGUI_ANIM_OF(&handle->anim), 1);

#if EGUI_CONFIG_FUNCTION_ANIM_DELAY
    egui_animation_set_delay(EGUI_ANIM_OF(&handle->anim), delay_ms);
#else
    (void)delay_ms;
#endif

    if (handle->is_fade_out)
    {
        egui_animation_handle_set(EGUI_ANIM_OF(&handle->anim), &fade_out_handle);
    }
    else
    {
        egui_animation_handle_set(EGUI_ANIM_OF(&handle->anim), NULL);
    }

    egui_animation_start(EGUI_ANIM_OF(&handle->anim));
}

void egui_view_fade_in(egui_view_fade_t *handle, egui_view_t *view, uint16_t duration_ms, uint16_t delay_ms)
{
    if (handle == NULL || view == NULL)
    {
        return;
    }
    handle->is_fade_out = 0;
    /* Ensure the view is visible before the animation starts. */
    egui_view_set_visible(view, 1);
    /* Fade from transparent to opaque. */
    fade_start(handle, view, EGUI_ALPHA_0, EGUI_ALPHA_100, duration_ms, delay_ms);
}

void egui_view_fade_out(egui_view_fade_t *handle, egui_view_t *view, uint16_t duration_ms, uint16_t delay_ms)
{
    if (handle == NULL || view == NULL)
    {
        return;
    }
    handle->is_fade_out = 1;
    /* Fade from current alpha to transparent. */
    fade_start(handle, view, view->alpha, EGUI_ALPHA_0, duration_ms, delay_ms);
}
