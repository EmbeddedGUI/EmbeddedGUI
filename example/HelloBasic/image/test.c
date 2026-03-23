#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

// views in root
static egui_view_image_t image_1;

extern const egui_image_std_t egui_res_image_star_rgb565_8;

#define IMAGE_WIDTH  96
#define IMAGE_HEIGHT 96

// View params
EGUI_VIEW_IMAGE_PARAMS_INIT(image_1_params, 0, 0, IMAGE_WIDTH, IMAGE_HEIGHT, (egui_image_t *)&egui_res_image_star_rgb565_8);

void test_init_ui(void)
{
    // Init all views
    // image_1
    egui_view_image_init_with_params(EGUI_VIEW_OF(&image_1), &image_1_params);
    egui_view_image_set_image_type(EGUI_VIEW_OF(&image_1), EGUI_VIEW_IMAGE_TYPE_NORMAL);
    egui_view_set_margin_all(EGUI_VIEW_OF(&image_1), 6);

    // Add To Root
    egui_core_add_user_root_view(EGUI_VIEW_OF(&image_1));

    // Layout childrens of root view
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_CENTER);
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    switch (action_index)
    {
    case 0:
        egui_view_image_set_image_color(EGUI_VIEW_OF(&image_1), EGUI_COLOR_HEX(0xFFB24C), EGUI_ALPHA_60);
        recording_request_snapshot();
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    case 1:
        egui_view_image_set_image_color(EGUI_VIEW_OF(&image_1), EGUI_COLOR_BLACK, 0);
        recording_request_snapshot();
        EGUI_SIM_SET_WAIT(p_action, 220);
        return true;
    default:
        return false;
    }
}
#endif
