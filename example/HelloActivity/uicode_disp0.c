#include "egui.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "core/egui_core_activity.h"
#include "core/egui_core_dialog.h"
#include "core/egui_timer.h"
#include "egui_activity_test.h"
#include "egui_dialog_test.h"
#include "uicode_disp0.h"

#if 1
EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(anim_start_open_param, EGUI_CONFIG_SCEEN_WIDTH, 0, 0, 0);
egui_animation_translate_t anim_start_open;

EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(anim_start_close_param, 0, -EGUI_CONFIG_SCEEN_WIDTH, 0, 0);
egui_animation_translate_t anim_start_close;

EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(anim_finish_open_param, -EGUI_CONFIG_SCEEN_WIDTH, 0, 0, 0);
egui_animation_translate_t anim_finish_open;

EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(anim_finish_close_param, 0, EGUI_CONFIG_SCEEN_WIDTH, 0, 0);
egui_animation_translate_t anim_finish_close;
#else
EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(anim_start_open_param, 0, 0, EGUI_CONFIG_SCEEN_HEIGHT, 0);
egui_animation_translate_t anim_start_open;

EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(anim_start_close_param, 0, 0, 0, -EGUI_CONFIG_SCEEN_HEIGHT);
egui_animation_translate_t anim_start_close;

EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(anim_finish_open_param, 0, 0, -EGUI_CONFIG_SCEEN_HEIGHT, 0);
egui_animation_translate_t anim_finish_open;

EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(anim_finish_close_param, 0, 0, 0, EGUI_CONFIG_SCEEN_HEIGHT);
egui_animation_translate_t anim_finish_close;
#endif

EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(anim_dialog_start_param, 0, 0, EGUI_CONFIG_SCEEN_HEIGHT, 0);
egui_animation_translate_t anim_dialog_start;

EGUI_ANIMATION_TRANSLATE_PARAMS_INIT(anim_dialog_finish_param, 0, 0, 0, -EGUI_CONFIG_SCEEN_HEIGHT);
egui_animation_translate_t anim_dialog_finish;

typedef enum uicode_stress_action
{
    UICODE_STRESS_ACTION_END = 0,
    UICODE_STRESS_ACTION_ACTIVITY_ANIM_ON,
    UICODE_STRESS_ACTION_ACTIVITY_ANIM_OFF,
    UICODE_STRESS_ACTION_DIALOG_ANIM_ON,
    UICODE_STRESS_ACTION_DIALOG_ANIM_OFF,
    UICODE_STRESS_ACTION_START_NEXT_ACTIVITY,
    UICODE_STRESS_ACTION_FINISH_CURRENT_ACTIVITY,
    UICODE_STRESS_ACTION_START_DIALOG,
    UICODE_STRESS_ACTION_FINISH_DIALOG,
    UICODE_STRESS_ACTION_VERIFY_SETTLED,
} uicode_stress_action_t;

typedef struct uicode_stress_step
{
    uint16_t delay_ms;
    uint8_t action;
} uicode_stress_step_t;

static const uicode_stress_step_t activity_stress_steps[] = {
        {0, UICODE_STRESS_ACTION_ACTIVITY_ANIM_ON},         {0, UICODE_STRESS_ACTION_START_NEXT_ACTIVITY},  {60, UICODE_STRESS_ACTION_FINISH_CURRENT_ACTIVITY},
        {60, UICODE_STRESS_ACTION_START_NEXT_ACTIVITY},     {60, UICODE_STRESS_ACTION_START_NEXT_ACTIVITY}, {60, UICODE_STRESS_ACTION_FINISH_CURRENT_ACTIVITY},
        {60, UICODE_STRESS_ACTION_FINISH_CURRENT_ACTIVITY}, {360, UICODE_STRESS_ACTION_ACTIVITY_ANIM_OFF},  {20, UICODE_STRESS_ACTION_START_NEXT_ACTIVITY},
        {20, UICODE_STRESS_ACTION_FINISH_CURRENT_ACTIVITY}, {20, UICODE_STRESS_ACTION_START_NEXT_ACTIVITY}, {20, UICODE_STRESS_ACTION_FINISH_CURRENT_ACTIVITY},
        {120, UICODE_STRESS_ACTION_VERIFY_SETTLED},         {0, UICODE_STRESS_ACTION_ACTIVITY_ANIM_ON},     {0, UICODE_STRESS_ACTION_END},
};

