#include <stdlib.h>

#include "egui.h"
#include "uicode.h"
#include "egui_view_coverflow_strip.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

static egui_view_coverflow_strip_t coverflow_strip;

static const egui_view_coverflow_strip_snapshot_t snapshots[] = {
        {"Focus Stack", "LIVE", "Active route centered", "live queue", "Back", "Next", 0},
        {"Night Pulse", "SCAN", "Side cards stay recallable", "scan lane", "Queue", "Queue", 1},
        {"Archive Fold", "SAFE", "Locked mode keeps strip calm", "safe shelf", "Hold", "Hold", 2},
        {"Signal Deck", "SYNC", "Tap center to advance", "sync deck", "Link", "Link", 1},
};

void test_init_ui(void)
{
    egui_view_coverflow_strip_init(EGUI_VIEW_OF(&coverflow_strip));
    egui_view_set_size(EGUI_VIEW_OF(&coverflow_strip), 240, 280);
    egui_view_coverflow_strip_set_snapshots(EGUI_VIEW_OF(&coverflow_strip), snapshots, 4);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&coverflow_strip));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static const egui_sim_action_t actions[] = {
            EGUI_SIM_WAIT(400), EGUI_SIM_CLICK(196, 126, 700),
            EGUI_SIM_WAIT(300), EGUI_SIM_CLICK(44, 126, 700),
            EGUI_SIM_WAIT(300), EGUI_SIM_CLICK(120, 136, 700),
            EGUI_SIM_WAIT(300), EGUI_SIM_CLICK(196, 126, 700),
            EGUI_SIM_WAIT(500), EGUI_SIM_END(),
    };

    if (actions[action_index].type == EGUI_SIM_ACTION_NONE)
    {
        return false;
    }
    *p_action = actions[action_index];
    return true;
}
#endif
