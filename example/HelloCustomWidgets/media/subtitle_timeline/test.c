#include <stdlib.h>

#include "egui.h"
#include "uicode.h"
#include "egui_view_subtitle_timeline.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

static egui_view_subtitle_timeline_t subtitle_timeline;

static const egui_view_subtitle_timeline_cue_t cues[] = {
        {"Host", "LIVE", "Camera pans across intro", "00:01.2 - intro", 0, 1},
        {"Guest", "SCAN", "Reply subtitle stretches wide", "00:02.6 - reply", 1, 2},
        {"Editor", "SAFE", "Locked cue keeps pause calm", "00:01.6 - hold", 2, 0},
        {"Narrator", "SYNC", "Next cue lands on cut", "00:02.1 - sync", 1, 3},
};

void test_init_ui(void)
{
    egui_view_subtitle_timeline_init(EGUI_VIEW_OF(&subtitle_timeline));
    egui_view_set_size(EGUI_VIEW_OF(&subtitle_timeline), 240, 280);
    egui_view_subtitle_timeline_set_cues(EGUI_VIEW_OF(&subtitle_timeline), cues, 4);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&subtitle_timeline));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static const egui_sim_action_t actions[] = {
            EGUI_SIM_WAIT(400),
            EGUI_SIM_CLICK(200, 138, 700),
            EGUI_SIM_WAIT(300),
            EGUI_SIM_CLICK(40, 138, 700),
            EGUI_SIM_WAIT(300),
            EGUI_SIM_CLICK(120, 134, 700),
            EGUI_SIM_WAIT(300),
            EGUI_SIM_CLICK(200, 138, 700),
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
