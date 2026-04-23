#include "egui_api.h"
#include "egui_core.h"
#include "egui_core_internal.h"
#include "anim/egui_animation.h"

/**
 * @file egui_core_animation.c
 * @brief Small core-side helpers that own the active animation list for one GUI core.
 */

/** Append one animation handle to the core's active animation list. */
void egui_core_animation_append(egui_core_t *core, egui_animation_t *anim)
{
    egui_slist_append(&core->scene.anims, &anim->node);
}

/** Remove one animation handle from the core's active animation list. */
void egui_core_animation_remove(egui_core_t *core, egui_animation_t *anim)
{
    egui_slist_find_and_remove(&core->scene.anims, &anim->node);
}

/** Poll every active animation once using a shared timestamp so the whole frame advances consistently. */
void egui_core_animation_polling_work(egui_core_t *core)
{
    egui_snode_t *p_head;
    egui_snode_t *p_next;
    egui_animation_t *tmp;
    uint32_t anim_work_timestamp = egui_api_timer_get_current_core(core);

    if (!egui_slist_is_empty(&core->scene.anims))
    {
        // The SAFE traversal variant allows animation callbacks to stop/remove themselves during update.
        EGUI_SLIST_FOR_EACH_NODE_SAFE(&core->scene.anims, p_head, p_next)
        {
            tmp = EGUI_SLIST_ENTRY(p_head, egui_animation_t, node);
            tmp->api->update(tmp, anim_work_timestamp);
        }
    }
}
