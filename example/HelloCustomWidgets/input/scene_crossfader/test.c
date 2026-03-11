#include <stdlib.h>

#include "egui.h"
#include "uicode.h"
#include "egui_view_scene_crossfader.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

static egui_view_scene_crossfader_t scene_crossfader;

static const egui_view_scene_crossfader_state_t states[] = {
        {"Aurora Mix", "BLEND", "Pad into neon lead", "A 68 | B 32", "PAD", "LEAD", 32, 7, 5, 0, 0},
        {"Drift Swap", "SWAP", "Keys into tape bass", "A 44 | B 56", "KEYS", "BASS", 56, 5, 8, 1, 1},
        {"Pulse Hold", "HOLD", "Vox bed under arp", "A 78 | B 22", "VOX", "ARP", 22, 8, 4, 2, 2},
        {"Night Cut", "CUT", "FX swell into kick", "A 18 | B 82", "FX", "KICK", 82, 3, 9, 3, 3},
};

void test_init_ui(void)
{
    egui_view_scene_crossfader_init(EGUI_VIEW_OF(&scene_crossfader));
    egui_view_set_size(EGUI_VIEW_OF(&scene_crossfader), 240, 280);
    egui_view_scene_crossfader_set_states(EGUI_VIEW_OF(&scene_crossfader), states, 4);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&scene_crossfader));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static const egui_sim_action_t actions[] = {
            EGUI_SIM_WAIT(400), EGUI_SIM_CLICK(219, 142, 700), EGUI_SIM_WAIT(300), EGUI_SIM_CLICK(21, 142, 700),
            EGUI_SIM_WAIT(300), EGUI_SIM_CLICK(118, 138, 700), EGUI_SIM_WAIT(300), EGUI_SIM_CLICK(219, 142, 700),
            EGUI_SIM_WAIT(300), EGUI_SIM_CLICK(219, 142, 700), EGUI_SIM_WAIT(500), EGUI_SIM_END(),
    };

    if (actions[action_index].type == EGUI_SIM_ACTION_NONE)
    {
        return false;
    }
    *p_action = actions[action_index];
    return true;
}
#endif
