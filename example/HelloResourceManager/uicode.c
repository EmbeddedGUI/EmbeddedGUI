#include "egui.h"
#include <stdlib.h>
#include <math.h>
#include "uicode.h"

#include "app_egui_resource_generate.h"

// views in root
static egui_view_image_t image_1;
static egui_view_label_t label_1;
static egui_view_label_t label_2;

// View params
EGUI_VIEW_IMAGE_PARAMS_INIT(image_1_params, 10, 10, 100, 100, (egui_image_t *)&egui_res_image_test_rgb565_4_bin);
EGUI_VIEW_LABEL_PARAMS_INIT(label_1_params, 10, 10, 160, 160, "Hello World!", (egui_font_t *)&egui_res_font_test_16_4, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
EGUI_VIEW_LABEL_PARAMS_INIT(label_2_params, 10, 100, 160, 160, "External Resource!", (egui_font_t *)&egui_res_font_test_16_4_bin, EGUI_COLOR_WHITE,
                            EGUI_ALPHA_100);

void uicode_init_ui(void)
{
    // Init all views

    // image_1
    egui_view_image_init_with_params(EGUI_VIEW_OF(&image_1), &image_1_params);
    // egui_view_image_set_image_type(EGUI_VIEW_OF(&image_1), EGUI_VIEW_IMAGE_TYPE_RESIZE);
    // egui_view_image_set_image(EGUI_VIEW_OF(&image_1), (egui_image_t *)&egui_res_image_star_rgb565_0);
    // egui_view_image_set_image(EGUI_VIEW_OF(&image_1), (egui_image_t *)&egui_res_image_star_rgb565_1);
    // egui_view_image_set_image(EGUI_VIEW_OF(&image_1), (egui_image_t *)&egui_res_image_star_rgb565_2);
    // egui_view_image_set_image(EGUI_VIEW_OF(&image_1), (egui_image_t *)&egui_res_image_star_rgb565_4);
    // egui_view_image_set_image(EGUI_VIEW_OF(&image_1), (egui_image_t *)&egui_res_image_star_rgb565_8);
    // egui_view_image_set_image(EGUI_VIEW_OF(&image_1), (egui_image_t *)&egui_res_image_star_rgb565_0_bin);
    // egui_view_image_set_image(EGUI_VIEW_OF(&image_1), (egui_image_t *)&egui_res_image_star_rgb565_1_bin);
    // egui_view_image_set_image(EGUI_VIEW_OF(&image_1), (egui_image_t *)&egui_res_image_star_rgb565_2_bin);
    // egui_view_image_set_image(EGUI_VIEW_OF(&image_1), (egui_image_t *)&egui_res_image_star_rgb565_4_bin);
    // egui_view_image_set_image(EGUI_VIEW_OF(&image_1), (egui_image_t *)&egui_res_image_star_rgb565_8_bin);

    // label_1
    egui_view_label_init_with_params(EGUI_VIEW_OF(&label_1), &label_1_params);

    // label_2
    egui_view_label_init_with_params(EGUI_VIEW_OF(&label_2), &label_2_params);

    // Add To Root
    egui_core_add_user_root_view(EGUI_VIEW_OF(&image_1));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&label_1));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&label_2));
}

static egui_timer_t ui_timer;
void egui_view_test_timer_callback(egui_timer_t *timer)
{
    // test scroll
    // egui_view_scroll_to(EGUI_VIEW_OF(&test_view), 50, 10);
    // egui_view_scroll_by(EGUI_VIEW_OF(&test_view), 50, 50);

    // egui_view_scroll_by(EGUI_VIEW_OF(&test_view_group_1), 5, 5);

    // egui_view_set_visible(EGUI_VIEW_OF(&test_view_1), !egui_view_get_visible(EGUI_VIEW_OF(&test_view_1)));

    // egui_view_set_enable(EGUI_VIEW_OF(&button_1), !egui_view_get_enable(EGUI_VIEW_OF(&button_1)));

    // egui_view_set_alpha(EGUI_VIEW_OF(&test_view_group_1), EGUI_ALPHA_50);

    // egui_view_set_alpha(EGUI_VIEW_OF(&test_view_1), EGUI_ALPHA_50);
}

void uicode_create_ui(void)
{
    uicode_init_ui();

    ui_timer.callback = egui_view_test_timer_callback;
    egui_timer_start_timer(&ui_timer, 1000, 1000);
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
            recording_request_snapshot();
        EGUI_SIM_SET_WAIT(p_action, 200);
        return true;
    default:
        return false;
    }
}
#endif
