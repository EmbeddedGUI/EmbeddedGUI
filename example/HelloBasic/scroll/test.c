#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

// views in root
static egui_view_scroll_t scroll_1;

// views in scroll_1
static egui_view_label_t label_1;
static egui_view_label_t label_2;
static egui_view_label_t label_3;

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_1_param_normal, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_1_params, &bg_1_param_normal, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_1, &bg_1_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_2_param_normal, EGUI_COLOR_ORANGE, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_2_params, &bg_2_param_normal, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_2, &bg_2_params);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_3_param_normal, EGUI_COLOR_BLUE, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_3_params, &bg_3_param_normal, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_3, &bg_3_params);

void test_init_ui(void)
{
    // Init all views
    // scroll_1
    egui_view_scroll_init((egui_view_t *)&scroll_1);
    egui_view_set_position((egui_view_t *)&scroll_1, 0, 0);
    egui_view_scroll_set_size((egui_view_t *)&scroll_1, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);

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

    // label_3
    egui_view_label_init((egui_view_t *)&label_3);
    egui_view_set_size((egui_view_t *)&label_3, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT/2);
    egui_view_label_set_text((egui_view_t *)&label_3, "Item3");
    egui_view_label_set_align_type((egui_view_t *)&label_3, EGUI_ALIGN_CENTER);
    egui_view_label_set_font((egui_view_t *)&label_3, (egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_font_color((egui_view_t *)&label_3, EGUI_COLOR_BLACK, EGUI_ALPHA_100);

    // background
    egui_view_set_background((egui_view_t *)&label_1, (egui_background_t *)&bg_1);
    egui_view_set_background((egui_view_t *)&label_2, (egui_background_t *)&bg_2);
    egui_view_set_background((egui_view_t *)&label_3, (egui_background_t *)&bg_3);

    // Add childs to scroll_1
    egui_view_scroll_add_child((egui_view_t *)&scroll_1, (egui_view_t *)&label_1);
    egui_view_scroll_add_child((egui_view_t *)&scroll_1, (egui_view_t *)&label_2);
    egui_view_scroll_add_child((egui_view_t *)&scroll_1, (egui_view_t *)&label_3);

    // Re-layout childs
    egui_view_scroll_layout_childs((egui_view_t *)&scroll_1);

    // Add To Root
    egui_core_add_user_root_view((egui_view_t *)&scroll_1);
}
