#include "egui.h"
#include <stdio.h>
#include <stdlib.h>
#include "uicode.h"

// 4 toggle buttons: XS / S / M / L
static egui_view_toggle_button_t toggle_xs;
static egui_view_toggle_button_t toggle_s;
static egui_view_toggle_button_t toggle_m;
static egui_view_toggle_button_t toggle_l;

// Grid container
static egui_view_gridlayout_t grid;

static const char *toggle_names[] = {
        "favorite",
        "alerts",
        "visible",
        "settings",
};

// View params
EGUI_VIEW_GRIDLAYOUT_PARAMS_INIT(grid_params, 0, 0, 220, 300, 1, EGUI_ALIGN_CENTER);

// Size XS (108x30)
EGUI_VIEW_TOGGLE_BUTTON_PARAMS_INIT(toggle_xs_params, 0, 0, 108, 30, "Favorite", 0);
// Size S (132x34)
EGUI_VIEW_TOGGLE_BUTTON_PARAMS_INIT(toggle_s_params, 0, 0, 132, 34, "Alerts", 1);
// Size M (168x38)
EGUI_VIEW_TOGGLE_BUTTON_PARAMS_INIT(toggle_m_params, 0, 0, 168, 38, "Visible", 0);
// Size L (208x44)
EGUI_VIEW_TOGGLE_BUTTON_PARAMS_INIT(toggle_l_params, 0, 0, 208, 44, "Settings", 1);

#if EGUI_CONFIG_RECORDING_TEST
static uint8_t runtime_fail_reported;

static void report_runtime_failure(const char *message)
{
    if (runtime_fail_reported)
    {
        return;
    }

    runtime_fail_reported = 1;
    printf("[RUNTIME_CHECK_FAIL] %s\n", message);
}
#endif

static void on_toggled_cb(egui_view_t *self, uint8_t is_toggled)
{
    const char *name = "unknown";

    if (self == EGUI_VIEW_OF(&toggle_xs))
    {
        name = toggle_names[0];
    }
    else if (self == EGUI_VIEW_OF(&toggle_s))
    {
        name = toggle_names[1];
    }
    else if (self == EGUI_VIEW_OF(&toggle_m))
    {
        name = toggle_names[2];
    }
    else if (self == EGUI_VIEW_OF(&toggle_l))
    {
        name = toggle_names[3];
    }

    (void)name;
    (void)is_toggled;
    EGUI_LOG_INF("Toggle toggled: %s=%d\r\n", name, is_toggled);
}

