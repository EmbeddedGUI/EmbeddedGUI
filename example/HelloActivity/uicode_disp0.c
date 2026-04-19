#include "egui.h"
#include <stdlib.h>
#include <math.h>
#include "uicode_disp0.h"
#include "egui_activity_test.h"
#include "egui_dialog_test.h"

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

static egui_dialog_test_t dialog;

static egui_toast_std_t toast;

static int index = 0;
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
    egui_activity_set_start_anim((egui_activity_t *)p_activity, EGUI_ANIM_OF(&anim_start_open), EGUI_ANIM_OF(&anim_start_close));
    egui_activity_set_finish_anim((egui_activity_t *)p_activity, EGUI_ANIM_OF(&anim_finish_open), EGUI_ANIM_OF(&anim_finish_close));
    egui_activity_start((egui_activity_t *)p_activity, current_activity);
    return 0;
}

int uicode_start_dialog(egui_activity_t *activity)
{
    egui_dialog_start((egui_dialog_t *)&dialog, activity);
    return 0;
}

static void uicode_disp0_init_ui(egui_core_t *core)
{
    // anim_dialog_start
    egui_animation_translate_init(EGUI_ANIM_OF(&anim_dialog_start));
    egui_animation_translate_params_set(&anim_dialog_start, &anim_dialog_start_param);
    egui_animation_duration_set(EGUI_ANIM_OF(&anim_dialog_start), 500);

    // anim_dialog_finish
    egui_animation_translate_init(EGUI_ANIM_OF(&anim_dialog_finish));
    egui_animation_translate_params_set(&anim_dialog_finish, &anim_dialog_finish_param);
    egui_animation_duration_set(EGUI_ANIM_OF(&anim_dialog_finish), 500);
    egui_animation_is_fill_before_set(EGUI_ANIM_OF(&anim_dialog_finish), true);

    // Init toast
    egui_toast_std_init((egui_toast_t *)&toast, core);
    egui_toast_set_as_default((egui_toast_t *)&toast);

    // Init dialog
    egui_dialog_test_init((egui_dialog_t *)&dialog, core);
    // Set dialog start/finish animation
    egui_dialog_set_anim((egui_dialog_t *)&dialog, EGUI_ANIM_OF(&anim_dialog_start), EGUI_ANIM_OF(&anim_dialog_finish));

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

    // Start activity
    uicode_start_next_activity(NULL);
}

void uicode_disp0_init(egui_core_t *core)
{
    uicode_disp0_init_ui(core);
}

#if EGUI_CONFIG_RECORDING_TEST
// Recording actions: navigate activities and open dialog
// Activity transitions have 300ms animation, 500ms interval ensures completion
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    switch (action_index)
    {
    case 0:
        EGUI_SIM_SET_WAIT(p_action, 500);
        return true;
    case 1:
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y1 = EGUI_CONFIG_SCEEN_HEIGHT / 2 - 20;
        p_action->interval_ms = 500;
        return true;
    case 2:
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y1 = EGUI_CONFIG_SCEEN_HEIGHT / 2 - 20;
        p_action->interval_ms = 500;
        return true;
    case 3:
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y1 = EGUI_CONFIG_SCEEN_HEIGHT / 2 + 60;
        p_action->interval_ms = 500;
        return true;
    case 4:
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y1 = EGUI_CONFIG_SCEEN_HEIGHT / 2 + 20;
        p_action->interval_ms = 500;
        return true;
    case 5:
        p_action->type = EGUI_SIM_ACTION_CLICK;
        p_action->x1 = EGUI_CONFIG_SCEEN_WIDTH / 2;
        p_action->y1 = EGUI_CONFIG_SCEEN_HEIGHT / 2 + 20;
        p_action->interval_ms = 500;
        return true;
    default:
        return false;
    }
}
#endif
