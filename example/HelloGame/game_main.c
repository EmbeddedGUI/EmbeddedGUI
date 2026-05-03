#include "game_app.h"
#include "uicode_disp0.h"

#define HG_RECORD_RESET_X         (EGUI_CONFIG_SCREEN_WIDTH - 82)
#define HG_RECORD_BUTTON_Y        19
#define HG_RECORD_PAUSE_X         (EGUI_CONFIG_SCREEN_WIDTH - 34)
#define HG_RECORD_CLICK_DELAY_MS  80
#define HG_RECORD_ACTION_DELAY_MS 220

#if EGUI_CONFIG_FUNCTION_RECORDING_TEST && EGUI_CONFIG_FUNCTION_SUPPORT_KEY && EGUI_PORT == EGUI_PORT_TYPE_PC
#include "sdl_port.h"
#endif

static hello_game_view_t game_view;

#if EGUI_CONFIG_FUNCTION_RECORDING_TEST
static void hello_game_record_click(egui_sim_action_t *p_action, int x, int y, int interval_ms)
{
    p_action->type = EGUI_SIM_ACTION_CLICK;
    p_action->x1 = x;
    p_action->y1 = y;
    p_action->x2 = 0;
    p_action->y2 = 0;
    p_action->steps = 0;
    p_action->interval_ms = interval_ms;
    p_action->display_id = 0;
}
#endif

hello_game_view_t *hello_game_get_view(void)
{
    return &game_view;
}

void test_init_ui(egui_core_t *core)
{
    hello_game_view_init(&game_view, core, hello_game_get_descriptor());
    egui_core_add_user_root_view(EGUI_VIEW_OF(&game_view));
}

#if EGUI_CONFIG_FUNCTION_RECORDING_TEST && EGUI_CONFIG_FUNCTION_SUPPORT_KEY
static void hello_game_record_key_dispatch(egui_core_t *core, uintptr_t user_data)
{
    uint8_t key_code = (uint8_t)user_data;

    if (core == NULL)
    {
        return;
    }

    egui_input_add_key(core, EGUI_KEY_EVENT_ACTION_DOWN, key_code, 0, 0);
    egui_input_add_key(core, EGUI_KEY_EVENT_ACTION_UP, key_code, 0, 0);
}

static void hello_game_record_key(hello_game_view_t *view, uint8_t key_code)
{
    egui_core_t *core;

    if (view == NULL)
    {
        return;
    }

    core = egui_view_get_core(EGUI_VIEW_OF(view));
    if (core == NULL)
    {
        return;
    }

#if EGUI_PORT == EGUI_PORT_TYPE_PC
    egui_port_post_core_task_sync(core, hello_game_record_key_dispatch, (uintptr_t)key_code, 200);
#else
    egui_input_add_key(core, EGUI_KEY_EVENT_ACTION_DOWN, key_code, 0, 0);
    egui_input_add_key(core, EGUI_KEY_EVENT_ACTION_UP, key_code, 0, 0);
#endif
}
#endif

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
        hello_game_record_click(p_action, HG_RECORD_PAUSE_X, HG_RECORD_BUTTON_Y, HG_RECORD_CLICK_DELAY_MS);
        return true;
    case 1:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, HG_RECORD_ACTION_DELAY_MS);
        return true;
    case 2:
        hello_game_record_click(p_action, HG_RECORD_PAUSE_X, HG_RECORD_BUTTON_Y, HG_RECORD_CLICK_DELAY_MS);
        return true;
    case 3:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, HG_RECORD_ACTION_DELAY_MS);
        return true;
    case 4:
        if (first_call)
        {
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
            hello_game_record_key(view, EGUI_KEY_CODE_RIGHT);
#else
            hello_game_view_record_step(view, 0);
#endif
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, HG_RECORD_ACTION_DELAY_MS);
        return true;
    case 5:
        if (first_call)
        {
            hello_game_view_record_step(view, 0);
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, HG_RECORD_ACTION_DELAY_MS);
        return true;
    case 6:
        if (first_call)
        {
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
            hello_game_record_key(view, EGUI_KEY_CODE_SPACE);
#else
            hello_game_view_record_step(view, 1);
#endif
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, HG_RECORD_ACTION_DELAY_MS);
        return true;
    case 7:
        if (first_call)
        {
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
            hello_game_record_key(view, EGUI_KEY_CODE_DOWN);
#else
            hello_game_view_record_step(view, 2);
#endif
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, HG_RECORD_ACTION_DELAY_MS);
        return true;
    case 8:
        if (first_call)
        {
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
            hello_game_record_key(view, EGUI_KEY_CODE_SPACE);
            hello_game_record_key(view, EGUI_KEY_CODE_LEFT);
#else
            hello_game_view_record_step(view, 3);
#endif
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, HG_RECORD_ACTION_DELAY_MS);
        return true;
    case 9:
        hello_game_record_click(p_action, HG_RECORD_PAUSE_X, HG_RECORD_BUTTON_Y, HG_RECORD_CLICK_DELAY_MS);
        return true;
    case 10:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, HG_RECORD_ACTION_DELAY_MS);
        return true;
    case 11:
        if (first_call)
        {
            hello_game_view_record_step(view, 1);
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, HG_RECORD_ACTION_DELAY_MS);
        return true;
    case 12:
        hello_game_record_click(p_action, HG_RECORD_PAUSE_X, HG_RECORD_BUTTON_Y, HG_RECORD_CLICK_DELAY_MS);
        return true;
    case 13:
        if (first_call)
        {
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, HG_RECORD_ACTION_DELAY_MS);
        return true;
    case 14:
        if (first_call)
        {
            hello_game_view_record_step(view, 2);
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, HG_RECORD_ACTION_DELAY_MS);
        return true;
    case 15:
        hello_game_record_click(p_action, HG_RECORD_RESET_X, HG_RECORD_BUTTON_Y, HG_RECORD_ACTION_DELAY_MS);
        return true;
    case 16:
        if (first_call)
        {
#if EGUI_CONFIG_FUNCTION_SUPPORT_KEY
            hello_game_record_key(view, EGUI_KEY_CODE_R);
#else
            hello_game_view_record_step(view, 2);
#endif
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 500);
        return true;
    default:
        return false;
    }
}
#endif
