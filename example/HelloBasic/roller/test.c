#include "egui.h"
#include <stdlib.h>
#include <stdio.h>
#include "uicode.h"

// 4 rollers: XS / S / M / L
static egui_view_roller_t roller_xs;
static egui_view_roller_t roller_s;
static egui_view_roller_t roller_m;
static egui_view_roller_t roller_l;

static const char *weekdays[] = {"Mon", "Tue", "Wed", "Thu", "Fri"};

// Grid container
static egui_view_gridlayout_t grid;

// View params
EGUI_VIEW_GRIDLAYOUT_PARAMS_INIT(grid_params, 0, 0, 230, 300, 1, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);

// Size XS (90x42)
EGUI_VIEW_ROLLER_PARAMS_INIT(roller_xs_params, 0, 0, 90, 42, weekdays, 5, 0);
// Size S (110x56)
EGUI_VIEW_ROLLER_PARAMS_INIT(roller_s_params, 0, 0, 110, 56, weekdays, 5, 1);
// Size M (140x70)
EGUI_VIEW_ROLLER_PARAMS_INIT(roller_m_params, 0, 0, 140, 70, weekdays, 5, 2);
// Size L (170x86)
EGUI_VIEW_ROLLER_PARAMS_INIT(roller_l_params, 0, 0, 170, 86, weekdays, 5, 3);

#if EGUI_CONFIG_RECORDING_TEST
static uint8_t runtime_fail_reported;
#endif

static void on_roller_selected(egui_view_t *self, uint8_t index)
{
    EGUI_LOG_INF("Roller selected: %s (index=%d)\r\n", weekdays[index], index);
}

void test_init_ui(void)
{
    // Init grid
    egui_view_gridlayout_init_with_params(EGUI_VIEW_OF(&grid), &grid_params);

#if EGUI_CONFIG_RECORDING_TEST
    runtime_fail_reported = 0;
#endif

    // Init all rollers
    egui_view_roller_init_with_params(EGUI_VIEW_OF(&roller_xs), &roller_xs_params);
    egui_view_roller_set_on_selected_listener(EGUI_VIEW_OF(&roller_xs), on_roller_selected);

    egui_view_roller_init_with_params(EGUI_VIEW_OF(&roller_s), &roller_s_params);
    egui_view_roller_set_on_selected_listener(EGUI_VIEW_OF(&roller_s), on_roller_selected);

    egui_view_roller_init_with_params(EGUI_VIEW_OF(&roller_m), &roller_m_params);
    egui_view_roller_set_on_selected_listener(EGUI_VIEW_OF(&roller_m), on_roller_selected);

    egui_view_roller_init_with_params(EGUI_VIEW_OF(&roller_l), &roller_l_params);
    egui_view_roller_set_on_selected_listener(EGUI_VIEW_OF(&roller_l), on_roller_selected);

    // Set margins
    egui_view_set_margin_all(EGUI_VIEW_OF(&roller_xs), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&roller_s), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&roller_m), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&roller_l), 6);

    // Add children to grid
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&roller_xs));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&roller_s));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&roller_m));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&roller_l));

    // Layout grid children
    egui_view_gridlayout_layout_childs(EGUI_VIEW_OF(&grid));

    // Add grid to root
    egui_core_add_user_root_view(EGUI_VIEW_OF(&grid));

    // Center grid on screen
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
}

#if EGUI_CONFIG_RECORDING_TEST
static void report_runtime_failure(const char *message)
{
    if (runtime_fail_reported)
    {
        return;
    }

    runtime_fail_reported = 1;
    printf("[RUNTIME_CHECK_FAIL] %s\n", message);
}

bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static int last_action = -1;
    int first_call = action_index != last_action;

    last_action = action_index;

    switch (action_index)
    {
    case 0:
        p_action->type = EGUI_SIM_ACTION_DRAG;
        egui_sim_get_view_pos(&roller_xs, 0.5f, 0.8f, &p_action->x1, &p_action->y1);
        egui_sim_get_view_pos(&roller_xs, 0.5f, 0.2f, &p_action->x2, &p_action->y2);
        p_action->steps = 10;
        p_action->interval_ms = 1000;
        return true;
    case 1:
        p_action->type = EGUI_SIM_ACTION_DRAG;
        egui_sim_get_view_pos(&roller_s, 0.5f, 0.8f, &p_action->x1, &p_action->y1);
        egui_sim_get_view_pos(&roller_s, 0.5f, 0.2f, &p_action->x2, &p_action->y2);
        p_action->steps = 10;
        p_action->interval_ms = 1000;
        return true;
    case 2:
        p_action->type = EGUI_SIM_ACTION_DRAG;
        egui_sim_get_view_pos(&roller_m, 0.5f, 0.2f, &p_action->x1, &p_action->y1);
        egui_sim_get_view_pos(&roller_m, 0.5f, 0.8f, &p_action->x2, &p_action->y2);
        p_action->steps = 10;
        p_action->interval_ms = 1000;
        return true;
    case 3:
        p_action->type = EGUI_SIM_ACTION_DRAG;
        egui_sim_get_view_pos(&roller_l, 0.5f, 0.2f, &p_action->x1, &p_action->y1);
        egui_sim_get_view_pos(&roller_l, 0.5f, 0.8f, &p_action->x2, &p_action->y2);
        p_action->steps = 10;
        p_action->interval_ms = 1000;
        return true;
    case 4:
        if (first_call)
        {
            if (egui_view_roller_get_current_index(EGUI_VIEW_OF(&roller_xs)) == 0)
            {
                report_runtime_failure("roller_xs index did not change after drag");
            }
            if (egui_view_roller_get_current_index(EGUI_VIEW_OF(&roller_s)) == 1)
            {
                report_runtime_failure("roller_s index did not change after drag");
            }
            if (egui_view_roller_get_current_index(EGUI_VIEW_OF(&roller_m)) == 2)
            {
                report_runtime_failure("roller_m index did not change after drag");
            }
            if (egui_view_roller_get_current_index(EGUI_VIEW_OF(&roller_l)) == 3)
            {
                report_runtime_failure("roller_l index did not change after drag");
            }
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 320);
        return true;
    default:
        return false;
    }
}
#endif
