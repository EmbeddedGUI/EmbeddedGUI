#include <stdlib.h>

#include "egui.h"
#include "uicode.h"
#include "egui_view_envelope_stage_editor.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

static egui_view_envelope_stage_editor_t envelope_stage_editor;

static const egui_view_envelope_stage_editor_state_t states[] = {
        {"Glass Pad", "SNAP", "Fast rise, soft decay", "Env A 24ms", 0, {7, 4, 3, 5}, 0},
        {"Dust Pluck", "DUB", "Short hit, low sustain", "Env B 12ms", 1, {8, 2, 1, 3}, 1},
        {"Neon Bass", "LOOP", "Flat sustain, long tail", "Env C loop", 2, {5, 3, 4, 6}, 2},
        {"Velvet Vox", "TRIM", "Slow attack, soft drop", "Env D 48ms", 3, {6, 5, 2, 7}, 3},
};

void test_init_ui(void)
{
    egui_view_envelope_stage_editor_init(EGUI_VIEW_OF(&envelope_stage_editor));
    egui_view_set_size(EGUI_VIEW_OF(&envelope_stage_editor), 240, 280);
    egui_view_envelope_stage_editor_set_states(EGUI_VIEW_OF(&envelope_stage_editor), states, 4);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&envelope_stage_editor));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static const egui_sim_action_t actions[] = {
            EGUI_SIM_WAIT(400),
            EGUI_SIM_CLICK(219, 142, 700),
            EGUI_SIM_WAIT(300),
            EGUI_SIM_CLICK(21, 142, 700),
            EGUI_SIM_WAIT(300),
            EGUI_SIM_CLICK(118, 136, 700),
            EGUI_SIM_WAIT(300),
            EGUI_SIM_CLICK(219, 142, 700),
            EGUI_SIM_WAIT(300),
            EGUI_SIM_CLICK(219, 142, 700),
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
