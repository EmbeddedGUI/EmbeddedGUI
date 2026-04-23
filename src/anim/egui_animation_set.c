#include <stdio.h>
#include <assert.h>

#include "egui_animation_set.h"
#include "widget/egui_view.h"
#include "core/egui_api.h"

/**
 * @file egui_animation_set.c
 * @brief Animation-set helper that starts multiple child animations together and mirrors their overall lifecycle.
 */

/** Append one child animation to the set and mark it as owned by a parent animation set. */
void egui_animation_set_add_animation(egui_animation_set_t *self, egui_animation_t *anim)
{
    egui_slist_append(&self->childs, (egui_snode_t *)&anim->node);

    anim->is_inside_animation = true;
}

/** Choose which parent properties should be copied into every child before the set starts them. */
void egui_animation_set_set_mask(egui_animation_set_t *self, int is_mask_repeat_count, int is_mask_repeat_mode, int is_mask_duration, int is_mask_target_view,
                                 int is_mask_interpolator)
{
    self->is_mask_repeat_count = is_mask_repeat_count;
    self->is_mask_repeat_mode = is_mask_repeat_mode;
    self->is_mask_duration = is_mask_duration;
    self->is_mask_target_view = is_mask_target_view;
    self->is_mask_interpolator = is_mask_interpolator;
}

/** Start all child animations, optionally overriding child settings with the parent-set mask flags. */
void egui_animation_set_on_start(egui_animation_t *self)
{
    EGUI_LOCAL_INIT(egui_animation_set_t);

    egui_snode_t *p_head;
    egui_snode_t *p_next;
    egui_animation_t *tmp;

    if (!egui_slist_is_empty(&local->childs))
    {
        EGUI_SLIST_FOR_EACH_NODE_SAFE(&local->childs, p_head, p_next)
        {
            tmp = EGUI_SLIST_ENTRY(p_head, egui_animation_t, node);

            if (local->is_mask_repeat_count)
            {
                egui_animation_repeat_count_set(tmp, self->repeat_count);
            }

            if (local->is_mask_repeat_mode)
            {
                egui_animation_repeat_mode_set(tmp, self->repeat_mode);
            }

            if (local->is_mask_duration)
            {
                egui_animation_duration_set(tmp, self->duration);
            }

            if (local->is_mask_target_view)
            {
                egui_animation_target_view_set(tmp, self->target_view);
            }

            if (local->is_mask_interpolator)
            {
                egui_animation_interpolator_set(tmp, self->interpolator);
            }

            egui_animation_start(tmp);
        }
    }
}

/** Placeholder update hook: the set itself does not apply a property each frame. */
void egui_animation_set_on_update(egui_animation_t *self, egui_float_t fraction)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(fraction);
}

/** Drive every child animation and translate their aggregate started/ended state into one parent lifecycle. */
void egui_animation_set_update(egui_animation_t *self, uint32_t current_time)
{
    EGUI_LOCAL_INIT(egui_animation_set_t);

    int started = 0;
    int ended = 1;

    egui_snode_t *p_head;
    egui_snode_t *p_next;
    egui_animation_t *tmp;
    if (!egui_slist_is_empty(&local->childs))
    {
        EGUI_SLIST_FOR_EACH_NODE_SAFE(&local->childs, p_head, p_next)
        {
            tmp = EGUI_SLIST_ENTRY(p_head, egui_animation_t, node);

            tmp->api->update(tmp, current_time);

            started |= tmp->is_started;
            ended &= tmp->is_ended;
        }
    }

    if (started && !self->is_started)
    {
        self->is_started = true;
        egui_animation_notify_start(self);
    }

    if (ended && !self->is_ended)
    {
        egui_animation_stop(self);

        self->is_ended = true;
        egui_animation_notify_end(self);
    }
}

const egui_animation_api_t egui_animation_set_t_api_table = {
        .on_start = egui_animation_set_on_start,
        .update = egui_animation_set_update,
        .on_update = egui_animation_set_on_update,
};

/** Initialize an animation set and replace the base API table with the group-aware implementation. */
void egui_animation_set_init(egui_animation_t *self)
{
    EGUI_LOCAL_INIT(egui_animation_set_t);
    egui_animation_init(self);
    self->api = &egui_animation_set_t_api_table;

    egui_slist_init(&local->childs);
}
