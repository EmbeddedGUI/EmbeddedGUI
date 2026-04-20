#include "egui_api.h"
#include "egui_core.h"
#include "egui_core_internal.h"
#include "anim/egui_animation.h"

void egui_core_animation_append(egui_core_t *core, egui_animation_t *anim)
{
    egui_slist_append(&core->scene.anims, &anim->node);
}

void egui_core_animation_remove(egui_core_t *core, egui_animation_t *anim)
{
    egui_slist_find_and_remove(&core->scene.anims, &anim->node);
}

void egui_core_animation_polling_work(egui_core_t *core)
{
    egui_snode_t *p_head;
    egui_snode_t *p_next;
    egui_animation_t *tmp;
    uint32_t anim_work_timestamp = egui_api_timer_get_current_core(core);

    if (!egui_slist_is_empty(&core->scene.anims))
    {
        EGUI_SLIST_FOR_EACH_NODE_SAFE(&core->scene.anims, p_head, p_next)
        {
            tmp = EGUI_SLIST_ENTRY(p_head, egui_animation_t, node);
            tmp->api->update(tmp, anim_work_timestamp);
        }
    }
}
