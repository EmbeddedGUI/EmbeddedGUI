#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

// views in root
static egui_view_scroll_t scroll_1;

// views in scroll_1
static egui_view_label_t label_1;
static egui_view_label_t label_2;
static egui_view_label_t label_3;

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_1_param_normal, EGUI_THEME_SURFACE, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_1_params, &bg_1_param_normal, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_1, &bg_1_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_2_param_normal, EGUI_THEME_SURFACE_VARIANT, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_2_params, &bg_2_param_normal, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_2, &bg_2_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_3_param_normal, EGUI_THEME_TRACK_BG, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_3_params, &bg_3_param_normal, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_3, &bg_3_params);

#define LABEL_WIDTH  EGUI_CONFIG_SCEEN_WIDTH
#define LABEL_HEIGHT (EGUI_CONFIG_SCEEN_HEIGHT / 2)

// View params
EGUI_VIEW_SCROLL_PARAMS_INIT(scroll_1_params, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
EGUI_VIEW_LABEL_PARAMS_INIT(label_1_params, 0, 0, LABEL_WIDTH, LABEL_HEIGHT, "Item1", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_PRIMARY, EGUI_ALPHA_100);
EGUI_VIEW_LABEL_PARAMS_INIT(label_2_params, 0, 0, LABEL_WIDTH, LABEL_HEIGHT, "Item2", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_PRIMARY, EGUI_ALPHA_100);
EGUI_VIEW_LABEL_PARAMS_INIT(label_3_params, 0, 0, LABEL_WIDTH, LABEL_HEIGHT, "Item3", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_PRIMARY, EGUI_ALPHA_100);

void test_init_ui(void)
{
    // Init all views
    // scroll_1
    egui_view_scroll_init_with_params(EGUI_VIEW_OF(&scroll_1), &scroll_1_params);
    egui_view_scroll_set_scrollbar_enabled(EGUI_VIEW_OF(&scroll_1), 1);

    // label_1
    egui_view_label_init_with_params(EGUI_VIEW_OF(&label_1), &label_1_params);

    // label_2
    egui_view_label_init_with_params(EGUI_VIEW_OF(&label_2), &label_2_params);

    // label_3
    egui_view_label_init_with_params(EGUI_VIEW_OF(&label_3), &label_3_params);

    // background
    egui_view_set_background(EGUI_VIEW_OF(&label_1), EGUI_BG_OF(&bg_1));
    egui_view_set_background(EGUI_VIEW_OF(&label_2), EGUI_BG_OF(&bg_2));
    egui_view_set_background(EGUI_VIEW_OF(&label_3), EGUI_BG_OF(&bg_3));

    // Add childs to scroll_1
    egui_view_scroll_add_child(EGUI_VIEW_OF(&scroll_1), EGUI_VIEW_OF(&label_1));
    egui_view_scroll_add_child(EGUI_VIEW_OF(&scroll_1), EGUI_VIEW_OF(&label_2));
    egui_view_scroll_add_child(EGUI_VIEW_OF(&scroll_1), EGUI_VIEW_OF(&label_3));

    // Re-layout childs
    egui_view_scroll_layout_childs(EGUI_VIEW_OF(&scroll_1));

    // Add To Root
    egui_core_add_user_root_view(EGUI_VIEW_OF(&scroll_1));
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    switch (action_index)
    {
    case 0: // swipe up to scroll down
        p_action->type = EGUI_SIM_ACTION_SWIPE;
        p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y1 = EGUI_CONFIG_SCEEN_HEIGHT * 3 / 4;
        p_action->x2 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y2 = EGUI_CONFIG_SCEEN_HEIGHT / 4;
        p_action->steps = 5;
        p_action->interval_ms = 800;
        return true;
    case 1: // swipe down to scroll back
        p_action->type = EGUI_SIM_ACTION_SWIPE;
        p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y1 = EGUI_CONFIG_SCEEN_HEIGHT / 4;
        p_action->x2 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y2 = EGUI_CONFIG_SCEEN_HEIGHT * 3 / 4;
        p_action->steps = 5;
        p_action->interval_ms = 800;
        return true;
    default:
        return false;
    }
}
#endif
