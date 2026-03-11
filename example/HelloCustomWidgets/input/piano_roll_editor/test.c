#include <stdlib.h>

#include "egui.h"
#include "uicode.h"
#include "egui_view_piano_roll_editor.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

static egui_view_piano_roll_editor_t piano_roll_editor;

static const egui_view_piano_roll_editor_state_t states[] = {
        {"Glass Arp", "LATCH", "C4 lane holds 2 bars", "Loop 01-02", 1, 1, 0, 2, 0},
        {"Dust Chord", "DUB", "Stacked notes punch in", "Punch 02-04", 2, 3, 1, 3, 1},
        {"Neon Lead", "SCAN", "High note rolls ahead", "Scan 03-04", 0, 4, 2, 2, 2},
        {"Velvet Bass", "TRIM", "Lower lane nudges late", "Trim 01-03", 4, 2, 0, 3, 3},
};

void test_init_ui(void)
{
    egui_view_piano_roll_editor_init(EGUI_VIEW_OF(&piano_roll_editor));
    egui_view_set_size(EGUI_VIEW_OF(&piano_roll_editor), 240, 280);
    egui_view_piano_roll_editor_set_states(EGUI_VIEW_OF(&piano_roll_editor), states, 4);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&piano_roll_editor));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static const egui_sim_action_t actions[] = {
            EGUI_SIM_WAIT(400), EGUI_SIM_CLICK(218, 143, 700), EGUI_SIM_WAIT(300), EGUI_SIM_CLICK(20, 143, 700),
            EGUI_SIM_WAIT(300), EGUI_SIM_CLICK(118, 136, 700), EGUI_SIM_WAIT(300), EGUI_SIM_CLICK(218, 143, 700),
            EGUI_SIM_WAIT(300), EGUI_SIM_CLICK(218, 143, 700), EGUI_SIM_WAIT(500), EGUI_SIM_END(),
    };

    if (actions[action_index].type == EGUI_SIM_ACTION_NONE)
    {
        return false;
    }
    *p_action = actions[action_index];
    return true;
}
#endif
