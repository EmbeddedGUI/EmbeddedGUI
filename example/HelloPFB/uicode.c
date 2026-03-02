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

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_2_param_normal_0, EGUI_COLOR_PURPLE, EGUI_ALPHA_90);
EGUI_BACKGROUND_PARAM_INIT(bg_2_params_0, &bg_2_param_normal_0, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_2_0, &bg_2_params_0);

EGUI_BACKGROUND_COLOR_PARAM_INIT_SOLID(bg_2_param_normal_1, EGUI_COLOR_PURPLE, EGUI_ALPHA_60);
EGUI_BACKGROUND_PARAM_INIT(bg_2_params_1, &bg_2_param_normal_1, NULL, NULL);
EGUI_BACKGROUND_COLOR_STATIC_CONST_INIT(bg_2_1, &bg_2_params_1);

#define LABEL_WIDTH  100
#define LABEL_HEIGHT 50

#define LABEL_DIFF_DIM 10

// View params
EGUI_VIEW_LABEL_PARAMS_INIT(label_vertical_params, 0, 0, LABEL_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT, "Vertical", EGUI_CONFIG_FONT_DEFAULT, EGUI_COLOR_WHITE,
                            EGUI_ALPHA_100);
EGUI_VIEW_LABEL_PARAMS_INIT(label_horizontal_params, 0, 0, EGUI_CONFIG_SCEEN_WIDTH, LABEL_HEIGHT, "Horizontal", EGUI_CONFIG_FONT_DEFAULT, EGUI_COLOR_WHITE,
                            EGUI_ALPHA_100);
EGUI_VIEW_LABEL_PARAMS_INIT(label_1_params, LABEL_WIDTH, LABEL_HEIGHT, LABEL_WIDTH, LABEL_HEIGHT, "Single 1", EGUI_CONFIG_FONT_DEFAULT, EGUI_COLOR_WHITE,
                            EGUI_ALPHA_100);
EGUI_VIEW_LABEL_PARAMS_INIT(label_2_params, LABEL_WIDTH + LABEL_DIFF_DIM, LABEL_HEIGHT * 2 + LABEL_DIFF_DIM, LABEL_WIDTH, LABEL_HEIGHT, "Single 2",
                            EGUI_CONFIG_FONT_DEFAULT, EGUI_COLOR_WHITE, EGUI_ALPHA_100);

void uicode_init_ui(void)
{
    // Init all views
    // label_vertical
    egui_view_label_init_with_params(EGUI_VIEW_OF(&label_vertical), &label_vertical_params);

    // label_horizontal
    egui_view_label_init_with_params(EGUI_VIEW_OF(&label_horizontal), &label_horizontal_params);

    // label_1
    egui_view_label_init_with_params(EGUI_VIEW_OF(&label_1), &label_1_params);

    // label_2
    egui_view_label_init_with_params(EGUI_VIEW_OF(&label_2), &label_2_params);

    // background
    egui_view_set_background(EGUI_VIEW_OF(&label_vertical), EGUI_BG_OF(&bg_vertical_0));
    egui_view_set_background(EGUI_VIEW_OF(&label_horizontal), EGUI_BG_OF(&bg_horizontal_0));
    egui_view_set_background(EGUI_VIEW_OF(&label_1), EGUI_BG_OF(&bg_1_0));
    egui_view_set_background(EGUI_VIEW_OF(&label_2), EGUI_BG_OF(&bg_2_0));

    // Add To Root
    egui_core_add_user_root_view(EGUI_VIEW_OF(&label_vertical));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&label_horizontal));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&label_1));
    egui_core_add_user_root_view(EGUI_VIEW_OF(&label_2));
}

static void egui_test_change_background_vertical(void)
{
    if (EGUI_VIEW_OF(&label_vertical)->background == EGUI_BG_OF(&bg_vertical_0))
    {
        egui_view_set_background(EGUI_VIEW_OF(&label_vertical), EGUI_BG_OF(&bg_vertical_1));
    }
    else
    {
        egui_view_set_background(EGUI_VIEW_OF(&label_vertical), EGUI_BG_OF(&bg_vertical_0));
    }
}

static void egui_test_change_background_horizontal(void)
{
    if (EGUI_VIEW_OF(&label_horizontal)->background == EGUI_BG_OF(&bg_horizontal_0))
    {
        egui_view_set_background(EGUI_VIEW_OF(&label_horizontal), EGUI_BG_OF(&bg_horizontal_1));
    }
    else
    {
        egui_view_set_background(EGUI_VIEW_OF(&label_horizontal), EGUI_BG_OF(&bg_horizontal_0));
    }
}

