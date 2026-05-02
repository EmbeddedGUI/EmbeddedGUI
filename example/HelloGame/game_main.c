#include "game_app.h"
#include "uicode_disp0.h"

static hello_game_view_t game_view;

hello_game_view_t *hello_game_get_view(void)
{
    return &game_view;
}

void test_init_ui(egui_core_t *core)
{
    hello_game_view_init(&game_view, core, hello_game_get_descriptor());
    egui_core_add_user_root_view(EGUI_VIEW_OF(&game_view));
}

#if EGUI_CONFIG_FUNCTION_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static int last_action = -1;
    int first_call = action_index != last_action;
    hello_game_view_t *view = hello_game_get_view();

    last_action = action_index;

    switch (action_index)
    {
    case 0:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 250);
        return true;
    case 1:
        if (first_call)
        {
            hello_game_view_record_step(view, 0);
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 300);
        return true;
    case 2:
        if (first_call)
        {
            hello_game_view_record_step(view, 1);
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 300);
        return true;
    case 3:
        if (first_call)
        {
            hello_game_view_record_step(view, 2);
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 300);
        return true;
    case 4:
        if (first_call)
        {
            hello_game_view_record_step(view, 3);
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 500);
        return true;
    default:
        return false;
    }
}
#endif
