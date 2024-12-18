#include "egui.h"
#include <stdlib.h>
#include <math.h>
#include "uicode.h"

#include "egui_view_test_performance.h"

#define TEST_FOR_FULL_SCREEN_COMPARE 0

#define TEST_REPEAT_COUNT 10

// views in test_view_group_1
static egui_view_test_performance_t test_view;

void uicode_init_ui(void)
{
    // Init all views
    // test_view
    egui_view_test_performance_init((egui_view_t *)&test_view);
    egui_view_set_position((egui_view_t *)&test_view, 0, 0);
    egui_view_set_size((egui_view_t *)&test_view, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);

    // Add To Root
    egui_core_add_user_root_view((egui_view_t *)&test_view);
}

static void user_manu_refresh_screen(void)
{
    // Manual refresh screen three time to test performance.
    egui_core_force_refresh();
    egui_core_refresh_screen();

#if EGUI_PORT == EGUI_PORT_TYPE_PC
    // Wait for 500ms, for pc need do this, because egui_polling_refresh_display() will use too much time.
    egui_api_delay(500);
#endif

    uint32_t start_time = egui_api_timer_get_current();
    for (int i = 0; i < TEST_REPEAT_COUNT; i++)
    {
        egui_core_force_refresh();
        egui_polling_refresh_display();
    }
    // get time.
    start_time = egui_api_timer_get_current() - start_time;
    EGUI_LOG_INF("Refresh Screen Time: %d.%dms\r\n", start_time / TEST_REPEAT_COUNT, start_time % TEST_REPEAT_COUNT);

    // egui_api_refresh_display() in pc will use too much time, so we calc time.
    egui_api_refresh_display();
}

static const char *egui_view_test_performance_type_string(int test_mode)
{
    switch (test_mode)
    {
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_LINE:
        return "LINE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_565:
        return "IMAGE_565";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_565_1:
        return "IMAGE_565_1";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_565_2:
        return "IMAGE_565_2";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_565_4:
        return "IMAGE_565_4";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_565_8:
        return "IMAGE_565_8";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_565:
        return "IMAGE_RESIZE_565";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_565_1:
        return "IMAGE_RESIZE_565_1";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_565_2:
        return "IMAGE_RESIZE_565_2";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_565_4:
        return "IMAGE_RESIZE_565_4";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_IMAGE_RESIZE_565_8:
        return "IMAGE_RESIZE_565_8";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_TEXT:
        return "TEXT";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_TEXT_RECT:
        return "TEXT_RECT";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_RECTANGLE:
        return "RECTANGLE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_CIRCLE:
        return "CIRCLE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ARC:
        return "ARC";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ROUND_RECTANGLE:
        return "ROUND_RECTANGLE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ROUND_RECTANGLE_CORNERS:
        return "ROUND_RECTANGLE_CORNERS";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_RECTANGLE_FILL:
        return "RECTANGLE_FILL";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_CIRCLE_FILL:
        return "CIRCLE_FILL";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ARC_FILL:
        return "ARC_FILL";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ROUND_RECTANGLE_FILL:
        return "ROUND_RECTANGLE_FILL";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ROUND_RECTANGLE_CORNERS_FILL:
        return "ROUND_RECTANGLE_CORNERS_FILL";

    default:
        return "Unknown";
    }
}

#if TEST_FOR_FULL_SCREEN_COMPARE
static egui_color_int_t test_full_screen_pfb[EGUI_CONFIG_SCEEN_WIDTH * EGUI_CONFIG_SCEEN_HEIGHT];
#endif
void egui_view_test_performance_with_test_mode(int test_mode)
{
    EGUI_LOG_INF("=========== Test Mode: %s ===========\r\n", egui_view_test_performance_type_string(test_mode));
    test_view.test_mode = test_mode;
    user_manu_refresh_screen();

#if TEST_FOR_FULL_SCREEN_COMPARE
    EGUI_LOG_INF("Test Full Screen Compare\r\n");
    // Set Full Screen
    egui_color_int_t *pfb = egui_core_get_pfb_buffer_ptr();
    egui_core_pfb_set_buffer(test_full_screen_pfb, EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);
    // Manual refresh screen three time to test performance.
    user_manu_refresh_screen();
    // Recovery pfb
    egui_core_pfb_set_buffer(pfb, EGUI_CONFIG_PFB_WIDTH, EGUI_CONFIG_PFB_HEIGHT);
#endif
}

void egui_view_test_performance_process(void)
{
    for (int i = 0; i < EGUI_VIEW_TEST_PERFORMANCE_TYPE_MAX; i++)
    {
        egui_view_test_performance_with_test_mode(i);
    }

    // Test Single
    // egui_view_test_performance_with_test_mode(EGUI_VIEW_TEST_PERFORMANCE_TYPE_RECTANGLE_FILL);
}

static egui_timer_t ui_timer;
void egui_view_test_performance_timer_callback(egui_timer_t *timer)
{
    egui_view_test_performance_process();
}

void uicode_create_ui(void)
{
    uicode_init_ui();

    // For test performance, use manual refresh screen.
    egui_core_stop_auto_refresh_screen();

    ui_timer.callback = egui_view_test_performance_timer_callback;
    egui_timer_start_timer(&ui_timer, 100, 0);
}