static const uicode_stress_step_t dialog_stress_steps[] = {
        {0, UICODE_STRESS_ACTION_DIALOG_ANIM_ON},
        {0, UICODE_STRESS_ACTION_START_DIALOG},
        {80, UICODE_STRESS_ACTION_FINISH_DIALOG},
        {80, UICODE_STRESS_ACTION_START_DIALOG},
        {40, UICODE_STRESS_ACTION_START_DIALOG},
        {80, UICODE_STRESS_ACTION_FINISH_DIALOG},
        {320, UICODE_STRESS_ACTION_DIALOG_ANIM_OFF},
        {20, UICODE_STRESS_ACTION_START_DIALOG},
        {20, UICODE_STRESS_ACTION_FINISH_DIALOG},
        {20, UICODE_STRESS_ACTION_START_DIALOG},
        {20, UICODE_STRESS_ACTION_FINISH_DIALOG},
        {160, UICODE_STRESS_ACTION_ACTIVITY_ANIM_ON},
        {20, UICODE_STRESS_ACTION_START_NEXT_ACTIVITY},
        {60, UICODE_STRESS_ACTION_START_DIALOG},
        {60, UICODE_STRESS_ACTION_FINISH_DIALOG},
        {60, UICODE_STRESS_ACTION_FINISH_CURRENT_ACTIVITY},
        {360, UICODE_STRESS_ACTION_VERIFY_SETTLED},
        {0, UICODE_STRESS_ACTION_DIALOG_ANIM_ON},
        {0, UICODE_STRESS_ACTION_END},
};

static egui_dialog_test_t dialog;
static egui_toast_std_t toast;
static egui_core_t *s_core;
static egui_timer_t s_stress_timer;
static const uicode_stress_step_t *s_stress_steps;
static uint8_t s_stress_step_index;
static uint8_t s_stress_running;
static uint8_t s_activity_anim_enabled = 1U;
static uint8_t s_dialog_anim_enabled = 1U;

#if EGUI_CONFIG_FUNCTION_RECORDING_TEST
static uint8_t s_runtime_fail_reported;
static uint8_t s_recording_activity_wait_done;
static uint8_t s_recording_dialog_wait_done;
#endif

static int index = 0;

static void uicode_apply_activity_anim_enabled(uint8_t enabled)
{
    s_activity_anim_enabled = enabled;

    if (s_core == NULL)
    {
        return;
    }

    egui_core_activity_set_start_anim(s_core, enabled ? EGUI_ANIM_OF(&anim_start_open) : NULL, enabled ? EGUI_ANIM_OF(&anim_start_close) : NULL);
    egui_core_activity_set_finish_anim(s_core, enabled ? EGUI_ANIM_OF(&anim_finish_open) : NULL, enabled ? EGUI_ANIM_OF(&anim_finish_close) : NULL);
}

static void uicode_apply_dialog_anim_enabled(uint8_t enabled)
{
    s_dialog_anim_enabled = enabled;

    if (s_core == NULL)
    {
        return;
    }

    egui_core_dialog_set_anim(s_core, enabled ? EGUI_ANIM_OF(&anim_dialog_start) : NULL, enabled ? EGUI_ANIM_OF(&anim_dialog_finish) : NULL);
}

static egui_activity_t *uicode_get_current_activity(void)
{
    if (s_core == NULL)
    {
        return NULL;
    }

    return egui_core_activity_get_current_active(s_core);
}

static void uicode_show_status_toast(egui_activity_t *activity, const char *text)
{
    if (activity != NULL)
    {
        egui_activity_show_toast_info(activity, text);
    }
    else if (s_core != NULL)
    {
        egui_toast_show_info((egui_toast_t *)&toast, text);
    }
}

#if EGUI_CONFIG_FUNCTION_RECORDING_TEST
static void uicode_report_runtime_failure(const char *message)
{
    if (s_runtime_fail_reported)
    {
        return;
    }

    s_runtime_fail_reported = 1U;
    printf("[RUNTIME_CHECK_FAIL] %s\n", message);
}
#endif

static void uicode_verify_stress_settled(void)
{
    if (s_core == NULL)
    {
        return;
    }

#if EGUI_CONFIG_FUNCTION_RECORDING_TEST
    if (egui_core_dialog_get(s_core) != NULL)
    {
        uicode_report_runtime_failure("stress scenario left dialog open");
    }
    if (egui_core_activity_get_current_active(s_core) == NULL)
    {
        uicode_report_runtime_failure("stress scenario left activity stack empty");
    }
#endif
}

