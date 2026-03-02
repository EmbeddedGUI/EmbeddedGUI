#include "egui.h"
#include <stdlib.h>
#include <math.h>
#include "uicode.h"

#include "egui_view_test_performance.h"

#if EGUI_CONFIG_RECORDING_TEST
#include "core/egui_input_simulator.h"
#endif

#define TEST_FOR_FULL_SCREEN_COMPARE 0

#if EGUI_PORT == EGUI_PORT_TYPE_QEMU
#define TEST_REPEAT_COUNT 3
#else
#define TEST_REPEAT_COUNT 10
#endif

// views in test_view_group_1
static egui_view_test_performance_t test_view;

static const char *egui_view_test_performance_type_string(int test_mode);

void uicode_init_ui(void)
{
    // Init all views
    // test_view
    egui_view_test_performance_init(EGUI_VIEW_OF(&test_view));
    egui_view_set_position(EGUI_VIEW_OF(&test_view), 0, 0);
    egui_view_set_size(EGUI_VIEW_OF(&test_view), EGUI_CONFIG_SCEEN_WIDTH, EGUI_CONFIG_SCEEN_HEIGHT);

    // Add To Root
    egui_core_add_user_root_view(EGUI_VIEW_OF(&test_view));
}

static void user_manu_refresh_screen(void)
{
    // Manual refresh screen three time to test performance.
    egui_core_force_refresh();
    egui_core_refresh_screen();

#if EGUI_PORT == EGUI_PORT_TYPE_PC
    // Brief delay for SDL event processing
    egui_api_delay(500);
#endif

#if EGUI_PORT == EGUI_PORT_TYPE_QEMU
    /* Use microsecond timer for high-resolution measurement in QEMU */
    extern uint32_t qemu_get_tick_us(void);
    uint32_t start_us = qemu_get_tick_us();
    for (int i = 0; i < TEST_REPEAT_COUNT; i++)
    {
        egui_core_force_refresh();
        egui_polling_refresh_display();
    }
    uint32_t total_us = qemu_get_tick_us() - start_us;
    uint32_t avg_us = total_us / TEST_REPEAT_COUNT;
    EGUI_LOG_INF("Refresh Screen Time: %d.%03dms\r\n", avg_us / 1000, avg_us % 1000);

    // Structured output for automated parsing
    EGUI_LOG_INF("PERF_RESULT:%s:%d.%03d\r\n", egui_view_test_performance_type_string(test_view.test_mode), avg_us / 1000, avg_us % 1000);
#else
    uint32_t start_time = egui_api_timer_get_current();
    for (int i = 0; i < TEST_REPEAT_COUNT; i++)
    {
        egui_core_force_refresh();
        egui_polling_refresh_display();
    }
    // get time.
    start_time = egui_api_timer_get_current() - start_time;
    EGUI_LOG_INF("Refresh Screen Time: %d.%dms\r\n", start_time / TEST_REPEAT_COUNT, start_time % TEST_REPEAT_COUNT);

    // Structured output for automated parsing
    EGUI_LOG_INF("PERF_RESULT:%s:%d.%d\r\n", egui_view_test_performance_type_string(test_view.test_mode), start_time / TEST_REPEAT_COUNT,
                 start_time % TEST_REPEAT_COUNT);
#endif

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

    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_TRIANGLE:
        return "TRIANGLE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_TRIANGLE_FILL:
        return "TRIANGLE_FILL";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ELLIPSE:
        return "ELLIPSE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ELLIPSE_FILL:
        return "ELLIPSE_FILL";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_POLYGON:
        return "POLYGON";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_POLYGON_FILL:
        return "POLYGON_FILL";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_BEZIER_QUAD:
        return "BEZIER_QUAD";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_BEZIER_CUBIC:
        return "BEZIER_CUBIC";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_CIRCLE_HQ:
        return "CIRCLE_HQ";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_CIRCLE_FILL_HQ:
        return "CIRCLE_FILL_HQ";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ARC_HQ:
        return "ARC_HQ";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ARC_FILL_HQ:
        return "ARC_FILL_HQ";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_LINE_HQ:
        return "LINE_HQ";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_GRADIENT_RECT:
        return "GRADIENT_RECT";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_GRADIENT_ROUND_RECT:
        return "GRADIENT_ROUND_RECT";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_GRADIENT_CIRCLE:
        return "GRADIENT_CIRCLE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_GRADIENT_TRIANGLE:
        return "GRADIENT_TRIANGLE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_SHADOW:
        return "SHADOW";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_SHADOW_ROUND:
        return "SHADOW_ROUND";

    // Mask performance tests
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_RECT_FILL_NO_MASK:
        return "MASK_RECT_FILL_NO_MASK";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_RECT_FILL_ROUND_RECT:
        return "MASK_RECT_FILL_ROUND_RECT";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_RECT_FILL_CIRCLE:
        return "MASK_RECT_FILL_CIRCLE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_RECT_FILL_IMAGE:
        return "MASK_RECT_FILL_IMAGE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_NO_MASK:
        return "MASK_IMAGE_NO_MASK";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_ROUND_RECT:
        return "MASK_IMAGE_ROUND_RECT";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_CIRCLE:
        return "MASK_IMAGE_CIRCLE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_IMAGE_IMAGE:
        return "MASK_IMAGE_IMAGE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_ROUND_RECT_FILL_NO_MASK:
        return "MASK_ROUND_RECT_FILL_NO_MASK";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_MASK_ROUND_RECT_FILL_WITH_MASK:
        return "MASK_ROUND_RECT_FILL_WITH_MASK";

    // Animation performance tests
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ANIMATION_TRANSLATE:
        return "ANIMATION_TRANSLATE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ANIMATION_ALPHA:
        return "ANIMATION_ALPHA";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ANIMATION_SCALE:
        return "ANIMATION_SCALE";
    case EGUI_VIEW_TEST_PERFORMANCE_TYPE_ANIMATION_SET:
        return "ANIMATION_SET";

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
        if (!egui_view_test_performance_is_enabled(i))
        {
            EGUI_LOG_INF("=========== Test Mode: %s [SKIP] ===========\r\n", egui_view_test_performance_type_string(i));
            EGUI_LOG_INF("PERF_SKIP:%s\r\n", egui_view_test_performance_type_string(i));
            continue;
        }
        egui_view_test_performance_with_test_mode(i);
    }

    EGUI_LOG_INF("PERF_COMPLETE\r\n");

#if EGUI_PORT == EGUI_PORT_TYPE_QEMU
    extern void qemu_notify_perf_complete(void);
    qemu_notify_perf_complete();
#endif

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

#if EGUI_CONFIG_RECORDING_TEST
/**
 * @brief Recording actions for HelloPerformace.
 * Provide enough WAIT actions to capture all performance test types.
 * Each enabled test takes ~1-2 seconds at 4x speed.
 */
bool egui_port_get_recording_action(int action_index, egui_sim_action_t *p_action)
{
    // 60 wait actions to capture all test types
    if (action_index >= 60)
    {
        return false;
    }
    p_action->type = EGUI_SIM_ACTION_WAIT;
    p_action->interval_ms = 500;
    return true;
}
#endif
