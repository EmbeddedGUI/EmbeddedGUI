#include "egui.h"
#include <stdlib.h>
#include "uicode_disp0.h"

// 4 progress bars: XS / S / M / L
static egui_view_progress_bar_t progress_xs;
static egui_view_progress_bar_t progress_s;
static egui_view_progress_bar_t progress_m;
static egui_view_progress_bar_t progress_l;

// Grid container
static egui_view_gridlayout_t grid;

// View params
EGUI_VIEW_GRIDLAYOUT_PARAMS_INIT(grid_params, 0, 0, 230, 300, 1, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);

// Size XS (80x12)
EGUI_VIEW_PROGRESS_BAR_PARAMS_INIT(progress_xs_params, 0, 0, 180, 14, 25);
// Size S (100x14)
EGUI_VIEW_PROGRESS_BAR_PARAMS_INIT(progress_s_params, 0, 0, 180, 16, 50);
// Size M (140x16)
EGUI_VIEW_PROGRESS_BAR_PARAMS_INIT(progress_m_params, 0, 0, 180, 20, 75);
// Size L (200x20)
EGUI_VIEW_PROGRESS_BAR_PARAMS_INIT(progress_l_params, 0, 0, 180, 24, 100);

void test_init_ui(egui_core_t *core)
{
    // Init grid
    egui_view_gridlayout_init_with_params(EGUI_VIEW_OF(&grid), core, &grid_params);

    // Init all views
    egui_view_progress_bar_init_with_params(EGUI_VIEW_OF(&progress_xs), core, &progress_xs_params);
    egui_view_progress_bar_init_with_params(EGUI_VIEW_OF(&progress_s), core, &progress_s_params);
    egui_view_progress_bar_init_with_params(EGUI_VIEW_OF(&progress_m), core, &progress_m_params);
    egui_view_progress_bar_init_with_params(EGUI_VIEW_OF(&progress_l), core, &progress_l_params);

    // Set margins
    egui_view_set_margin_all(EGUI_VIEW_OF(&progress_xs), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&progress_s), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&progress_m), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&progress_l), 6);

    // Add children to grid
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&progress_xs));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&progress_s));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&progress_m));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&progress_l));

    // Layout grid children
    egui_view_gridlayout_layout_childs(EGUI_VIEW_OF(&grid));

    // Add grid to root
    egui_core_add_user_root_view(EGUI_VIEW_OF(&grid));

    // Center grid on screen
    egui_view_layout_user_root(EGUI_VIEW_OF(&grid), EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
}

#if EGUI_CONFIG_FUNCTION_RECORDING_TEST
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
