#include "egui.h"
#include <stdlib.h>
#include <math.h>
#include "uicode.h"

#include "app_egui_resource_generate.h"

// views in test_view_group_1
static egui_view_image_t image_1;

void uicode_init_ui(void)
{
    // Init all views
    // image_1
    egui_view_image_init((egui_view_t *)&image_1);
    egui_view_set_position((egui_view_t *)&image_1, 10, 10);
    egui_view_set_size((egui_view_t *)&image_1, 100, 100);
    // egui_view_image_set_image_type((egui_view_t *)&image_1, EGUI_VIEW_IMAGE_TYPE_RESIZE);
    // egui_view_image_set_image((egui_view_t *)&image_1, (egui_image_t *)&egui_res_image_star_rgb565_0);
    // egui_view_image_set_image((egui_view_t *)&image_1, (egui_image_t *)&egui_res_image_star_rgb565_1);
    // egui_view_image_set_image((egui_view_t *)&image_1, (egui_image_t *)&egui_res_image_star_rgb565_2);
    // egui_view_image_set_image((egui_view_t *)&image_1, (egui_image_t *)&egui_res_image_star_rgb565_4);
    // egui_view_image_set_image((egui_view_t *)&image_1, (egui_image_t *)&egui_res_image_star_rgb565_8);
    // egui_view_image_set_image((egui_view_t *)&image_1, (egui_image_t *)&egui_res_image_star_rgb565_0_bin);
    // egui_view_image_set_image((egui_view_t *)&image_1, (egui_image_t *)&egui_res_image_star_rgb565_1_bin);
    // egui_view_image_set_image((egui_view_t *)&image_1, (egui_image_t *)&egui_res_image_star_rgb565_2_bin);
    // egui_view_image_set_image((egui_view_t *)&image_1, (egui_image_t *)&egui_res_image_star_rgb565_4_bin);
    // egui_view_image_set_image((egui_view_t *)&image_1, (egui_image_t *)&egui_res_image_star_rgb565_8_bin);

    egui_view_image_set_image((egui_view_t *)&image_1, (egui_image_t *)&egui_res_image_test_rgb565_4_bin);

    // Add To Root
    egui_core_add_user_root_view((egui_view_t *)&image_1);
}

static egui_timer_t ui_timer;
void egui_view_test_timer_callback(egui_timer_t *timer)
{
    // test scroll
    // egui_view_scroll_to((egui_view_t*)&test_view, 50, 10);
    // egui_view_scroll_by((egui_view_t*)&test_view, 50, 50);

    // egui_view_scroll_by((egui_view_t *)&test_view_group_1, 5, 5);

    // egui_view_set_visible((egui_view_t *)&test_view_1, !egui_view_get_visible((egui_view_t *)&test_view_1));

    // egui_view_set_enable((egui_view_t *)&button_1, !egui_view_get_enable((egui_view_t *)&button_1));

    // egui_view_set_alpha((egui_view_t *)&test_view_group_1, EGUI_ALPHA_50);

    // egui_view_set_alpha((egui_view_t *)&test_view_1, EGUI_ALPHA_50);
}

void uicode_create_ui(void)
{
    uicode_init_ui();

    ui_timer.callback = egui_view_test_timer_callback;
    egui_timer_start_timer(&ui_timer, 1000, 1000);
}
