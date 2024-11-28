#include "egui.h"
#include <stdlib.h>
#include <math.h>
#include "uicode.h"

// views in root
static egui_view_label_t label_vertical;
static egui_view_label_t label_horizontal;
static egui_view_label_t label_1;
static egui_view_label_t label_2;

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_vertical_param_normal_0, EGUI_COLOR_YELLOW, EGUI_ALPHA_90);
EGUI_BACKGROUND_PARAM_INIT(bg_vertical_params_0, &bg_vertical_param_normal_0, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_vertical_0, &bg_vertical_params_0);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_vertical_param_normal_1, EGUI_COLOR_YELLOW, EGUI_ALPHA_60);
EGUI_BACKGROUND_PARAM_INIT(bg_vertical_params_1, &bg_vertical_param_normal_1, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_vertical_1, &bg_vertical_params_1);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_horizontal_param_normal_0, EGUI_COLOR_ORANGE, EGUI_ALPHA_90);
EGUI_BACKGROUND_PARAM_INIT(bg_horizontal_params_0, &bg_horizontal_param_normal_0, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_horizontal_0, &bg_horizontal_params_0);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_horizontal_param_normal_1, EGUI_COLOR_ORANGE, EGUI_ALPHA_60);
EGUI_BACKGROUND_PARAM_INIT(bg_horizontal_params_1, &bg_horizontal_param_normal_1, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_horizontal_1, &bg_horizontal_params_1);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_1_param_normal_0, EGUI_COLOR_GREEN, EGUI_ALPHA_90);
EGUI_BACKGROUND_PARAM_INIT(bg_1_params_0, &bg_1_param_normal_0, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_1_0, &bg_1_params_0);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_1_param_normal_1, EGUI_COLOR_GREEN, EGUI_ALPHA_60);
EGUI_BACKGROUND_PARAM_INIT(bg_1_params_1, &bg_1_param_normal_1, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_1_1, &bg_1_params_1);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_2_param_normal_0, EGUI_COLOR_BLUE, EGUI_ALPHA_90);
EGUI_BACKGROUND_PARAM_INIT(bg_2_params_0, &bg_2_param_normal_0, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_2_0, &bg_2_params_0);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_2_param_normal_1, EGUI_COLOR_BLUE, EGUI_ALPHA_60);
EGUI_BACKGROUND_PARAM_INIT(bg_2_params_1, &bg_2_param_normal_1, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_2_1, &bg_2_params_1);

