#include <stdlib.h>

#include "egui.h"
#include "uicode.h"
#include "egui_view_tape_loop_editor.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

static egui_view_tape_loop_editor_t tape_loop_editor;

static const egui_view_tape_loop_editor_state_t states[] = {
        {"Lead Drop", "LOOP", "Center loop catches intro", "Loop in 00:12", 1, 0},
        {"Verse Slip", "SLIP", "Left reel trims entry", "Slip -03 fr", 0, 1},
        {"Cut Return", "CUT", "Right reel snaps exit", "Cut out +02", 2, 2},
        {"Cue Catch", "CUE", "Center deck locks marker", "Cue 01:48", 1, 1},
};

void test_init_ui(void)
{
    egui_view_tape_loop_editor_init(EGUI_VIEW_OF(&tape_loop_editor));
    egui_view_set_size(EGUI_VIEW_OF(&tape_loop_editor), 240, 280);
    egui_view_tape_loop_editor_set_states(EGUI_VIEW_OF(&tape_loop_editor), states, 4);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&tape_loop_editor));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static const egui_sim_action_t actions[] = {
            EGUI_SIM_WAIT(400),
            EGUI_SIM_CLICK(202, 146, 700),
            EGUI_SIM_WAIT(300),
            EGUI_SIM_CLICK(38, 146, 700),
            EGUI_SIM_WAIT(300),
            EGUI_SIM_CLICK(120, 136, 700),
            EGUI_SIM_WAIT(300),
            EGUI_SIM_CLICK(202, 146, 700),
            EGUI_SIM_WAIT(300),
            EGUI_SIM_CLICK(202, 146, 700),
            EGUI_SIM_WAIT(500),
            EGUI_SIM_END(),
    };

    if (actions[action_index].type == EGUI_SIM_ACTION_NONE)
    {
        return false;
    }
    *p_action = actions[action_index];
    return true;
}
#endif
