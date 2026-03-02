#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

// views in root
static egui_view_linearlayout_t layout_1;

// views in layout_1
static egui_view_label_t label_1;
static egui_view_label_t label_2;

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_1_param_normal, EGUI_THEME_SURFACE, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_1_params, &bg_1_param_normal, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_1, &bg_1_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_2_param_normal, EGUI_THEME_SURFACE_VARIANT, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_2_params, &bg_2_param_normal, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_2, &bg_2_params);

#define LABEL_WIDTH  EGUI_CONFIG_SCEEN_WIDTH
#define LABEL_HEIGHT (EGUI_CONFIG_SCEEN_HEIGHT / 2)

// View params
EGUI_VIEW_LINEARLAYOUT_PARAMS_INIT(layout_1_params, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT, EGUI_ALIGN_CENTER);
EGUI_VIEW_LABEL_PARAMS_INIT(label_1_params, 0, 0, LABEL_WIDTH, LABEL_HEIGHT, "Item1", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_PRIMARY, EGUI_ALPHA_100);
EGUI_VIEW_LABEL_PARAMS_INIT(label_2_params, 0, 0, LABEL_WIDTH, LABEL_HEIGHT, "Item2", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_PRIMARY, EGUI_ALPHA_100);

void test_init_ui(void)
{
    // Init all views
    // layout_1
    egui_view_linearlayout_init_with_params(EGUI_VIEW_OF(&layout_1), &layout_1_params);

    // label_1
    egui_view_label_init_with_params(EGUI_VIEW_OF(&label_1), &label_1_params);

    // label_2
    egui_view_label_init_with_params(EGUI_VIEW_OF(&label_2), &label_2_params);

    // background
    egui_view_set_background(EGUI_VIEW_OF(&label_1), EGUI_BG_OF(&bg_1));
    egui_view_set_background(EGUI_VIEW_OF(&label_2), EGUI_BG_OF(&bg_2));

    // Add childs to layout_1
    egui_view_group_add_child(EGUI_VIEW_OF(&layout_1), EGUI_VIEW_OF(&label_1));
    egui_view_group_add_child(EGUI_VIEW_OF(&layout_1), EGUI_VIEW_OF(&label_2));

    // Re-layout childs
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&layout_1));

    // Add To Root
    egui_core_add_user_root_view(EGUI_VIEW_OF(&layout_1));
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    if (action_index >= 1)
    {
        return false;
    }
    EGUI_SIM_SET_WAIT(p_action, 1500);
    return true;
}
#endif
