#include "egui.h"
#include <stdlib.h>
#include <math.h>
#include "uicode.h"
#include "uicode_viewpage.h"
#include "uicode_scroll.h"

#define TEST_ITEM_HEIGHT EGUI_CONFIG_SCEEN_HEIGHT

// views in root
static egui_view_viewpage_t test_viewpage_view;

// views in test_viewpage_view
static egui_view_label_t label_1;
static egui_view_label_t label_2;
static egui_view_label_t label_3;
static egui_view_button_t button_1;

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_1_param_normal, EGUI_COLOR_GREEN, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_1_params, &bg_1_param_normal, NULL, NULL);
static egui_background_color_t bg_1;

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_2_param_normal, EGUI_COLOR_ORANGE, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_2_params, &bg_2_param_normal, NULL, NULL);
static egui_background_color_t bg_2;

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_3_param_normal, EGUI_COLOR_BLUE, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_3_params, &bg_3_param_normal, NULL, NULL);
static egui_background_color_t bg_3;

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_button_param_normal, EGUI_COLOR_WHITE, EGUI_ALPHA_100);
EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_button_param_pressed, EGUI_COLOR_YELLOW, EGUI_ALPHA_100);
EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_button_param_disabled, EGUI_COLOR_RED, EGUI_ALPHA_100);
EGUI_BACKGROUND_PARAM_INIT(bg_button_params, &bg_button_param_normal, &bg_button_param_pressed, &bg_button_param_disabled);
static egui_background_color_t bg_button;

static char button_str[20] = "Click me!";
static void button_click_cb(egui_view_t *self)
{
    EGUI_LOG_INF("Clicked\n");

    static uint32_t cnt = 1;
    egui_api_sprintf(button_str, "Clicked %ds", cnt);
    EGUI_LOG_INF("button_str: %s\n", button_str);

    egui_view_label_set_text(self, button_str);
    cnt++;
}

egui_view_t *uicode_init_ui_viewpage(void)
{
    // Init all views
    // test_view
    egui_view_viewpage_init((egui_view_t *)&test_viewpage_view);
    egui_view_set_position((egui_view_t *)&test_viewpage_view, 0, 0);
    egui_view_viewpage_set_size((egui_view_t *)&test_viewpage_view, EGUI_CONFIG_SCEEN_WIDTH, TEST_ITEM_HEIGHT);

    // label_1
    egui_view_label_init((egui_view_t *)&label_1);
    egui_view_set_position((egui_view_t *)&label_1, 0, 0);
    egui_view_set_size((egui_view_t *)&label_1, EGUI_CONFIG_SCEEN_WIDTH, TEST_ITEM_HEIGHT);
    egui_view_label_set_text((egui_view_t *)&label_1, "Item 1");
    egui_view_label_set_align_type((egui_view_t *)&label_1, EGUI_ALIGN_CENTER);
    egui_view_label_set_font((egui_view_t *)&label_1, (egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_font_color((egui_view_t *)&label_1, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

    // label_2
    egui_view_label_init((egui_view_t *)&label_2);
    egui_view_set_position((egui_view_t *)&label_2, 0, 0);
    egui_view_set_size((egui_view_t *)&label_2, EGUI_CONFIG_SCEEN_WIDTH, TEST_ITEM_HEIGHT);
    egui_view_label_set_text((egui_view_t *)&label_2, "Item 2");
    egui_view_label_set_align_type((egui_view_t *)&label_2, EGUI_ALIGN_CENTER);
    egui_view_label_set_font((egui_view_t *)&label_2, (egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_font_color((egui_view_t *)&label_2, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

    // label_3
    egui_view_label_init((egui_view_t *)&label_3);
    egui_view_set_position((egui_view_t *)&label_3, 0, 0);
    egui_view_set_size((egui_view_t *)&label_3, EGUI_CONFIG_SCEEN_WIDTH, TEST_ITEM_HEIGHT);
    egui_view_label_set_text((egui_view_t *)&label_3, "Item 3");
    egui_view_label_set_align_type((egui_view_t *)&label_3, EGUI_ALIGN_CENTER);
    egui_view_label_set_font((egui_view_t *)&label_3, (egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_font_color((egui_view_t *)&label_3, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

    // button_1
    egui_view_button_init((egui_view_t *)&button_1);
    egui_view_set_position((egui_view_t *)&button_1, 0, 0);
    egui_view_set_size((egui_view_t *)&button_1, EGUI_CONFIG_SCEEN_WIDTH, TEST_ITEM_HEIGHT);
    egui_view_label_set_text((egui_view_t *)&button_1, button_str);
    egui_view_label_set_align_type((egui_view_t *)&button_1, EGUI_ALIGN_CENTER);
    egui_view_label_set_font((egui_view_t *)&button_1, (egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_font_color((egui_view_t *)&button_1, EGUI_COLOR_BLACK, EGUI_ALPHA_100);
    egui_view_set_on_click_listener((egui_view_t *)&button_1, button_click_cb);

    // bg_1
    egui_background_color_init((egui_background_t *)&bg_1);
    egui_background_set_params((egui_background_t *)&bg_1, &bg_1_params);
    egui_view_set_background((egui_view_t *)&label_1, (egui_background_t *)&bg_1);

    // bg_2
    egui_background_color_init((egui_background_t *)&bg_2);
    egui_background_set_params((egui_background_t *)&bg_2, &bg_2_params);
    egui_view_set_background((egui_view_t *)&label_2, (egui_background_t *)&bg_2);

    // bg_3
    egui_background_color_init((egui_background_t *)&bg_3);
    egui_background_set_params((egui_background_t *)&bg_3, &bg_3_params);
    egui_view_set_background((egui_view_t *)&label_3, (egui_background_t *)&bg_3);

    // bg_button
    egui_background_color_init((egui_background_t *)&bg_button);
    egui_background_set_params((egui_background_t *)&bg_button, &bg_button_params);
    egui_view_set_background((egui_view_t *)&button_1, (egui_background_t *)&bg_button);

    // Add To Scroll View
    egui_view_viewpage_add_child((egui_view_t *)&test_viewpage_view, uicode_init_ui_scroll());
    egui_view_viewpage_add_child((egui_view_t *)&test_viewpage_view, (egui_view_t *)&label_1);
    egui_view_viewpage_add_child((egui_view_t *)&test_viewpage_view, (egui_view_t *)&label_2);
    egui_view_viewpage_add_child((egui_view_t *)&test_viewpage_view, (egui_view_t *)&label_3);
    egui_view_viewpage_add_child((egui_view_t *)&test_viewpage_view, (egui_view_t *)&button_1);

    egui_view_viewpage_layout_childs((egui_view_t *)&test_viewpage_view);

    return (egui_view_t *)&test_viewpage_view;
}
