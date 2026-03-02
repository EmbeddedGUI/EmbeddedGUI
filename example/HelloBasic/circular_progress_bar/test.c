#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

// 4 circular progress bars: XS / S / M / L
static egui_view_circular_progress_bar_t cpb_xs;
static egui_view_circular_progress_bar_t cpb_s;
static egui_view_circular_progress_bar_t cpb_m;
static egui_view_circular_progress_bar_t cpb_l;

// Grid container
static egui_view_gridlayout_t grid;

// View params
EGUI_VIEW_GRIDLAYOUT_PARAMS_INIT(grid_params, 0, 0, 230, 300, 2, EGUI_ALIGN_CENTER);

// Size XS (64x64), stroke 2
EGUI_VIEW_CIRCULAR_PROGRESS_BAR_PARAMS_INIT(cpb_xs_params, 0, 0, 64, 64, 25);
// Size S (80x80), stroke 4
EGUI_VIEW_CIRCULAR_PROGRESS_BAR_PARAMS_INIT(cpb_s_params, 0, 0, 80, 80, 50);
// Size M (96x96), stroke 6
EGUI_VIEW_CIRCULAR_PROGRESS_BAR_PARAMS_INIT(cpb_m_params, 0, 0, 96, 96, 75);
// Size L (112x112), stroke 8
EGUI_VIEW_CIRCULAR_PROGRESS_BAR_PARAMS_INIT(cpb_l_params, 0, 0, 112, 112, 100);

static void progress_changed_cb(egui_view_t *self, uint8_t progress)
{
    EGUI_LOG_INF("Circular Progress: %d\n", progress);
}

void test_init_ui(void)
{
    // Init grid
    egui_view_gridlayout_init_with_params(EGUI_VIEW_OF(&grid), &grid_params);

    // Init all circular progress bars
    egui_view_circular_progress_bar_init_with_params(EGUI_VIEW_OF(&cpb_xs), &cpb_xs_params);
    egui_view_circular_progress_bar_set_process(EGUI_VIEW_OF(&cpb_xs), 25);
    egui_view_circular_progress_bar_set_bk_color(EGUI_VIEW_OF(&cpb_xs), EGUI_THEME_TRACK_BG);
    egui_view_circular_progress_bar_set_progress_color(EGUI_VIEW_OF(&cpb_xs), EGUI_THEME_PRIMARY_DARK);
    egui_view_circular_progress_bar_set_stroke_width(EGUI_VIEW_OF(&cpb_xs), 2);
    egui_view_circular_progress_bar_set_on_progress_listener(EGUI_VIEW_OF(&cpb_xs), progress_changed_cb);

    egui_view_circular_progress_bar_init_with_params(EGUI_VIEW_OF(&cpb_s), &cpb_s_params);
    egui_view_circular_progress_bar_set_process(EGUI_VIEW_OF(&cpb_s), 50);
    egui_view_circular_progress_bar_set_bk_color(EGUI_VIEW_OF(&cpb_s), EGUI_THEME_TRACK_BG);
    egui_view_circular_progress_bar_set_progress_color(EGUI_VIEW_OF(&cpb_s), EGUI_THEME_PRIMARY);
    egui_view_circular_progress_bar_set_stroke_width(EGUI_VIEW_OF(&cpb_s), 4);
    egui_view_circular_progress_bar_set_on_progress_listener(EGUI_VIEW_OF(&cpb_s), progress_changed_cb);

    egui_view_circular_progress_bar_init_with_params(EGUI_VIEW_OF(&cpb_m), &cpb_m_params);
    egui_view_circular_progress_bar_set_process(EGUI_VIEW_OF(&cpb_m), 75);
    egui_view_circular_progress_bar_set_bk_color(EGUI_VIEW_OF(&cpb_m), EGUI_THEME_TRACK_BG);
    egui_view_circular_progress_bar_set_progress_color(EGUI_VIEW_OF(&cpb_m), EGUI_THEME_SECONDARY);
    egui_view_circular_progress_bar_set_stroke_width(EGUI_VIEW_OF(&cpb_m), 6);
    egui_view_circular_progress_bar_set_on_progress_listener(EGUI_VIEW_OF(&cpb_m), progress_changed_cb);

    egui_view_circular_progress_bar_init_with_params(EGUI_VIEW_OF(&cpb_l), &cpb_l_params);
    egui_view_circular_progress_bar_set_process(EGUI_VIEW_OF(&cpb_l), 100);
    egui_view_circular_progress_bar_set_bk_color(EGUI_VIEW_OF(&cpb_l), EGUI_THEME_TRACK_BG);
    egui_view_circular_progress_bar_set_progress_color(EGUI_VIEW_OF(&cpb_l), EGUI_THEME_DANGER);
    egui_view_circular_progress_bar_set_stroke_width(EGUI_VIEW_OF(&cpb_l), 8);
    egui_view_circular_progress_bar_set_on_progress_listener(EGUI_VIEW_OF(&cpb_l), progress_changed_cb);

    // Set margins
    egui_view_set_margin_all(EGUI_VIEW_OF(&cpb_xs), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&cpb_s), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&cpb_m), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&cpb_l), 6);

    // Add children to grid
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&cpb_xs));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&cpb_s));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&cpb_m));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&cpb_l));

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
