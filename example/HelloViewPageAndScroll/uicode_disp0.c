#include "egui.h"
#include <stdlib.h>
#include <math.h>
#include "uicode_disp0.h"
#include "uicode_viewpage.h"
#include "uicode_scroll.h"

static egui_view_t *uicode_disp0_init_ui(egui_core_t *core)
{
    egui_view_t *root = uicode_disp0_init_viewpage(core);

    // Add To Root
    egui_core_add_user_root_view(root);

    return root;
}

static egui_timer_t ui_timer;
void egui_view_test_timer_callback(egui_timer_t *timer)
{
}

void uicode_disp0_init(egui_core_t *core)
{
    egui_view_t *root = uicode_disp0_init_ui(core);

    ui_timer.callback = egui_view_test_timer_callback;
    egui_view_start_timer(root, &ui_timer, 1000, 1000);
}

#if EGUI_CONFIG_RECORDING_TEST
// Recording actions: swipe through viewpage items and back
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    switch (action_index)
    {
    case 0:
        EGUI_SIM_SET_WAIT(p_action, 500);
        return true;
    case 1:
    case 2:
    case 3:
    case 4:
        p_action->type = EGUI_SIM_ACTION_SWIPE;
        p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH * 3 / 4;
        p_action->y1 = EGUI_CONFIG_SCEEN_HEIGHT / 2;
        p_action->x2 = EGUI_CONFIG_SCEEN_WIDTH / 4;
        p_action->y2 = EGUI_CONFIG_SCEEN_HEIGHT / 2;
        p_action->steps = 5;
        p_action->interval_ms = 800;
        return true;
    case 5:
    case 6:
        p_action->type = EGUI_SIM_ACTION_SWIPE;
        p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH / 4;
        p_action->y1 = EGUI_CONFIG_SCEEN_HEIGHT / 2;
        p_action->x2 = EGUI_CONFIG_SCEEN_WIDTH * 3 / 4;
        p_action->y2 = EGUI_CONFIG_SCEEN_HEIGHT / 2;
        p_action->steps = 5;
        p_action->interval_ms = 800;
        return true;
    default:
        return false;
    }
}
#endif
