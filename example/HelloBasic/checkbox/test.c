#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

// 4 checkboxes: XS / S / M / L
static egui_view_checkbox_t checkbox_xs;
static egui_view_checkbox_t checkbox_s;
static egui_view_checkbox_t checkbox_m;
static egui_view_checkbox_t checkbox_l;

// Grid container
static egui_view_gridlayout_t grid;

// View params
EGUI_VIEW_GRIDLAYOUT_PARAMS_INIT(grid_params, 0, 0, 230, 300, 1, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);

// Size XS (160x26)
EGUI_VIEW_CHECKBOX_PARAMS_INIT_WITH_TEXT(checkbox_xs_params, 0, 0, 160, 26, 0, "Checkbox XS");
// Size S (170x34)
EGUI_VIEW_CHECKBOX_PARAMS_INIT_WITH_TEXT(checkbox_s_params, 0, 0, 170, 34, 1, "Checkbox S");
// Size M (180x42)
EGUI_VIEW_CHECKBOX_PARAMS_INIT_WITH_TEXT(checkbox_m_params, 0, 0, 180, 42, 0, "Checkbox M");
// Size L (190x52)
EGUI_VIEW_CHECKBOX_PARAMS_INIT_WITH_TEXT(checkbox_l_params, 0, 0, 190, 52, 1, "Checkbox L");

static void checkbox_checked_cb(egui_view_t *self, int is_checked)
{
    EGUI_LOG_INF("Checkbox checked: %d\n", is_checked);
}

void test_init_ui(void)
{
    // Init grid
    egui_view_gridlayout_init_with_params(EGUI_VIEW_OF(&grid), &grid_params);

    // Init all views
    egui_view_checkbox_init_with_params(EGUI_VIEW_OF(&checkbox_xs), &checkbox_xs_params);
    egui_view_checkbox_set_on_checked_listener(EGUI_VIEW_OF(&checkbox_xs), checkbox_checked_cb);

    egui_view_checkbox_init_with_params(EGUI_VIEW_OF(&checkbox_s), &checkbox_s_params);
    egui_view_checkbox_set_on_checked_listener(EGUI_VIEW_OF(&checkbox_s), checkbox_checked_cb);

    egui_view_checkbox_init_with_params(EGUI_VIEW_OF(&checkbox_m), &checkbox_m_params);
    egui_view_checkbox_set_on_checked_listener(EGUI_VIEW_OF(&checkbox_m), checkbox_checked_cb);

    egui_view_checkbox_init_with_params(EGUI_VIEW_OF(&checkbox_l), &checkbox_l_params);
    egui_view_checkbox_set_on_checked_listener(EGUI_VIEW_OF(&checkbox_l), checkbox_checked_cb);

    // Set margins
    egui_view_set_margin_all(EGUI_VIEW_OF(&checkbox_xs), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&checkbox_s), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&checkbox_m), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&checkbox_l), 6);

    // Add children to grid
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&checkbox_xs));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&checkbox_s));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&checkbox_m));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&checkbox_l));

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
        EGUI_SIM_SET_CLICK_VIEW(p_action, &checkbox_xs, 1000);
        return true;
    case 1:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &checkbox_s, 1000);
        return true;
    case 2:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &checkbox_m, 1000);
        return true;
    case 3:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &checkbox_l, 1000);
        return true;
    default:
        return false;
    }
}
#endif
