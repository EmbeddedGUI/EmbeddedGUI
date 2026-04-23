#include "egui.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "uicode_disp0.h"

static egui_view_window_t window_1;
static egui_view_label_t content_label_1;
static egui_view_label_t content_label_2;

#if EGUI_CONFIG_FUNCTION_RECORDING_TEST
#define WINDOW_VERIFY_RETRY_MAX 3U

static uint8_t runtime_fail_reported;
static uint8_t window_verify_retry;
#endif

EGUI_VIEW_WINDOW_PARAMS_INIT(window_1_params, 10, 10, 220, 200, 30, "My Window");
EGUI_VIEW_LABEL_PARAMS_INIT(content_label_1_params, 10, 10, 200, 30, "Primary content line", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_PRIMARY, EGUI_ALPHA_100);
EGUI_VIEW_LABEL_PARAMS_INIT(content_label_2_params, 10, 50, 200, 30, "Secondary content line", EGUI_CONFIG_FONT_DEFAULT, EGUI_THEME_TEXT_SECONDARY,
                            EGUI_ALPHA_100);

static void test_window_on_close(egui_view_t *self)
{
    egui_view_window_set_title(self, "Closed");
}

void test_init_ui(egui_core_t *core)
{
#if EGUI_CONFIG_FUNCTION_RECORDING_TEST
    runtime_fail_reported = 0;
    window_verify_retry = 0U;
#endif

    // Init window
    egui_view_window_init_with_params(EGUI_VIEW_OF(&window_1), core, &window_1_params);
    egui_view_window_set_close_icon(EGUI_VIEW_OF(&window_1), EGUI_ICON_MS_CLOSE);
    egui_view_window_set_close_icon_font(EGUI_VIEW_OF(&window_1), EGUI_FONT_ICON_MS_20);
    egui_view_window_set_on_close(EGUI_VIEW_OF(&window_1), test_window_on_close);

    // Init content labels
    egui_view_label_init_with_params(EGUI_VIEW_OF(&content_label_1), core, &content_label_1_params);
    egui_view_label_init_with_params(EGUI_VIEW_OF(&content_label_2), core, &content_label_2_params);

    // Add content to window
    egui_view_window_add_content(EGUI_VIEW_OF(&window_1), EGUI_VIEW_OF(&content_label_1));
    egui_view_window_add_content(EGUI_VIEW_OF(&window_1), EGUI_VIEW_OF(&content_label_2));

    // Add window to root
    egui_core_add_user_root_view(EGUI_VIEW_OF(&window_1));
}

#if EGUI_CONFIG_FUNCTION_RECORDING_TEST
static void report_runtime_failure(const char *message)
{
    if (runtime_fail_reported)
    {
        return;
    }

    runtime_fail_reported = 1;
    printf("[RUNTIME_CHECK_FAIL] %s\n", message);
}

static uint8_t schedule_window_verify_retry(egui_sim_action_t *p_action)
{
    if (window_verify_retry >= WINDOW_VERIFY_RETRY_MAX)
    {
        return 0U;
    }

    window_verify_retry++;
    recording_request_snapshot();
    EGUI_SIM_SET_WAIT(p_action, 0);
    return 1U;
}

bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    switch (action_index)
    {
    case 0:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &window_1.close_label, 320);
        return true;
    case 1:
        if (strcmp(window_1.title_label.text, "Closed") != 0)
        {
            if (schedule_window_verify_retry(p_action))
            {
                return true;
            }
            report_runtime_failure("window close action did not update title");
        }
        window_verify_retry = 0U;
        recording_request_snapshot();
        EGUI_SIM_SET_WAIT(p_action, 320);
        return true;
    default:
        return false;
    }
}
#endif
