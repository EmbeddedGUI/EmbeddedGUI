#include <stdlib.h>

#include "egui.h"
#include "uicode.h"
#include "egui_view_priority_matrix.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

static egui_view_priority_matrix_t priority_matrix_dashboard;

static const egui_view_priority_matrix_snapshot_t primary_snapshots[] = {
        {"TRIAGE", "NOW", "Urgent lane owns focus", "priority lane live", 0, 0, 0, 0},
        {"TRIAGE", "PLAN", "Schedule lane leads next", "plan sweep set", 1, 1, 1, 1},
        {"TRIAGE", "HAND", "Delegate lane stays calm", "handoff matrix safe", 2, 3, 2, 2},
};

static const egui_view_priority_matrix_snapshot_t compact_snapshots[] = {
        {"TEAM", "SCAN", "Cluster scan open", "compact scan set", 2, 2, 0, 2},
        {"TEAM", "LOAD", "Hand-off queue fills", "compact load tuned", 1, 3, 1, 1},
        {"TEAM", "SAFE", "Grouped lane settled", "compact seal set", 0, 0, 2, 0},
};

static const egui_view_priority_matrix_snapshot_t locked_snapshots[] = {
        {"ARCH", "LOCK", "Review grid parked", "locked board set", 3, 4, 0, 3},
        {"ARCH", "HOLD", "Backlog hold visible", "locked hold steady", 1, 1, 1, 1},
        {"ARCH", "SYNC", "Archive bins aligned", "locked sync safe", 2, 2, 2, 2},
};

void test_init_ui(void)
{
    egui_view_priority_matrix_init(EGUI_VIEW_OF(&priority_matrix_dashboard));
    egui_view_set_size(EGUI_VIEW_OF(&priority_matrix_dashboard), 240, 280);
    egui_view_priority_matrix_set_primary_snapshots(EGUI_VIEW_OF(&priority_matrix_dashboard), primary_snapshots, 3);
    egui_view_priority_matrix_set_compact_snapshots(EGUI_VIEW_OF(&priority_matrix_dashboard), compact_snapshots, 3);
    egui_view_priority_matrix_set_locked_snapshots(EGUI_VIEW_OF(&priority_matrix_dashboard), locked_snapshots, 3);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&priority_matrix_dashboard));
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
