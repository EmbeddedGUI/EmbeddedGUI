#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

// 4 LEDs: XS / S / M / L
static egui_view_led_t led_xs;
static egui_view_led_t led_s;
static egui_view_led_t led_m;
static egui_view_led_t led_l;

// Grid container
static egui_view_gridlayout_t grid;

// View params
EGUI_VIEW_GRIDLAYOUT_PARAMS_INIT(grid_params, 0, 0, 230, 300, 2, EGUI_ALIGN_CENTER);

// Size XS (20x20)
EGUI_VIEW_LED_PARAMS_INIT(led_xs_params, 0, 0, 20, 20, 1);
// Size S (30x30)
EGUI_VIEW_LED_PARAMS_INIT(led_s_params, 0, 0, 30, 30, 0);
// Size M (40x40)
EGUI_VIEW_LED_PARAMS_INIT(led_m_params, 0, 0, 40, 40, 1);
// Size L (56x56)
EGUI_VIEW_LED_PARAMS_INIT(led_l_params, 0, 0, 56, 56, 1);

void test_init_ui(void)
{
    // Init grid
    egui_view_gridlayout_init_with_params(EGUI_VIEW_OF(&grid), &grid_params);

    // Init LED XS: green, on
    egui_view_led_init_with_params(EGUI_VIEW_OF(&led_xs), &led_xs_params);
    egui_view_led_set_colors(EGUI_VIEW_OF(&led_xs), EGUI_COLOR_GREEN, EGUI_COLOR_DARK_GREY);

    // Init LED S: red, off
    egui_view_led_init_with_params(EGUI_VIEW_OF(&led_s), &led_s_params);
    egui_view_led_set_colors(EGUI_VIEW_OF(&led_s), EGUI_COLOR_RED, EGUI_COLOR_DARK_GREY);

    // Init LED M: yellow, blinking
    egui_view_led_init_with_params(EGUI_VIEW_OF(&led_m), &led_m_params);
    egui_view_led_set_colors(EGUI_VIEW_OF(&led_m), EGUI_COLOR_YELLOW, EGUI_COLOR_DARK_GREY);
    egui_view_led_set_blink(EGUI_VIEW_OF(&led_m), 500);

    // Init LED L: blue, on
    egui_view_led_init_with_params(EGUI_VIEW_OF(&led_l), &led_l_params);
    egui_view_led_set_colors(EGUI_VIEW_OF(&led_l), EGUI_COLOR_BLUE, EGUI_COLOR_DARK_GREY);

    // Set margins
    egui_view_set_margin_all(EGUI_VIEW_OF(&led_xs), 5);
    egui_view_set_margin_all(EGUI_VIEW_OF(&led_s), 5);
    egui_view_set_margin_all(EGUI_VIEW_OF(&led_m), 5);
    egui_view_set_margin_all(EGUI_VIEW_OF(&led_l), 5);

    // Add children to grid
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&led_xs));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&led_s));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&led_m));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&led_l));

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
