#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

// Line views
static egui_view_line_t line_zigzag;
static egui_view_line_t line_triangle;
static egui_view_line_t line_wave;

// Zigzag points
static const egui_view_line_point_t zigzag_points[] = {
        {0, 40}, {30, 0}, {60, 40}, {90, 0}, {120, 40},
};

// Triangle points
static const egui_view_line_point_t triangle_points[] = {
        {0, 50},
        {40, 0},
        {80, 50},
        {0, 50},
};

// Wave-like points
static const egui_view_line_point_t wave_points[] = {
        {0, 20}, {20, 0}, {40, 20}, {60, 40}, {80, 20}, {100, 0}, {120, 20},
};

// View params
EGUI_VIEW_LINE_PARAMS_INIT(zigzag_params, 0, 0, 130, 50, 2, EGUI_THEME_PRIMARY);
EGUI_VIEW_LINE_PARAMS_INIT(triangle_params, 0, 0, 90, 60, 3, EGUI_THEME_DANGER);
EGUI_VIEW_LINE_PARAMS_INIT(wave_params, 0, 0, 130, 50, 2, EGUI_THEME_PRIMARY_DARK);

void test_init_ui(void)
{
    // Init zigzag line
    egui_view_line_init_with_params(EGUI_VIEW_OF(&line_zigzag), &zigzag_params);
    egui_view_line_set_points(EGUI_VIEW_OF(&line_zigzag), zigzag_points, 5);

    // Init triangle line
    egui_view_line_init_with_params(EGUI_VIEW_OF(&line_triangle), &triangle_params);
    egui_view_line_set_points(EGUI_VIEW_OF(&line_triangle), triangle_points, 4);

    // Init wave line with round cap
    egui_view_line_init_with_params(EGUI_VIEW_OF(&line_wave), &wave_params);
    egui_view_line_set_points(EGUI_VIEW_OF(&line_wave), wave_points, 7);
    egui_view_line_set_use_round_cap(EGUI_VIEW_OF(&line_wave), 1);

    // Set margins
    egui_view_set_margin_all(EGUI_VIEW_OF(&line_zigzag), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&line_triangle), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&line_wave), 6);

    // Add to root
    egui_core_add_user_root_view(EGUI_VIEW_OF(&line_zigzag));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&line_triangle));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&line_wave));

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
