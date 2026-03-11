#include <stdlib.h>

#include "egui.h"
#include "uicode.h"
#include "egui_view_flipdot_matrix_panel.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

static egui_view_flipdot_matrix_panel_t flipdot_matrix_panel;

static const egui_view_flipdot_matrix_panel_state_t states[] = {
        {"Gate North", "LIVE", "North row flashes open", "Queue row 01", 0, 0},
        {"Hold East", "HOLD", "Center row stays dim", "Hold row 02", 1, 1},
        {"Span West", "SCAN", "Lower row rolls left", "Scan lane 03", 2, 2},
        {"Link South", "LINK", "Center row ties board", "Link bus 04", 1, 1},
};

void test_init_ui(void)
{
    egui_view_flipdot_matrix_panel_init(EGUI_VIEW_OF(&flipdot_matrix_panel));
    egui_view_set_size(EGUI_VIEW_OF(&flipdot_matrix_panel), 240, 280);
    egui_view_flipdot_matrix_panel_set_states(EGUI_VIEW_OF(&flipdot_matrix_panel), states, 4);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&flipdot_matrix_panel));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static const egui_sim_action_t actions[] = {
            EGUI_SIM_WAIT(400), EGUI_SIM_CLICK(202, 146, 700), EGUI_SIM_WAIT(300), EGUI_SIM_CLICK(38, 146, 700),
            EGUI_SIM_WAIT(300), EGUI_SIM_CLICK(120, 136, 700), EGUI_SIM_WAIT(300), EGUI_SIM_CLICK(202, 146, 700),
            EGUI_SIM_WAIT(300), EGUI_SIM_CLICK(202, 146, 700), EGUI_SIM_WAIT(500), EGUI_SIM_END(),
    };

    if (actions[action_index].type == EGUI_SIM_ACTION_NONE)
    {
        return false;
    }
    *p_action = actions[action_index];
    return true;
}
#endif
