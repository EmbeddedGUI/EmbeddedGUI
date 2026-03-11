#include <stdlib.h>

#include "egui.h"
#include "uicode.h"
#include "egui_view_conversion_funnel.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

static egui_view_conversion_funnel_t funnel_dashboard;

static const egui_view_conversion_funnel_snapshot_t primary_snapshots[] = {
        {"FUNNEL", "LIVE", "Acquire stage wide", "live focus", 0, 0, 0, 0},
        {"FUNNEL", "MIX", "Consider stage bends", "mix bend", 1, 1, 1, 0},
        {"FUNNEL", "SAFE", "Close stage calm", "seal safe", 2, 2, 2, 0},
};

static const egui_view_conversion_funnel_snapshot_t compact_snapshots[] = {
        {"QUEUE", "SCAN", "Queue top open", "queue ready", 3, 0, 0, 1},
        {"QUEUE", "LOAD", "Middle drop set", "load tuned", 1, 1, 1, 1},
        {"QUEUE", "SAFE", "Queue close calm", "seal calm", 2, 2, 2, 1},
};

static const egui_view_conversion_funnel_snapshot_t locked_snapshots[] = {
        {"AUDIT", "LOCK", "Audit top set", "audit ready", 0, 0, 0, 1},
        {"AUDIT", "HOLD", "Hold band set", "hold steady", 3, 1, 1, 1},
        {"AUDIT", "SYNC", "Sync close set", "sync safe", 1, 2, 2, 1},
};

void test_init_ui(void)
{
    egui_view_conversion_funnel_init(EGUI_VIEW_OF(&funnel_dashboard));
    egui_view_set_size(EGUI_VIEW_OF(&funnel_dashboard), 240, 280);
    egui_view_conversion_funnel_set_primary_snapshots(EGUI_VIEW_OF(&funnel_dashboard), primary_snapshots, 3);
    egui_view_conversion_funnel_set_compact_snapshots(EGUI_VIEW_OF(&funnel_dashboard), compact_snapshots, 3);
    egui_view_conversion_funnel_set_locked_snapshots(EGUI_VIEW_OF(&funnel_dashboard), locked_snapshots, 3);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&funnel_dashboard));
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
