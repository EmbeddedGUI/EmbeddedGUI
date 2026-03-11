#include <stdlib.h>

#include "egui.h"
#include "uicode.h"
#include "egui_view_jog_shuttle_wheel.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

static egui_view_jog_shuttle_wheel_t jog_shuttle_wheel;

static const egui_view_jog_shuttle_wheel_mode_t modes[] = {
        {"Frame Jog", "JOG", "Outer ring scans frames", "+12 fr / turn", 2, 0},
        {"Cue Trim", "TRIM", "Edge trim sets cue", "-04 fr tighten", 5, 1},
        {"Loop Edge", "LOOP", "Loop in nudges ahead", "Loop in +02", 0, 2},
        {"Search Fine", "CUE", "Fine search locks hit", "Locate 01:12", 7, 1},
};

void test_init_ui(void)
{
    egui_view_jog_shuttle_wheel_init(EGUI_VIEW_OF(&jog_shuttle_wheel));
    egui_view_set_size(EGUI_VIEW_OF(&jog_shuttle_wheel), 240, 280);
    egui_view_jog_shuttle_wheel_set_modes(EGUI_VIEW_OF(&jog_shuttle_wheel), modes, 4);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&jog_shuttle_wheel));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static const egui_sim_action_t actions[] = {
            EGUI_SIM_WAIT(400), EGUI_SIM_CLICK(200, 146, 700), EGUI_SIM_WAIT(300), EGUI_SIM_CLICK(40, 146, 700),
            EGUI_SIM_WAIT(300), EGUI_SIM_CLICK(120, 134, 700), EGUI_SIM_WAIT(300), EGUI_SIM_CLICK(200, 146, 700),
            EGUI_SIM_WAIT(300), EGUI_SIM_CLICK(200, 146, 700), EGUI_SIM_WAIT(500), EGUI_SIM_END(),
    };

    if (actions[action_index].type == EGUI_SIM_ACTION_NONE)
    {
        return false;
    }
    *p_action = actions[action_index];
    return true;
}
#endif
