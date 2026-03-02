#include "egui.h"
#include <stdlib.h>
#include "uicode.h"
#include "egui_view_test_mask.h"

// views in root
static egui_view_test_mask_t test_mask_1;
static egui_view_test_mask_t test_mask_2;

// mask
egui_mask_circle_t mask_circle;
egui_mask_image_t mask_image;

extern const egui_image_std_t egui_res_image_test_rgb565_8;
extern const egui_image_std_t egui_res_image_star_rgb565_8;

void test_init_ui(void)
{
    // Init all views
    // test_mask_1
    egui_view_test_mask_init(EGUI_VIEW_OF(&test_mask_1));
    egui_view_image_set_image_type(EGUI_VIEW_OF(&test_mask_1.base), EGUI_VIEW_IMAGE_TYPE_NORMAL);
    egui_view_image_set_image(EGUI_VIEW_OF(&test_mask_1.base), (egui_image_t *)&egui_res_image_test_rgb565_8);
    egui_view_set_size(EGUI_VIEW_OF(&test_mask_1), 72, 72);
    egui_view_set_margin_all(EGUI_VIEW_OF(&test_mask_1), 6);

    // test_mask_2
    egui_view_test_mask_init(EGUI_VIEW_OF(&test_mask_2));
    egui_view_image_set_image_type(EGUI_VIEW_OF(&test_mask_2.base), EGUI_VIEW_IMAGE_TYPE_NORMAL);
    egui_view_image_set_image(EGUI_VIEW_OF(&test_mask_2.base), (egui_image_t *)&egui_res_image_test_rgb565_8);
    egui_view_set_size(EGUI_VIEW_OF(&test_mask_2), 72, 72);
    egui_view_set_margin_all(EGUI_VIEW_OF(&test_mask_2), 6);

    // mask_circle
    egui_mask_circle_init((egui_mask_t *)&mask_circle);
    egui_mask_set_position((egui_mask_t *)&mask_circle, 0, 0);
    egui_mask_set_size((egui_mask_t *)&mask_circle, 72, 72);

    // mask_image
    egui_mask_image_init((egui_mask_t *)&mask_image);
    egui_mask_set_position((egui_mask_t *)&mask_image, 0, 0);
    egui_mask_set_size((egui_mask_t *)&mask_image, 72, 72);
    egui_mask_image_set_image((egui_mask_t *)&mask_image, (egui_image_t *)&egui_res_image_star_rgb565_8);

    // set mask to view
    egui_view_test_mask_set_mask(EGUI_VIEW_OF(&test_mask_1), (egui_mask_t *)&mask_circle);
    egui_view_test_mask_set_mask(EGUI_VIEW_OF(&test_mask_2), (egui_mask_t *)&mask_image);

    // Add To Root
    egui_core_add_user_root_view(EGUI_VIEW_OF(&test_mask_1));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&test_mask_2));

    // Layout childrens of root view
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_CENTER);
}

#if EGUI_CONFIG_RECORDING_TEST
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    if (action_index >= 1)
    {
        return false;
    }
    EGUI_SIM_SET_WAIT(p_action, 1500);
    return true;
}
#endif