static void uicode_run_stress_action(uint8_t action)
{
    egui_activity_t *current_activity;
    egui_dialog_t *current_dialog;

    switch (action)
    {
    case UICODE_STRESS_ACTION_ACTIVITY_ANIM_ON:
        uicode_apply_activity_anim_enabled(1U);
        break;
    case UICODE_STRESS_ACTION_ACTIVITY_ANIM_OFF:
        uicode_apply_activity_anim_enabled(0U);
        break;
    case UICODE_STRESS_ACTION_DIALOG_ANIM_ON:
        uicode_apply_dialog_anim_enabled(1U);
        break;
    case UICODE_STRESS_ACTION_DIALOG_ANIM_OFF:
        uicode_apply_dialog_anim_enabled(0U);
        break;
    case UICODE_STRESS_ACTION_START_NEXT_ACTIVITY:
        current_activity = uicode_get_current_activity();
        uicode_start_next_activity(current_activity);
        break;
    case UICODE_STRESS_ACTION_FINISH_CURRENT_ACTIVITY:
        current_activity = uicode_get_current_activity();
        if (current_activity != NULL)
        {
            egui_activity_finish(current_activity);
        }
        break;
    case UICODE_STRESS_ACTION_START_DIALOG:
        current_activity = uicode_get_current_activity();
        if (current_activity != NULL)
        {
            uicode_start_dialog(current_activity);
        }
        break;
    case UICODE_STRESS_ACTION_FINISH_DIALOG:
        if (s_core != NULL)
        {
            current_dialog = egui_core_dialog_get(s_core);
            if (current_dialog != NULL)
            {
                egui_dialog_finish(current_dialog);
            }
        }
        break;
    case UICODE_STRESS_ACTION_VERIFY_SETTLED:
        uicode_verify_stress_settled();
        break;
    default:
        break;
    }
}

static void uicode_stress_timer_callback(egui_timer_t *timer)
{
    EGUI_UNUSED(timer);

    if (!s_stress_running || s_stress_steps == NULL || s_core == NULL)
    {
        return;
    }

    if (s_stress_steps[s_stress_step_index].action == UICODE_STRESS_ACTION_END)
    {
        s_stress_running = 0U;
        s_stress_steps = NULL;
        s_stress_step_index = 0U;
        uicode_apply_activity_anim_enabled(1U);
        uicode_apply_dialog_anim_enabled(1U);
        return;
    }

    uicode_run_stress_action(s_stress_steps[s_stress_step_index].action);
    s_stress_step_index++;

    if (s_stress_steps[s_stress_step_index].action == UICODE_STRESS_ACTION_END)
    {
        egui_timer_start_timer(s_core, &s_stress_timer, s_stress_steps[s_stress_step_index].delay_ms, 0);
        return;
    }

    egui_timer_start_timer(s_core, &s_stress_timer, s_stress_steps[s_stress_step_index].delay_ms, 0);
}

static int uicode_start_stress_sequence(egui_activity_t *activity, const uicode_stress_step_t *steps, const char *label)
{
    if (s_core == NULL || steps == NULL)
    {
        return -1;
    }
    if (s_stress_running)
    {
        uicode_show_status_toast(activity, "Stress already running");
        return -1;
    }

    s_stress_steps = steps;
    s_stress_step_index = 0U;
    s_stress_running = 1U;

    uicode_show_status_toast(activity, label);
    egui_timer_start_timer(s_core, &s_stress_timer, steps[0].delay_ms, 0);
    return 0;
}

int uicode_get_index(void)
{
    return index++;
}

#if TEST_ACTIVITY_DYNAMIC_ALLOC
egui_activity_test_t *uicode_get_empty_activity(egui_core_t *core)
{
    egui_activity_test_t *p_activity = (egui_activity_test_t *)egui_api_malloc(core, sizeof(egui_activity_test_t));

    return p_activity;
}
#else
#define ACTIVITY_ARRAY_SIZE 3
static int activity_index = 0;
egui_activity_test_t activity_test_array[ACTIVITY_ARRAY_SIZE];

egui_activity_test_t *uicode_get_empty_activity(egui_core_t *core)
{
    EGUI_UNUSED(core);
    if (activity_index >= ACTIVITY_ARRAY_SIZE)
    {
        return NULL;
    }
    return &activity_test_array[activity_index++];
}
#endif

