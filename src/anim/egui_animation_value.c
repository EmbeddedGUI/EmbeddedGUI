#include "egui_animation_value.h"

/**
 * @file egui_animation_value.c
 * @brief Generic integer value animator.
 *
 * On each frame, the interpolated int32_t value is passed to the registered
 * `on_value` callback so the caller can apply it to any target.
 */

/** Prepare initial state before the first frame fires. */
static void egui_animation_value_on_start(egui_animation_t *self)
{
    EGUI_LOCAL_INIT(egui_animation_value_t);
    local->current = local->from;
}

/** Convert the normalized fraction to an int32 value and fire the callback. */
static void egui_animation_value_on_update(egui_animation_t *self, egui_float_t fraction)
{
    EGUI_LOCAL_INIT(egui_animation_value_t);
    int32_t value = local->from + (int32_t)EGUI_FLOAT_MULT_LIMIT((local->to - local->from), fraction);

    local->current = value;

    if (local->on_value != NULL)
    {
        local->on_value(self, value);
    }
}

static const egui_animation_api_t egui_animation_value_t_api_table = {
        .on_start = egui_animation_value_on_start,
        .update = egui_animation_update,
        .on_update = egui_animation_value_on_update,
};

/**
 * @brief Set the start and end values for the interpolation range.
 */
void egui_animation_value_set_range(egui_animation_value_t *self, int32_t from, int32_t to)
{
    self->from = from;
    self->to = to;
}

/**
 * @brief Register the per-frame callback invoked with the current interpolated value.
 */
void egui_animation_value_set_on_value(egui_animation_value_t *self, egui_animation_value_on_value_t cb)
{
    self->on_value = cb;
}

/**
 * @brief Initialize the value animator.
 */
void egui_animation_value_init(egui_animation_t *self)
{
    EGUI_LOCAL_INIT(egui_animation_value_t);
    egui_animation_init(self);
    self->api = &egui_animation_value_t_api_table;
    local->from = 0;
    local->to = 0;
    local->current = 0;
    local->on_value = NULL;
}
