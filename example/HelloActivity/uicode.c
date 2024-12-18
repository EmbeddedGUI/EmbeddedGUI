#include "egui.h"
#include <stdlib.h>
#include <math.h>
#include "uicode.h"
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
egui_activity_test_t *uicode_get_empty_activity(void)
{
    egui_activity_test_t *p_activity = (egui_activity_test_t *)egui_api_malloc(sizeof(egui_activity_test_t));

    return p_activity;
}
#else
#define ACTIVITY_ARRAY_SIZE 3
static int activity_index = 0;
egui_activity_test_t activity_test_array[ACTIVITY_ARRAY_SIZE];

egui_activity_test_t *uicode_get_empty_activity(void)
{
    if (activity_index >= ACTIVITY_ARRAY_SIZE)
    {
        return NULL;
    }
    return &activity_test_array[activity_index++];
}
#endif

int uicode_start_next_activity(egui_activity_t *current_activity)
{
    egui_activity_test_t *p_activity = uicode_get_empty_activity();
    if (p_activity == NULL)
    {
        egui_core_toast_show_info("No more activity available");
        return -1;
    }

    egui_toast_t *p_toast = egui_core_toast_get();
    if (p_toast != NULL)
    {
        char *p_str = p_toast->api->get_str_buf(p_toast);

        egui_api_sprintf(p_str, "Start activity %d", index);
        egui_core_toast_show_info(p_str);
    }

    egui_activity_test_init((egui_activity_t *)p_activity);
    egui_activity_test_set_index((egui_activity_t *)p_activity, uicode_get_index());

    egui_core_activity_start((egui_activity_t *)p_activity, current_activity);
    return 0;
}

int uicode_start_dialog(egui_activity_t *activity)
{
    egui_core_dialog_start(activity, (egui_dialog_t *)&dialog);
    return 0;
}

void uicode_init_ui(void)
{
    // anim_dialog_start
    egui_animation_translate_init((egui_animation_t *)&anim_dialog_start);
    egui_animation_translate_params_set(&anim_dialog_start, &anim_dialog_start_param);
    egui_animation_duration_set((egui_animation_t *)&anim_dialog_start, 500);

    // anim_dialog_finish
    egui_animation_translate_init((egui_animation_t *)&anim_dialog_finish);
    egui_animation_translate_params_set(&anim_dialog_finish, &anim_dialog_finish_param);
    egui_animation_duration_set((egui_animation_t *)&anim_dialog_finish, 500);
    egui_animation_is_fill_before_set((egui_animation_t *)&anim_dialog_finish, true);

    // Init dialog
    egui_dialog_test_init((egui_dialog_t *)&dialog);
    // Set dialog start/finish animation
    egui_core_dialog_set_anim((egui_animation_t *)&anim_dialog_start, (egui_animation_t *)&anim_dialog_finish);

    // anim_start_open
    egui_animation_translate_init((egui_animation_t *)&anim_start_open);
    egui_animation_translate_params_set(&anim_start_open, &anim_start_open_param);
    egui_animation_duration_set((egui_animation_t *)&anim_start_open, 300);

    // anim_start_close
    egui_animation_translate_init((egui_animation_t *)&anim_start_close);
    egui_animation_translate_params_set(&anim_start_close, &anim_start_close_param);
    egui_animation_duration_set((egui_animation_t *)&anim_start_close, 300);
    egui_animation_is_fill_before_set((egui_animation_t *)&anim_start_close, true);

    // anim_finish_open
    egui_animation_translate_init((egui_animation_t *)&anim_finish_open);
    egui_animation_translate_params_set(&anim_finish_open, &anim_finish_open_param);
    egui_animation_duration_set((egui_animation_t *)&anim_finish_open, 300);

    // anim_finish_close
    egui_animation_translate_init((egui_animation_t *)&anim_finish_close);
    egui_animation_translate_params_set(&anim_finish_close, &anim_finish_close_param);
    egui_animation_duration_set((egui_animation_t *)&anim_finish_close, 300);
    egui_animation_is_fill_before_set((egui_animation_t *)&anim_finish_close, true);

    // Set activity start/finish animation
    egui_core_activity_set_start_anim((egui_animation_t *)&anim_start_open, (egui_animation_t *)&anim_start_close);
    egui_core_activity_set_finish_anim((egui_animation_t *)&anim_finish_open, (egui_animation_t *)&anim_finish_close);
    // Start activity
    uicode_start_next_activity(NULL);

    // Init toast
    egui_toast_std_init((egui_toast_t *)&toast);
    egui_core_toast_set((egui_toast_t *)&toast);
}

void uicode_create_ui(void)
{
    uicode_init_ui();
}
