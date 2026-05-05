#include "egui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "uicode_disp0.h"

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS && EGUI_CONFIG_FUNCTION_RECORDING_TEST
static egui_core_t *app_core;
static uint8_t runtime_fail_reported;
#endif

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

#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS && EGUI_CONFIG_FUNCTION_RECORDING_TEST
static void report_runtime_failure(const char *message)
{
    if (runtime_fail_reported)
    {
        return;
    }

    runtime_fail_reported = 1;
    printf("[RUNTIME_CHECK_FAIL] %s\n", message);
}

static void dispatch_key_event(uint8_t type, uint8_t key_code)
{
    egui_key_event_t key_event;

    memset(&key_event, 0, sizeof(key_event));
    key_event.type = type;
    key_event.key_code = key_code;
    egui_core_process_input_key(app_core, &key_event);
}

static void dispatch_key(uint8_t key_code)
{
    dispatch_key_event(EGUI_KEY_EVENT_ACTION_DOWN, key_code);
    dispatch_key_event(EGUI_KEY_EVENT_ACTION_UP, key_code);
}

static void dispatch_key_event_repeat(uint8_t type, uint8_t key_code, uint8_t count)
{
    uint8_t i;

    for (i = 0; i < count; i++)
    {
        dispatch_key_event(type, key_code);
    }
}

static uint8_t focused_is(egui_view_t *view)
{
    return egui_focus_manager_get_focused_view(app_core) == view;
}
#endif

void test_init_ui(egui_core_t *core)
{
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS && EGUI_CONFIG_FUNCTION_RECORDING_TEST
    app_core = core;
#endif

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
    static int last_action = -1;
    int first_call = (action_index != last_action);

    last_action = action_index;

    switch (action_index)
    {
    case 0:
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (first_call)
        {
            runtime_fail_reported = 0;
            recording_request_snapshot();
        }
#endif
        EGUI_SIM_SET_WAIT(p_action, 150);
        return true;
    case 1:
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (first_call)
        {
            dispatch_key(EGUI_KEY_CODE_RIGHT);
        }
#endif
        EGUI_SIM_SET_WAIT(p_action, 150);
        return true;
    case 2:
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (first_call && !focused_is(EGUI_VIEW_OF(&progress_xs)))
        {
            report_runtime_failure("direction key did not enter first progress bar");
        }
        if (first_call)
        {
            dispatch_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_RIGHT);
            recording_request_snapshot();
        }
#endif
        EGUI_SIM_SET_WAIT(p_action, 150);
        return true;
    case 3:
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (first_call && (!EGUI_VIEW_OF(&progress_xs)->is_pressed || progress_xs.process != 26))
        {
            report_runtime_failure("progress bar right key down did not press and step");
        }
        if (first_call)
        {
            dispatch_key_event_repeat(EGUI_KEY_EVENT_ACTION_REPEAT, EGUI_KEY_CODE_RIGHT, 8);
        }
#endif
        EGUI_SIM_SET_WAIT(p_action, 150);
        return true;
    case 4:
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (first_call && (!EGUI_VIEW_OF(&progress_xs)->is_pressed || progress_xs.process != 34))
        {
            report_runtime_failure("progress bar right key repeat did not keep stepping");
        }
        if (first_call)
        {
            dispatch_key_event_repeat(EGUI_KEY_EVENT_ACTION_LONG_PRESS, EGUI_KEY_CODE_RIGHT, 8);
        }
#endif
        EGUI_SIM_SET_WAIT(p_action, 150);
        return true;
    case 5:
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (first_call && (!EGUI_VIEW_OF(&progress_xs)->is_pressed || progress_xs.process != 42))
        {
            report_runtime_failure("progress bar right key long press did not keep stepping");
        }
        if (first_call)
        {
            dispatch_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_RIGHT);
            recording_request_snapshot();
        }
#endif
        EGUI_SIM_SET_WAIT(p_action, 150);
        return true;
    case 6:
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (first_call && (EGUI_VIEW_OF(&progress_xs)->is_pressed || progress_xs.process != 42))
        {
            report_runtime_failure("progress bar right key up did not release without extra step");
        }
        if (first_call)
        {
            dispatch_key_event(EGUI_KEY_EVENT_ACTION_DOWN, EGUI_KEY_CODE_LEFT);
            dispatch_key_event_repeat(EGUI_KEY_EVENT_ACTION_REPEAT, EGUI_KEY_CODE_LEFT, 8);
            dispatch_key_event(EGUI_KEY_EVENT_ACTION_UP, EGUI_KEY_CODE_LEFT);
        }
#endif
        EGUI_SIM_SET_WAIT(p_action, 150);
        return true;
    case 7:
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_CONFIG_FUNCTION_SUPPORT_FOCUS
        if (first_call && (EGUI_VIEW_OF(&progress_xs)->is_pressed || progress_xs.process != 33))
        {
            report_runtime_failure("progress bar left key repeat did not step back and release");
        }
        if (first_call)
        {
            recording_request_snapshot();
        }
#endif
        EGUI_SIM_SET_WAIT(p_action, 300);
        return true;
    default:
        return false;
    }
}
#endif
