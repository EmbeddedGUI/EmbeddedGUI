#include "egui.h"
#include <stdlib.h>
#include <stdio.h>
#include "uicode.h"

// 4 sliders: XS / S / M / L
static egui_view_slider_t slider_xs;
static egui_view_slider_t slider_s;
static egui_view_slider_t slider_m;
static egui_view_slider_t slider_l;

// Grid container
static egui_view_gridlayout_t grid;

// View params
EGUI_VIEW_GRIDLAYOUT_PARAMS_INIT(grid_params, 0, 0, 220, 300, 1, EGUI_ALIGN_CENTER);

// Size XS (100x22)
EGUI_VIEW_SLIDER_PARAMS_INIT(slider_xs_params, 0, 0, 100, 22, 20);
// Size S (130x26)
EGUI_VIEW_SLIDER_PARAMS_INIT(slider_s_params, 0, 0, 130, 26, 40);
// Size M (165x30)
EGUI_VIEW_SLIDER_PARAMS_INIT(slider_m_params, 0, 0, 165, 30, 60);
// Size L (200x34)
EGUI_VIEW_SLIDER_PARAMS_INIT(slider_l_params, 0, 0, 200, 34, 80);

#if EGUI_CONFIG_RECORDING_TEST
static uint8_t runtime_fail_reported;
#endif

static void slider_value_changed_cb(egui_view_t *self, uint8_t value)
{
    EGUI_LOG_INF("Slider value: %d\n", value);
}

void test_init_ui(void)
{
    // Init grid
    egui_view_gridlayout_init_with_params(EGUI_VIEW_OF(&grid), &grid_params);

#if EGUI_CONFIG_RECORDING_TEST
    runtime_fail_reported = 0;
#endif

    // Init all views
    egui_view_slider_init_with_params(EGUI_VIEW_OF(&slider_xs), &slider_xs_params);
    egui_view_slider_set_on_value_changed_listener(EGUI_VIEW_OF(&slider_xs), slider_value_changed_cb);

    egui_view_slider_init_with_params(EGUI_VIEW_OF(&slider_s), &slider_s_params);
    egui_view_slider_set_on_value_changed_listener(EGUI_VIEW_OF(&slider_s), slider_value_changed_cb);

    egui_view_slider_init_with_params(EGUI_VIEW_OF(&slider_m), &slider_m_params);
    egui_view_slider_set_on_value_changed_listener(EGUI_VIEW_OF(&slider_m), slider_value_changed_cb);

    egui_view_slider_init_with_params(EGUI_VIEW_OF(&slider_l), &slider_l_params);
    egui_view_slider_set_on_value_changed_listener(EGUI_VIEW_OF(&slider_l), slider_value_changed_cb);

    // Set margins
    egui_view_set_margin_all(EGUI_VIEW_OF(&slider_xs), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&slider_s), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&slider_m), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&slider_l), 6);

    // Add children to grid
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&slider_xs));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&slider_s));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&slider_m));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&slider_l));

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
        egui_sim_get_view_pos(&slider_xs, 0.1f, 0.5f, &p_action->x1, &p_action->y1);
        egui_sim_get_view_pos(&slider_xs, 0.9f, 0.5f, &p_action->x2, &p_action->y2);
        p_action->steps = 10;
        p_action->interval_ms = 1000;
        return true;
    case 1:
        p_action->type = EGUI_SIM_ACTION_DRAG;
        egui_sim_get_view_pos(&slider_s, 0.1f, 0.5f, &p_action->x1, &p_action->y1);
        egui_sim_get_view_pos(&slider_s, 0.9f, 0.5f, &p_action->x2, &p_action->y2);
        p_action->steps = 10;
        p_action->interval_ms = 1000;
        return true;
    case 2:
        p_action->type = EGUI_SIM_ACTION_DRAG;
        egui_sim_get_view_pos(&slider_m, 0.1f, 0.5f, &p_action->x1, &p_action->y1);
        egui_sim_get_view_pos(&slider_m, 0.9f, 0.5f, &p_action->x2, &p_action->y2);
        p_action->steps = 10;
        p_action->interval_ms = 1000;
        return true;
    case 3:
        p_action->type = EGUI_SIM_ACTION_DRAG;
        egui_sim_get_view_pos(&slider_l, 0.1f, 0.5f, &p_action->x1, &p_action->y1);
        egui_sim_get_view_pos(&slider_l, 0.9f, 0.5f, &p_action->x2, &p_action->y2);
        p_action->steps = 10;
        p_action->interval_ms = 1000;
        return true;
    case 4:
        if (first_call)
        {
            if (egui_view_slider_get_value(EGUI_VIEW_OF(&slider_xs)) == 20)
            {
                report_runtime_failure("slider_xs value did not change after drag");
            }
            if (egui_view_slider_get_value(EGUI_VIEW_OF(&slider_s)) == 40)
            {
                report_runtime_failure("slider_s value did not change after drag");
            }
            if (egui_view_slider_get_value(EGUI_VIEW_OF(&slider_m)) == 60)
            {
                report_runtime_failure("slider_m value did not change after drag");
            }
            if (egui_view_slider_get_value(EGUI_VIEW_OF(&slider_l)) == 80)
            {
                report_runtime_failure("slider_l value did not change after drag");
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
