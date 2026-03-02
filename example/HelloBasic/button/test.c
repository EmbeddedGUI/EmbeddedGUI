#include "egui.h"
#include <stdlib.h>
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

static char button_str_xs[20] = "XS";
static char button_str_s[20] = "S";
static char button_str_m[20] = "M";
static char button_str_l[20] = "L";

static void button_click_cb(egui_view_t *self)
{
    EGUI_LOG_INF("Button clicked\n");
}

void test_init_ui(void)
{
    // Init grid
    egui_view_gridlayout_init_with_params(EGUI_VIEW_OF(&grid), &grid_params);

    // Init button XS
    egui_view_button_init_with_params(EGUI_VIEW_OF(&button_xs), &button_xs_params);
    egui_view_label_set_text(EGUI_VIEW_OF(&button_xs), button_str_xs);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&button_xs), button_click_cb);
    // Init button S
    egui_view_button_init_with_params(EGUI_VIEW_OF(&button_s), &button_s_params);
    egui_view_label_set_text(EGUI_VIEW_OF(&button_s), button_str_s);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&button_s), button_click_cb);

    // Init button M
    egui_view_button_init_with_params(EGUI_VIEW_OF(&button_m), &button_m_params);
    egui_view_label_set_text(EGUI_VIEW_OF(&button_m), button_str_m);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&button_m), button_click_cb);

    // Init button L
    egui_view_button_init_with_params(EGUI_VIEW_OF(&button_l), &button_l_params);
    egui_view_label_set_text(EGUI_VIEW_OF(&button_l), button_str_l);
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
    switch (action_index)
    {
    case 0:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &button_xs, 1000);
        return true;
    case 1:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &button_s, 1000);
        return true;
    case 2:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &button_m, 1000);
        return true;
    case 3:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &button_l, 1000);
        return true;
    default:
        return false;
    }
}
#endif
