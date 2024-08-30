#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

// views in root
static egui_view_linearlayout_t layout_1;

// views in layout_1
static egui_view_label_t label_1;
static egui_view_label_t label_2;

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_1_param_normal, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_1_params, &bg_1_param_normal, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_1, &bg_1_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_2_param_normal, EGUI_COLOR_ORANGE, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_2_params, &bg_2_param_normal, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_2, &bg_2_params);

void test_init_ui(void)
{
    // Init all views
    // layout_1
    egui_view_linearlayout_init((egui_view_t *)&layout_1);
    egui_view_set_position((egui_view_t *)&layout_1, 0, 0);
    egui_view_set_size((egui_view_t *)&layout_1, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    egui_view_linearlayout_set_orientation((egui_view_t *)&layout_1, EGUI_LAYOUT_VERTICAL);
    egui_view_linearlayout_set_align_type((egui_view_t *)&layout_1, EGUI_ALIGN_CENTER);

    // label_1
    egui_view_label_init((egui_view_t *)&label_1);
    egui_view_set_size((egui_view_t *)&label_1, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT/2);
    egui_view_label_set_text((egui_view_t *)&label_1, "Item1");
    egui_view_label_set_align_type((egui_view_t *)&label_1, EGUI_ALIGN_CENTER);
    egui_view_label_set_font((egui_view_t *)&label_1, (egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_font_color((egui_view_t *)&label_1, EGUI_COLOR_BLACK, EGUI_ALPHA_100);

    // label_2
    egui_view_label_init((egui_view_t *)&label_2);
    egui_view_set_size((egui_view_t *)&label_2, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT/2);
    egui_view_label_set_text((egui_view_t *)&label_2, "Item2");
    egui_view_label_set_align_type((egui_view_t *)&label_2, EGUI_ALIGN_CENTER);
    egui_view_label_set_font((egui_view_t *)&label_2, (egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_font_color((egui_view_t *)&label_2, EGUI_COLOR_BLACK, EGUI_ALPHA_100);

    // background
    egui_view_set_background((egui_view_t *)&label_1, (egui_background_t *)&bg_1);
    egui_view_set_background((egui_view_t *)&label_2, (egui_background_t *)&bg_2);

    // Add childs to layout_1
    egui_view_group_add_child((egui_view_t *)&layout_1, (egui_view_t *)&label_1);
    egui_view_group_add_child((egui_view_t *)&layout_1, (egui_view_t *)&label_2);

    // Re-layout childs
    egui_view_linearlayout_layout_childs((egui_view_t *)&layout_1);

    // Add To Root
    egui_core_add_user_root_view((egui_view_t *)&layout_1);
}