int uicode_start_next_activity(egui_activity_t *current_activity)
{
    egui_core_t *core = (current_activity != NULL) ? egui_activity_get_core(current_activity) : egui_toast_get_core((egui_toast_t *)&toast);
    egui_activity_test_t *p_activity = uicode_get_empty_activity(core);
    egui_toast_t *p_toast = (current_activity != NULL) ? egui_activity_get_toast(current_activity) : NULL;

    if (p_activity == NULL)
    {
        if (current_activity != NULL)
        {
            egui_activity_show_toast_info(current_activity, "No more activity available");
        }
        else if (egui_toast_get_core((egui_toast_t *)&toast) != NULL)
        {
            egui_toast_show_info((egui_toast_t *)&toast, "No more activity available");
        }
        return -1;
    }

    if (p_toast != NULL)
    {
        char *p_str = p_toast->api->get_str_buf(p_toast);

        egui_api_sprintf(p_str, "Start activity %d", index);
        egui_toast_show_info(p_toast, p_str);
    }

    egui_activity_test_init((egui_activity_t *)p_activity, core);
    egui_activity_test_set_index((egui_activity_t *)p_activity, uicode_get_index());
    uicode_apply_activity_anim_enabled(s_activity_anim_enabled);
    egui_activity_start((egui_activity_t *)p_activity, current_activity);
    return 0;
}

int uicode_start_dialog(egui_activity_t *activity)
{
    uicode_apply_dialog_anim_enabled(s_dialog_anim_enabled);
    egui_dialog_start((egui_dialog_t *)&dialog, activity);
    return 0;
}

int uicode_start_activity_stress(egui_activity_t *activity)
{
    return uicode_start_stress_sequence(activity, activity_stress_steps, "Run activity stress");
}

int uicode_start_dialog_stress(egui_activity_t *activity)
{
    return uicode_start_stress_sequence(activity, dialog_stress_steps, "Run dialog stress");
}

int uicode_is_stress_running(void)
{
    return s_stress_running;
}

static void uicode_disp0_init_ui(egui_core_t *core)
{
    s_core = core;
    s_stress_steps = NULL;
    s_stress_step_index = 0U;
    s_stress_running = 0U;
    s_activity_anim_enabled = 1U;
    s_dialog_anim_enabled = 1U;
#if EGUI_CONFIG_FUNCTION_RECORDING_TEST
    s_runtime_fail_reported = 0U;
    s_recording_activity_wait_done = 0U;
    s_recording_dialog_wait_done = 0U;
#endif

    // anim_dialog_start
    egui_animation_translate_init(EGUI_ANIM_OF(&anim_dialog_start));
    egui_animation_translate_params_set(&anim_dialog_start, &anim_dialog_start_param);
    egui_animation_duration_set(EGUI_ANIM_OF(&anim_dialog_start), 500);

    // anim_dialog_finish
    egui_animation_translate_init(EGUI_ANIM_OF(&anim_dialog_finish));
    egui_animation_translate_params_set(&anim_dialog_finish, &anim_dialog_finish_param);
    egui_animation_duration_set(EGUI_ANIM_OF(&anim_dialog_finish), 500);
    egui_animation_is_fill_before_set(EGUI_ANIM_OF(&anim_dialog_finish), true);

    // anim_start_open
    egui_animation_translate_init(EGUI_ANIM_OF(&anim_start_open));
    egui_animation_translate_params_set(&anim_start_open, &anim_start_open_param);
    egui_animation_duration_set(EGUI_ANIM_OF(&anim_start_open), 300);

    // anim_start_close
    egui_animation_translate_init(EGUI_ANIM_OF(&anim_start_close));
    egui_animation_translate_params_set(&anim_start_close, &anim_start_close_param);
    egui_animation_duration_set(EGUI_ANIM_OF(&anim_start_close), 300);
    egui_animation_is_fill_before_set(EGUI_ANIM_OF(&anim_start_close), true);

    // anim_finish_open
    egui_animation_translate_init(EGUI_ANIM_OF(&anim_finish_open));
    egui_animation_translate_params_set(&anim_finish_open, &anim_finish_open_param);
    egui_animation_duration_set(EGUI_ANIM_OF(&anim_finish_open), 300);

    // anim_finish_close
    egui_animation_translate_init(EGUI_ANIM_OF(&anim_finish_close));
    egui_animation_translate_params_set(&anim_finish_close, &anim_finish_close_param);
    egui_animation_duration_set(EGUI_ANIM_OF(&anim_finish_close), 300);
    egui_animation_is_fill_before_set(EGUI_ANIM_OF(&anim_finish_close), true);

    egui_timer_init_timer(&s_stress_timer, NULL, uicode_stress_timer_callback);

    // Init toast
    egui_toast_std_init((egui_toast_t *)&toast, core);
    egui_toast_set_as_default((egui_toast_t *)&toast);

    // Init dialog
    egui_dialog_test_init((egui_dialog_t *)&dialog, core);
    uicode_apply_dialog_anim_enabled(1U);
    uicode_apply_activity_anim_enabled(1U);

    // Start activity
    uicode_start_next_activity(NULL);
}

