#include "egui.h"
#include <stdlib.h>
#include <math.h>
#include "uicode.h"

// views in root
static egui_view_label_t label_1;
static egui_view_button_t button_1;
static egui_view_linearlayout_t layout_1;

// EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID_STROKE(bg_button_param_normal, EGUI_COLOR_WHITE, EGUI_ALPHA_60, 10, EGUI_COLOR_GREEN, EGUI_ALPHA_60);
EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_button_param_normal, EGUI_COLOR_WHITE, EGUI_ALPHA_100, 15, 5, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
// EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_CORNERS_STROKE(bg_button_param_normal, EGUI_COLOR_WHITE, EGUI_ALPHA_60, 20, 25, 30, 35, 10,
// EGUI_COLOR_GREEN, EGUI_ALPHA_60);
// EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_button_param_normal, EGUI_COLOR_WHITE, EGUI_ALPHA_100, 10);
// EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_CORNERS(bg_button_param_pressed, EGUI_COLOR_RED, EGUI_ALPHA_100, 5, 10, 15, 20);
// EGUI_BACKGROUND_COLOR_PARAM_INIT_CIRCLE(bg_button_param_pressed, EGUI_COLOR_RED, EGUI_ALPHA_100, 20);
// EGUI_BACKGROUND_COLOR_PARAM_INIT_CIRCLE_STROKE(bg_button_param_pressed, EGUI_COLOR_WHITE, EGUI_ALPHA_60, 20, 10, EGUI_COLOR_GREEN, EGUI_ALPHA_60);
// EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE(bg_button_param_pressed, EGUI_COLOR_GREEN, EGUI_ALPHA_100, 10);
EGUI_BACKGROUND_COLOR_PARAM_INIT_ROUND_RECTANGLE_STROKE(bg_button_param_pressed, EGUI_COLOR_DARK_GREY, EGUI_ALPHA_100, 15, 5, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_button_param_disabled, EGUI_COLOR_YELLOW, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_button_params, &bg_button_param_normal, &bg_button_param_pressed, &bg_button_param_disabled);
static egui_background_color_t bg_button;

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

#define BUTTON_WIDTH  150
#define BUTTON_HEIGHT 50

#define LABEL_WIDTH  150
#define LABEL_HEIGHT 50
void uicode_init_ui(void)
{
    // Init all views
    // layout_1
    egui_view_linearlayout_init((egui_view_t *)&layout_1);
    egui_view_set_position((egui_view_t *)&layout_1, 0, 0);
    egui_view_set_size((egui_view_t *)&layout_1, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    egui_view_linearlayout_set_align_type((egui_view_t *)&layout_1, EGUI_ALIGN_CENTER);

    // label_1
    egui_view_label_init((egui_view_t *)&label_1);
    egui_view_set_position((egui_view_t *)&label_1, 0, 0);
    egui_view_set_size((egui_view_t *)&label_1, LABEL_WIDTH, LABEL_HEIGHT);
    egui_view_label_set_text((egui_view_t *)&label_1, "Hello World!");
    egui_view_label_set_align_type((egui_view_t *)&label_1, EGUI_ALIGN_CENTER);
    egui_view_label_set_font((egui_view_t *)&label_1, (egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_font_color((egui_view_t *)&label_1, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

    // egui_color_t color = {{.blue=255, .red=0, .green=0}};

    // button_1
    egui_view_button_init((egui_view_t *)&button_1);
    egui_view_set_position((egui_view_t *)&button_1, 0, 0);
    egui_view_set_size((egui_view_t *)&button_1, BUTTON_WIDTH, BUTTON_HEIGHT);
    egui_view_label_set_text((egui_view_t *)&button_1, button_str);
    egui_view_label_set_align_type((egui_view_t *)&button_1, EGUI_ALIGN_CENTER);
    egui_view_label_set_font((egui_view_t *)&button_1, (egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_font_color((egui_view_t *)&button_1, EGUI_COLOR_BLACK, EGUI_ALPHA_100);
    egui_view_set_on_click_listener((egui_view_t *)&button_1, button_click_cb);

    // bg_button
    egui_background_color_init((egui_background_t *)&bg_button);
    egui_background_set_params((egui_background_t *)&bg_button, &bg_button_params);
    egui_view_set_background((egui_view_t *)&button_1, (egui_background_t *)&bg_button);

    // Add childs to layout_1
    egui_view_group_add_child((egui_view_t *)&layout_1, (egui_view_t *)&label_1);
    egui_view_group_add_child((egui_view_t *)&layout_1, (egui_view_t *)&button_1);

    // Re-layout childs
    egui_view_linearlayout_layout_childs((egui_view_t *)&layout_1);

    // Add To Root
    egui_core_add_user_root_view((egui_view_t *)&layout_1);
}

void uicode_create_ui(void)
{
    uicode_init_ui();
}
