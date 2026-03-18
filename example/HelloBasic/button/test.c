#include "egui.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "uicode.h"

// 4 buttons: XS / S / M / L
static egui_view_button_t button_xs;
static egui_view_button_t button_s;
static egui_view_button_t button_m;
static egui_view_button_t button_l;

// Grid container
static egui_view_gridlayout_t grid;

// View params
EGUI_VIEW_GRIDLAYOUT_PARAMS_INIT(grid_params, 0, 0, 220, 300, 1, EGUI_ALIGN_CENTER);

// Size XS (96x30)
EGUI_VIEW_BUTTON_PARAMS_INIT(button_xs_params, 0, 0, 96, 30, NULL, EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT, EGUI_ALPHA_100);
// Size S (120x34)
EGUI_VIEW_BUTTON_PARAMS_INIT(button_s_params, 0, 0, 120, 34, NULL, EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT, EGUI_ALPHA_100);
// Size M (160x38)
EGUI_VIEW_BUTTON_PARAMS_INIT(button_m_params, 0, 0, 160, 38, NULL, EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT, EGUI_ALPHA_100);
// Size L (200x44)
EGUI_VIEW_BUTTON_PARAMS_INIT(button_l_params, 0, 0, 200, 44, NULL, EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT, EGUI_ALPHA_100);

static char button_str_xs[20] = "Run";
static char button_str_s[20] = "Sync";
static char button_str_m[20] = "Search";
static char button_str_l[20] = "Settings";

#if EGUI_CONFIG_RECORDING_TEST
static uint8_t runtime_fail_reported;
static uint8_t button_click_counts[4];

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

static void button_click_cb(egui_view_t *self)
{
#if EGUI_CONFIG_RECORDING_TEST
    if (self == EGUI_VIEW_OF(&button_xs))
    {
        button_click_counts[0]++;
    }
    else if (self == EGUI_VIEW_OF(&button_s))
    {
        button_click_counts[1]++;
    }
    else if (self == EGUI_VIEW_OF(&button_m))
    {
        button_click_counts[2]++;
    }
    else if (self == EGUI_VIEW_OF(&button_l))
    {
        button_click_counts[3]++;
    }
#endif
    EGUI_LOG_INF("Button clicked\n");
}

void test_init_ui(void)
{
#if EGUI_CONFIG_RECORDING_TEST
    runtime_fail_reported = 0;
    memset(button_click_counts, 0, sizeof(button_click_counts));
#endif
    // Init grid
    egui_view_gridlayout_init_with_params(EGUI_VIEW_OF(&grid), &grid_params);

    // Init button XS
    egui_view_button_init_with_params(EGUI_VIEW_OF(&button_xs), &button_xs_params);
    egui_view_label_set_text(EGUI_VIEW_OF(&button_xs), button_str_xs);
    egui_view_button_set_icon(EGUI_VIEW_OF(&button_xs), EGUI_ICON_MS_PLAY_ARROW);
    egui_view_button_set_icon_font(EGUI_VIEW_OF(&button_xs), EGUI_FONT_ICON_MS_16);
    egui_view_button_set_icon_text_gap(EGUI_VIEW_OF(&button_xs), 4);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&button_xs), button_click_cb);
    // Init button S
    egui_view_button_init_with_params(EGUI_VIEW_OF(&button_s), &button_s_params);
    egui_view_label_set_text(EGUI_VIEW_OF(&button_s), button_str_s);
    egui_view_button_set_icon(EGUI_VIEW_OF(&button_s), EGUI_ICON_MS_SYNC);
    egui_view_button_set_icon_font(EGUI_VIEW_OF(&button_s), EGUI_FONT_ICON_MS_16);
    egui_view_button_set_icon_text_gap(EGUI_VIEW_OF(&button_s), 4);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&button_s), button_click_cb);

    // Init button M
    egui_view_button_init_with_params(EGUI_VIEW_OF(&button_m), &button_m_params);
    egui_view_label_set_text(EGUI_VIEW_OF(&button_m), button_str_m);
    egui_view_button_set_icon(EGUI_VIEW_OF(&button_m), EGUI_ICON_MS_SEARCH);
    egui_view_button_set_icon_font(EGUI_VIEW_OF(&button_m), EGUI_FONT_ICON_MS_20);
    egui_view_button_set_icon_text_gap(EGUI_VIEW_OF(&button_m), 5);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&button_m), button_click_cb);

    // Init button L
    egui_view_button_init_with_params(EGUI_VIEW_OF(&button_l), &button_l_params);
    egui_view_label_set_text(EGUI_VIEW_OF(&button_l), button_str_l);
    egui_view_button_set_icon(EGUI_VIEW_OF(&button_l), EGUI_ICON_MS_SETTINGS);
    egui_view_button_set_icon_font(EGUI_VIEW_OF(&button_l), EGUI_FONT_ICON_MS_20);
    egui_view_button_set_icon_text_gap(EGUI_VIEW_OF(&button_l), 6);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&button_l), button_click_cb);

    // Set margins
    egui_view_set_margin_all(EGUI_VIEW_OF(&button_xs), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&button_s), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&button_m), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&button_l), 6);

    // Add children to grid
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&button_xs));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&button_s));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&button_m));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&button_l));

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
        if (first_call && (button_click_counts[0] || button_click_counts[1] || button_click_counts[2] || button_click_counts[3]))
        {
            report_runtime_failure("button initial click count mismatch");
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, &button_xs, 1000);
        return true;
    case 1:
        if (first_call && button_click_counts[0] != 1)
        {
            report_runtime_failure("button_xs click was not delivered exactly once");
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, &button_s, 1000);
        return true;
    case 2:
        if (first_call && button_click_counts[1] != 1)
        {
            report_runtime_failure("button_s click was not delivered exactly once");
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, &button_m, 1000);
        return true;
    case 3:
        if (first_call && button_click_counts[2] != 1)
        {
            report_runtime_failure("button_m click was not delivered exactly once");
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, &button_l, 1000);
        return true;
    case 4:
        if (first_call)
        {
            if (button_click_counts[3] != 1)
            {
                report_runtime_failure("button_l click was not delivered exactly once");
            }
            if (button_click_counts[0] != 1 || button_click_counts[1] != 1 || button_click_counts[2] != 1 || button_click_counts[3] != 1)
            {
                report_runtime_failure("button click coverage incomplete");
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
