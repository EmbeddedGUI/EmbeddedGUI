#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

// Horizontal scale (0-100)
static egui_view_scale_t scale_h;
// Vertical scale (0-50)
static egui_view_scale_t scale_v;

EGUI_VIEW_SCALE_PARAMS_INIT(scale_h_params, 10, 10, 180, 40, 0, 100, 6);
EGUI_VIEW_SCALE_PARAMS_INIT(scale_v_params, 195, 10, 50, 200, 0, 50, 6);

void test_init_ui(void)
{
    // Init horizontal scale
    egui_view_scale_init_with_params(EGUI_VIEW_OF(&scale_h), &scale_h_params);
    egui_view_scale_set_font(EGUI_VIEW_OF(&scale_h), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_scale_set_value(EGUI_VIEW_OF(&scale_h), 60);
    egui_view_scale_show_indicator(EGUI_VIEW_OF(&scale_h), 1);

    // Init vertical scale
    egui_view_scale_init_with_params(EGUI_VIEW_OF(&scale_v), &scale_v_params);
    egui_view_scale_set_font(EGUI_VIEW_OF(&scale_v), (const egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_scale_set_orientation(EGUI_VIEW_OF(&scale_v), 0);
    egui_view_scale_set_value(EGUI_VIEW_OF(&scale_v), 30);
    egui_view_scale_show_indicator(EGUI_VIEW_OF(&scale_v), 1);

    // Add to root
    egui_core_add_user_root_view(EGUI_VIEW_OF(&scale_h));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&scale_v));
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