void test_init_ui(void)
{
#if EGUI_CONFIG_RECORDING_TEST
    runtime_fail_reported = 0;
#endif
    // Init grid
    egui_view_gridlayout_init_with_params(EGUI_VIEW_OF(&grid), &grid_params);

    // Init all toggle buttons
    egui_view_toggle_button_init_with_params(EGUI_VIEW_OF(&toggle_xs), &toggle_xs_params);
    egui_view_toggle_button_set_on_toggled_listener(EGUI_VIEW_OF(&toggle_xs), on_toggled_cb);
    egui_view_toggle_button_set_icon(EGUI_VIEW_OF(&toggle_xs), EGUI_ICON_MS_HEART);
    egui_view_toggle_button_set_icon_font(EGUI_VIEW_OF(&toggle_xs), EGUI_FONT_ICON_MS_16);
    egui_view_toggle_button_set_icon_text_gap(EGUI_VIEW_OF(&toggle_xs), 4);
    egui_view_toggle_button_set_text_color(EGUI_VIEW_OF(&toggle_xs), EGUI_COLOR_WHITE);

    egui_view_toggle_button_init_with_params(EGUI_VIEW_OF(&toggle_s), &toggle_s_params);
    egui_view_toggle_button_set_on_toggled_listener(EGUI_VIEW_OF(&toggle_s), on_toggled_cb);
    egui_view_toggle_button_set_icon(EGUI_VIEW_OF(&toggle_s), EGUI_ICON_MS_NOTIFICATIONS);
    egui_view_toggle_button_set_icon_font(EGUI_VIEW_OF(&toggle_s), EGUI_FONT_ICON_MS_16);
    egui_view_toggle_button_set_icon_text_gap(EGUI_VIEW_OF(&toggle_s), 6);
    egui_view_toggle_button_set_text_color(EGUI_VIEW_OF(&toggle_s), EGUI_COLOR_WHITE);

    egui_view_toggle_button_init_with_params(EGUI_VIEW_OF(&toggle_m), &toggle_m_params);
    egui_view_toggle_button_set_on_toggled_listener(EGUI_VIEW_OF(&toggle_m), on_toggled_cb);
    egui_view_toggle_button_set_icon(EGUI_VIEW_OF(&toggle_m), EGUI_ICON_MS_VISIBILITY);
    egui_view_toggle_button_set_icon_font(EGUI_VIEW_OF(&toggle_m), EGUI_FONT_ICON_MS_20);
    egui_view_toggle_button_set_icon_text_gap(EGUI_VIEW_OF(&toggle_m), 6);
    egui_view_toggle_button_set_text_color(EGUI_VIEW_OF(&toggle_m), EGUI_COLOR_WHITE);

    egui_view_toggle_button_init_with_params(EGUI_VIEW_OF(&toggle_l), &toggle_l_params);
    egui_view_toggle_button_set_on_toggled_listener(EGUI_VIEW_OF(&toggle_l), on_toggled_cb);
    egui_view_toggle_button_set_icon(EGUI_VIEW_OF(&toggle_l), EGUI_ICON_MS_SETTINGS);
    egui_view_toggle_button_set_icon_font(EGUI_VIEW_OF(&toggle_l), EGUI_FONT_ICON_MS_20);
    egui_view_toggle_button_set_icon_text_gap(EGUI_VIEW_OF(&toggle_l), 8);
    egui_view_toggle_button_set_text_color(EGUI_VIEW_OF(&toggle_l), EGUI_COLOR_WHITE);

    // Set margins
    egui_view_set_margin_all(EGUI_VIEW_OF(&toggle_xs), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&toggle_s), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&toggle_m), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&toggle_l), 6);

    // Add children to grid
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&toggle_xs));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&toggle_s));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&toggle_m));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&toggle_l));

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
    static int last_action = -1;
    int first_call = (action_index != last_action);

    last_action = action_index;

    switch (action_index)
    {
    case 0:
        if (first_call &&
            (egui_view_toggle_button_is_toggled(EGUI_VIEW_OF(&toggle_xs)) != 0 || egui_view_toggle_button_is_toggled(EGUI_VIEW_OF(&toggle_s)) != 1 ||
             egui_view_toggle_button_is_toggled(EGUI_VIEW_OF(&toggle_m)) != 0 || egui_view_toggle_button_is_toggled(EGUI_VIEW_OF(&toggle_l)) != 1))
        {
            report_runtime_failure("toggle initial state mismatch");
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, &toggle_xs, 1000);
        return true;
    case 1:
        if (first_call && egui_view_toggle_button_is_toggled(EGUI_VIEW_OF(&toggle_xs)) != 1)
        {
            report_runtime_failure("toggle_xs did not toggle on");
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, &toggle_s, 1000);
        return true;
    case 2:
        if (first_call && egui_view_toggle_button_is_toggled(EGUI_VIEW_OF(&toggle_s)) != 0)
        {
            report_runtime_failure("toggle_s did not toggle off");
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, &toggle_m, 1000);
        return true;
    case 3:
        if (first_call && egui_view_toggle_button_is_toggled(EGUI_VIEW_OF(&toggle_m)) != 1)
        {
            report_runtime_failure("toggle_m did not toggle on");
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, &toggle_l, 1000);
        return true;
    case 4:
        if (first_call)
        {
            if (egui_view_toggle_button_is_toggled(EGUI_VIEW_OF(&toggle_l)) != 0)
            {
                report_runtime_failure("toggle_l did not toggle off");
            }
            if (egui_view_toggle_button_is_toggled(EGUI_VIEW_OF(&toggle_xs)) != 1 || egui_view_toggle_button_is_toggled(EGUI_VIEW_OF(&toggle_s)) != 0 ||
                egui_view_toggle_button_is_toggled(EGUI_VIEW_OF(&toggle_m)) != 1 || egui_view_toggle_button_is_toggled(EGUI_VIEW_OF(&toggle_l)) != 0)
            {
                report_runtime_failure("toggle final state mismatch");
            }
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    default:
        return false;
    }
}
#endif
