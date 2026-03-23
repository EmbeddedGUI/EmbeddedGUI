#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

#ifndef EGUI_EXAMPLE_DIRTY_ANIMATION_CHECK
#define EGUI_EXAMPLE_DIRTY_ANIMATION_CHECK 0
#endif

// 2 analog clocks
static egui_view_analog_clock_t clock_1;
static egui_view_analog_clock_t clock_2;

// Grid container
static egui_view_gridlayout_t grid;

#if EGUI_EXAMPLE_DIRTY_ANIMATION_CHECK
static egui_timer_t dirty_anim_timer;
static uint8_t clock_1_hour = 10;
static uint8_t clock_1_minute = 10;
static uint8_t clock_1_second = 30;
static uint8_t clock_2_hour = 3;
static uint8_t clock_2_minute = 45;

static void dirty_anim_timer_callback(egui_timer_t *timer)
{
    (void)timer;

    clock_1_second++;
    if (clock_1_second >= 60)
    {
        clock_1_second = 0;
        clock_1_minute++;
        if (clock_1_minute >= 60)
        {
            clock_1_minute = 0;
            clock_1_hour = (uint8_t)((clock_1_hour + 1) % 12);
        }
    }

    clock_2_minute = (uint8_t)((clock_2_minute + 1) % 60);
    if (clock_2_minute == 0)
    {
        clock_2_hour = (uint8_t)((clock_2_hour + 1) % 12);
    }

    egui_view_analog_clock_set_time(EGUI_VIEW_OF(&clock_1), clock_1_hour, clock_1_minute, clock_1_second);
    egui_view_analog_clock_set_time(EGUI_VIEW_OF(&clock_2), clock_2_hour, clock_2_minute, 0);
}
#endif

// View params
EGUI_VIEW_GRIDLAYOUT_PARAMS_INIT(grid_params, 0, 0, 240, 300, 2, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);

EGUI_VIEW_ANALOG_CLOCK_PARAMS_INIT(clock_1_params, 0, 0, 110, 110, 10, 10, 30);
EGUI_VIEW_ANALOG_CLOCK_PARAMS_INIT(clock_2_params, 0, 0, 94, 94, 3, 45, 0);

void test_init_ui(void)
{
#if EGUI_EXAMPLE_DIRTY_ANIMATION_CHECK
    clock_1_hour = 10;
    clock_1_minute = 10;
    clock_1_second = 30;
    clock_2_hour = 3;
    clock_2_minute = 45;
#endif
    // Init grid
    egui_view_gridlayout_init_with_params(EGUI_VIEW_OF(&grid), &grid_params);

    // Init clock 1: 10:10:30 with seconds
    egui_view_analog_clock_init_with_params(EGUI_VIEW_OF(&clock_1), &clock_1_params);

    // Init clock 2: 3:45:00 without seconds
    egui_view_analog_clock_init_with_params(EGUI_VIEW_OF(&clock_2), &clock_2_params);
    egui_view_analog_clock_show_second(EGUI_VIEW_OF(&clock_2), 0);

    // Set margins
    egui_view_set_margin_all(EGUI_VIEW_OF(&clock_1), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&clock_2), 6);

    // Add children to grid
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&clock_1));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&clock_2));

    // Layout grid children
    egui_view_gridlayout_layout_childs(EGUI_VIEW_OF(&grid));

    // Add grid to root
    egui_core_add_user_root_view(EGUI_VIEW_OF(&grid));

    // Center grid on screen
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);

#if EGUI_EXAMPLE_DIRTY_ANIMATION_CHECK
    egui_timer_init_timer(&dirty_anim_timer, NULL, dirty_anim_timer_callback);
    egui_timer_start_timer(&dirty_anim_timer, 120, 120);
#endif
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
