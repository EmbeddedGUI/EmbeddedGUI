#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

// views in root
static egui_view_image_t image_1;

extern const egui_image_std_t egui_res_image_star_rgb565_8;

void test_init_ui(void)
{
    // Init all views
    // button_1
    egui_view_image_init((egui_view_t *)&image_1);
    egui_view_image_set_image_type((egui_view_t *)&image_1, EGUI_VIEW_IMAGE_TYPE_NORMAL);
    egui_view_image_set_image((egui_view_t *)&image_1, (egui_image_t *)&egui_res_image_star_rgb565_8);
    egui_view_set_size((egui_view_t *)&image_1, 80, 80);

    // Add To Root
    egui_core_add_user_root_view((egui_view_t *)&image_1);

    // Layout childrens of root view
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_CENTER);
}
