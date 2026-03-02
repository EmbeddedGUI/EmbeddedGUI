#include "egui.h"
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

static void button_click_cb(egui_view_t *self)
{
    EGUI_LOG_INF("Clicked\n");
}

void test_init_ui(void)
{
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
    switch (action_index)
    {
    case 0:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &button_1, 1000);
        return true;
    case 1:
        EGUI_SIM_SET_CLICK_VIEW(p_action, &button_1, 1000);
        return true;
    default:
        return false;
    }
}
#endif
