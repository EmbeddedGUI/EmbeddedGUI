#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

// 4 toggle buttons: XS / S / M / L
static egui_view_toggle_button_t toggle_xs;
static egui_view_toggle_button_t toggle_s;
static egui_view_toggle_button_t toggle_m;
static egui_view_toggle_button_t toggle_l;

// Grid container
static egui_view_gridlayout_t grid;

// View params
EGUI_VIEW_GRIDLAYOUT_PARAMS_INIT(grid_params, 0, 0, 220, 300, 1, EGUI_ALIGN_CENTER);

// Size XS (96x30)
EGUI_VIEW_TOGGLE_BUTTON_PARAMS_INIT(toggle_xs_params, 0, 0, 96, 30, "XS", 0);
// Size S (120x34)
EGUI_VIEW_TOGGLE_BUTTON_PARAMS_INIT(toggle_s_params, 0, 0, 120, 34, "S", 1);
// Size M (160x38)
EGUI_VIEW_TOGGLE_BUTTON_PARAMS_INIT(toggle_m_params, 0, 0, 160, 38, "M", 0);
// Size L (200x44)
EGUI_VIEW_TOGGLE_BUTTON_PARAMS_INIT(toggle_l_params, 0, 0, 200, 44, "L", 1);

static void on_toggled_cb(egui_view_t *self, uint8_t is_toggled)
{
    EGUI_LOG_INF("Toggle toggled: %d\r\n", is_toggled);
}

void test_init_ui(void)
{
    // Init grid
    egui_view_gridlayout_init_with_params(EGUI_VIEW_OF(&grid), &grid_params);

    // Init all toggle buttons
    egui_view_toggle_button_init_with_params(EGUI_VIEW_OF(&toggle_xs), &toggle_xs_params);
    egui_view_toggle_button_set_on_toggled_listener(EGUI_VIEW_OF(&toggle_xs), on_toggled_cb);

    egui_view_toggle_button_init_with_params(EGUI_VIEW_OF(&toggle_s), &toggle_s_params);
    egui_view_toggle_button_set_on_toggled_listener(EGUI_VIEW_OF(&toggle_s), on_toggled_cb);

    egui_view_toggle_button_init_with_params(EGUI_VIEW_OF(&toggle_m), &toggle_m_params);
    egui_view_toggle_button_set_on_toggled_listener(EGUI_VIEW_OF(&toggle_m), on_toggled_cb);

    egui_view_toggle_button_init_with_params(EGUI_VIEW_OF(&toggle_l), &toggle_l_params);
    egui_view_toggle_button_set_on_toggled_listener(EGUI_VIEW_OF(&toggle_l), on_toggled_cb);

    // Set margins
    egui_view_set_margin_all(EGUI_VIEW_OF(&toggle_xs), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&toggle_s), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&toggle_m), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&toggle_l), 6);

    // Add children to grid
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&toggle_xs));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&toggle_s));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&toggle_m));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&toggle_l));

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
        EGUI_SIM_SET_CLICK_VIEW(p_action, &toggle_xs, 1000);
        return true;
    case 1:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &toggle_s, 1000);
        return true;
    case 2:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &toggle_m, 1000);
        return true;
    case 3:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &toggle_l, 1000);
        return true;
    default:
        return false;
    }
}
#endif
