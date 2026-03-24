#include "egui.h"
#include "uicode.h"

#include "egui_view_test.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

static egui_view_test_t perf_scene;

void uicode_create_ui(void)
{
    egui_view_test_init(EGUI_VIEW_OF(&perf_scene));
    egui_view_set_position(EGUI_VIEW_OF(&perf_scene), 0, 0);
    egui_view_set_size(EGUI_VIEW_OF(&perf_scene), EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);

    egui_core_add_user_root_view(EGUI_VIEW_OF(&perf_scene));
    egui_view_invalidate(EGUI_VIEW_OF(&perf_scene));
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static int last_action = -1;
    int first_call = (action_index != last_action);
    last_action = action_index;

    switch (action_index)
    {
    case 0:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 200);
        return true;
    case 1:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 900);
        return true;
    case 2:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 900);
        return true;
    case 3:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 900);
        return true;
    default:
        return false;
    }
}
#endif
