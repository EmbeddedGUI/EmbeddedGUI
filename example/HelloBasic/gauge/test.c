#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

// 4 gauges: XS / S / M / L
static egui_view_gauge_t gauge_xs;
static egui_view_gauge_t gauge_s;
static egui_view_gauge_t gauge_m;
static egui_view_gauge_t gauge_l;

// Grid container
static egui_view_gridlayout_t grid;

// View params
EGUI_VIEW_GRIDLAYOUT_PARAMS_INIT(grid_params, 0, 0, 230, 300, 2, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);

// Size XS (64x64)
EGUI_VIEW_GAUGE_PARAMS_INIT(gauge_xs_params, 0, 0, 64, 64, 25);
// Size S (80x80)
EGUI_VIEW_GAUGE_PARAMS_INIT(gauge_s_params, 0, 0, 80, 80, 50);
// Size M (96x96)
EGUI_VIEW_GAUGE_PARAMS_INIT(gauge_m_params, 0, 0, 96, 96, 75);
// Size L (112x112)
EGUI_VIEW_GAUGE_PARAMS_INIT(gauge_l_params, 0, 0, 112, 112, 90);

void test_init_ui(void)
{
    // Init grid
    egui_view_gridlayout_init_with_params(EGUI_VIEW_OF(&grid), &grid_params);

    // Init all gauges
    egui_view_gauge_init_with_params(EGUI_VIEW_OF(&gauge_xs), &gauge_xs_params);
    egui_view_gauge_init_with_params(EGUI_VIEW_OF(&gauge_s), &gauge_s_params);
    egui_view_gauge_init_with_params(EGUI_VIEW_OF(&gauge_m), &gauge_m_params);
    egui_view_gauge_init_with_params(EGUI_VIEW_OF(&gauge_l), &gauge_l_params);

    // Set text color for better visibility
    egui_view_gauge_set_text_color(EGUI_VIEW_OF(&gauge_xs), EGUI_COLOR_WHITE);
    egui_view_gauge_set_text_color(EGUI_VIEW_OF(&gauge_s), EGUI_COLOR_WHITE);
    egui_view_gauge_set_text_color(EGUI_VIEW_OF(&gauge_m), EGUI_COLOR_WHITE);
    egui_view_gauge_set_text_color(EGUI_VIEW_OF(&gauge_l), EGUI_COLOR_WHITE);

    // Set margins
    egui_view_set_margin_all(EGUI_VIEW_OF(&gauge_xs), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&gauge_s), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&gauge_m), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&gauge_l), 6);

    // Add children to grid
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&gauge_xs));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&gauge_s));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&gauge_m));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&gauge_l));

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
