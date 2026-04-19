#include "egui_api.h"
#include "egui_common.h"
#include "egui_core.h"
#include "egui_core_internal.h"
#include "anim/egui_animation.h"
#include "app/egui_activity.h"
#include "widget/egui_view.h"

static void on_activity_anim_start_open_end(egui_animation_t *self)
{
    egui_core_t *core = egui_view_get_core(self->target_view);
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("on_activity_anim_start_open_end\n");
#endif
    if (core == NULL)
    {
        return;
    }

    if (core->scene.activity_open)
    {
        core->scene.activity_open->api->on_resume(core->scene.activity_open);
    }
}

static const egui_animation_handle_t activity_anim_start_open_hanlde = {
        .start = NULL,
        .end = on_activity_anim_start_open_end,
        .repeat = NULL,
};

static void on_activity_anim_start_close_end(egui_animation_t *self)
{
    egui_core_t *core = egui_view_get_core(self->target_view);
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("on_activity_anim_start_close_end\n");
#endif
    if (core == NULL)
    {
        return;
    }

    if (core->scene.activity_close)
    {
        core->scene.activity_close->api->on_stop(core->scene.activity_close);
    }
}

static const egui_animation_handle_t activity_anim_start_close_hanlde = {
        .start = NULL,
        .end = on_activity_anim_start_close_end,
        .repeat = NULL,
};

static void on_activity_anim_finish_open_end(egui_animation_t *self)
{
    egui_core_t *core = egui_view_get_core(self->target_view);
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("on_activity_anim_finish_open_end\n");
#endif
    if (core == NULL)
    {
        return;
    }

    if (core->scene.activity_open)
    {
        core->scene.activity_open->api->on_resume(core->scene.activity_open);
    }
}

static const egui_animation_handle_t activity_anim_finish_open_hanlde = {
        .start = NULL,
        .end = on_activity_anim_finish_open_end,
        .repeat = NULL,
};

static void on_activity_anim_finish_close_end(egui_animation_t *self)
{
    egui_core_t *core = egui_view_get_core(self->target_view);
#if EGUI_CONFIG_DEBUG_CLASS_NAME
    EGUI_LOG_DBG("on_activity_anim_finish_close_end\n");
#endif
    if (core == NULL)
    {
        return;
    }

    if (core->scene.activity_close)
    {
        core->scene.activity_close->api->on_stop(core->scene.activity_close);
    }
}

static const egui_animation_handle_t activity_anim_finish_close_hanlde = {
        .start = NULL,
        .end = on_activity_anim_finish_close_end,
        .repeat = NULL,
};

void egui_core_activity_set_start_anim(egui_core_t *core, egui_animation_t *open_anim, egui_animation_t *close_anim)
{
    core->scene.activity_anim_start_open = open_anim;
    core->scene.activity_anim_start_close = close_anim;

    if (open_anim)
    {
        egui_animation_handle_set(open_anim, &activity_anim_start_open_hanlde);
    }
    if (close_anim)
    {
        egui_animation_handle_set(close_anim, &activity_anim_start_close_hanlde);
    }
}

void egui_core_activity_set_finish_anim(egui_core_t *core, egui_animation_t *open_anim, egui_animation_t *close_anim)
{
    core->scene.activity_anim_finish_open = open_anim;
    core->scene.activity_anim_finish_close = close_anim;

    if (open_anim)
    {
        egui_animation_handle_set(open_anim, &activity_anim_finish_open_hanlde);
    }
    if (close_anim)
    {
        egui_animation_handle_set(close_anim, &activity_anim_finish_close_hanlde);
    }
}
