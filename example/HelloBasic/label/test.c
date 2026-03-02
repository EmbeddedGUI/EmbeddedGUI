#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

// 4 labels: XS / S / M / L
static egui_view_label_t label_xs;
static egui_view_label_t label_s;
static egui_view_label_t label_m;
static egui_view_label_t label_l;

// Grid container
static egui_view_gridlayout_t grid;

// View params
EGUI_VIEW_GRIDLAYOUT_PARAMS_INIT(grid_params, 0, 0, 230, 300, 1, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);

// Size XS (80x24)
EGUI_VIEW_LABEL_PARAMS_INIT(label_xs_params, 0, 0, 200, 24, "Label XS", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_SECONDARY, EGUI_ALPHA_100);
// Size S (100x30)
EGUI_VIEW_LABEL_PARAMS_INIT(label_s_params, 0, 0, 200, 30, "Label S", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_PRIMARY, EGUI_ALPHA_100);
// Size M (140x36)
EGUI_VIEW_LABEL_PARAMS_INIT(label_m_params, 0, 0, 200, 36, "Label M", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_PRIMARY, EGUI_ALPHA_100);
// Size L (200x44)
EGUI_VIEW_LABEL_PARAMS_INIT(label_l_params, 0, 0, 200, 44, "Label L", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_PRIMARY_DARK, EGUI_ALPHA_100);

void test_init_ui(void)
{
    // Init grid
    egui_view_gridlayout_init_with_params(EGUI_VIEW_OF(&grid), &grid_params);

    // Init all views
    egui_view_label_init_with_params(EGUI_VIEW_OF(&label_xs), &label_xs_params);
    egui_view_label_init_with_params(EGUI_VIEW_OF(&label_s), &label_s_params);
    egui_view_label_init_with_params(EGUI_VIEW_OF(&label_m), &label_m_params);
    egui_view_label_init_with_params(EGUI_VIEW_OF(&label_l), &label_l_params);

    // Set margins
    egui_view_set_margin_all(EGUI_VIEW_OF(&label_xs), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&label_s), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&label_m), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&label_l), 6);

    egui_view_label_set_align_type(EGUI_VIEW_OF(&label_xs), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&label_s), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&label_m), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&label_l), EGUI_ALIGN_LEFT | EGUI_ALIGN_VCENTER);

    // Add children to grid
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&label_xs));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&label_s));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&label_m));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&label_l));

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
