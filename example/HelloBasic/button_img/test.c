#include "egui.h"
#include <stdio.h>
#include <stdlib.h>
#include "uicode.h"

// views in root
static egui_view_button_t button_1;

extern const egui_image_std_t egui_res_image_test_rgb565_8;
extern const egui_image_std_t egui_res_image_star_rgb565_8;
EGUI_BACKGROUND_IMAGE_PARAM_INIT(bg_button_param_normal, (egui_image_t *)&egui_res_image_test_rgb565_8);
EGUI_BACKGROUND_IMAGE_PARAM_INIT(bg_button_param_pressed, (egui_image_t *)&egui_res_image_star_rgb565_8);
EGUI_BACKGROUND_IMAGE_PARAM_INIT(bg_button_param_disabled, (egui_image_t *)&egui_res_image_star_rgb565_8);
EGUI_BACKGROUND_PARAM_INIT(bg_button_params, &bg_button_param_normal, &bg_button_param_pressed, &bg_button_param_disabled);
EGUI_BACKGROUND_IMAGE_STATIC_CONST_INIT(bg_button, &bg_button_params);

#define BUTTON_WIDTH  96
#define BUTTON_HEIGHT 96

// View params
EGUI_VIEW_BUTTON_PARAMS_INIT_SIMPLE(button_1_params, 0, 0, BUTTON_WIDTH, BUTTON_HEIGHT, NULL);

#if EGUI_CONFIG_RECORDING_TEST
static uint8_t runtime_fail_reported;
static uint8_t button_click_count;

static void report_runtime_failure(const char *message)
{
    if (runtime_fail_reported)
    {
        return;
    }

    runtime_fail_reported = 1;
    printf("[RUNTIME_CHECK_FAIL] %s\n", message);
}
#endif

static void button_click_cb(egui_view_t *self)
{
    (void)self;
#if EGUI_CONFIG_RECORDING_TEST
    button_click_count++;
#endif
    EGUI_LOG_INF("Clicked\n");
}

void test_init_ui(void)
{
#if EGUI_CONFIG_RECORDING_TEST
    runtime_fail_reported = 0;
    button_click_count = 0;
#endif

    // Init all views
    // button_1
    egui_view_button_init_with_params(EGUI_VIEW_OF(&button_1), &button_1_params);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&button_1), button_click_cb);

    // bg_button
    egui_view_set_background(EGUI_VIEW_OF(&button_1), EGUI_BG_OF(&bg_button));
    egui_view_set_margin_all(EGUI_VIEW_OF(&button_1), 6);

    // Add To Root
    egui_core_add_user_root_view(EGUI_VIEW_OF(&button_1));

    // Layout childrens of root view
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_CENTER);
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
        if (first_call && button_click_count != 0)
        {
            report_runtime_failure("button_img initial click count mismatch");
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, &button_1, 1000);
        return true;
    case 1:
        if (first_call && button_click_count != 1)
        {
            report_runtime_failure("button_img first click was not delivered");
        }
        EGUI_SIM_SET_CLICK_VIEW(p_action, &button_1, 1000);
        return true;
    case 2:
        if (first_call)
        {
            if (button_click_count != 2)
            {
                report_runtime_failure("button_img second click was not delivered");
            }
            recording_request_snapshot();
        }
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    default:
        return false;
    }
}
#endif
