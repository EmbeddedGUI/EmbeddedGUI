#include "egui.h"
#include <stdlib.h>

#include "egui_view_dirty_demo.h"
#include "uicode_disp0.h"

static egui_view_viewpage_t viewpage;
static egui_view_dirty_demo_t demo_views[EGUI_VIEW_DIRTY_DEMO_PAGE_COUNT];
static egui_animation_t frame_anim;

EGUI_VIEW_VIEWPAGE_PARAMS_INIT(viewpage_params, 0, 0, EGUI_CONFIG_SCREEN_WIDTH, EGUI_CONFIG_SCREEN_HEIGHT);

static void hello_dirty_frame_anim_on_start(egui_animation_t *self)
{
    EGUI_UNUSED(self);
}

static void hello_dirty_frame_anim_update(egui_animation_t *self, uint32_t current_time)
{
    uint8_t page;

    EGUI_UNUSED(self);

    page = viewpage.current_page_index;
    if (page < EGUI_VIEW_DIRTY_DEMO_PAGE_COUNT)
    {
        egui_view_dirty_demo_step(EGUI_VIEW_OF(&demo_views[page]), current_time);
    }
}

static void hello_dirty_frame_anim_on_update(egui_animation_t *self, egui_float_t fraction)
{
    EGUI_UNUSED(self);
    EGUI_UNUSED(fraction);
}

static const egui_animation_api_t hello_dirty_frame_anim_api = {
        .on_start = hello_dirty_frame_anim_on_start,
        .update = hello_dirty_frame_anim_update,
        .on_update = hello_dirty_frame_anim_on_update,
};

static void uicode_disp0_init_ui(egui_core_t *core)
{
    egui_view_viewpage_init_with_params(EGUI_VIEW_OF(&viewpage), core, &viewpage_params);
    egui_view_viewpage_set_scrollbar_enabled(EGUI_VIEW_OF(&viewpage), 0);

    for (uint8_t i = 0; i < EGUI_VIEW_DIRTY_DEMO_PAGE_COUNT; i++)
    {
        egui_view_dirty_demo_init(EGUI_VIEW_OF(&demo_views[i]), core, i);
        egui_view_set_size(EGUI_VIEW_OF(&demo_views[i]), EGUI_CONFIG_SCREEN_WIDTH, EGUI_CONFIG_SCREEN_HEIGHT);
        egui_view_viewpage_add_child(EGUI_VIEW_OF(&viewpage), EGUI_VIEW_OF(&demo_views[i]));
    }

    egui_view_viewpage_layout_childs(EGUI_VIEW_OF(&viewpage));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&viewpage));

    egui_animation_init(&frame_anim);
    frame_anim.api = &hello_dirty_frame_anim_api;
    egui_animation_target_view_set(&frame_anim, EGUI_VIEW_OF(&viewpage));
    egui_animation_start(&frame_anim);
}

void uicode_disp0_init(egui_core_t *core)
{
    uicode_disp0_init_ui(core);
}

#if EGUI_CONFIG_FUNCTION_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static int last_action = -1;
    int first_call = action_index != last_action;

    last_action = action_index;

    if (action_index >= EGUI_VIEW_DIRTY_DEMO_PAGE_COUNT)
    {
        return false;
    }

    if (first_call)
    {
        egui_view_viewpage_set_current_page(EGUI_VIEW_OF(&viewpage), action_index);
        egui_view_dirty_demo_reset_frame_clock(EGUI_VIEW_OF(&demo_views[action_index]));
        recording_request_snapshot();
    }

    EGUI_SIM_SET_WAIT(p_action, 800);
    return true;
}
#endif
