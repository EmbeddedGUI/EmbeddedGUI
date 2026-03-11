#include <stdlib.h>

#include "egui.h"
#include "uicode.h"
#include "egui_view_equalizer_curve_editor.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

static egui_view_equalizer_curve_editor_t equalizer_curve_editor;

static const egui_view_equalizer_curve_editor_band_t bands[] = {
        {"Air Shelf", "LIVE", "Top shelf opens air", "+3 dB @ 8k", {0, 1, 2, 3, 4}, 3, 0},
        {"Vocal Dip", "SCAN", "Mid cut clears voice", "-2 dB @ 1k", {1, -1, -2, 0, 1}, 2, 1},
        {"Bass Hold", "SAFE", "Low end stays flat", "+1 dB @ 120", {2, 1, 0, 0, -1}, 0, 2},
        {"Presence", "SYNC", "Upper mids step up", "+2 dB @ 3k", {0, 1, 3, 2, 1}, 2, 1},
};

void test_init_ui(void)
{
    egui_view_equalizer_curve_editor_init(EGUI_VIEW_OF(&equalizer_curve_editor));
    egui_view_set_size(EGUI_VIEW_OF(&equalizer_curve_editor), 240, 280);
    egui_view_equalizer_curve_editor_set_bands(EGUI_VIEW_OF(&equalizer_curve_editor), bands, 4);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&equalizer_curve_editor));
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_HCENTER | EGUI_ALIGN_VCENTER);
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static const egui_sim_action_t actions[] = {
            EGUI_SIM_WAIT(400), EGUI_SIM_CLICK(200, 138, 700), EGUI_SIM_WAIT(300), EGUI_SIM_CLICK(40, 138, 700),
            EGUI_SIM_WAIT(300), EGUI_SIM_CLICK(120, 134, 700), EGUI_SIM_WAIT(300), EGUI_SIM_CLICK(200, 138, 700),
            EGUI_SIM_WAIT(300), EGUI_SIM_CLICK(200, 138, 700), EGUI_SIM_WAIT(500), EGUI_SIM_END(),
    };

    if (actions[action_index].type == EGUI_SIM_ACTION_NONE)
    {
        return false;
    }
    *p_action = actions[action_index];
    return true;
}
#endif
