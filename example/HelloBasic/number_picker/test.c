#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

// 4 number pickers: XS / S / M / L
static egui_view_number_picker_t picker_xs;
static egui_view_number_picker_t picker_s;
static egui_view_number_picker_t picker_m;
static egui_view_number_picker_t picker_l;

// Grid container
static egui_view_gridlayout_t grid;

// View params
EGUI_VIEW_GRIDLAYOUT_PARAMS_INIT(grid_params, 0, 0, 230, 300, 2, EGUI_ALIGN_CENTER);

// Size XS (40x60)
EGUI_VIEW_NUMBER_PICKER_PARAMS_INIT(picker_xs_params, 0, 0, 40, 60, 10, 0, 100);
// Size S (50x80)
EGUI_VIEW_NUMBER_PICKER_PARAMS_INIT(picker_s_params, 0, 0, 50, 80, 30, 0, 100);
// Size M (60x100)
EGUI_VIEW_NUMBER_PICKER_PARAMS_INIT(picker_m_params, 0, 0, 60, 100, 50, 0, 100);
// Size L (80x120)
EGUI_VIEW_NUMBER_PICKER_PARAMS_INIT(picker_l_params, 0, 0, 80, 120, 80, 0, 100);

static void picker_value_changed_cb(egui_view_t *self, int16_t value)
{
    EGUI_LOG_INF("NumberPicker value: %d\n", value);
}

void test_init_ui(void)
{
    // Init grid
    egui_view_gridlayout_init_with_params(EGUI_VIEW_OF(&grid), &grid_params);

    // Init all pickers
    egui_view_number_picker_init_with_params(EGUI_VIEW_OF(&picker_xs), &picker_xs_params);
    egui_view_number_picker_set_on_value_changed_listener(EGUI_VIEW_OF(&picker_xs), picker_value_changed_cb);
    egui_view_number_picker_set_step(EGUI_VIEW_OF(&picker_xs), 5);
    picker_xs.text_color = EGUI_THEME_TEXT_SECONDARY;

    egui_view_number_picker_init_with_params(EGUI_VIEW_OF(&picker_s), &picker_s_params);
    egui_view_number_picker_set_on_value_changed_listener(EGUI_VIEW_OF(&picker_s), picker_value_changed_cb);
    egui_view_number_picker_set_step(EGUI_VIEW_OF(&picker_s), 5);
    picker_s.text_color = EGUI_THEME_TEXT;

    egui_view_number_picker_init_with_params(EGUI_VIEW_OF(&picker_m), &picker_m_params);
    egui_view_number_picker_set_on_value_changed_listener(EGUI_VIEW_OF(&picker_m), picker_value_changed_cb);
    egui_view_number_picker_set_step(EGUI_VIEW_OF(&picker_m), 5);
    picker_m.text_color = EGUI_THEME_PRIMARY_DARK;

    egui_view_number_picker_init_with_params(EGUI_VIEW_OF(&picker_l), &picker_l_params);
    egui_view_number_picker_set_on_value_changed_listener(EGUI_VIEW_OF(&picker_l), picker_value_changed_cb);
    egui_view_number_picker_set_step(EGUI_VIEW_OF(&picker_l), 5);
    picker_l.text_color = EGUI_THEME_PRIMARY;

    // Set margins
    egui_view_set_margin_all(EGUI_VIEW_OF(&picker_xs), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&picker_s), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&picker_m), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&picker_l), 6);

    // Add children to grid
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&picker_xs));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&picker_s));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&picker_m));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&picker_l));

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
        p_action->type = EGUI_SIM_ACTION_DRAG;
        egui_sim_get_view_pos(&picker_xs, 0.5f, 0.8f, &p_action->x1, &p_action->y1);
        egui_sim_get_view_pos(&picker_xs, 0.5f, 0.2f, &p_action->x2, &p_action->y2);
        p_action->steps = 10;
        p_action->interval_ms = 1000;
        return true;
    case 1:
        p_action->type = EGUI_SIM_ACTION_DRAG;
        egui_sim_get_view_pos(&picker_s, 0.5f, 0.8f, &p_action->x1, &p_action->y1);
        egui_sim_get_view_pos(&picker_s, 0.5f, 0.2f, &p_action->x2, &p_action->y2);
        p_action->steps = 10;
        p_action->interval_ms = 1000;
        return true;
    case 2:
        p_action->type = EGUI_SIM_ACTION_DRAG;
        egui_sim_get_view_pos(&picker_m, 0.5f, 0.2f, &p_action->x1, &p_action->y1);
        egui_sim_get_view_pos(&picker_m, 0.5f, 0.8f, &p_action->x2, &p_action->y2);
        p_action->steps = 10;
        p_action->interval_ms = 1000;
        return true;
    case 3:
        p_action->type = EGUI_SIM_ACTION_DRAG;
        egui_sim_get_view_pos(&picker_l, 0.5f, 0.2f, &p_action->x1, &p_action->y1);
        egui_sim_get_view_pos(&picker_l, 0.5f, 0.8f, &p_action->x2, &p_action->y2);
        p_action->steps = 10;
        p_action->interval_ms = 1000;
        return true;
    default:
        return false;
    }
}
#endif
