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

// View params
EGUI_VIEW_VIEWPAGE_PARAMS_INIT(test_viewpage_view_params, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, TEST_ITEM_HEIGHT);
EGUI_VIEW_LABEL_PARAMS_INIT(viewpage_label_1_params, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, TEST_ITEM_HEIGHT, "Item 1", EGUI_CONFIG_FONT_DEFAULT, EGUI_COLOR_BLACK,
                            EGUI_ALPHA_100);
EGUI_VIEW_LABEL_PARAMS_INIT(viewpage_label_2_params, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, TEST_ITEM_HEIGHT, "Item 2", EGUI_CONFIG_FONT_DEFAULT, EGUI_COLOR_BLACK,
                            EGUI_ALPHA_100);
EGUI_VIEW_LABEL_PARAMS_INIT(viewpage_label_3_params, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, TEST_ITEM_HEIGHT, "Item 3", EGUI_CONFIG_FONT_DEFAULT, EGUI_COLOR_WHITE,
                            EGUI_ALPHA_100);
EGUI_VIEW_LABEL_PARAMS_INIT(viewpage_button_1_params, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, TEST_ITEM_HEIGHT, NULL, EGUI_CONFIG_FONT_DEFAULT, EGUI_COLOR_BLACK,
                            EGUI_ALPHA_100);

egui_view_t *uicode_init_ui_viewpage(void)
{
    // Init all views
    // test_view
    egui_view_viewpage_init_with_params(EGUI_VIEW_OF(&test_viewpage_view), &test_viewpage_view_params);

    // label_1
    egui_view_label_init_with_params(EGUI_VIEW_OF(&label_1), &viewpage_label_1_params);

    // label_2
    egui_view_label_init_with_params(EGUI_VIEW_OF(&label_2), &viewpage_label_2_params);

    // label_3
    egui_view_label_init_with_params(EGUI_VIEW_OF(&label_3), &viewpage_label_3_params);

    // button_1
    egui_view_button_init_with_params(EGUI_VIEW_OF(&button_1), &viewpage_button_1_params);
    egui_view_label_set_text(EGUI_VIEW_OF(&button_1), button_str);
    egui_view_set_on_click_listener(EGUI_VIEW_OF(&button_1), button_click_cb);

    // bg_1
    egui_background_color_init_with_params(EGUI_BG_OF(&bg_1), &bg_1_params);
    egui_view_set_background(EGUI_VIEW_OF(&label_1), EGUI_BG_OF(&bg_1));

    // bg_2
    egui_background_color_init_with_params(EGUI_BG_OF(&bg_2), &bg_2_params);
    egui_view_set_background(EGUI_VIEW_OF(&label_2), EGUI_BG_OF(&bg_2));

    // bg_3
    egui_background_color_init_with_params(EGUI_BG_OF(&bg_3), &bg_3_params);
    egui_view_set_background(EGUI_VIEW_OF(&label_3), EGUI_BG_OF(&bg_3));

    // bg_button
    egui_background_color_init_with_params(EGUI_BG_OF(&bg_button), &bg_button_params);
    egui_view_set_background(EGUI_VIEW_OF(&button_1), EGUI_BG_OF(&bg_button));

    // Add To Scroll View
    egui_view_viewpage_add_child(EGUI_VIEW_OF(&test_viewpage_view), uicode_init_ui_scroll());
    egui_view_viewpage_add_child(EGUI_VIEW_OF(&test_viewpage_view), EGUI_VIEW_OF(&label_1));
    egui_view_viewpage_add_child(EGUI_VIEW_OF(&test_viewpage_view), EGUI_VIEW_OF(&label_2));
    egui_view_viewpage_add_child(EGUI_VIEW_OF(&test_viewpage_view), EGUI_VIEW_OF(&label_3));
    egui_view_viewpage_add_child(EGUI_VIEW_OF(&test_viewpage_view), EGUI_VIEW_OF(&button_1));

    egui_view_viewpage_layout_childs(EGUI_VIEW_OF(&test_viewpage_view));

    return EGUI_VIEW_OF(&test_viewpage_view);
}