void uicode_disp0_init(egui_core_t *core)
{
    uicode_disp0_init_ui(core);
}

#if EGUI_CONFIG_FUNCTION_RECORDING_TEST
static egui_activity_test_t *uicode_get_current_activity_test(void)
{
    return (egui_activity_test_t *)uicode_get_current_activity();
}

static uint8_t uicode_set_click_current_activity_button(egui_sim_action_t *p_action, uint8_t button_index, uint32_t interval_ms)
{
    egui_activity_test_t *activity = uicode_get_current_activity_test();
    egui_view_t *button_view = NULL;

    if (activity == NULL || p_action == NULL)
    {
        return 0U;
    }

    switch (button_index)
    {
    case 4:
        button_view = EGUI_VIEW_OF(&activity->button_4);
        break;
    case 5:
        button_view = EGUI_VIEW_OF(&activity->button_5);
        break;
    default:
        return 0U;
    }

    EGUI_SIM_SET_CLICK_VIEW(p_action, button_view, interval_ms);
    return 1U;
}

static uint8_t uicode_recording_wait_for_stress_slot(uint8_t *wait_done, egui_sim_action_t *p_action, const char *timeout_message, uint8_t is_last_slot)
{
    if (wait_done == NULL || p_action == NULL)
    {
        return 0U;
    }

    if (*wait_done == 0U)
    {
        if (uicode_is_stress_running())
        {
            if (is_last_slot)
            {
                uicode_report_runtime_failure(timeout_message);
            }
            EGUI_SIM_SET_WAIT(p_action, 80);
            return 1U;
        }

        *wait_done = 1U;
        uicode_verify_stress_settled();
        recording_request_snapshot();
        EGUI_SIM_SET_WAIT(p_action, 180);
        return 1U;
    }

    EGUI_SIM_SET_WAIT(p_action, 50);
    return 1U;
}

bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    enum
    {
        UICODE_RECORD_ACTION_ACTIVITY_WAIT_BEGIN = 2,
        UICODE_RECORD_ACTION_ACTIVITY_WAIT_END = 41,
        UICODE_RECORD_ACTION_DIALOG_CLICK = 42,
        UICODE_RECORD_ACTION_DIALOG_WAIT_BEGIN = 43,
        UICODE_RECORD_ACTION_DIALOG_WAIT_END = 90,
    };
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
        EGUI_SIM_SET_WAIT(p_action, 180);
        return true;
    case 1:
        if (first_call)
        {
            s_recording_activity_wait_done = 0U;
        }
        if (!uicode_set_click_current_activity_button(p_action, 4, 160))
        {
            uicode_report_runtime_failure("activity stress button was not reachable");
            EGUI_SIM_SET_WAIT(p_action, 160);
        }
        return true;
    default:
        break;
    }

    if (action_index >= UICODE_RECORD_ACTION_ACTIVITY_WAIT_BEGIN && action_index <= UICODE_RECORD_ACTION_ACTIVITY_WAIT_END)
    {
        return uicode_recording_wait_for_stress_slot(&s_recording_activity_wait_done, p_action, "activity stress did not settle",
                                                     action_index == UICODE_RECORD_ACTION_ACTIVITY_WAIT_END);
    }

    if (action_index == UICODE_RECORD_ACTION_DIALOG_CLICK)
    {
        egui_activity_t *activity = uicode_get_current_activity();

        if (first_call)
        {
            s_recording_dialog_wait_done = 0U;

            if (activity == NULL)
            {
                uicode_report_runtime_failure("dialog stress activity was not reachable");
            }
            else
            {
                uicode_start_dialog_stress(activity);
                recording_request_snapshot();
            }
        }

        EGUI_SIM_SET_WAIT(p_action, 160);
        return true;
    }

    if (action_index >= UICODE_RECORD_ACTION_DIALOG_WAIT_BEGIN && action_index <= UICODE_RECORD_ACTION_DIALOG_WAIT_END)
    {
        return uicode_recording_wait_for_stress_slot(&s_recording_dialog_wait_done, p_action, "dialog stress did not settle",
                                                     action_index == UICODE_RECORD_ACTION_DIALOG_WAIT_END);
    }

    return false;
}
#endif
