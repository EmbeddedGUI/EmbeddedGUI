#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

// 4 spinners: XS / S / M / L
static egui_view_spinner_t spinner_xs;
static egui_view_spinner_t spinner_s;
static egui_view_spinner_t spinner_m;
static egui_view_spinner_t spinner_l;

// Grid container
static egui_view_gridlayout_t grid;

// View params
EGUI_VIEW_GRIDLAYOUT_PARAMS_INIT(grid_params, 0, 0, 230, 300, 1, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);

// Size XS (34x34)
EGUI_VIEW_SPINNER_PARAMS_INIT(spinner_xs_params, 0, 0, 34, 34);
// Size S (46x46)
EGUI_VIEW_SPINNER_PARAMS_INIT(spinner_s_params, 0, 0, 46, 46);
// Size M (62x62)
EGUI_VIEW_SPINNER_PARAMS_INIT(spinner_m_params, 0, 0, 62, 62);
// Size L (82x82)
EGUI_VIEW_SPINNER_PARAMS_INIT(spinner_l_params, 0, 0, 82, 82);

void test_init_ui(void)
{
    // Init grid
    egui_view_gridlayout_init_with_params(EGUI_VIEW_OF(&grid), &grid_params);

    // Init all spinners
    egui_view_spinner_init_with_params(EGUI_VIEW_OF(&spinner_xs), &spinner_xs_params);
    egui_view_spinner_start(EGUI_VIEW_OF(&spinner_xs));

    egui_view_spinner_init_with_params(EGUI_VIEW_OF(&spinner_s), &spinner_s_params);
    egui_view_spinner_start(EGUI_VIEW_OF(&spinner_s));

    egui_view_spinner_init_with_params(EGUI_VIEW_OF(&spinner_m), &spinner_m_params);
    egui_view_spinner_start(EGUI_VIEW_OF(&spinner_m));

    egui_view_spinner_init_with_params(EGUI_VIEW_OF(&spinner_l), &spinner_l_params);
    egui_view_spinner_start(EGUI_VIEW_OF(&spinner_l));

    // Set margins
    egui_view_set_margin_all(EGUI_VIEW_OF(&spinner_xs), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&spinner_s), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&spinner_m), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&spinner_l), 6);

    // Add children to grid
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&spinner_xs));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&spinner_s));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&spinner_m));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&spinner_l));

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
