#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

// 4 dividers: XS / S / M / L
static egui_view_divider_t divider_xs;
static egui_view_divider_t divider_s;
static egui_view_divider_t divider_m;
static egui_view_divider_t divider_l;

// Grid container
static egui_view_gridlayout_t grid;

// View params
EGUI_VIEW_GRIDLAYOUT_PARAMS_INIT(grid_params, 0, 0, 230, 300, 1, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);

// Size XS (96x2)
EGUI_VIEW_DIVIDER_PARAMS_INIT(divider_xs_params, 0, 0, 96, 2, EGUI_THEME_BORDER);
// Size S (132x2)
EGUI_VIEW_DIVIDER_PARAMS_INIT(divider_s_params, 0, 0, 132, 2, EGUI_THEME_TRACK_BG);
// Size M (168x3)
EGUI_VIEW_DIVIDER_PARAMS_INIT(divider_m_params, 0, 0, 168, 3, EGUI_THEME_PRIMARY);
// Size L (204x3)
EGUI_VIEW_DIVIDER_PARAMS_INIT(divider_l_params, 0, 0, 204, 3, EGUI_THEME_PRIMARY_DARK);

void test_init_ui(void)
{
    // Init grid
    egui_view_gridlayout_init_with_params(EGUI_VIEW_OF(&grid), &grid_params);

    // Init all dividers
    egui_view_divider_init_with_params(EGUI_VIEW_OF(&divider_xs), &divider_xs_params);
    egui_view_divider_init_with_params(EGUI_VIEW_OF(&divider_s), &divider_s_params);
    egui_view_divider_init_with_params(EGUI_VIEW_OF(&divider_m), &divider_m_params);
    egui_view_divider_init_with_params(EGUI_VIEW_OF(&divider_l), &divider_l_params);

    // Set margins
    egui_view_set_margin_all(EGUI_VIEW_OF(&divider_xs), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&divider_s), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&divider_m), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&divider_l), 6);

    // Add children to grid
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&divider_xs));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&divider_s));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&divider_m));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&divider_l));

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
