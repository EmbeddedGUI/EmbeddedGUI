#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

// 4 radio buttons: XS / S / M / L
static egui_view_radio_button_t radio_xs;
static egui_view_radio_button_t radio_s;
static egui_view_radio_button_t radio_m;
static egui_view_radio_button_t radio_l;
static egui_view_radio_group_t radio_group;

// Grid container
static egui_view_gridlayout_t grid;

// View params
EGUI_VIEW_GRIDLAYOUT_PARAMS_INIT(grid_params, 0, 0, 230, 300, 1, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);

// Size XS (160x26)
EGUI_VIEW_RADIO_BUTTON_PARAMS_INIT_WITH_TEXT(radio_xs_params, 0, 0, 160, 26, 1, "Radio XS");
// Size S (170x34)
EGUI_VIEW_RADIO_BUTTON_PARAMS_INIT_WITH_TEXT(radio_s_params, 0, 0, 170, 34, 0, "Radio S");
// Size M (180x42)
EGUI_VIEW_RADIO_BUTTON_PARAMS_INIT_WITH_TEXT(radio_m_params, 0, 0, 180, 42, 0, "Radio M");
// Size L (190x52)
EGUI_VIEW_RADIO_BUTTON_PARAMS_INIT_WITH_TEXT(radio_l_params, 0, 0, 190, 52, 0, "Radio L");

static void radio_changed_cb(egui_view_t *self, int index)
{
    EGUI_LOG_INF("Radio selected index: %d\n", index);
}

void test_init_ui(void)
{
    // Init grid
    egui_view_gridlayout_init_with_params(EGUI_VIEW_OF(&grid), &grid_params);

    // Init radio group
    egui_view_radio_group_init(&radio_group);
    egui_view_radio_group_set_on_changed_listener(&radio_group, radio_changed_cb);

    // Init all radio buttons
    egui_view_radio_button_init_with_params(EGUI_VIEW_OF(&radio_xs), &radio_xs_params);
    egui_view_radio_group_add(&radio_group, EGUI_VIEW_OF(&radio_xs));

    egui_view_radio_button_init_with_params(EGUI_VIEW_OF(&radio_s), &radio_s_params);
    egui_view_radio_group_add(&radio_group, EGUI_VIEW_OF(&radio_s));

    egui_view_radio_button_init_with_params(EGUI_VIEW_OF(&radio_m), &radio_m_params);
    egui_view_radio_group_add(&radio_group, EGUI_VIEW_OF(&radio_m));

    egui_view_radio_button_init_with_params(EGUI_VIEW_OF(&radio_l), &radio_l_params);
    egui_view_radio_group_add(&radio_group, EGUI_VIEW_OF(&radio_l));

    // Set margins
    egui_view_set_margin_all(EGUI_VIEW_OF(&radio_xs), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&radio_s), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&radio_m), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&radio_l), 6);

    // Add children to grid
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&radio_xs));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&radio_s));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&radio_m));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&radio_l));

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
        EGUI_SIM_SET_CLICK_VIEW(p_action, &radio_s, 1000);
        return true;
    case 1:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &radio_m, 1000);
        return true;
    case 2:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &radio_l, 1000);
        return true;
    case 3:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &radio_xs, 1000);
        return true;
    default:
        return false;
    }
}
#endif