#define LABEL_WIDTH  100
#define LABEL_HEIGHT 50
void uicode_init_ui(void)
{
    // Init all views
    // label_vertical
    egui_view_label_init((egui_view_t *)&label_vertical);
    egui_view_set_position((egui_view_t *)&label_vertical, 0, 0);
    egui_view_set_size((egui_view_t *)&label_vertical, LABEL_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    egui_view_label_set_text((egui_view_t *)&label_vertical, "Vertical");
    egui_view_label_set_align_type((egui_view_t *)&label_vertical, EGUI_ALIGN_CENTER);
    egui_view_label_set_font((egui_view_t *)&label_vertical, (egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_font_color((egui_view_t *)&label_vertical, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

    // label_horizontal
    egui_view_label_init((egui_view_t *)&label_horizontal);
    egui_view_set_position((egui_view_t *)&label_horizontal, 0, 0);
    egui_view_set_size((egui_view_t *)&label_horizontal, EGUI_CONFIG_SCEEN_WIDTH, LABEL_HEIGHT);
    egui_view_label_set_text((egui_view_t *)&label_horizontal, "Horizontal");
    egui_view_label_set_align_type((egui_view_t *)&label_horizontal, EGUI_ALIGN_CENTER);
    egui_view_label_set_font((egui_view_t *)&label_horizontal, (egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_font_color((egui_view_t *)&label_horizontal, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

    // label_1
    egui_view_label_init((egui_view_t *)&label_1);
    egui_view_set_position((egui_view_t *)&label_1, LABEL_WIDTH, LABEL_HEIGHT);
    egui_view_set_size((egui_view_t *)&label_1, LABEL_WIDTH, LABEL_HEIGHT);
    egui_view_label_set_text((egui_view_t *)&label_1, "Single 1");
    egui_view_label_set_align_type((egui_view_t *)&label_1, EGUI_ALIGN_CENTER);
    egui_view_label_set_font((egui_view_t *)&label_1, (egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_font_color((egui_view_t *)&label_1, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

    // label_2
    egui_view_label_init((egui_view_t *)&label_2);
    egui_view_set_position((egui_view_t *)&label_2, LABEL_WIDTH + 10, LABEL_HEIGHT * 2 + 10);
    egui_view_set_size((egui_view_t *)&label_2, LABEL_WIDTH, LABEL_HEIGHT);
    egui_view_label_set_text((egui_view_t *)&label_2, "Single 2");
    egui_view_label_set_align_type((egui_view_t *)&label_2, EGUI_ALIGN_CENTER);
    egui_view_label_set_font((egui_view_t *)&label_2, (egui_font_t *)EGUI_CONFIG_FONT_DEFAULT);
    egui_view_label_set_font_color((egui_view_t *)&label_2, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

    // background
    egui_view_set_background((egui_view_t *)&label_vertical, (egui_background_t *)&bg_vertical_0);
    egui_view_set_background((egui_view_t *)&label_horizontal, (egui_background_t *)&bg_horizontal_0);
    egui_view_set_background((egui_view_t *)&label_1, (egui_background_t *)&bg_1_0);
    egui_view_set_background((egui_view_t *)&label_2, (egui_background_t *)&bg_2_0);

    // Add To Root
    egui_core_add_user_root_view((egui_view_t *)&label_vertical);
    egui_core_add_user_root_view((egui_view_t *)&label_horizontal);
    egui_core_add_user_root_view((egui_view_t *)&label_1);
    egui_core_add_user_root_view((egui_view_t *)&label_2);
}

static void egui_test_change_background_vertical(void)
{
    if(((egui_view_t *)&label_vertical)->background == (egui_background_t *)&bg_vertical_0)
    {
        egui_view_set_background((egui_view_t *)&label_vertical, (egui_background_t *)&bg_vertical_1);
    }
    else
    {
        egui_view_set_background((egui_view_t *)&label_vertical, (egui_background_t *)&bg_vertical_0);
    }
}

static void egui_test_change_background_horizontal(void)
{
    if(((egui_view_t *)&label_horizontal)->background == (egui_background_t *)&bg_horizontal_0)
    {
        egui_view_set_background((egui_view_t *)&label_horizontal, (egui_background_t *)&bg_horizontal_1);
    }
    else
    {
        egui_view_set_background((egui_view_t *)&label_horizontal, (egui_background_t *)&bg_horizontal_0);
    }
}

static void egui_test_change_background_1(void)
{
    if(((egui_view_t *)&label_1)->background == (egui_background_t *)&bg_1_0)
    {
        egui_view_set_background((egui_view_t *)&label_1, (egui_background_t *)&bg_1_1);
    }
    else
    {
        egui_view_set_background((egui_view_t *)&label_1, (egui_background_t *)&bg_1_0);
    }
}

static void egui_test_change_background_2(void)
{
    if(((egui_view_t *)&label_2)->background == (egui_background_t *)&bg_2_0)
    {
        egui_view_set_background((egui_view_t *)&label_2, (egui_background_t *)&bg_2_1);
    }
    else
    {
        egui_view_set_background((egui_view_t *)&label_2, (egui_background_t *)&bg_2_0);
    }
}

static int test_count = 0;
static egui_timer_t egui_test_refresh_timer;
static void egui_test_refresh_timer_callback(egui_timer_t *timer)
{
    switch (test_count)
    {
    case 0:
        EGUI_LOG_INF("Test Single Vertical\r\n");
        egui_test_change_background_vertical();
        break;
    case 1:
        EGUI_LOG_INF("Test Single Horizontal\r\n");
        egui_test_change_background_horizontal();
        break;
    case 2:
        EGUI_LOG_INF("Test Single Label 1\r\n");
        egui_test_change_background_1();
        break;
    case 3:
        EGUI_LOG_INF("Test Single Label 2\r\n");
        egui_test_change_background_2();
        break;
    
    default:
        break;
    }
    if(test_count++ > 4)
    {
        test_count = 0;
    }
}

void uicode_create_ui(void)
{
    uicode_init_ui();

    egui_test_refresh_timer.callback = egui_test_refresh_timer_callback;
    egui_timer_start_timer(&egui_test_refresh_timer, 5000, 5000);
}
