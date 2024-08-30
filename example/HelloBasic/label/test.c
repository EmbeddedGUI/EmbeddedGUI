#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

// views in root
static egui_view_label_t label_1;

void test_init_ui(void)
{
    // Init all views
    // label_1
    egui_view_label_init((egui_view_t *)&label_1);
    egui_view_set_position((egui_view_t *)&label_1, 0, 0);
    egui_view_set_size((egui_view_t *)&label_1, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    egui_view_label_set_text((egui_view_t *)&label_1, "Hello World!");
    egui_view_label_set_align_type((egui_view_t *)&label_1, EGUI_ALIGN_CENTER);
    egui_view_label_set_font((egui_view_t *)&label_1, (egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_font_color((egui_view_t *)&label_1, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

    // Add To Root
    egui_core_add_user_root_view((egui_view_t *)&label_1);
}
