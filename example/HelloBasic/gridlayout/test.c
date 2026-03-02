#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

// views in root
static egui_view_gridlayout_t grid;
static egui_view_switch_t sw_1;
static egui_view_switch_t sw_2;
static egui_view_switch_t sw_3;
static egui_view_switch_t sw_4;

#define GRID_WIDTH  200
#define GRID_HEIGHT 200
#define ITEM_WIDTH  60
#define ITEM_HEIGHT 30

// Grid params - 2 columns, center aligned
EGUI_VIEW_GRIDLAYOUT_PARAMS_INIT(grid_params, 0, 0, GRID_WIDTH, GRID_HEIGHT, 2, EGUI_ALIGN_CENTER);

// Item params
EGUI_VIEW_SWITCH_PARAMS_INIT(sw_1_params, 0, 0, ITEM_WIDTH, ITEM_HEIGHT, 1);
EGUI_VIEW_SWITCH_PARAMS_INIT(sw_2_params, 0, 0, ITEM_WIDTH, ITEM_HEIGHT, 0);
EGUI_VIEW_SWITCH_PARAMS_INIT(sw_3_params, 0, 0, ITEM_WIDTH, ITEM_HEIGHT, 0);
EGUI_VIEW_SWITCH_PARAMS_INIT(sw_4_params, 0, 0, ITEM_WIDTH, ITEM_HEIGHT, 1);

void test_init_ui(void)
{
    // Init grid layout
    egui_view_gridlayout_init_with_params(EGUI_VIEW_OF(&grid), &grid_params);

    // Init child items
    egui_view_switch_init_with_params(EGUI_VIEW_OF(&sw_1), &sw_1_params);
    egui_view_switch_init_with_params(EGUI_VIEW_OF(&sw_2), &sw_2_params);
    egui_view_switch_init_with_params(EGUI_VIEW_OF(&sw_3), &sw_3_params);
    egui_view_switch_init_with_params(EGUI_VIEW_OF(&sw_4), &sw_4_params);

    // Set margins for spacing
    egui_view_set_margin(EGUI_VIEW_OF(&sw_1), 6, 6, 6, 6);
    egui_view_set_margin(EGUI_VIEW_OF(&sw_2), 6, 6, 6, 6);
    egui_view_set_margin(EGUI_VIEW_OF(&sw_3), 6, 6, 6, 6);
    egui_view_set_margin(EGUI_VIEW_OF(&sw_4), 6, 6, 6, 6);

    // Add children to grid
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&sw_1));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&sw_2));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&sw_3));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&sw_4));

    // Layout children in grid
    egui_view_gridlayout_layout_childs(EGUI_VIEW_OF(&grid));

    // Add grid To Root
    egui_core_add_user_root_view(EGUI_VIEW_OF(&grid));

    // Layout childrens of root view
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_CENTER);
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    switch (action_index)
    {
    case 0:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &sw_1, 1000);
        return true;
    case 1:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &sw_2, 1000);
        return true;
    case 2:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &sw_3, 1000);
        return true;
    case 3:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &sw_4, 1000);
        return true;
    default:
        return false;
    }
}
#endif
