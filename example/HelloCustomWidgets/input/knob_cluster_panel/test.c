#include <stdlib.h>

#include "egui.h"
#include "uicode.h"
#include "egui_view_knob_cluster_panel.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

static egui_view_knob_cluster_panel_t knob_cluster_panel;

static const egui_view_knob_cluster_panel_state_t states[] = {
        {"Drive Bus", "GAIN", "Center knob lifts drive", "+4 dB center", 1, 0},
        {"Air Trim", "AIR", "Right knob opens top", "+2 dB high", 2, 1},
        {"Low Hold", "BASS", "Left knob locks low", "-1 dB low", 0, 2},
        {"Mix Glue", "MIX", "All three settle bus", "Glue 62%", 1, 1},
};

void test_init_ui(void)
{
    egui_view_knob_cluster_panel_init(EGUI_VIEW_OF(&knob_cluster_panel));
    egui_view_set_size(EGUI_VIEW_OF(&knob_cluster_panel), 240, 280);
    egui_view_knob_cluster_panel_set_states(EGUI_VIEW_OF(&knob_cluster_panel), states, 4);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&knob_cluster_panel));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static const egui_sim_action_t actions[] = {
            EGUI_SIM_WAIT(400), EGUI_SIM_CLICK(200, 144, 700), EGUI_SIM_WAIT(300), EGUI_SIM_CLICK(40, 144, 700),
            EGUI_SIM_WAIT(300), EGUI_SIM_CLICK(120, 138, 700), EGUI_SIM_WAIT(300), EGUI_SIM_CLICK(200, 144, 700),
            EGUI_SIM_WAIT(300), EGUI_SIM_CLICK(200, 144, 700), EGUI_SIM_WAIT(500), EGUI_SIM_END(),
    };

    if (actions[action_index].type == EGUI_SIM_ACTION_NONE)
    {
        return false;
    }
    *p_action = actions[action_index];
    return true;
}
#endif
