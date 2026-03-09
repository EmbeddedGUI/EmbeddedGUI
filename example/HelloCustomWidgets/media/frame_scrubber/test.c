#include <stdlib.h>

#include "egui.h"
#include "uicode.h"
#include "egui_view_frame_scrubber.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

static egui_view_frame_scrubber_t frame_scrubber;

static const egui_view_frame_scrubber_snapshot_t snapshots[] = {
        {"Edit Reel", "LIVE", "Playhead centered on shot", "live reel", 2, 1, 0},
        {"Color Pass", "SCAN", "Marker jumps to bright scene", "scan cue", 4, 3, 1},
        {"Safe Trim", "SAFE", "Read-only hold keeps clip calm", "safe trim", 1, 1, 2},
        {"Sync Roll", "SYNC", "Preview windows queue next frame", "sync roll", 3, 4, 1},
};

void test_init_ui(void)
{
    egui_view_frame_scrubber_init(EGUI_VIEW_OF(&frame_scrubber));
    egui_view_set_size(EGUI_VIEW_OF(&frame_scrubber), 240, 280);
    egui_view_frame_scrubber_set_snapshots(EGUI_VIEW_OF(&frame_scrubber), snapshots, 4);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&frame_scrubber));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static const egui_sim_action_t actions[] = {
            EGUI_SIM_WAIT(400),
            EGUI_SIM_CLICK(200, 136, 700),
            EGUI_SIM_WAIT(300),
            EGUI_SIM_CLICK(40, 136, 700),
            EGUI_SIM_WAIT(300),
            EGUI_SIM_CLICK(120, 132, 700),
            EGUI_SIM_WAIT(300),
            EGUI_SIM_CLICK(200, 136, 700),
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
