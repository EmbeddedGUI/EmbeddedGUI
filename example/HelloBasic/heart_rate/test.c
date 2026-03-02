#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

// 2 heart rate displays
static egui_view_heart_rate_t hr_1;
static egui_view_heart_rate_t hr_2;

// Grid container
static egui_view_gridlayout_t grid;

// View params
EGUI_VIEW_GRIDLAYOUT_PARAMS_INIT(grid_params, 0, 0, 240, 300, 2, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);

EGUI_VIEW_HEART_RATE_PARAMS_INIT(hr_1_params, 0, 0, 108, 124, 72);
EGUI_VIEW_HEART_RATE_PARAMS_INIT(hr_2_params, 0, 0, 108, 124, 88);

void test_init_ui(void)
{
    // Init grid
    egui_view_gridlayout_init_with_params(EGUI_VIEW_OF(&grid), &grid_params);

    // Init heart rate displays
    egui_view_heart_rate_init_with_params(EGUI_VIEW_OF(&hr_1), &hr_1_params);
    egui_view_heart_rate_init_with_params(EGUI_VIEW_OF(&hr_2), &hr_2_params);
    egui_view_heart_rate_set_animate(EGUI_VIEW_OF(&hr_1), 1);
    egui_view_heart_rate_set_animate(EGUI_VIEW_OF(&hr_2), 1);
    egui_view_heart_rate_set_pulse_phase(EGUI_VIEW_OF(&hr_1), 80);
    egui_view_heart_rate_set_pulse_phase(EGUI_VIEW_OF(&hr_2), 160);
    egui_view_heart_rate_set_heart_color(EGUI_VIEW_OF(&hr_1), EGUI_THEME_DANGER);
    egui_view_heart_rate_set_heart_color(EGUI_VIEW_OF(&hr_2), EGUI_THEME_PRIMARY);

    // Set margins
    egui_view_set_margin_all(EGUI_VIEW_OF(&hr_1), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&hr_2), 6);

    // Add children to grid
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&hr_1));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&hr_2));

    // Layout grid children
    egui_view_gridlayout_layout_childs(EGUI_VIEW_OF(&grid));

    // Add grid to root
    egui_core_add_user_root_view(EGUI_VIEW_OF(&grid));

    // Center grid on screen
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
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
