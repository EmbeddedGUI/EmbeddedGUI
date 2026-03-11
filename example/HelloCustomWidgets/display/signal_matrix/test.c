#include <stdlib.h>

#include "egui.h"
#include "uicode.h"
#include "egui_view_signal_matrix.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

static egui_view_signal_matrix_t signal_dashboard;

static const egui_view_signal_matrix_snapshot_t primary_snapshots[] = {
        {"GRID", "LIVE", "Five lanes in sync", "grid live", {2, 3, 4, 2, 3}, 1, 0},
        {"GRID", "SCAN", "Focus column tracks", "scan sweep", {1, 4, 4, 3, 2}, 2, 1},
        {"GRID", "SAFE", "Guard matrix calm", "guard safe", {1, 1, 2, 1, 1}, 0, 2},
};

static const egui_view_signal_matrix_snapshot_t compact_snapshots[] = {
        {"MESH", "SCAN", "Mesh lane set", "mesh set", {2, 2, 4, 1, 2}, 0, 0},
        {"MESH", "LOAD", "Load cell rise", "load tuned", {3, 4, 2, 1, 3}, 2, 1},
        {"MESH", "SAFE", "Mesh calm", "seal calm", {1, 1, 2, 1, 1}, 1, 2},
};

static const egui_view_signal_matrix_snapshot_t locked_snapshots[] = {
        {"LOCK", "HOLD", "Lock lane set", "lock set", {2, 3, 1, 2, 1}, 1, 0},
        {"LOCK", "SYNC", "Sync cell set", "sync set", {1, 4, 2, 3, 1}, 2, 1},
        {"LOCK", "SAFE", "Audit calm", "audit safe", {1, 1, 1, 2, 1}, 0, 2},
};

void test_init_ui(void)
{
    egui_view_signal_matrix_init(EGUI_VIEW_OF(&signal_dashboard));
    egui_view_set_size(EGUI_VIEW_OF(&signal_dashboard), 240, 280);
    egui_view_signal_matrix_set_primary_snapshots(EGUI_VIEW_OF(&signal_dashboard), primary_snapshots, 3);
    egui_view_signal_matrix_set_compact_snapshots(EGUI_VIEW_OF(&signal_dashboard), compact_snapshots, 3);
    egui_view_signal_matrix_set_locked_snapshots(EGUI_VIEW_OF(&signal_dashboard), locked_snapshots, 3);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&signal_dashboard));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static const egui_sim_action_t actions[] = {
            EGUI_SIM_WAIT(400), EGUI_SIM_CLICK(120, 98, 700),
            EGUI_SIM_WAIT(300), EGUI_SIM_CLICK(64, 240, 700),
            EGUI_SIM_WAIT(300), EGUI_SIM_CLICK(176, 240, 700),
            EGUI_SIM_WAIT(300), EGUI_SIM_CLICK(120, 98, 700),
            EGUI_SIM_WAIT(300), EGUI_SIM_CLICK(64, 240, 700),
            EGUI_SIM_WAIT(300), EGUI_SIM_CLICK(176, 240, 700),
            EGUI_SIM_WAIT(300), EGUI_SIM_CLICK(64, 240, 700),
            EGUI_SIM_WAIT(300), EGUI_SIM_CLICK(176, 240, 700),
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
