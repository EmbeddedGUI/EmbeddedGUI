#include <stdlib.h>

#include "egui.h"
#include "uicode.h"
#include "egui_view_sankey_flow.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

static egui_view_sankey_flow_t sankey_dashboard;

static const egui_view_sankey_flow_snapshot_t primary_snapshots[] = {
        {"PIPELINE", "LIVE", "Source lanes push ahead", "primary route live", 0, 0, 0, 0},
        {"PIPELINE", "MIX", "Middle stage bends heavy", "bridge mix set", 1, 1, 1, 0},
        {"PIPELINE", "SAFE", "Split export stays calm", "seal flow safe", 2, 2, 2, 0},
};

static const egui_view_sankey_flow_snapshot_t compact_snapshots[] = {
        {"QUEUE", "SCAN", "Queue lanes open", "route set", 1, 0, 0, 1},
        {"QUEUE", "LOAD", "Mix lane gains", "mix tuned", 3, 1, 1, 1},
        {"QUEUE", "SAFE", "Queue flow rests", "seal calm", 2, 2, 2, 1},
};

static const egui_view_sankey_flow_snapshot_t locked_snapshots[] = {
        {"AUDIT", "LOCK", "Review route parked", "audit set", 3, 0, 0, 1},
        {"AUDIT", "HOLD", "Gate lane waits", "hold steady", 0, 1, 1, 1},
        {"AUDIT", "SYNC", "Export bins aligned", "sync safe", 1, 2, 2, 1},
};

void test_init_ui(void)
{
    egui_view_sankey_flow_init(EGUI_VIEW_OF(&sankey_dashboard));
    egui_view_set_size(EGUI_VIEW_OF(&sankey_dashboard), 240, 280);
    egui_view_sankey_flow_set_primary_snapshots(EGUI_VIEW_OF(&sankey_dashboard), primary_snapshots, 3);
    egui_view_sankey_flow_set_compact_snapshots(EGUI_VIEW_OF(&sankey_dashboard), compact_snapshots, 3);
    egui_view_sankey_flow_set_locked_snapshots(EGUI_VIEW_OF(&sankey_dashboard), locked_snapshots, 3);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&sankey_dashboard));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static const egui_sim_action_t actions[] = {
            EGUI_SIM_WAIT(400),
            EGUI_SIM_CLICK(120, 95, 700),
            EGUI_SIM_WAIT(300),
            EGUI_SIM_CLICK(64, 234, 700),
            EGUI_SIM_WAIT(300),
            EGUI_SIM_CLICK(176, 234, 700),
            EGUI_SIM_WAIT(300),
            EGUI_SIM_CLICK(120, 95, 700),
            EGUI_SIM_WAIT(300),
            EGUI_SIM_CLICK(64, 234, 700),
            EGUI_SIM_WAIT(300),
            EGUI_SIM_CLICK(176, 234, 700),
            EGUI_SIM_WAIT(800),
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
