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

static void button_click_cb(egui_view_t *self)
{
    EGUI_LOG_INF("Clicked\n");
}

void test_init_ui(void)
{
    // Init all views
    // button_1
    egui_view_button_init((egui_view_t *)&button_1);
    egui_view_set_size((egui_view_t *)&button_1, 80, 80);
    egui_view_set_on_click_listener((egui_view_t *)&button_1, button_click_cb);

    // bg_button
    egui_view_set_background((egui_view_t *)&button_1, (egui_background_t *)&bg_button);

    // Add To Root
    egui_core_add_user_root_view((egui_view_t *)&button_1);

    // Layout childrens of root view
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_CENTER);
}
