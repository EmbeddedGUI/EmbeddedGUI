#include "egui.h"
#include <stdlib.h>
#include "uicode.h"
#include "egui_view_test_mask.h"

// views in root
static egui_view_test_mask_t test_mask_1;
static egui_view_test_mask_t test_mask_2;
static egui_view_test_mask_t test_mask_3;
static egui_view_test_mask_t test_mask_4;

// mask
egui_mask_circle_t mask_circle;
egui_mask_image_t mask_image;
egui_mask_circle_t mask_circle_transform;
egui_mask_round_rectangle_t mask_round_rect;

extern const egui_image_std_t egui_res_image_test_rgb565_8;
extern const egui_image_std_t egui_res_image_star_rgb565_8;

#define TEST_MASK_SIZE   56
#define TEST_MASK_MARGIN 4

void test_init_ui(void)
{
    // Init all views
    // test_mask_1
    egui_view_test_mask_init(EGUI_VIEW_OF(&test_mask_1));
    egui_view_test_mask_set_text_transform(EGUI_VIEW_OF(&test_mask_1), (egui_font_t *)&egui_res_font_montserrat_24_4, "A", -18, 256, EGUI_COLOR_WHITE,
                                           EGUI_ALPHA_100, EGUI_COLOR_HEX(0x163B47), EGUI_ALPHA_100);
    egui_view_set_size(EGUI_VIEW_OF(&test_mask_1), TEST_MASK_SIZE, TEST_MASK_SIZE);
    egui_view_set_margin_all(EGUI_VIEW_OF(&test_mask_1), TEST_MASK_MARGIN);

    // test_mask_2
    egui_view_test_mask_init(EGUI_VIEW_OF(&test_mask_2));
    egui_view_image_set_image_type(EGUI_VIEW_OF(&test_mask_2.base), EGUI_VIEW_IMAGE_TYPE_NORMAL);
    egui_view_image_set_image(EGUI_VIEW_OF(&test_mask_2.base), (egui_image_t *)&egui_res_image_test_rgb565_8);
    egui_view_test_mask_set_image_transform(EGUI_VIEW_OF(&test_mask_2), 45, 200, EGUI_COLOR_HEX(0x14263C), EGUI_ALPHA_100);
    egui_view_set_size(EGUI_VIEW_OF(&test_mask_2), TEST_MASK_SIZE, TEST_MASK_SIZE);
    egui_view_set_margin_all(EGUI_VIEW_OF(&test_mask_2), TEST_MASK_MARGIN);

    // test_mask_3
    egui_view_test_mask_init(EGUI_VIEW_OF(&test_mask_3));
    egui_view_image_set_image_type(EGUI_VIEW_OF(&test_mask_3.base), EGUI_VIEW_IMAGE_TYPE_NORMAL);
    egui_view_image_set_image(EGUI_VIEW_OF(&test_mask_3.base), (egui_image_t *)&egui_res_image_test_rgb565_8);
    egui_view_test_mask_set_image_transform(EGUI_VIEW_OF(&test_mask_3), 30, 176, EGUI_COLOR_HEX(0x11331F), EGUI_ALPHA_100);
    egui_view_set_size(EGUI_VIEW_OF(&test_mask_3), TEST_MASK_SIZE, TEST_MASK_SIZE);
    egui_view_set_margin_all(EGUI_VIEW_OF(&test_mask_3), TEST_MASK_MARGIN);

    // test_mask_4
    egui_view_test_mask_init(EGUI_VIEW_OF(&test_mask_4));
    egui_view_test_mask_set_text_transform(EGUI_VIEW_OF(&test_mask_4), (egui_font_t *)&egui_res_font_montserrat_24_4, "A", 18, 256, EGUI_COLOR_WHITE,
                                           EGUI_ALPHA_100, EGUI_COLOR_HEX(0x23304A), EGUI_ALPHA_100);
    egui_view_set_size(EGUI_VIEW_OF(&test_mask_4), TEST_MASK_SIZE, TEST_MASK_SIZE);
    egui_view_set_margin_all(EGUI_VIEW_OF(&test_mask_4), TEST_MASK_MARGIN);

    // mask_circle
    egui_mask_circle_init((egui_mask_t *)&mask_circle);
    egui_mask_set_position((egui_mask_t *)&mask_circle, 0, 0);
    egui_mask_set_size((egui_mask_t *)&mask_circle, TEST_MASK_SIZE, TEST_MASK_SIZE);

    // mask_image
    egui_mask_image_init((egui_mask_t *)&mask_image);
    egui_mask_set_position((egui_mask_t *)&mask_image, 0, 0);
    egui_mask_set_size((egui_mask_t *)&mask_image, TEST_MASK_SIZE, TEST_MASK_SIZE);
    egui_mask_image_set_image((egui_mask_t *)&mask_image, (egui_image_t *)&egui_res_image_star_rgb565_8);

    // mask_circle_transform
    egui_mask_circle_init((egui_mask_t *)&mask_circle_transform);
    egui_mask_set_position((egui_mask_t *)&mask_circle_transform, 0, 0);
    egui_mask_set_size((egui_mask_t *)&mask_circle_transform, TEST_MASK_SIZE, TEST_MASK_SIZE);

    // mask_round_rect
    egui_mask_round_rectangle_init((egui_mask_t *)&mask_round_rect);
    egui_mask_set_position((egui_mask_t *)&mask_round_rect, 0, 0);
    egui_mask_set_size((egui_mask_t *)&mask_round_rect, TEST_MASK_SIZE, TEST_MASK_SIZE);
    egui_mask_round_rectangle_set_radius((egui_mask_t *)&mask_round_rect, 10);

    // set mask to view
    egui_view_test_mask_set_mask(EGUI_VIEW_OF(&test_mask_1), (egui_mask_t *)&mask_circle);
    egui_view_test_mask_set_mask(EGUI_VIEW_OF(&test_mask_2), (egui_mask_t *)&mask_image);
    egui_view_test_mask_set_mask(EGUI_VIEW_OF(&test_mask_3), (egui_mask_t *)&mask_circle_transform);
    egui_view_test_mask_set_mask(EGUI_VIEW_OF(&test_mask_4), (egui_mask_t *)&mask_round_rect);

    // Add To Root
    egui_core_add_user_root_view(EGUI_VIEW_OF(&test_mask_1));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&test_mask_2));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&test_mask_3));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&test_mask_4));

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
    EGUI_SIM_SET_WAIT(p_action, 1800);
    return true;
}
#endif
