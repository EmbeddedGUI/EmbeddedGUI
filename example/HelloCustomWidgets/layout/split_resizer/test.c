#include <stdlib.h>

#include "egui.h"
#include "uicode.h"
#include "egui_view_split_resizer.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

static egui_view_split_resizer_t resizer_dashboard;

static const egui_view_split_resizer_snapshot_t primary_snapshots[] = {
        {"SPLIT", "EDIT", "Three panes ready", "primary pane live", 46, 30, 0, 0},
        {"SPLIT", "FOCUS", "Center pane tuned", "focus pane lock", 32, 38, 1, 0},
        {"SPLIT", "SAFE", "Balanced panes calm", "layout seal safe", 34, 33, 2, 0},
};

static const egui_view_split_resizer_snapshot_t column_snapshots[] = {
        {"COL", "STACK", "Column set", "column group", 42, 28, 0, 1},
        {"COL", "SCAN", "Tall panes", "scan group", 28, 34, 1, 1},
        {"COL", "SAFE", "Column calm", "seal stack", 34, 22, 2, 1},
};

static const egui_view_split_resizer_snapshot_t locked_snapshots[] = {
        {"GRID", "LIVE", "Dock set", "render grid", 40, 28, 0, 0},
        {"GRID", "WARN", "Queue panes", "guard split", 30, 34, 1, 0},
        {"GRID", "LOCK", "Export calm", "archive split", 34, 26, 2, 0},
};

void test_init_ui(void)
{
    egui_view_split_resizer_init(EGUI_VIEW_OF(&resizer_dashboard));
    egui_view_set_size(EGUI_VIEW_OF(&resizer_dashboard), 240, 280);
    egui_view_split_resizer_set_primary_snapshots(EGUI_VIEW_OF(&resizer_dashboard), primary_snapshots, 3);
    egui_view_split_resizer_set_column_snapshots(EGUI_VIEW_OF(&resizer_dashboard), column_snapshots, 3);
    egui_view_split_resizer_set_locked_snapshots(EGUI_VIEW_OF(&resizer_dashboard), locked_snapshots, 3);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&resizer_dashboard));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static const egui_sim_action_t actions[] = {
            EGUI_SIM_WAIT(400), EGUI_SIM_CLICK(120, 95, 700),
            EGUI_SIM_WAIT(300), EGUI_SIM_CLICK(64, 234, 700),
            EGUI_SIM_WAIT(300), EGUI_SIM_CLICK(176, 234, 700),
            EGUI_SIM_WAIT(300), EGUI_SIM_CLICK(120, 95, 700),
            EGUI_SIM_WAIT(300), EGUI_SIM_CLICK(64, 234, 700),
            EGUI_SIM_WAIT(300), EGUI_SIM_CLICK(176, 234, 700),
            EGUI_SIM_WAIT(800), EGUI_SIM_END(),
    };

    if (actions[action_index].type == EGUI_SIM_ACTION_NONE)
    {
        return false;
    }
    *p_action = actions[action_index];
    return true;
}
#endif
