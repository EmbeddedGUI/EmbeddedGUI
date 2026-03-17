#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

// 4 switches: XS / S / M / L
static egui_view_switch_t switch_xs;
static egui_view_switch_t switch_s;
static egui_view_switch_t switch_m;
static egui_view_switch_t switch_l;

// Grid container
static egui_view_gridlayout_t grid;

// View params
EGUI_VIEW_GRIDLAYOUT_PARAMS_INIT(grid_params, 0, 0, 230, 300, 1, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);

// Size XS (72x28)
EGUI_VIEW_SWITCH_PARAMS_INIT(switch_xs_params, 0, 0, 72, 28, 0);
// Size S (84x32)
EGUI_VIEW_SWITCH_PARAMS_INIT(switch_s_params, 0, 0, 84, 32, 1);
// Size M (96x38)
EGUI_VIEW_SWITCH_PARAMS_INIT(switch_m_params, 0, 0, 96, 38, 0);
// Size L (112x44)
EGUI_VIEW_SWITCH_PARAMS_INIT(switch_l_params, 0, 0, 112, 44, 1);

static void switch_checked_cb(egui_view_t *self, int is_checked)
{
    EGUI_LOG_INF("Switch checked: %d\n", is_checked);
}

void test_init_ui(void)
{
    // Init grid
    egui_view_gridlayout_init_with_params(EGUI_VIEW_OF(&grid), &grid_params);

    // Init all views
    egui_view_switch_init_with_params(EGUI_VIEW_OF(&switch_xs), &switch_xs_params);
    egui_view_switch_set_state_icons(EGUI_VIEW_OF(&switch_xs), EGUI_ICON_MS_DONE, EGUI_ICON_MS_CROSS);
    egui_view_switch_set_icon_font(EGUI_VIEW_OF(&switch_xs), EGUI_FONT_ICON_MS_20);
    egui_view_switch_set_on_checked_listener(EGUI_VIEW_OF(&switch_xs), switch_checked_cb);

    egui_view_switch_init_with_params(EGUI_VIEW_OF(&switch_s), &switch_s_params);
    egui_view_switch_set_state_icons(EGUI_VIEW_OF(&switch_s), EGUI_ICON_MS_DONE, EGUI_ICON_MS_CROSS);
    egui_view_switch_set_icon_font(EGUI_VIEW_OF(&switch_s), EGUI_FONT_ICON_MS_20);
    egui_view_switch_set_on_checked_listener(EGUI_VIEW_OF(&switch_s), switch_checked_cb);

    egui_view_switch_init_with_params(EGUI_VIEW_OF(&switch_m), &switch_m_params);
    egui_view_switch_set_state_icons(EGUI_VIEW_OF(&switch_m), EGUI_ICON_MS_DONE, EGUI_ICON_MS_CROSS);
    egui_view_switch_set_icon_font(EGUI_VIEW_OF(&switch_m), EGUI_FONT_ICON_MS_24);
    egui_view_switch_set_on_checked_listener(EGUI_VIEW_OF(&switch_m), switch_checked_cb);

    egui_view_switch_init_with_params(EGUI_VIEW_OF(&switch_l), &switch_l_params);
    egui_view_switch_set_state_icons(EGUI_VIEW_OF(&switch_l), EGUI_ICON_MS_DONE, EGUI_ICON_MS_CROSS);
    egui_view_switch_set_icon_font(EGUI_VIEW_OF(&switch_l), EGUI_FONT_ICON_MS_24);
    egui_view_switch_set_on_checked_listener(EGUI_VIEW_OF(&switch_l), switch_checked_cb);

    // Set margins
    egui_view_set_margin_all(EGUI_VIEW_OF(&switch_xs), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&switch_s), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&switch_m), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&switch_l), 6);

    // Add children to grid
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&switch_xs));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&switch_s));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&switch_m));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&switch_l));

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
    switch (action_index)
    {
    case 0:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &switch_xs, 1000);
        return true;
    case 1:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &switch_s, 1000);
        return true;
    case 2:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &switch_m, 1000);
        return true;
    case 3:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &switch_l, 1000);
        return true;
    default:
        return false;
    }
}
#endif
