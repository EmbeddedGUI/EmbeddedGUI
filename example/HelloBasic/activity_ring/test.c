#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

#ifndef EGUI_EXAMPLE_DIRTY_ANIMATION_CHECK
#define EGUI_EXAMPLE_DIRTY_ANIMATION_CHECK 0
#endif

// 2 activity rings
static egui_view_activity_ring_t ring_1;
static egui_view_activity_ring_t ring_2;

// Grid container
static egui_view_gridlayout_t grid;

#if EGUI_EXAMPLE_DIRTY_ANIMATION_CHECK
static egui_timer_t dirty_anim_timer;
static uint8_t dirty_anim_tick;

static uint8_t clamp_percent(int value)
{
    if (value < 0)
    {
        return 0;
    }
    if (value > 100)
    {
        return 100;
    }
    return (uint8_t)value;
}

static void dirty_anim_timer_callback(egui_timer_t *timer)
{
    int frame;

    (void)timer;

    dirty_anim_tick = (uint8_t)((dirty_anim_tick + 1) % 100);
    frame = dirty_anim_tick;

    egui_view_activity_ring_set_value(EGUI_VIEW_OF(&ring_1), 0, clamp_percent(25 + ((frame * 3) % 70)));
    egui_view_activity_ring_set_value(EGUI_VIEW_OF(&ring_1), 1, clamp_percent(18 + ((frame * 5) % 64)));
    egui_view_activity_ring_set_value(EGUI_VIEW_OF(&ring_1), 2, clamp_percent(12 + ((frame * 7) % 56)));

    egui_view_activity_ring_set_value(EGUI_VIEW_OF(&ring_2), 0, clamp_percent(20 + (((frame + 10) * 4) % 68)));
    egui_view_activity_ring_set_value(EGUI_VIEW_OF(&ring_2), 1, clamp_percent(14 + (((frame + 20) * 6) % 58)));
    egui_view_activity_ring_set_value(EGUI_VIEW_OF(&ring_2), 2, clamp_percent(10 + (((frame + 30) * 8) % 48)));
}
#endif

// View params
EGUI_VIEW_GRIDLAYOUT_PARAMS_INIT(grid_params, 0, 0, 240, 300, 2, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);

EGUI_VIEW_ACTIVITY_RING_PARAMS_INIT(ring_1_params, 0, 0, 116, 116);
EGUI_VIEW_ACTIVITY_RING_PARAMS_INIT(ring_2_params, 0, 0, 96, 96);

void test_init_ui(void)
{
#if EGUI_EXAMPLE_DIRTY_ANIMATION_CHECK
    dirty_anim_tick = 0;
#endif
    // Init grid
    egui_view_gridlayout_init_with_params(EGUI_VIEW_OF(&grid), &grid_params);

    // Init ring 1: 3 rings (default), values 75/50/30
    egui_view_activity_ring_init_with_params(EGUI_VIEW_OF(&ring_1), &ring_1_params);
    egui_view_activity_ring_set_stroke_width(EGUI_VIEW_OF(&ring_1), 12);
    egui_view_activity_ring_set_ring_gap(EGUI_VIEW_OF(&ring_1), 3);
    egui_view_activity_ring_set_value(EGUI_VIEW_OF(&ring_1), 0, 75);
    egui_view_activity_ring_set_value(EGUI_VIEW_OF(&ring_1), 1, 50);
    egui_view_activity_ring_set_value(EGUI_VIEW_OF(&ring_1), 2, 30);
    egui_view_activity_ring_set_ring_color(EGUI_VIEW_OF(&ring_1), 0, EGUI_COLOR_MAKE(0xEF, 0x44, 0x44));
    egui_view_activity_ring_set_ring_color(EGUI_VIEW_OF(&ring_1), 1, EGUI_COLOR_MAKE(0x10, 0xB9, 0x81));
    egui_view_activity_ring_set_ring_color(EGUI_VIEW_OF(&ring_1), 2, EGUI_COLOR_MAKE(0x38, 0xBD, 0xF8));
    egui_view_activity_ring_set_ring_bg_color(EGUI_VIEW_OF(&ring_1), 0, EGUI_COLOR_MAKE(0x3B, 0x15, 0x15));
    egui_view_activity_ring_set_ring_bg_color(EGUI_VIEW_OF(&ring_1), 1, EGUI_COLOR_MAKE(0x0A, 0x2E, 0x20));
    egui_view_activity_ring_set_ring_bg_color(EGUI_VIEW_OF(&ring_1), 2, EGUI_COLOR_MAKE(0x0E, 0x2F, 0x3E));

    // Init ring 2: 3 rings, values 90/60/20, no round cap
    egui_view_activity_ring_init_with_params(EGUI_VIEW_OF(&ring_2), &ring_2_params);
    egui_view_activity_ring_set_stroke_width(EGUI_VIEW_OF(&ring_2), 10);
    egui_view_activity_ring_set_ring_gap(EGUI_VIEW_OF(&ring_2), 3);
    egui_view_activity_ring_set_value(EGUI_VIEW_OF(&ring_2), 0, 90);
    egui_view_activity_ring_set_value(EGUI_VIEW_OF(&ring_2), 1, 60);
    egui_view_activity_ring_set_value(EGUI_VIEW_OF(&ring_2), 2, 20);
    egui_view_activity_ring_set_show_round_cap(EGUI_VIEW_OF(&ring_2), 0);
    egui_view_activity_ring_set_ring_color(EGUI_VIEW_OF(&ring_2), 0, EGUI_COLOR_MAKE(0xEF, 0x44, 0x44));
    egui_view_activity_ring_set_ring_color(EGUI_VIEW_OF(&ring_2), 1, EGUI_COLOR_MAKE(0x10, 0xB9, 0x81));
    egui_view_activity_ring_set_ring_color(EGUI_VIEW_OF(&ring_2), 2, EGUI_COLOR_MAKE(0x38, 0xBD, 0xF8));
    egui_view_activity_ring_set_ring_bg_color(EGUI_VIEW_OF(&ring_2), 0, EGUI_COLOR_MAKE(0x3B, 0x15, 0x15));
    egui_view_activity_ring_set_ring_bg_color(EGUI_VIEW_OF(&ring_2), 1, EGUI_COLOR_MAKE(0x0A, 0x2E, 0x20));
    egui_view_activity_ring_set_ring_bg_color(EGUI_VIEW_OF(&ring_2), 2, EGUI_COLOR_MAKE(0x0E, 0x2F, 0x3E));

    // Set margins
    egui_view_set_margin_all(EGUI_VIEW_OF(&ring_1), 6);
    egui_view_set_margin_all(EGUI_VIEW_OF(&ring_2), 6);

    // Add children to grid
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&ring_1));
    egui_view_group_add_child(EGUI_VIEW_OF(&grid), EGUI_VIEW_OF(&ring_2));

    // Layout grid children
    egui_view_gridlayout_layout_childs(EGUI_VIEW_OF(&grid));

    // Add grid to root
    egui_core_add_user_root_view(EGUI_VIEW_OF(&grid));

    // Center grid on screen
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);

#if EGUI_EXAMPLE_DIRTY_ANIMATION_CHECK
    egui_timer_init_timer(&dirty_anim_timer, NULL, dirty_anim_timer_callback);
    egui_timer_start_timer(&dirty_anim_timer, 80, 80);
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
