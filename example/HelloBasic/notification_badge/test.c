#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

// 4 badges: hidden(0), single digit(5), double digit(42), overflow(150 > 99+)
static egui_view_notification_badge_t badge_1;
static egui_view_notification_badge_t badge_2;
static egui_view_notification_badge_t badge_3;
static egui_view_notification_badge_t badge_4;

// Grid container
static egui_view_gridlayout_t grid;

// View params
EGUI_VIEW_GRIDLAYOUT_PARAMS_INIT(grid_params, 0, 0, 240, 300, 1, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);

EGUI_VIEW_NOTIFICATION_BADGE_PARAMS_INIT(badge_1_params, 0, 0, 34, 28, 0);
EGUI_VIEW_NOTIFICATION_BADGE_PARAMS_INIT(badge_2_params, 0, 0, 34, 28, 5);
EGUI_VIEW_NOTIFICATION_BADGE_PARAMS_INIT(badge_3_params, 0, 0, 44, 30, 42);
EGUI_VIEW_NOTIFICATION_BADGE_PARAMS_INIT(badge_4_params, 0, 0, 56, 30, 150);

void test_init_ui(void)
{
    // Init grid
    egui_view_gridlayout_init_with_params(EGUI_VIEW_OF(&grid), &grid_params);

    // Init all badges
    egui_view_notification_badge_init_with_params(EGUI_VIEW_OF(&badge_1), &badge_1_params);
    egui_view_notification_badge_init_with_params(EGUI_VIEW_OF(&badge_2), &badge_2_params);
    egui_view_notification_badge_init_with_params(EGUI_VIEW_OF(&badge_3), &badge_3_params);
    egui_view_notification_badge_init_with_params(EGUI_VIEW_OF(&badge_4), &badge_4_params);

    // Set margins
    egui_view_set_margin_all(EGUI_VIEW_OF(&badge_1), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&badge_2), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&badge_3), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&badge_4), 6);

    // Add children to grid
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&badge_1));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&badge_2));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&badge_3));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&badge_4));

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
