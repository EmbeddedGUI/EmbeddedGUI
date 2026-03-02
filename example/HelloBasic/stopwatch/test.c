#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

// 2 stopwatches
static egui_view_stopwatch_t sw1;
static egui_view_stopwatch_t sw2;

// Stopwatch backgrounds
EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_sw1_param, EGUI_THEME_SURFACE, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_sw1_params, &bg_sw1_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_sw1, &bg_sw1_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_sw2_param, EGUI_THEME_SURFACE_VARIANT, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_sw2_params, &bg_sw2_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_sw2, &bg_sw2_params);

// Layout container
static egui_view_linearlayout_t layout;

// View params
EGUI_VIEW_LINEARLAYOUT_PARAMS_INIT(layout_params, 0, 0, 240, 320, EGUI_ALIGN_CENTER);

EGUI_VIEW_STOPWATCH_PARAMS_INIT(sw1_params, 0, 0, 200, 40, "00:00.00", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_PRIMARY, EGUI_ALPHA_100);
EGUI_VIEW_STOPWATCH_PARAMS_INIT(sw2_params, 0, 0, 200, 40, "00:00", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_PRIMARY_DARK, EGUI_ALPHA_100);

void test_init_ui(void)
{
    // Init layout
    egui_view_linearlayout_init_with_params(EGUI_VIEW_OF(&layout), &layout_params);

    // Init SW 1: elapsed_ms=125340 (2:05.34), show_ms=1
    egui_view_stopwatch_init_with_params(EGUI_VIEW_OF(&sw1), &sw1_params);
    egui_view_stopwatch_set_show_ms(EGUI_VIEW_OF(&sw1), 1);
    egui_view_stopwatch_set_elapsed(EGUI_VIEW_OF(&sw1), 125340);

    // Init SW 2: elapsed_ms=3723000 (1:02:03), show_ms=0
    egui_view_stopwatch_init_with_params(EGUI_VIEW_OF(&sw2), &sw2_params);
    egui_view_stopwatch_set_show_ms(EGUI_VIEW_OF(&sw2), 0);
    egui_view_stopwatch_set_elapsed(EGUI_VIEW_OF(&sw2), 3723000);

    // Set backgrounds for better text contrast
    egui_view_set_background(EGUI_VIEW_OF(&sw1), EGUI_BG_OF(&bg_sw1));
    egui_view_set_background(EGUI_VIEW_OF(&sw2), EGUI_BG_OF(&bg_sw2));

    // Set margins
    egui_view_set_margin_all(EGUI_VIEW_OF(&sw1), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&sw2), 6);

    // Add children to layout
    egui_view_group_add_child(EGUI_VIEW_OF(&layout), EGUI_VIEW_OF(&sw1));
    egui_view_group_add_child(EGUI_VIEW_OF(&layout), EGUI_VIEW_OF(&sw2));

    // Layout children
    egui_view_linearlayout_layout_childs(EGUI_VIEW_OF(&layout));

    // Add layout to root
    egui_core_add_user_root_view(EGUI_VIEW_OF(&layout));

    // Center layout on screen
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
