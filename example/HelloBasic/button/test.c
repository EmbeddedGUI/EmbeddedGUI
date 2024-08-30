#include "egui.h"
#include <stdlib.h>
#include "uicode.h"

// views in root
static egui_view_button_t button_1;

EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_button_param_normal, EGUI_COLOR_WHITE, EGUI_ALPHA_100, 10, 2, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_button_param_pressed, EGUI_COLOR_DARK_GREY, EGUI_ALPHA_100, 10, 2, EGUI_COLOR_DARK_GREEN, EGUI_ALPHA_100);
EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_button_param_disabled, EGUI_COLOR_YELLOW, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_button_params, &bg_button_param_normal, &bg_button_param_pressed, &bg_button_param_disabled);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_button, &bg_button_params);

static char button_str[20] = "Click me!";
static void button_click_cb(egui_view_t *self)
{
    EGUI_LOG_INF("Clicked\n");

    static uint32_t cnt = 1;
    egui_api_sprintf(button_str, "Clicked %ds", cnt);
    EGUI_LOG_INF("button_str: %s\n", button_str);

    egui_view_label_set_text((egui_view_t *)self, button_str);
    cnt++;
}

void test_init_ui(void)
{
    // Init all views
    // button_1
    egui_view_button_init((egui_view_t *)&button_1);
    egui_view_set_position((egui_view_t *)&button_1, 0, 0);
    egui_view_set_size((egui_view_t *)&button_1, 120, 40);
    egui_view_label_set_text((egui_view_t *)&button_1, button_str);
    egui_view_label_set_align_type((egui_view_t *)&button_1, EGUI_ALIGN_CENTER);
    egui_view_label_set_font((egui_view_t *)&button_1, (egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_font_color((egui_view_t *)&button_1, EGUI_COLOR_BLACK, EGUI_ALPHA_100);
    egui_view_set_on_click_listener((egui_view_t *)&button_1, button_click_cb);

    // bg_button
    egui_view_set_background((egui_view_t *)&button_1, (egui_background_t *)&bg_button);

    // Add To Root
    egui_core_add_user_root_view((egui_view_t *)&button_1);

    // Layout childrens of root view
    egui_core_layout_childs_user_root_view(EGUI_LAYOUT_VERTICAL, EGUI_ALIGN_CENTER);
}
