#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

// Clock 1: 24h format with seconds
static egui_view_digital_clock_t clock_24h;

// Clock 2: 12h format without seconds
static egui_view_digital_clock_t clock_12h;

// Clock backgrounds
EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_24h_param, EGUI_THEME_SURFACE, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_24h_params, &bg_24h_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_24h, &bg_24h_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_12h_param, EGUI_THEME_SURFACE_VARIANT, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_12h_params, &bg_12h_param, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_12h, &bg_12h_params);

// View params
EGUI_VIEW_DIGITAL_CLOCK_PARAMS_INIT(clock_24h_params, 0, 0, 200, 50, "00:00:00", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_PRIMARY_DARK, EGUI_ALPHA_100);
EGUI_VIEW_DIGITAL_CLOCK_PARAMS_INIT(clock_12h_params, 0, 0, 200, 50, "12:00 AM", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_PRIMARY_DARK, EGUI_ALPHA_100);

void test_init_ui(void)
{
    // Init clock 1: 24h format, 14:30:45
    egui_view_digital_clock_init_with_params(EGUI_VIEW_OF(&clock_24h), &clock_24h_params);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&clock_24h), EGUI_ALIGN_CENTER);
    egui_view_digital_clock_set_time(EGUI_VIEW_OF(&clock_24h), 14, 30, 45);

    // Init clock 2: 12h format, 14:30:00, no seconds
    egui_view_digital_clock_init_with_params(EGUI_VIEW_OF(&clock_12h), &clock_12h_params);
    egui_view_label_set_align_type(EGUI_VIEW_OF(&clock_12h), EGUI_ALIGN_CENTER);
    egui_view_digital_clock_set_format(EGUI_VIEW_OF(&clock_12h), 0);
    egui_view_digital_clock_set_show_second(EGUI_VIEW_OF(&clock_12h), 0);
    egui_view_digital_clock_set_time(EGUI_VIEW_OF(&clock_12h), 14, 30, 0);

    // Set backgrounds for better contrast
    egui_view_set_background(EGUI_VIEW_OF(&clock_24h), EGUI_BG_OF(&bg_24h));
    egui_view_set_background(EGUI_VIEW_OF(&clock_12h), EGUI_BG_OF(&bg_12h));

    // Add to root
    egui_core_add_user_root_view(EGUI_VIEW_OF(&clock_24h));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&clock_12h));

    // Layout
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_CENTER);
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
