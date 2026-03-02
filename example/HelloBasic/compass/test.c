#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

// 2 compasses
static egui_view_compass_t compass_1;
static egui_view_compass_t compass_2;

// Grid container
static egui_view_gridlayout_t grid;

// View params
EGUI_VIEW_GRIDLAYOUT_PARAMS_INIT(grid_params, 0, 0, 240, 300, 2, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);

EGUI_VIEW_COMPASS_PARAMS_INIT(compass_1_params, 0, 0, 116, 116, 0);
EGUI_VIEW_COMPASS_PARAMS_INIT(compass_2_params, 0, 0, 92, 92, 135);

void test_init_ui(void)
{
    // Init grid
    egui_view_gridlayout_init_with_params(EGUI_VIEW_OF(&grid), &grid_params);

    // Init compasses
    egui_view_compass_init_with_params(EGUI_VIEW_OF(&compass_1), &compass_1_params);
    egui_view_compass_init_with_params(EGUI_VIEW_OF(&compass_2), &compass_2_params);
    egui_view_compass_set_show_degree(EGUI_VIEW_OF(&compass_2), 1);

    // Set margins
    egui_view_set_margin_all(EGUI_VIEW_OF(&compass_1), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&compass_2), 6);

    // Add children to grid
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&compass_1));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&compass_2));

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