static void egui_test_change_background_1(void)
{
    if (EGUI_VIEW_OF(&label_1)->background == EGUI_BG_OF(&bg_1_0))
    {
        egui_view_set_background(EGUI_VIEW_OF(&label_1), EGUI_BG_OF(&bg_1_1));
    }
    else
    {
        egui_view_set_background(EGUI_VIEW_OF(&label_1), EGUI_BG_OF(&bg_1_0));
    }
}

static void egui_test_change_background_2(void)
{
    if (EGUI_VIEW_OF(&label_2)->background == EGUI_BG_OF(&bg_2_0))
    {
        egui_view_set_background(EGUI_VIEW_OF(&label_2), EGUI_BG_OF(&bg_2_1));
    }
    else
    {
        egui_view_set_background(EGUI_VIEW_OF(&label_2), EGUI_BG_OF(&bg_2_0));
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
        EGUI_LOG_INF("Test Single Label1\r\n");
        egui_test_change_background_1();
        break;
    case 3:
        EGUI_LOG_INF("Test Single Label2\r\n");
        egui_test_change_background_2();
        break;

    case 4:
        EGUI_LOG_INF("Test Two Vertical Horizontal\r\n");
        egui_test_change_background_vertical();
        egui_test_change_background_horizontal();
        break;
    case 5:
        EGUI_LOG_INF("Test Two Label1 Label2\r\n");
        egui_test_change_background_1();
        egui_test_change_background_2();
        break;
    case 6:
        EGUI_LOG_INF("Test Two Vertical Label1\r\n");
        egui_test_change_background_vertical();
        egui_test_change_background_1();
        break;
    case 7:
        EGUI_LOG_INF("Test Two Vertical Label2\r\n");
        egui_test_change_background_vertical();
        egui_test_change_background_2();
        break;
    case 8:
        EGUI_LOG_INF("Test Two Horizontal Label1\r\n");
        egui_test_change_background_horizontal();
        egui_test_change_background_1();
        break;
    case 9:
        EGUI_LOG_INF("Test Two Horizontal Label2\r\n");
        egui_test_change_background_horizontal();
        egui_test_change_background_2();
        break;

    case 10:
        EGUI_LOG_INF("Test Move Label1\r\n");
        egui_view_scroll_by(EGUI_VIEW_OF(&label_1), 10, 10);
        break;
    case 11:
        EGUI_LOG_INF("Test Move Label1 Back\r\n");
        egui_view_scroll_by(EGUI_VIEW_OF(&label_1), -10, -10);
        break;
    case 12:
        EGUI_LOG_INF("Test Move Label2\r\n");
        egui_view_scroll_by(EGUI_VIEW_OF(&label_2), 10, 10);
        break;
    case 13:
        EGUI_LOG_INF("Test Move Label2 Back\r\n");
        egui_view_scroll_by(EGUI_VIEW_OF(&label_2), -10, -10);
        break;

    case 14:
        EGUI_LOG_INF("Test Move Big Label2\r\n");
        egui_view_scroll_by(EGUI_VIEW_OF(&label_2), 0, (LABEL_HEIGHT + 5));
        break;
    case 15:
        EGUI_LOG_INF("Test Move Big Label2 Back\r\n");
        egui_view_scroll_by(EGUI_VIEW_OF(&label_2), 0, -(LABEL_HEIGHT + 5));
        break;

    case 16:
        EGUI_LOG_INF("Test Two Move Big Label2 Label1\r\n");
        egui_view_scroll_by(EGUI_VIEW_OF(&label_2), 0, (LABEL_HEIGHT + 5));
        egui_view_scroll_by(EGUI_VIEW_OF(&label_1), LABEL_DIFF_DIM, LABEL_DIFF_DIM + 5);
        break;
    case 17:
        EGUI_LOG_INF("Test Two Move Big Label2 Label1\r\n");
        egui_view_scroll_by(EGUI_VIEW_OF(&label_2), 0, -(LABEL_HEIGHT + 5));
        egui_view_scroll_by(EGUI_VIEW_OF(&label_1), -(LABEL_DIFF_DIM), -(LABEL_DIFF_DIM + 5));
        break;

    default:
        break;
    }
    if (++test_count > 17)
    {
        test_count = 0;
    }
}

void uicode_create_ui(void)
{
    uicode_init_ui();

    egui_test_refresh_timer.callback = egui_test_refresh_timer_callback;
    egui_timer_start_timer(&egui_test_refresh_timer, 1000, 3000);
}

#if EGUI_CONFIG_RECORDING_TEST
// Recording actions: wait and observe timer-driven PFB refresh tests
// Timer period is 3000ms, so waits must match to capture different states
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    static int last_action = -1;
    int first_call = (action_index != last_action);
    last_action = action_index;

    if (action_index >= 3)
    {
        return false;
    }
    if (first_call)
        recording_request_snapshot();
    EGUI_SIM_SET_WAIT(p_action, 3000);
    return true;
}
#endif
