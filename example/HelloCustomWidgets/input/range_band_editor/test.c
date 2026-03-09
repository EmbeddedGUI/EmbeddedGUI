#include <stdlib.h>

#include "egui.h"
#include "uicode.h"
#include "egui_view_range_band_editor.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

static egui_view_range_band_editor_t range_dashboard;

static const egui_view_range_band_editor_snapshot_t primary_snapshots[] = {
        {"RANGE", "LIVE", "Open span wide", "range live", 14, 78, 46, 0},
        {"RANGE", "MIX", "Middle band tight", "mix band", 28, 68, 58, 1},
        {"RANGE", "SAFE", "Guard span calm", "guard safe", 22, 54, 30, 2},
};

static const egui_view_range_band_editor_snapshot_t compact_snapshots[] = {
        {"QUEUE", "SCAN", "Queue span set", "queue ready", 10, 52, 24, 0},
        {"QUEUE", "LOAD", "Load gate set", "load tuned", 24, 72, 48, 1},
        {"QUEUE", "SAFE", "Queue calm", "seal calm", 34, 62, 56, 2},
};

static const egui_view_range_band_editor_snapshot_t locked_snapshots[] = {
        {"AUDIT", "LOCK", "Audit band set", "audit ready", 18, 60, 36, 0},
        {"AUDIT", "HOLD", "Hold limit set", "hold steady", 26, 44, 30, 1},
        {"AUDIT", "SYNC", "Sync band set", "sync safe", 32, 70, 52, 2},
};

void test_init_ui(void)
{
    egui_view_range_band_editor_init(EGUI_VIEW_OF(&range_dashboard));
    egui_view_set_size(EGUI_VIEW_OF(&range_dashboard), 240, 280);
    egui_view_range_band_editor_set_primary_snapshots(EGUI_VIEW_OF(&range_dashboard), primary_snapshots, 3);
    egui_view_range_band_editor_set_compact_snapshots(EGUI_VIEW_OF(&range_dashboard), compact_snapshots, 3);
    egui_view_range_band_editor_set_locked_snapshots(EGUI_VIEW_OF(&range_dashboard), locked_snapshots, 3);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&range_dashboard));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static const egui_sim_action_t actions[] = {
            EGUI_SIM_WAIT(400),
            EGUI_SIM_CLICK(120, 98, 700),
            EGUI_SIM_WAIT(300),
            EGUI_SIM_CLICK(64, 238, 700),
            EGUI_SIM_WAIT(300),
            EGUI_SIM_CLICK(176, 238, 700),
            EGUI_SIM_WAIT(300),
            EGUI_SIM_CLICK(120, 98, 700),
            EGUI_SIM_WAIT(300),
            EGUI_SIM_CLICK(64, 238, 700),
            EGUI_SIM_WAIT(300),
            EGUI_SIM_CLICK(176, 238, 700),
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
